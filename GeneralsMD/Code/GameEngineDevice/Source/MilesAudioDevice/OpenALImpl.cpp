/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * OpenALImpl.cpp
 *
 * Implements the OAL_* functions declared in MSS.h, providing a real 64-bit
 * audio backend for the former MSS (Miles Sound System) stubs.
 * Uses OpenAL Soft for audio playback.
 *
 * Supported:
 *   - 2-D sound effects (PCM WAV, 8/16-bit mono/stereo)
 *   - IMA-ADPCM decompression for 2-D samples
 *   - Streamed audio (music, speech) via game VFS callbacks
 *   - Volume / pan control
 *   - End-of-sample callbacks (polled from main thread each frame)
 *   - End-of-stream callbacks (for music looping)
 *   - Fake 3-D provider so selectProvider() succeeds (3-D positional stubs)
 */

/* =========================================================================
   Includes
   ========================================================================= */

/* Include MSS.h only for its type definitions.  The AIL_* macros will expand
   to OAL_* calls, but those calls are the DEFINITIONS being compiled here,
   so they are safe to use internally after the macro layer is in place.
   We do NOT expand them inside this file since we call OAL_* directly. */
#include "MSS/MSS.h"

/* OpenAL Soft */
#include <AL/al.h>
#include <AL/alc.h>

/* Windows / C run-time */
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>

/* =========================================================================
   Configuration
   ========================================================================= */

#define OAL_MAX_SAMPLES      64   /* pre-allocated 2D sample pool size     */
#define OAL_NUM_STREAM_BUFS   3   /* triple buffering for streams           */
#define OAL_STREAM_BUF_BYTES  (32 * 1024)  /* 32 KB per streaming buffer   */
#define OAL_MAX_PROVIDERS     4   /* how many fake 3-D providers            */
#define OAL_USER_DATA_SLOTS   8   /* per-sample user data slots             */

/* =========================================================================
   Internal structures
   ========================================================================= */

struct OALSample
{
    ALuint        source;
    ALuint        buffer;              /* 0 = no buffer loaded                  */
    float         volume;
    float         pan;
    int           sample_rate;
    AILEOSCALLBACK eos_callback;
    intptr_t      user_data[OAL_USER_DATA_SLOTS];
    bool          in_use;
    volatile bool pending_eos;         /* streaming thread sets, drain reads    */
    bool          is_3d;              /* true = positional (AL_SOURCE_RELATIVE=FALSE) */
};

struct OALStream
{
    ALuint        source;
    ALuint        bufs[OAL_NUM_STREAM_BUFS];
    bool          valid;
    volatile bool should_stop;
    volatile bool is_paused;
    HSTREAM       handle;           /* 1-based index into s_streamTable – survives the
                                       UnsignedInt truncation in MilesAudioManager     */

    float         volume;
    float         pan;

    /* WAV format info */
    int           channels;
    int           sample_rate;
    int           bytes_per_sample;    /* bits-per-sample / 8                   */
    ALenum        al_format;

    /* VFS streaming state */
    U32           file_handle;
    long          data_offset;         /* byte offset to PCM in the file        */
    long          data_size;           /* total PCM data bytes                  */
    long          data_remaining;      /* bytes left in current playthrough     */

    /* Looping / callbacks */
    int           loop_count;          /* -1 = infinite, 0 = done, n = n more  */
    AILSTREAM_CALLBACK stream_callback;

    /* Position tracking (ms) */
    volatile long current_ms;
    long          total_ms;

    /* Optional MCI-backed fallback for MP3 music/stream assets. */
    bool          use_mci;
    char          temp_path[MAX_PATH];
    char          mci_alias[32];

    /* Win32 thread */
    HANDLE        thread;
    HANDLE        wake_event;

    /* ADPCM in-memory playback.  When the source file was IMA-ADPCM compressed,
       we decompress the whole file to a PCM WAV in memory and stream from that
       buffer instead of using VFS callbacks.  NULL for ordinary PCM streams. */
    unsigned char *decoded_pcm;      /* malloc'd full decoded WAV; NULL = file stream */
    long           decoded_pcm_pos;  /* current read offset within decoded_pcm       */
};

/* =========================================================================
   Global state
   ========================================================================= */

static ALCdevice  *g_alDevice  = nullptr;
static ALCcontext *g_alContext = nullptr;

/* VFS file callbacks set by AIL_set_file_callbacks */
static AIL_FILE_OPEN_CALLBACK  g_cbOpen  = nullptr;
static AIL_FILE_CLOSE_CALLBACK g_cbClose = nullptr;
static AIL_FILE_SEEK_CALLBACK  g_cbSeek  = nullptr;
static AIL_FILE_READ_CALLBACK  g_cbRead  = nullptr;

/* 2-D sample pool (static so pointers stay below 4 GB for the 32-bit handle) */
static OALSample g_samples[OAL_MAX_SAMPLES];

/* Fake 3-D provider names exposed by OAL_enumerate_3D_providers */
static const char *g_providerNames[OAL_MAX_PROVIDERS] = {
    "Miles Fast 2D Positional Audio",
    "Dolby Surround",
    nullptr,
    nullptr
};
static int g_providerCount = 2;

/* Completion queue: streaming thread posts OALSample* needing EOS callback */
#define COMPLETION_QUEUE_SIZE 64
struct CompletionEntry { OALSample *sample; };
static CompletionEntry g_completionQueue[COMPLETION_QUEUE_SIZE];
static volatile LONG   g_cqHead = 0;   /* written by streaming thread */
static volatile LONG   g_cqTail = 0;   /* read by main thread         */

/* Quick-play handles (mission briefings) */
struct QuickAudio { ALuint source; ALuint buffer; };
#define MAX_QUICK_AUDIO 8
static QuickAudio g_quickAudio[MAX_QUICK_AUDIO];

/* Listener world-space position (set via AIL_set_3D_position on the listener) */
static float g_listenerX = 0.f, g_listenerY = 0.f, g_listenerZ = 0.f;

/* Per-sample 3D world position and distance settings */
struct OALSample3DPos { float x, y, z, min_dist, max_dist; };
static OALSample3DPos g_sample3D[OAL_MAX_SAMPLES];

/* =========================================================================
   Helpers
   ========================================================================= */

static void oal_push_eos(OALSample *s)
{
    LONG head = g_cqHead;
    LONG next = (head + 1) % COMPLETION_QUEUE_SIZE;
    if (next != g_cqTail) {
        g_completionQueue[head].sample = s;
        InterlockedExchange(&g_cqHead, next);
    }
}

/* Retrieve OALSample from a 1-based index handle (fits in 32-bit UnsignedInt) */
static inline OALSample *smp_from_handle(HSAMPLE s)
{
    uintptr_t i = (uintptr_t)s;
    if (i < 1 || i > (uintptr_t)OAL_MAX_SAMPLES) return nullptr;
    return &g_samples[i - 1];
}

/* 1-based index stream table.  The index (1..OAL_MAX_STREAMS) fits in a 32-bit
   UnsignedInt, so it survives the static_cast<UnsignedInt>(reinterpret_cast<intptr_t>)
   truncation in MilesAudioManager::setStreamCompleted / findPlayingAudioFrom.   */
#define OAL_MAX_STREAMS 8
static OALStream *s_streamTable[OAL_MAX_STREAMS];   /* zero-initialised by C++ */

static HSTREAM stream_alloc_slot(OALStream *st)
{
    for (int i = 0; i < OAL_MAX_STREAMS; ++i) {
        if (!s_streamTable[i]) {
            s_streamTable[i] = st;
            HSTREAM h = (HSTREAM)(uintptr_t)(i + 1);
            st->handle = h;
            return h;
        }
    }
    return nullptr;   /* table full */
}

static inline OALStream *stream_from_handle(HSTREAM h)
{
    uintptr_t i = (uintptr_t)h;
    if (i < 1 || i > (uintptr_t)OAL_MAX_STREAMS) return nullptr;
    return s_streamTable[i - 1];
}

static void stream_free_slot(HSTREAM h)
{
    uintptr_t i = (uintptr_t)h;
    if (i >= 1 && i <= (uintptr_t)OAL_MAX_STREAMS) s_streamTable[i - 1] = nullptr;
}

static bool oal_is_mp3_data(const unsigned char *data, size_t size)
{
    if (!data || size < 2) {
        return false;
    }

    if (size >= 3 && memcmp(data, "ID3", 3) == 0) {
        return true;
    }

    return data[0] == 0xFF && (data[1] & 0xE0) == 0xE0;
}

static void oal_apply_mci_volume(OALStream *st)
{
    if (!st || !st->use_mci || st->mci_alias[0] == '\0') {
        return;
    }

    int volume = (int)(st->volume * 1000.0f + 0.5f);
    if (volume < 0) volume = 0;
    if (volume > 1000) volume = 1000;

    char cmd[128];
    _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "setaudio %s volume to %d", st->mci_alias, volume);
    mciSendStringA(cmd, nullptr, 0, nullptr);
}

static void oal_close_mci_stream(OALStream *st)
{
    if (!st) {
        return;
    }

    if (st->mci_alias[0] != '\0') {
        char cmd[96];
        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "stop %s", st->mci_alias);
        mciSendStringA(cmd, nullptr, 0, nullptr);
        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "close %s", st->mci_alias);
        mciSendStringA(cmd, nullptr, 0, nullptr);
        st->mci_alias[0] = '\0';
    }

    if (st->temp_path[0] != '\0') {
        DeleteFileA(st->temp_path);
        st->temp_path[0] = '\0';
    }
}

/* Called once per frame from OAL_set_3D_orientation to fire queued callbacks.
   Emits (HSAMPLE)(1-based index) so the value survives a UnsignedInt round-trip
   in MilesAudioManager::notifyOfAudioCompletion / findPlayingAudioFrom. */
static void oal_drain_eos_queue()
{
    while (g_cqTail != g_cqHead) {
        OALSample *s = g_completionQueue[g_cqTail].sample;
        g_cqTail = (g_cqTail + 1) % COMPLETION_QUEUE_SIZE;
        if (s && s->eos_callback) {
            uintptr_t idx = (uintptr_t)(s - g_samples) + 1; /* 1-based */
            s->eos_callback((HSAMPLE)idx);
        }
    }
}

/* Poll all 2-D sample sources; push completions for any that have stopped. */
static void oal_poll_sample_completions()
{
    for (int i = 0; i < OAL_MAX_SAMPLES; ++i) {
        OALSample *s = &g_samples[i];
        if (!s->in_use || !s->eos_callback || s->pending_eos) continue;
        if (s->source == 0) continue;
        ALint state = AL_STOPPED;
        alGetSourcei(s->source, AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED) {
            s->pending_eos = true;
            oal_push_eos(s);
        }
    }
}

/* Apply volume + pan to an AL source (pan via stereo position on X axis). */
static void apply_gain_pan(ALuint src, float vol, float pan)
{
    alSourcef(src, AL_GAIN, vol < 0.0f ? 0.0f : vol);
    /* Simple constant-power pan using stereo position */
    float x = (pan - 0.5f) * 2.0f;   /* -1 = full left, +1 = full right */
    alSource3f(src, AL_POSITION,  x, 0.0f, -1.0f);
    alSource3f(src, AL_VELOCITY,  0.0f, 0.0f, 0.0f);
    alSourcei(src,  AL_SOURCE_RELATIVE, AL_TRUE);
}

/* Update an AL source to reflect its 3D world position and distances. */
static void oal_update_3d_source(OALSample *smp, int i)
{
    if (!smp->source) return;
    alSourcei(smp->source, AL_SOURCE_RELATIVE, AL_FALSE);
    alSource3f(smp->source, AL_POSITION, g_sample3D[i].x, g_sample3D[i].y, g_sample3D[i].z);
    alSource3f(smp->source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcef(smp->source, AL_GAIN, smp->volume < 0.0f ? 0.0f : smp->volume);
    float minD = g_sample3D[i].min_dist;
    float maxD = g_sample3D[i].max_dist;
    if (maxD > 0.0f) {
        alSourcef(smp->source, AL_REFERENCE_DISTANCE, minD > 0.0f ? minD : 1.0f);
        alSourcef(smp->source, AL_MAX_DISTANCE, maxD);
        alSourcef(smp->source, AL_ROLLOFF_FACTOR, 1.0f);
    }
}

/* =========================================================================
   WAV / ADPCM helpers
   ========================================================================= */

/* RIFF chunk header */
#pragma pack(push,1)
struct RIFFChunk  { char id[4]; DWORD size; };
struct WaveFmt {
    WORD  audioFormat;
    WORD  numChannels;
    DWORD sampleRate;
    DWORD byteRate;
    WORD  blockAlign;
    WORD  bitsPerSample;
};
struct WaveFmtEx {
    WORD  cbSize;
    WORD  samplesPerBlock;
};
#pragma pack(pop)

static ALenum wav_al_format(int channels, int bits)
{
    if (channels == 1) return (bits == 16) ? AL_FORMAT_MONO16   : AL_FORMAT_MONO8;
    else               return (bits == 16) ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8;
}

/* =========================================================================
   IMA ADPCM decoder
   ========================================================================= */

static const int ima_step_tab[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,
    50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,
    253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,
    1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,
    3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
    10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,
    27086,29794,32767
};
static const int ima_idx_tab[16] = {
    -1,-1,-1,-1, 2, 4, 6, 8,
    -1,-1,-1,-1, 2, 4, 6, 8
};

static SHORT ima_decode_nibble(int nib, int *pred, int *idx)
{
    int step = ima_step_tab[*idx];
    int diff = step >> 3;
    if (nib & 1) diff += step >> 2;
    if (nib & 2) diff += step >> 1;
    if (nib & 4) diff += step;
    if (nib & 8) diff  = -diff;
    *pred += diff;
    if (*pred >  32767) *pred =  32767;
    if (*pred < -32768) *pred = -32768;
    *idx += ima_idx_tab[nib & 0xF];
    if (*idx < 0)  *idx = 0;
    if (*idx > 88) *idx = 88;
    return (SHORT)*pred;
}

/*
 * Decompress one Microsoft IMA-ADPCM WAV file.
 * Returns a malloc'd buffer of 16-bit PCM samples + a WAV header.
 * Total byte size stored in *outSize.
 */
static void *ima_adpcm_decode(const AILSOUNDINFO *info,
                               const unsigned char *src_buf,
                               U32 *outSize)
{
    const WaveFmt *fmt  = nullptr;
    const unsigned char *pcm_data = nullptr;
    long pcm_data_size = 0;
    int block_align    = info->_oal_block_align;
    int spb            = info->_oal_spb;       /* samples per block        */
    int channels       = info->channels;
    int sample_rate    = info->sample_rate;

    if (!src_buf || block_align <= 0 || spb <= 0) return nullptr;

    /* Locate data pointer from original buffer.
       For IMA-ADPCM, bit_depth = 4 (bits per compressed sample), so
       bit_depth/8 = 0 in integer arithmetic.  Instead, derive the raw
       compressed data size from num_samples (= num_blocks * spb) and
       block_align, which were parsed from the WAV header. */
    pcm_data      = (const unsigned char *)src_buf + info->data_offset;
    long num_blocks = ((long)info->num_samples + spb - 1) / spb;
    pcm_data_size = num_blocks * (long)block_align; /* raw compressed bytes */
    if (pcm_data_size <= 0) return nullptr;

    /* Number of 16-bit PCM output samples per channel */
    long out_samples_per_ch = num_blocks * (long)spb;
    long out_bytes = out_samples_per_ch * channels * sizeof(SHORT);

    /* Build a PCM WAV header + PCM data */
    int fmt_size  = sizeof(RIFFChunk) + 4 + sizeof(RIFFChunk) + sizeof(WaveFmt) + sizeof(RIFFChunk);
    int total_out = fmt_size + (int)out_bytes;
    unsigned char *out = (unsigned char *)malloc(total_out);
    if (!out) return nullptr;
    memset(out, 0, total_out);

    /* Write RIFF header */
    unsigned char *p = out;
    memcpy(p, "RIFF", 4); p += 4;
    *(DWORD*)p = (DWORD)(total_out - 8); p += 4;
    memcpy(p, "WAVE", 4); p += 4;

    /* fmt chunk */
    memcpy(p, "fmt ", 4); p += 4;
    *(DWORD*)p = sizeof(WaveFmt); p += 4;
    WaveFmt *wf = (WaveFmt*)p; p += sizeof(WaveFmt);
    wf->audioFormat   = WAVE_FORMAT_PCM;
    wf->numChannels   = (WORD)channels;
    wf->sampleRate    = (DWORD)sample_rate;
    wf->bitsPerSample = 16;
    wf->blockAlign    = (WORD)(channels * 2);
    wf->byteRate      = (DWORD)(sample_rate * channels * 2);

    /* data chunk */
    memcpy(p, "data", 4); p += 4;
    *(DWORD*)p = (DWORD)out_bytes; p += 4;

    SHORT *pcm_out = (SHORT*)p;

    /* Decode each block */
    const unsigned char *blk = pcm_data;
    const unsigned char *blk_end = pcm_data + pcm_data_size;
    long out_idx = 0;

    while (blk < blk_end) {
        long blk_remain = (long)(blk_end - blk);
        if (blk_remain <= 0) break;
        int this_blk = (int)(blk_remain < block_align ? blk_remain : block_align);

        if (channels == 1) {
            /* Mono: 4-byte preamble (int16 pred, int8 idx, int8 padding) */
            if (this_blk < 4) break;
            int pred  = (SHORT)(blk[0] | (blk[1] << 8));
            int idx   = blk[2]; if (idx > 88) idx = 88;
            pcm_out[out_idx++] = (SHORT)pred;
            const unsigned char *nb = blk + 4;
            int bytes_left = this_blk - 4;
            while (bytes_left-- > 0 && out_idx < out_samples_per_ch) {
                int byte = *nb++;
                pcm_out[out_idx++] = ima_decode_nibble(byte & 0xF,  &pred, &idx);
                if (out_idx < out_samples_per_ch)
                    pcm_out[out_idx++] = ima_decode_nibble((byte >> 4) & 0xF, &pred, &idx);
            }
        } else {
            /* Stereo: 8-byte preamble (2 × 4 bytes) then interleaved 4-byte sub-blocks */
            if (this_blk < 8) break;
            int predL = (SHORT)(blk[0] | (blk[1] << 8));
            int idxL  = blk[2]; if (idxL > 88) idxL = 88;
            int predR = (SHORT)(blk[4] | (blk[5] << 8));
            int idxR  = blk[6]; if (idxR > 88) idxR = 88;
            pcm_out[out_idx++] = (SHORT)predL; /* L */
            pcm_out[out_idx++] = (SHORT)predR; /* R */

            const unsigned char *nb = blk + 8;
            int bytes_left = this_blk - 8;
            /* Process 8-byte sub-blocks: 4 bytes L, 4 bytes R */
            while (bytes_left >= 8) {
                for (int i = 0; i < 4; i++) {
                    int byte = nb[i];
                    SHORT s0 = ima_decode_nibble(byte & 0xF,       &predL, &idxL);
                    SHORT s1 = ima_decode_nibble((byte >> 4) & 0xF, &predL, &idxL);
                    pcm_out[out_idx]     = s0;
                    pcm_out[out_idx + 8] = s1;
                    out_idx++;
                }
                out_idx += 4; /* skip the 4 interleaved R slots we pre-filled */
                for (int i = 0; i < 4; i++) {
                    int byte = nb[4 + i];
                    SHORT s0 = ima_decode_nibble(byte & 0xF,       &predR, &idxR);
                    SHORT s1 = ima_decode_nibble((byte >> 4) & 0xF, &predR, &idxR);
                    /* interleave R alongside L */
                    /* already pre-indexed; raw stereo: LRLRLR... */
                }
                nb += 8;
                bytes_left -= 8;
            }
        }
        blk += this_blk;
    }

    *outSize = (U32)total_out;
    return out;
}

/* =========================================================================
   OAL_* implementations
   ========================================================================= */

extern "C"
{

/* --- System --------------------------------------------------------------- */

void OAL_startup(void)
{
    if (g_alDevice) return;   /* already started */

    g_alDevice = alcOpenDevice(nullptr);
    if (!g_alDevice) return;

    g_alContext = alcCreateContext(g_alDevice, nullptr);
    if (!g_alContext) { alcCloseDevice(g_alDevice); g_alDevice = nullptr; return; }

    alcMakeContextCurrent(g_alContext);

    /* Initialise sample pool */
    memset(g_samples, 0, sizeof(g_samples));
    for (int i = 0; i < OAL_MAX_SAMPLES; ++i) {
        alGenSources(1, &g_samples[i].source);
        g_samples[i].volume = 1.0f;
        g_samples[i].pan    = 0.5f;
        g_samples[i].in_use = false;
        alSourcei(g_samples[i].source, AL_SOURCE_RELATIVE, AL_TRUE);
    }

    /* Listener at origin */
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    float orient[6] = { 0.0f,0.0f,-1.0f, 0.0f,1.0f,0.0f };
    alListenerfv(AL_ORIENTATION, orient);

    /* Linear distance model for 3D sounds */
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

    memset(g_sample3D, 0, sizeof(g_sample3D));
    g_listenerX = g_listenerY = g_listenerZ = 0.0f;

    memset(g_completionQueue, 0, sizeof(g_completionQueue));
    g_cqHead = g_cqTail = 0;
    memset(g_quickAudio, 0, sizeof(g_quickAudio));
}

void OAL_shutdown(void)
{
    if (!g_alDevice) return;

    /* Stop / delete all pooled sample sources */
    for (int i = 0; i < OAL_MAX_SAMPLES; ++i) {
        if (g_samples[i].source) {
            alSourceStop(g_samples[i].source);
            if (g_samples[i].buffer) {
                alSourcei(g_samples[i].source, AL_BUFFER, 0);
                alDeleteBuffers(1, &g_samples[i].buffer);
            }
            alDeleteSources(1, &g_samples[i].source);
        }
    }
    memset(g_samples, 0, sizeof(g_samples));

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(g_alContext);
    alcCloseDevice(g_alDevice);
    g_alContext = nullptr;
    g_alDevice  = nullptr;
}

int OAL_quick_startup(int /*useDigital*/, int /*useMidi*/,
                       int /*outputRate*/, int /*outputBits*/, int /*outputChannels*/)
{
    return 1;   /* success — device already open from OAL_startup() */
}

void OAL_quick_handles(void **digitalHandlePtr, void **midiHandlePtr, void **dlsHandlePtr)
{
    /* Hand the caller a non-NULL "digital handle" so downstream checks pass */
    if (digitalHandlePtr) *digitalHandlePtr = (void*)g_alDevice;
    if (midiHandlePtr)    *midiHandlePtr    = nullptr;
    if (dlsHandlePtr)     *dlsHandlePtr     = nullptr;
}

void OAL_set_file_callbacks(AIL_FILE_OPEN_CALLBACK open_cb, AIL_FILE_CLOSE_CALLBACK close_cb,
                             AIL_FILE_SEEK_CALLBACK seek_cb,  AIL_FILE_READ_CALLBACK  read_cb)
{
    g_cbOpen  = open_cb;
    g_cbClose = close_cb;
    g_cbSeek  = seek_cb;
    g_cbRead  = read_cb;
}

void OAL_MSS_version(char *buffer, int size)
{
    if (buffer && size > 0) _snprintf(buffer, size, "OpenAL Soft (OAL bridge)");
}

int OAL_get_timer_highest_delay(void) { return 0; }

/* --- WAV / ADPCM --------------------------------------------------------- */

void OAL_WAV_info(const void *buf, AILSOUNDINFO *info)
{
    if (!buf || !info) return;
    memset(info, 0, sizeof(*info));
    info->_oal_src_buf = const_cast<void*>(buf);

    const unsigned char *p   = (const unsigned char *)buf;
    const unsigned char *end = p + 0x7FFFFFFF; /* large sentinel */

    if (memcmp(p, "RIFF", 4) != 0) return;
    p += 8;   /* skip 'RIFF' + size */
    if (memcmp(p, "WAVE", 4) != 0) return;
    p += 4;

    const WaveFmt *fmt_data = nullptr;
    int block_align = 0;
    int spb = 0;

    while (p + 8 <= end) {
        char chunkId[4];
        memcpy(chunkId, p, 4); p += 4;
        DWORD chunkSize = *(const DWORD*)p; p += 4;

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            fmt_data = (const WaveFmt*)p;
            info->sample_rate = (int)fmt_data->sampleRate;
            info->channels    = (int)fmt_data->numChannels;
            info->bit_depth   = (int)fmt_data->bitsPerSample;
            info->format      = (int)fmt_data->audioFormat;
            block_align       = (int)fmt_data->blockAlign;
            info->_oal_block_align = block_align;

            if (info->format == WAVE_FORMAT_IMA_ADPCM && chunkSize >= (DWORD)(sizeof(WaveFmt) + sizeof(WaveFmtEx))) {
                const WaveFmtEx *ext = (const WaveFmtEx*)(p + sizeof(WaveFmt));
                spb = (int)ext->samplesPerBlock;
            } else if (info->format == WAVE_FORMAT_IMA_ADPCM) {
                /* calculate spb: (blockAlign - 4*numChannels)*8 / (4*numChannels) + 1 */
                spb = ((block_align - 4 * info->channels) * 8) / (4 * info->channels) + 1;
            }
            info->_oal_spb = spb;

        } else if (memcmp(chunkId, "data", 4) == 0) {
            info->data_offset = (int)(p - (const unsigned char *)buf);
            long data_bytes   = (long)chunkSize;
            if (info->format == WAVE_FORMAT_PCM && info->bit_depth > 0 && info->channels > 0) {
                info->num_samples = (int)(data_bytes / (info->channels * (info->bit_depth / 8)));
            } else if (info->format == WAVE_FORMAT_IMA_ADPCM && spb > 0 && block_align > 0) {
                long num_blocks   = (data_bytes + block_align - 1) / block_align;
                info->num_samples = (int)(num_blocks * spb);
            }
            /* data chunk found — we have all we need */
            return;
        }

        /* Move to next chunk (align to WORD) */
        DWORD skip = (chunkSize + 1) & ~1u;
        p += skip;
    }
}

void OAL_decompress_ADPCM(const AILSOUNDINFO *info, void **outBuffer, U32 *outSize)
{
    if (!info || !outBuffer || !outSize) return;
    *outBuffer = nullptr;
    *outSize   = 0;
    if (!info->_oal_src_buf) return;

    U32 sz = 0;
    void *decoded = ima_adpcm_decode(info,
                                      (const unsigned char*)info->_oal_src_buf,
                                      &sz);
    if (decoded) {
        *outBuffer = decoded;
        *outSize   = sz;
    }
}

void OAL_mem_free_lock(void *ptr)
{
    free(ptr);   /* matches malloc in ima_adpcm_decode */
}

/* --- 2-D sample management ----------------------------------------------- */

HSAMPLE OAL_allocate_sample(void * /*driver*/)
{
    if (!g_alDevice) return nullptr;
    for (int i = 0; i < OAL_MAX_SAMPLES; ++i) {
        if (!g_samples[i].in_use) {
            g_samples[i].in_use       = true;
            g_samples[i].is_3d        = false;
            g_samples[i].eos_callback = nullptr;
            g_samples[i].volume       = 1.0f;
            g_samples[i].pan          = 0.5f;
            g_samples[i].sample_rate  = 22050;
            g_samples[i].pending_eos  = false;
            alSourcef(g_samples[i].source, AL_PITCH, 1.0f);  /* reset pitch from previous use */
            memset(g_samples[i].user_data, 0, sizeof(g_samples[i].user_data));
            /* Return 1-based index so the handle fits in a 32-bit UnsignedInt */
            return (HSAMPLE)(uintptr_t)(i + 1);
        }
    }
    return nullptr;
}

void OAL_release_sample(HSAMPLE s)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    alSourceStop(smp->source);
    if (smp->buffer) {
        alSourcei(smp->source, AL_BUFFER, 0);
        alDeleteBuffers(1, &smp->buffer);
        smp->buffer = 0;
    }
    /* Restore to relative mode so the slot is safe for future 2D use */
    alSourcei(smp->source, AL_SOURCE_RELATIVE, AL_TRUE);
    smp->eos_callback = nullptr;
    smp->in_use       = false;
    smp->is_3d        = false;
    smp->pending_eos  = false;
    /* Reset sample_rate and pitch so pooled slots don't accumulate pitchShift
       multiplications across reuses.  initFilters3D uses OAL_sample_playback_rate()
       as its base — if that returns a stale rate from a previous sound, successive
       uses produce rate = prev_rate * pitchShift^N → exponential drift. */
    smp->sample_rate  = 22050;
    alSourcef(smp->source, AL_PITCH, 1.0f);
}

void OAL_init_sample(HSAMPLE s)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    alSourceStop(smp->source);
    if (smp->buffer) {
        alSourcei(smp->source, AL_BUFFER, 0);
        alDeleteBuffers(1, &smp->buffer);
        smp->buffer = 0;
    }
    smp->eos_callback = nullptr;
    smp->pending_eos  = false;
    alSourcef(smp->source, AL_PITCH, 1.0f);  /* ensure pitch is neutral for next use */
}

void OAL_set_sample_file(HSAMPLE s, const void *data, int /*block*/)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp || !data) return;

    /* Parse WAV from the buffer */
    AILSOUNDINFO si; memset(&si, 0, sizeof(si));
    OAL_WAV_info(data, &si);
    if (si.format != WAVE_FORMAT_PCM && si.format != WAVE_FORMAT_IMA_ADPCM) return;

    const void   *pcm_ptr  = nullptr;
    int           pcm_size = 0;
    ALenum        al_fmt   = AL_FORMAT_MONO16;
    int           al_rate  = si.sample_rate;
    void         *tmp_buf  = nullptr;   /* malloc'd ADPCM output (must be freed) */

    if (si.format == WAVE_FORMAT_IMA_ADPCM) {
        U32 sz = 0;
        OAL_decompress_ADPCM(&si, &tmp_buf, &sz);
        if (!tmp_buf) return;
        /* tmp_buf is a new PCM WAV; re-parse it */
        AILSOUNDINFO si2; memset(&si2, 0, sizeof(si2));
        OAL_WAV_info(tmp_buf, &si2);
        if (si2.format != WAVE_FORMAT_PCM) { free(tmp_buf); return; }
        pcm_ptr  = (const unsigned char*)tmp_buf + si2.data_offset;
        pcm_size = si2.num_samples * si2.channels * (si2.bit_depth / 8);
        al_fmt   = wav_al_format(si2.channels, si2.bit_depth);
        al_rate  = si2.sample_rate;
    } else {
        pcm_ptr  = (const unsigned char*)data + si.data_offset;
        pcm_size = si.num_samples * si.channels * (si.bit_depth / 8);
        al_fmt   = wav_al_format(si.channels, si.bit_depth);
    }

    if (!pcm_ptr || pcm_size <= 0) { if (tmp_buf) free(tmp_buf); return; }

    if (smp->buffer) {
        alSourcei(smp->source, AL_BUFFER, 0);
        alDeleteBuffers(1, &smp->buffer);
        smp->buffer = 0;
    }
    alGenBuffers(1, &smp->buffer);
    alBufferData(smp->buffer, al_fmt, pcm_ptr, pcm_size, al_rate);
    alSourcei(smp->source, AL_BUFFER, (ALint)smp->buffer);
    apply_gain_pan(smp->source, smp->volume, smp->pan);

    if (tmp_buf) free(tmp_buf);
}

void OAL_start_sample(HSAMPLE s)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    smp->pending_eos = false;
    /* For 2D sounds: initFilters is called BEFORE AIL_set_sample_file, so
       OAL_set_sample_playback_rate couldn't apply pitch (no buffer yet).
       The desired rate (22050 * pitchShift) was stored in smp->sample_rate.
       Now that the buffer is loaded we compute and apply pitch here.
       For 3D sounds: pitch was already set correctly by OAL_set_sample_playback_rate
       (called from initFilters3D after the buffer was loaded), so we skip this. */
    if (!smp->is_3d && smp->buffer && smp->sample_rate > 0) {
        ALint bufRate = 0;
        alGetBufferi(smp->buffer, AL_FREQUENCY, &bufRate);
        if (bufRate > 0)
            alSourcef(smp->source, AL_PITCH, (float)smp->sample_rate / (float)bufRate);
    }
    alSourcePlay(smp->source);
}

void OAL_pause_sample(HSAMPLE s)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    ALint state = AL_STOPPED;
    alGetSourcei(smp->source, AL_SOURCE_STATE, &state);
    if (state == AL_PLAYING) alSourcePause(smp->source);
}

void OAL_stop_sample(HSAMPLE s)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    alSourceStop(smp->source);
}

void OAL_resume_sample(HSAMPLE s)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    ALint state = AL_STOPPED;
    alGetSourcei(smp->source, AL_SOURCE_STATE, &state);
    if (state == AL_PAUSED) alSourcePlay(smp->source);
}

void OAL_set_sample_volume(HSAMPLE s, float vol)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    smp->volume = vol;
    if (smp->is_3d) {
        alSourcef(smp->source, AL_GAIN, vol < 0.0f ? 0.0f : vol);
    } else {
        apply_gain_pan(smp->source, smp->volume, smp->pan);
    }
}

void OAL_set_sample_pan(HSAMPLE s, float pan)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp || smp->is_3d) return;  /* pan has no meaning in 3D mode */
    smp->pan = pan;
    apply_gain_pan(smp->source, smp->volume, smp->pan);
}

void OAL_set_sample_volume_pan(HSAMPLE s, float vol, float pan)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    smp->volume = vol;
    if (smp->is_3d) {
        alSourcef(smp->source, AL_GAIN, vol < 0.0f ? 0.0f : vol);
    } else {
        smp->pan = pan;
        apply_gain_pan(smp->source, smp->volume, smp->pan);
    }
}

void OAL_sample_volume_pan(HSAMPLE s, float *volOut, float *panOut)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) { if (panOut) *panOut = 0.5f; return; }
    if (volOut) *volOut = smp->volume;
    if (panOut) *panOut = smp->pan;
}

void OAL_set_sample_playback_rate(HSAMPLE s, int rate)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp || rate <= 0) return;
    smp->sample_rate = rate;
    /* Adjust pitch relative to buffer's original rate */
    ALint bufRate = 0;
    if (smp->buffer) alGetBufferi(smp->buffer, AL_FREQUENCY, &bufRate);
    if (bufRate > 0) alSourcef(smp->source, AL_PITCH, (float)rate / (float)bufRate);
}

int OAL_sample_playback_rate(HSAMPLE s)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return 22050;
    /* When a buffer is loaded, return its actual native sample rate.
       initFilters3D calls this AFTER loading the buffer, so returning the
       real buffer rate lets the pitch multiplier work at the correct base:
         new_rate = buf_rate * pitchShift → pitch = new_rate / buf_rate = pitchShift  */
    if (smp->buffer) {
        ALint freq = 0;
        alGetBufferi(smp->buffer, AL_FREQUENCY, &freq);
        if (freq > 0) return (int)freq;
    }
    return smp->sample_rate > 0 ? smp->sample_rate : 22050;
}

void OAL_register_EOS_callback(HSAMPLE s, AILEOSCALLBACK cb)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp) return;
    smp->eos_callback = cb;
    if (!cb) smp->pending_eos = false;
}

void OAL_set_sample_user_data(HSAMPLE s, int index, intptr_t value)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp || index < 0 || index >= OAL_USER_DATA_SLOTS) return;
    smp->user_data[index] = value;
}

intptr_t OAL_sample_user_data(HSAMPLE s, int index)
{
    OALSample *smp = smp_from_handle(s);
    if (!smp || index < 0 || index >= OAL_USER_DATA_SLOTS) return 0;
    return smp->user_data[index];
}

void OAL_set_sample_processor(HSAMPLE /*s*/, int /*type*/, HPROVIDER /*p*/) {}
void OAL_set_filter_sample_preference(HSAMPLE /*s*/, const char * /*pref*/, const void * /*v*/) {}

/* --- Streaming ----------------------------------------------------------- */

#define OAL_SEEK_FROM_START 0

static DWORD WINAPI stream_thread(LPVOID param)
{
    OALStream *st = (OALStream*)param;

    if (st->use_mci) {
        if (st->mci_alias[0] == '\0') {
            _snprintf_s(st->mci_alias, sizeof(st->mci_alias), _TRUNCATE, "zhmci_%u", (unsigned int)(uintptr_t)st->handle);
        }

        char cmd[2 * MAX_PATH + 64];
        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "open \"%s\" type mpegvideo alias %s", st->temp_path, st->mci_alias);
        if (mciSendStringA(cmd, nullptr, 0, nullptr) != 0) {
            st->should_stop = true;
            return 0;
        }

        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "set %s time format milliseconds", st->mci_alias);
        mciSendStringA(cmd, nullptr, 0, nullptr);

        char buffer[64] = { 0 };
        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "status %s length", st->mci_alias);
        if (mciSendStringA(cmd, buffer, sizeof(buffer), nullptr) == 0) {
            st->total_ms = atol(buffer);
        }

        oal_apply_mci_volume(st);
        _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "play %s from 0", st->mci_alias);
        mciSendStringA(cmd, nullptr, 0, nullptr);

        while (!st->should_stop) {
            WaitForSingleObject(st->wake_event, 20);

            if (st->should_stop) {
                break;
            }

            if (st->is_paused) {
                continue;
            }

            _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "status %s position", st->mci_alias);
            if (mciSendStringA(cmd, buffer, sizeof(buffer), nullptr) == 0) {
                st->current_ms = atol(buffer);
            }

            char mode[32] = { 0 };
            _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "status %s mode", st->mci_alias);
            if (mciSendStringA(cmd, mode, sizeof(mode), nullptr) != 0) {
                st->should_stop = true;
                break;
            }

            if (_stricmp(mode, "playing") != 0 && _stricmp(mode, "seeking") != 0) {
                bool will_loop = (st->loop_count != 0);
                if (st->loop_count > 0) {
                    --st->loop_count;
                }

                if (will_loop) {
                    _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "seek %s to start", st->mci_alias);
                    mciSendStringA(cmd, nullptr, 0, nullptr);
                    _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "play %s from 0", st->mci_alias);
                    mciSendStringA(cmd, nullptr, 0, nullptr);
                    continue;
                }

                if (st->stream_callback) {
                    st->stream_callback(st->handle);
                }

                st->should_stop = true;
                break;
            }
        }

        oal_close_mci_stream(st);
        return 0;
    }

    unsigned char tmp[OAL_STREAM_BUF_BYTES];

    /* Read one chunk and return bytes actually read */
    auto read_chunk = [&](ALuint alBuf, bool *at_end) -> ALsizei {
        if (st->data_remaining <= 0) { *at_end = true; return 0; }
        int want = OAL_STREAM_BUF_BYTES;
        if (want > st->data_remaining) want = (int)st->data_remaining;
        ALsizei got = 0;
        if (st->decoded_pcm) {
            /* In-memory mode: serve bytes directly from the decompressed PCM buffer */
            alBufferData(alBuf, st->al_format,
                         st->decoded_pcm + st->decoded_pcm_pos,
                         (ALsizei)want, (ALsizei)st->sample_rate);
            st->decoded_pcm_pos += want;
            got = (ALsizei)want;
        } else if (g_cbRead) {
            /* File mode: read via VFS callback */
            U32 n = g_cbRead(st->file_handle, tmp, (U32)want);
            if (n > 0) alBufferData(alBuf, st->al_format, tmp, (ALsizei)n, (ALsizei)st->sample_rate);
            got = (ALsizei)n;
        }
        st->data_remaining -= (long)got;
        if (st->data_remaining <= 0) *at_end = true;
        return got;
    };

    /* Fill and queue initial buffers */
    for (int i = 0; i < OAL_NUM_STREAM_BUFS; ++i) {
        bool at_end = false;
        ALsizei got = read_chunk(st->bufs[i], &at_end);
        if (got > 0) alSourceQueueBuffers(st->source, 1, &st->bufs[i]);
        if (at_end) break;
    }

    alSourcePlay(st->source);

    while (!st->should_stop) {
        WaitForSingleObject(st->wake_event, 20);   /* ~50 Hz poll rate */

        if (st->should_stop) break;
        if (st->is_paused) continue;

        /* Check if the source stalled (underrun) */
        ALint state = AL_STOPPED;
        alGetSourcei(st->source, AL_SOURCE_STATE, &state);

        ALint processed = 0;
        alGetSourcei(st->source, AL_BUFFERS_PROCESSED, &processed);

        while (processed-- > 0) {
            ALuint dequed = 0;
            alSourceUnqueueBuffers(st->source, 1, &dequed);

            bool at_end = false;
            ALsizei got = read_chunk(dequed, &at_end);

            if (got > 0) {
                alSourceQueueBuffers(st->source, 1, &dequed);
            }

            if (at_end) {
                /* End of file: decide whether to loop */
                bool will_loop = (st->loop_count != 0);
                if (st->loop_count > 0) --st->loop_count;

                if (will_loop) {
                    /* Rewind back to the start of PCM data */
                    if (st->decoded_pcm) {
                        st->decoded_pcm_pos = st->data_offset;  /* rewind in-memory ptr */
                    } else if (g_cbSeek) {
                        g_cbSeek(st->file_handle, (S32)st->data_offset, OAL_SEEK_FROM_START);
                    }
                    st->data_remaining = st->data_size;
                    /* Only refill 'dequed' immediately if it was NOT already queued above
                       (i.e. got == 0 means read_chunk returned nothing and skipped queuing).
                       If got > 0 the buffer was already submitted; writing to it again while
                       it is in OpenAL's queue is undefined behaviour.  The next processed
                       buffer will naturally read from the rewound position. */
                    if (got == 0) {
                        got = read_chunk(dequed, &at_end);
                        if (got > 0) alSourceQueueBuffers(st->source, 1, &dequed);
                    }
                } else {
                    /* Notify completion via stream callback */
                    if (st->stream_callback) {
                        st->stream_callback(st->handle);
                    }
                    st->should_stop = true;
                    break;
                }
            }
        }

        /* Keep source playing if it stalled and has queued buffers */
        if (state == AL_STOPPED && !st->should_stop) {
            ALint queued = 0;
            alGetSourcei(st->source, AL_BUFFERS_QUEUED, &queued);
            if (queued > 0) alSourcePlay(st->source);
        }

        /* Update current position */
        if (st->total_ms > 0) {
            long consumed = st->data_size - st->data_remaining;
            int bps = st->sample_rate * st->channels * st->bytes_per_sample;
            if (bps > 0) st->current_ms = (long)((consumed * 1000LL) / bps);
        }
    }

    alSourceStop(st->source);

    /* Drain any queued buffers */
    ALint queued = 0;
    alGetSourcei(st->source, AL_BUFFERS_QUEUED, &queued);
    while (queued-- > 0) {
        ALuint dequed = 0;
        alSourceUnqueueBuffers(st->source, 1, &dequed);
    }

    return 0;
}

HSTREAM OAL_open_stream(void * /*driver*/, const char *filename, int /*flags*/)
{
    if (!g_alDevice || !filename || !g_cbOpen || !g_cbRead) return nullptr;

    /* Open file via VFS callbacks */
    U32 fh = 0;
    if (!g_cbOpen(filename, &fh) || fh == 0) return nullptr;

    /* Read the WAV header to get format and data offset */
    /* Read first 4 KB which should cover any WAV header */
    enum { HDR_MAX = 4096 };
    unsigned char hdr_buf[HDR_MAX];
    memset(hdr_buf, 0, HDR_MAX);
    if (g_cbRead) g_cbRead(fh, hdr_buf, HDR_MAX);

    AILSOUNDINFO si; memset(&si, 0, sizeof(si));
    si._oal_src_buf = hdr_buf;
    OAL_WAV_info(hdr_buf, &si);

    if (oal_is_mp3_data(hdr_buf, HDR_MAX)) {
        if (!g_cbSeek || !g_cbRead) {
            if (g_cbClose) g_cbClose(fh);
            return nullptr;
        }

        g_cbSeek(fh, 0, OAL_SEEK_FROM_START);

        std::vector<unsigned char> fileBytes;
        unsigned char chunk[OAL_STREAM_BUF_BYTES];
        for (;;) {
            U32 got = g_cbRead(fh, chunk, (U32)sizeof(chunk));
            if (got == 0) {
                break;
            }

            fileBytes.insert(fileBytes.end(), chunk, chunk + got);
            if (got < (U32)sizeof(chunk)) {
                break;
            }
        }

        if (g_cbClose) g_cbClose(fh);
        fh = 0;

        if (fileBytes.empty()) {
            return nullptr;
        }

        char tempDir[MAX_PATH] = { 0 };
        char tempPath[MAX_PATH] = { 0 };
        if (!GetTempPathA(MAX_PATH, tempDir) || !GetTempFileNameA(tempDir, "zhm", 0, tempPath)) {
            return nullptr;
        }

        DeleteFileA(tempPath);
        const char *ext = strrchr(filename, '.');
        char *dot = strrchr(tempPath, '.');
        if (dot && ext && *ext) {
            strcpy_s(dot, MAX_PATH - (dot - tempPath), ext);
        }

        HANDLE tempFile = CreateFileA(tempPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (tempFile == INVALID_HANDLE_VALUE) {
            return nullptr;
        }

        DWORD written = 0;
        BOOL ok = WriteFile(tempFile, &fileBytes[0], (DWORD)fileBytes.size(), &written, nullptr);
        CloseHandle(tempFile);
        if (!ok || written != (DWORD)fileBytes.size()) {
            DeleteFileA(tempPath);
            return nullptr;
        }

        OALStream *st = new OALStream();
        memset(st, 0, sizeof(*st));
        st->valid       = true;
        st->volume      = 1.0f;
        st->pan         = 0.5f;
        st->loop_count  = 0;
        st->current_ms  = 0;
        st->total_ms    = 0;
        st->use_mci     = true;
        strcpy_s(st->temp_path, tempPath);
        st->wake_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        HSTREAM h = stream_alloc_slot(st);
        if (!h) {
            if (st->wake_event) CloseHandle(st->wake_event);
            DeleteFileA(st->temp_path);
            delete st;
            return nullptr;
        }

        return h;
    }

    /* ----- ADPCM: decompress entire file to PCM, then stream from memory ----- */
    if (si.format == WAVE_FORMAT_IMA_ADPCM && si._oal_spb > 0 && si._oal_block_align > 0) {
        long num_blocks    = ((long)si.num_samples + si._oal_spb - 1) / si._oal_spb;
        long adpcm_data_sz = num_blocks * (long)si._oal_block_align;
        long full_file_sz  = (long)si.data_offset + adpcm_data_sz;

        unsigned char *file_buf = (unsigned char*)malloc((size_t)full_file_sz);
        if (!file_buf) { if (g_cbClose) g_cbClose(fh); return nullptr; }

        /* hdr_buf already holds the first HDR_MAX bytes; copy and read the rest */
        long hdr_copy = (full_file_sz < (long)HDR_MAX) ? full_file_sz : (long)HDR_MAX;
        memcpy(file_buf, hdr_buf, (size_t)hdr_copy);
        if (full_file_sz > (long)HDR_MAX && g_cbSeek && g_cbRead) {
            g_cbSeek(fh, (S32)HDR_MAX, OAL_SEEK_FROM_START);
            g_cbRead(fh, file_buf + HDR_MAX, (U32)(full_file_sz - HDR_MAX));
        }
        if (g_cbClose) g_cbClose(fh);
        fh = 0;

        /* Decompress ADPCM → PCM WAV in memory */
        AILSOUNDINFO si_adpcm = si;
        si_adpcm._oal_src_buf = file_buf;
        U32   pcm_wav_size = 0;
        void *pcm_wav_buf  = nullptr;
        OAL_decompress_ADPCM(&si_adpcm, &pcm_wav_buf, &pcm_wav_size);
        free(file_buf);

        if (!pcm_wav_buf) return nullptr;

        /* Re-parse the decoded PCM WAV to find its sample data region */
        AILSOUNDINFO si_pcm; memset(&si_pcm, 0, sizeof(si_pcm));
        si_pcm._oal_src_buf = pcm_wav_buf;
        OAL_WAV_info(pcm_wav_buf, &si_pcm);

        if (si_pcm.format != WAVE_FORMAT_PCM || si_pcm.sample_rate == 0) {
            free(pcm_wav_buf); return nullptr;
        }

        long pcm_data_sz  = (long)si_pcm.num_samples * si_pcm.channels * (si_pcm.bit_depth / 8);
        long bps_out      = si_pcm.sample_rate * si_pcm.channels * (si_pcm.bit_depth / 8);
        long total_ms_out = (bps_out > 0) ? (long)((pcm_data_sz * 1000LL) / bps_out) : 0;

        OALStream *st = new OALStream();
        memset(st, 0, sizeof(*st));
        alGenSources(1, &st->source);
        alGenBuffers(OAL_NUM_STREAM_BUFS, st->bufs);
        alSourcei(st->source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(st->source, AL_POSITION, 0.0f, 0.0f, -1.0f);

        st->valid            = true;
        st->channels         = si_pcm.channels;
        st->sample_rate      = si_pcm.sample_rate;
        st->bytes_per_sample = si_pcm.bit_depth / 8;
        st->al_format        = wav_al_format(si_pcm.channels, si_pcm.bit_depth);
        st->file_handle      = 0;               /* data is in decoded_pcm           */
        st->decoded_pcm      = (unsigned char*)pcm_wav_buf;
        st->decoded_pcm_pos  = (long)si_pcm.data_offset;
        st->data_offset      = (long)si_pcm.data_offset;  /* for loop rewind        */
        st->data_size        = pcm_data_sz;
        st->data_remaining   = pcm_data_sz;
        st->volume           = 1.0f;
        st->pan              = 0.5f;
        st->loop_count       = 0;
        st->total_ms         = total_ms_out;
        st->current_ms       = 0;
        st->should_stop      = false;
        st->is_paused        = false;
        st->wake_event       = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        /* Register in indirection table and return a stable 1-based handle */
        HSTREAM h = stream_alloc_slot(st);
        if (!h) {
            CloseHandle(st->wake_event);
            free(st->decoded_pcm);
            alDeleteSources(1, &st->source);
            alDeleteBuffers(OAL_NUM_STREAM_BUFS, st->bufs);
            delete st;
            return nullptr;
        }
        return h;
    }

    /* ----- reject non-PCM, non-ADPCM formats --------------------------------- */
    if (si.format != WAVE_FORMAT_PCM || si.sample_rate == 0) {
        if (g_cbClose) g_cbClose(fh);
        return nullptr;
    }

    /* Seek to the data chunk start for streaming */
    if (g_cbSeek) g_cbSeek(fh, (S32)si.data_offset, OAL_SEEK_FROM_START);

    long bps      = si.sample_rate * si.channels * (si.bit_depth / 8);
    long data_sz  = (long)si.num_samples * si.channels * (si.bit_depth / 8);
    long total_ms = (bps > 0) ? (long)((data_sz * 1000LL) / bps) : 0;

    OALStream *st = new OALStream();
    memset(st, 0, sizeof(*st));
    alGenSources(1, &st->source);
    alGenBuffers(OAL_NUM_STREAM_BUFS, st->bufs);

    alSourcei(st->source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(st->source, AL_POSITION, 0.0f, 0.0f, -1.0f);

    st->valid            = true;
    st->channels         = si.channels;
    st->sample_rate      = si.sample_rate;
    st->bytes_per_sample = si.bit_depth / 8;
    st->al_format        = wav_al_format(si.channels, si.bit_depth);
    st->file_handle      = fh;
    st->decoded_pcm      = nullptr;    /* PCM file: no in-memory buffer          */
    st->decoded_pcm_pos  = 0;
    st->data_offset      = si.data_offset;
    st->data_size        = data_sz;
    st->data_remaining   = data_sz;
    st->volume           = 1.0f;
    st->pan              = 0.5f;
    st->loop_count       = 0;    /* caller sets this via AIL_set_stream_loop_count */
    st->total_ms         = total_ms;
    st->current_ms       = 0;
    st->should_stop      = false;
    st->is_paused        = false;

    st->wake_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    /* Register in indirection table and return a stable 1-based handle */
    HSTREAM h = stream_alloc_slot(st);
    if (!h) {
        CloseHandle(st->wake_event);
        if (g_cbClose && st->file_handle) g_cbClose(st->file_handle);
        alDeleteSources(1, &st->source);
        alDeleteBuffers(OAL_NUM_STREAM_BUFS, st->bufs);
        delete st;
        return nullptr;
    }
    return h;
}

void OAL_start_stream(HSTREAM s)
{
    if (!s) return;
    OALStream *st = stream_from_handle(s);
    if (!st || !st->valid) return;

    /* Apply initial volume */
    if (!st->use_mci) {
        alSourcef(st->source, AL_GAIN, st->volume < 0.0f ? 0.0f : st->volume);
    }

    st->thread = CreateThread(nullptr, 0, stream_thread, st, 0, nullptr);
}

void OAL_close_stream(HSTREAM s)
{
    if (!s) return;
    OALStream *st = stream_from_handle(s);
    if (!st) return;

    /* Free the table slot immediately so the handle can be reused while we
       wait for the thread to exit.  The OALStream itself lives until delete. */
    stream_free_slot(s);

    st->should_stop = true;
    if (st->wake_event) SetEvent(st->wake_event);

    if (st->thread) {
        /* The stream thread should exit within one poll cycle (~20 ms) after
           being signalled.  Allow 200 ms so a slow in-flight read_chunk can
           drain, but do NOT block the main thread for the old 3-second
           timeout — that was the root cause of the options-menu freeze. */
        WaitForSingleObject(st->thread, 200);
        CloseHandle(st->thread);
        st->thread = nullptr;
    }
    if (st->wake_event) { CloseHandle(st->wake_event); st->wake_event = nullptr; }
    if (st->decoded_pcm) { free(st->decoded_pcm); st->decoded_pcm = nullptr; }
    if (g_cbClose && st->file_handle) g_cbClose(st->file_handle);
    if (st->use_mci) {
        oal_close_mci_stream(st);
    }
    if (st->source)  { alDeleteSources(1, &st->source); st->source = 0; }
    alDeleteBuffers(OAL_NUM_STREAM_BUFS, st->bufs);
    memset(st->bufs, 0, sizeof(st->bufs));

    st->valid = false;
    delete st;
}

void OAL_pause_stream(HSTREAM s, int pause_flag)
{
    if (!s) return;
    OALStream *st = stream_from_handle(s);
    if (!st) return;
    if (pause_flag) {
        st->is_paused = true;
        if (st->use_mci && st->mci_alias[0] != '\0') {
            char cmd[96];
            _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "pause %s", st->mci_alias);
            mciSendStringA(cmd, nullptr, 0, nullptr);
        } else {
            alSourcePause(st->source);
        }
    } else {
        st->is_paused = false;
        if (st->use_mci && st->mci_alias[0] != '\0') {
            char cmd[96];
            _snprintf_s(cmd, sizeof(cmd), _TRUNCATE, "resume %s", st->mci_alias);
            mciSendStringA(cmd, nullptr, 0, nullptr);
        } else {
            ALint state = AL_STOPPED;
            alGetSourcei(st->source, AL_SOURCE_STATE, &state);
            if (state == AL_PAUSED) alSourcePlay(st->source);
        }
        if (st->wake_event) SetEvent(st->wake_event);
    }
}

void OAL_set_stream_volume_pan(HSTREAM s, float vol, float pan)
{
    if (!s) return;
    OALStream *st = stream_from_handle(s);
    if (!st) return;
    st->volume = vol; st->pan = pan;
    if (st->use_mci) {
        oal_apply_mci_volume(st);
    } else {
        alSourcef(st->source, AL_GAIN, vol < 0.0f ? 0.0f : vol);
    }
}

void OAL_stream_volume_pan(HSTREAM s, float *volOut, float *panOut)
{
    if (!s) { if (panOut) *panOut = 0.5f; return; }
    OALStream *st = stream_from_handle(s);
    if (!st) { if (panOut) *panOut = 0.5f; return; }
    if (volOut) *volOut = st->volume;
    if (panOut) *panOut = st->pan;
}

void OAL_set_stream_loop_count(HSTREAM s, int count)
{
    if (!s) return;
    OALStream *st = stream_from_handle(s);
    if (st) st->loop_count = count;
}

int OAL_stream_loop_count(HSTREAM s)
{
    if (!s) return 0;
    OALStream *st = stream_from_handle(s);
    return st ? st->loop_count : 0;
}

void OAL_register_stream_callback(HSTREAM s, AILSTREAM_CALLBACK cb)
{
    if (!s) return;
    OALStream *st = stream_from_handle(s);
    if (st) st->stream_callback = cb;
}

void OAL_stream_ms_position(HSTREAM s, long *curMs, long *totMs)
{
    if (!s) {
        if (curMs) *curMs = 0;
        if (totMs) *totMs = 0;
        return;
    }
    OALStream *st = stream_from_handle(s);
    if (!st) {
        if (curMs) *curMs = 0;
        if (totMs) *totMs = 0;
        return;
    }
    if (curMs) *curMs = st->current_ms;
    if (totMs) *totMs = st->total_ms;
}

/* --- 3-D provider / listener --------------------------------------------- */

int OAL_enumerate_3D_providers(HPROENUM *next, HPROVIDER *provOut, char **nameOut)
{
    if (!next || !provOut || !nameOut) return 0;
    int idx = (int)(intptr_t)*next;
    if (idx >= g_providerCount || !g_providerNames[idx]) return 0;

    /* Use a small integer as the provider ID (non-NULL for validity checks) */
    *provOut  = (HPROVIDER)(intptr_t)(idx + 1);
    *nameOut  = const_cast<char*>(g_providerNames[idx]);
    *next     = (HPROENUM)(intptr_t)(idx + 1);
    return 1;
}

H3DPOBJECT OAL_open_3D_listener(HPROVIDER /*provider*/)
{
    /* Return a non-NULL fake pointer so m_listener != NULL */
    static int fake_listener = 0;
    return (H3DPOBJECT)&fake_listener;
}

void OAL_close_3D_listener(H3DPOBJECT /*listener*/) {}

void OAL_set_3D_orientation(H3DPOBJECT /*listener*/,
                              float fx, float fy, float fz,
                              float upx, float upy, float upz)
{
    /* Called every frame from setDeviceListenerPosition() — perfect place to
       poll sample completions and drain pending EOS callbacks. */
    oal_poll_sample_completions();
    oal_drain_eos_queue();
    /* Update AL listener orientation so 3D sounds pan correctly */
    ALfloat orient[6] = { fx, fy, fz, upx, upy, upz };
    alListenerfv(AL_ORIENTATION, orient);
}

/* -------------------------------------------------------------------------
   3-D positional sample functions.
   Rather than a separate sample pool, 3D samples share the same 64-slot pool
   as 2D samples (OAL_allocate_sample / index-based handle).  Position and
   distance are stored per-slot in g_sample3D[].
   ------------------------------------------------------------------------- */

HSAMPLE OAL_allocate_3D_sample(HPROVIDER /*prov*/)
{
    return OAL_allocate_sample(nullptr);
}

void OAL_release_3D_sample(HSAMPLE s)
{
    uintptr_t i = (uintptr_t)s - 1;
    if (i < OAL_MAX_SAMPLES) {
        OALSample *smp = &g_samples[i];
        if (smp->source) alSourcei(smp->source, AL_SOURCE_RELATIVE, AL_TRUE);
        g_sample3D[i] = OALSample3DPos{};
    }
    OAL_release_sample(s);
}

void OAL_set_3D_sample_file(HSAMPLE s, const void *buf)
{
    OAL_set_sample_file(s, buf, 0);
}

void OAL_register_3D_EOS_callback(HSAMPLE s, AILEOSCALLBACK cb)
{
    OAL_register_EOS_callback(s, cb);
}

/* OAL_set_3D_sample_position — dispatches on handle value:
 *   1..OAL_MAX_SAMPLES → 3-D sound source at world position (x,y,z)
 *   anything else      → listener position (handle is H3DPOBJECT &fake_listener) */
void OAL_set_3D_sample_position(void *handle, float x, float y, float z)
{
    uintptr_t h = (uintptr_t)handle;
    if (h >= 1 && h <= (uintptr_t)OAL_MAX_SAMPLES) {
        int i = (int)h - 1;
        OALSample *smp = &g_samples[i];
        if (!smp->in_use) return;
        g_sample3D[i].x = x;
        g_sample3D[i].y = y;
        g_sample3D[i].z = z;
        smp->is_3d = true;
        oal_update_3d_source(smp, i);
    } else {
        /* Listener position */
        g_listenerX = x; g_listenerY = y; g_listenerZ = z;
        alListener3f(AL_POSITION, x, y, z);
    }
}

void OAL_set_3D_sample_distances(HSAMPLE s, float minDist, float maxDist)
{
    uintptr_t i = (uintptr_t)s - 1;
    if (i >= OAL_MAX_SAMPLES) return;
    g_sample3D[i].min_dist = minDist;
    g_sample3D[i].max_dist = maxDist;
    OALSample *smp = &g_samples[i];
    if (smp->in_use && smp->source) {
        alSourcef(smp->source, AL_REFERENCE_DISTANCE, minDist > 0.0f ? minDist : 1.0f);
        alSourcef(smp->source, AL_MAX_DISTANCE,       maxDist > 0.0f ? maxDist : 1000.0f);
        alSourcef(smp->source, AL_ROLLOFF_FACTOR, 1.0f);
    }
}

int OAL_enumerate_filters(HPROENUM *next, HPROVIDER *provOut, char **nameOut)
{
    (void)next; (void)provOut; (void)nameOut;
    return 0;   /* no filters */
}

/* --- Quick-play (mission briefings) -------------------------------------- */

HAUDIO OAL_quick_load_and_play(const char *filename, int loops, int /*wait*/)
{
    if (!g_alDevice || !filename || !g_cbOpen) return nullptr;

    /* Find a free quick-audio slot */
    int slot = -1;
    for (int i = 0; i < MAX_QUICK_AUDIO; ++i) {
        if (!g_quickAudio[i].source) { slot = i; break; }
        ALint state = AL_STOPPED;
        alGetSourcei(g_quickAudio[i].source, AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED) {
            alSourcei(g_quickAudio[i].source, AL_BUFFER, 0);
            alDeleteBuffers(1, &g_quickAudio[i].buffer);
            g_quickAudio[i].buffer = 0;
            slot = i; break;
        }
    }
    if (slot < 0) return nullptr;

    /* Open and read via VFS */
    U32 fh = 0;
    if (!g_cbOpen(filename, &fh)) return nullptr;

    /* Read entire file */
    unsigned char hdr[8];
    U32 riff_size = 0;
    if (g_cbRead(fh, hdr, 8) < 8) { g_cbClose(fh); return nullptr; }
    if (memcmp(hdr, "RIFF", 4) != 0) { g_cbClose(fh); return nullptr; }
    riff_size = *(DWORD*)(hdr + 4);

    unsigned char *buf = (unsigned char*)malloc(riff_size + 8);
    if (!buf) { g_cbClose(fh); return nullptr; }
    memcpy(buf, hdr, 8);
    g_cbRead(fh, buf + 8, riff_size);
    g_cbClose(fh);

    AILSOUNDINFO si; memset(&si, 0, sizeof(si));
    si._oal_src_buf = buf;
    OAL_WAV_info(buf, &si);
    if (si.format != WAVE_FORMAT_PCM) { free(buf); return nullptr; }

    const void *pcm = buf + si.data_offset;
    int pcm_size    = si.num_samples * si.channels * (si.bit_depth / 8);
    ALenum al_fmt   = wav_al_format(si.channels, si.bit_depth);

    if (!g_quickAudio[slot].source) alGenSources(1, &g_quickAudio[slot].source);
    alGenBuffers(1, &g_quickAudio[slot].buffer);
    alBufferData(g_quickAudio[slot].buffer, al_fmt, pcm, pcm_size, si.sample_rate);
    alSourcei(g_quickAudio[slot].source, AL_BUFFER, (ALint)g_quickAudio[slot].buffer);
    alSourcei(g_quickAudio[slot].source, AL_LOOPING, (loops != 1) ? AL_TRUE : AL_FALSE);
    alSourcef(g_quickAudio[slot].source, AL_GAIN, 1.0f);
    alSourcei(g_quickAudio[slot].source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(g_quickAudio[slot].source, AL_POSITION, 0.0f, 0.0f, -1.0f);
    alSourcePlay(g_quickAudio[slot].source);

    free(buf);
    return (HAUDIO)(intptr_t)(slot + 1);   /* 1-based so non-NULL */
}

void OAL_quick_set_volume(HAUDIO audio, float vol, float /*pan*/)
{
    if (!audio) return;
    int slot = (int)(intptr_t)audio - 1;
    if (slot < 0 || slot >= MAX_QUICK_AUDIO) return;
    alSourcef(g_quickAudio[slot].source, AL_GAIN, vol < 0.0f ? 0.0f : vol);
}

void OAL_quick_unload(HAUDIO audio)
{
    if (!audio) return;
    int slot = (int)(intptr_t)audio - 1;
    if (slot < 0 || slot >= MAX_QUICK_AUDIO) return;
    alSourceStop(g_quickAudio[slot].source);
    alSourcei(g_quickAudio[slot].source, AL_BUFFER, 0);
    if (g_quickAudio[slot].buffer) {
        alDeleteBuffers(1, &g_quickAudio[slot].buffer);
        g_quickAudio[slot].buffer = 0;
    }
}

} /* extern "C" */
