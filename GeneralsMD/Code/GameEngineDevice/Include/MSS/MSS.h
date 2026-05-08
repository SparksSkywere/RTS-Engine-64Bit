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

/**
 * @file MSS.h
 * @brief OpenAL Soft audio backend replacing the Miles Sound System (MSS) API.
 *
 * The AIL_* macros are replaced with calls to OAL_* functions implemented in
 * OpenALImpl.cpp, which drives OpenAL Soft for full 64-bit audio support.
 */

#pragma once

#ifndef __MSS_H__
#define __MSS_H__

#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type definitions */
typedef void* HSAMPLE;
typedef void* H3DSAMPLE;
typedef void* HSTREAM;
typedef void* HDIGDRIVER;
typedef unsigned int U32;
typedef int S32;
typedef float F32;
typedef void* HPROVIDER;
typedef void* H3DPOBJECT;
typedef void* HAUDIO;
typedef void* HPROENUM;
typedef LPDIRECTSOUND AILLPDIRECTSOUND;

#ifndef NULL
#define NULL 0
#endif

#define HPROENUM_FIRST ((HPROENUM)0)

#define DP_FILTER 0

#define AIL_3D_2_SPEAKER 2
#define AIL_3D_HEADPHONE 3
#define AIL_3D_4_SPEAKER 4
#define AIL_3D_SURROUND 5
#define AIL_3D_51_SPEAKER 6
#define AIL_3D_71_SPEAKER 7

/* MSS Sound Information Structure.
   The _oal_* fields are used internally by OpenALImpl.cpp and are never
   read by game code (which only checks the .format field). */
typedef struct {
	int   format;           /* WAVE_FORMAT_PCM=1, WAVE_FORMAT_IMA_ADPCM=0x11 */
	int   data_offset;      /* byte offset to PCM/ADPCM data inside the file buffer */
	int   num_samples;      /* total PCM samples (per channel) */
	int   sample_rate;
	int   channels;
	int   bit_depth;
	int   flags;
	/* internal fields used by OAL_WAV_info / OAL_decompress_ADPCM */
	void *_oal_src_buf;     /* pointer to the original full WAV file buffer */
	int   _oal_block_align; /* bytes per ADPCM block (blockAlign from fmt chunk) */
	int   _oal_spb;         /* samples per ADPCM block */
} AILSOUNDINFO;

/* Callback definitions */
#define AILCALLBACK __cdecl
typedef void (AILCALLBACK *AILEOSCALLBACK)(HSAMPLE S);
typedef void (AILCALLBACK *AILSTREAM_CALLBACK)(HSTREAM stream);

typedef U32 (AILCALLBACK *AIL_FILE_OPEN_CALLBACK)(char const *fileName, U32 *file_handle);
typedef void (AILCALLBACK *AIL_FILE_CLOSE_CALLBACK)(U32 fileHandle);
typedef S32 (AILCALLBACK *AIL_FILE_SEEK_CALLBACK)(U32 fileHandle, S32 offset, U32 type);
typedef U32 (AILCALLBACK *AIL_FILE_READ_CALLBACK)(U32 fileHandle, void *buffer, U32 bytes);

/* -------------------------------------------------------------------------
   OAL_ function declarations (implemented in OpenALImpl.cpp).
   These provide a real 64-bit audio backend via OpenAL Soft.
   ------------------------------------------------------------------------- */

/* System */
void OAL_startup(void);
void OAL_shutdown(void);
int  OAL_quick_startup(int useDigital, int useMidi, int outputRate, int outputBits, int outputChannels);
void OAL_quick_handles(void **digitalHandlePtr, void **midiHandlePtr, void **dlsHandlePtr);
void OAL_set_file_callbacks(AIL_FILE_OPEN_CALLBACK open_cb, AIL_FILE_CLOSE_CALLBACK close_cb, AIL_FILE_SEEK_CALLBACK seek_cb, AIL_FILE_READ_CALLBACK read_cb);
void OAL_MSS_version(char *buffer, int size);
int  OAL_get_timer_highest_delay(void);

/* WAV / ADPCM */
void OAL_WAV_info(const void *buffer, AILSOUNDINFO *soundInfoOut);
void OAL_decompress_ADPCM(const AILSOUNDINFO *soundInfo, void **outBuffer, U32 *outSize);
void OAL_mem_free_lock(void *ptr);

/* 2-D sample management */
HSAMPLE  OAL_allocate_sample(void *driver);
void     OAL_release_sample(HSAMPLE s);
void     OAL_init_sample(HSAMPLE s);
void     OAL_set_sample_file(HSAMPLE s, const void *data, int block);
void     OAL_start_sample(HSAMPLE s);
void     OAL_pause_sample(HSAMPLE s);
void     OAL_stop_sample(HSAMPLE s);
void     OAL_resume_sample(HSAMPLE s);
void     OAL_set_sample_volume(HSAMPLE s, float vol);
void     OAL_set_sample_pan(HSAMPLE s, float pan);
void     OAL_set_sample_volume_pan(HSAMPLE s, float vol, float pan);
void     OAL_sample_volume_pan(HSAMPLE s, float *volOut, float *panOut);
void     OAL_set_sample_playback_rate(HSAMPLE s, int rate);
int      OAL_sample_playback_rate(HSAMPLE s);
void     OAL_register_EOS_callback(HSAMPLE s, AILEOSCALLBACK cb);
void     OAL_set_sample_user_data(HSAMPLE s, int index, intptr_t value);
intptr_t OAL_sample_user_data(HSAMPLE s, int index);
void     OAL_set_sample_processor(HSAMPLE s, int type, HPROVIDER provider);
void     OAL_set_filter_sample_preference(HSAMPLE s, const char *pref, const void *val);

/* Streaming */
HSTREAM OAL_open_stream(void *driver, const char *filename, int flags);
void    OAL_start_stream(HSTREAM s);
void    OAL_close_stream(HSTREAM s);
void    OAL_pause_stream(HSTREAM s, int pause_flag);
void    OAL_set_stream_volume_pan(HSTREAM s, float vol, float pan);
void    OAL_stream_volume_pan(HSTREAM s, float *volOut, float *panOut);
void    OAL_set_stream_loop_count(HSTREAM s, int count);
int     OAL_stream_loop_count(HSTREAM s);
void    OAL_register_stream_callback(HSTREAM s, AILSTREAM_CALLBACK cb);
void    OAL_stream_ms_position(HSTREAM s, long *currentMs, long *totalMs);

/* 3-D provider / listener */
int        OAL_enumerate_3D_providers(HPROENUM *next, HPROVIDER *providerOut, char **nameOut);
H3DPOBJECT OAL_open_3D_listener(HPROVIDER provider);
void       OAL_close_3D_listener(H3DPOBJECT listener);
void       OAL_set_3D_orientation(H3DPOBJECT listener, float fx, float fy, float fz, float upx, float upy, float upz);
int        OAL_enumerate_filters(HPROENUM *next, HPROVIDER *providerOut, char **nameOut);

/* 3-D positional samples (share the 2D sample pool; handle = 1-based index) */
HSAMPLE OAL_allocate_3D_sample(HPROVIDER prov);
void    OAL_release_3D_sample(HSAMPLE s);
void    OAL_set_3D_sample_file(HSAMPLE s, const void *buf);
void    OAL_register_3D_EOS_callback(HSAMPLE s, AILEOSCALLBACK cb);
void    OAL_set_3D_sample_position(void *handle, float x, float y, float z);
void    OAL_set_3D_sample_distances(HSAMPLE s, float minDist, float maxDist);

/* Quick-play (used for mission briefings) */
HAUDIO OAL_quick_load_and_play(const char *filename, int loops, int wait);
void   OAL_quick_set_volume(HAUDIO audio, float vol, float pan);
void   OAL_quick_unload(HAUDIO audio);

/* -------------------------------------------------------------------------
   AIL_* macros — map to the real OAL_ implementations above.
   3-D positional audio functions remain as no-ops (not required for menus).
   ------------------------------------------------------------------------- */

/* System */
#define AIL_startup()       OAL_startup()
#define AIL_shutdown()      OAL_shutdown()
#define AIL_set_redist_directory(path) ((void)(path))
#define AIL_quick_startup(ud,um,oR,oB,oC) OAL_quick_startup((ud),(um),(oR),(oB),(oC))
#define AIL_quick_handles(dh,mh,dlsh)     OAL_quick_handles((void**)(dh),(void**)(mh),(void**)(dlsh))
#define AIL_set_file_callbacks(o,c,s,r)   OAL_set_file_callbacks((o),(c),(s),(r))
#define AIL_MSS_version(buf,sz)           OAL_MSS_version((buf),(sz))
#define AIL_get_timer_highest_delay()     OAL_get_timer_highest_delay()

/* WAV / ADPCM */
#define AIL_WAV_info(buf,info)            OAL_WAV_info((buf),(info))
#define AIL_decompress_ADPCM(info,ob,os)  OAL_decompress_ADPCM((info),(ob),(os))
#define AIL_mem_free_lock(ptr)            OAL_mem_free_lock(ptr)

/* 2-D sample management */
#define AIL_allocate_sample_handle(drv)       OAL_allocate_sample(drv)
#define AIL_release_sample_handle(s)          OAL_release_sample(s)
#define AIL_init_sample(s)                    OAL_init_sample(s)
#define AIL_set_sample_file(s,data,blk)       OAL_set_sample_file((s),(data),(blk))
#define AIL_start_sample(s)                   OAL_start_sample(s)
#define AIL_pause_sample(s)                   OAL_pause_sample(s)
#define AIL_stop_sample(s)                    OAL_stop_sample(s)
#define AIL_resume_sample(s)                  OAL_resume_sample(s)
#define AIL_set_sample_volume(s,v)            OAL_set_sample_volume((s),(v))
#define AIL_set_sample_pan(s,p)               OAL_set_sample_pan((s),(p))
#define AIL_set_sample_volume_pan(s,v,p)      OAL_set_sample_volume_pan((s),(v),(p))
#define AIL_sample_volume_pan(s,vo,po)        OAL_sample_volume_pan((s),(vo),(po))
#define AIL_set_sample_playback_rate(s,r)     OAL_set_sample_playback_rate((s),(r))
#define AIL_sample_playback_rate(s)           OAL_sample_playback_rate(s)
#define AIL_set_sample_pitch(s,p)             OAL_set_sample_playback_rate((s),(p))
#define AIL_register_EOS_callback(s,cb)       OAL_register_EOS_callback((s),(cb))
#define AIL_set_sample_user_data(s,i,v)       OAL_set_sample_user_data((s),(i),(intptr_t)(v))
#define AIL_sample_user_data(s,i)             OAL_sample_user_data((s),(i))
#define AIL_set_sample_processor(s,t,p)       OAL_set_sample_processor((s),(t),(p))
#define AIL_set_filter_sample_preference(s,pref,val) OAL_set_filter_sample_preference((s),(pref),(val))

/* Streaming */
#define AIL_open_stream(drv,fn,mem)           OAL_open_stream((drv),(fn),(mem))
#define AIL_start_stream(s)                   OAL_start_stream(s)
#define AIL_close_stream(s)                   OAL_close_stream(s)
#define AIL_pause_stream(s,flag)              OAL_pause_stream((s),(flag))
#define AIL_set_stream_volume_pan(s,v,p)      OAL_set_stream_volume_pan((s),(v),(p))
#define AIL_stream_volume_pan(s,vo,po)        OAL_stream_volume_pan((s),(vo),(po))
#define AIL_set_stream_loop_count(s,c)        OAL_set_stream_loop_count((s),(c))
#define AIL_stream_loop_count(s)              OAL_stream_loop_count(s)
#define AIL_register_stream_callback(s,cb)    OAL_register_stream_callback((s),(cb))
#define AIL_stream_ms_position(s,cur,tot)     OAL_stream_ms_position((s),(cur),(tot))
#define AIL_quick_unload(a)                   OAL_quick_unload(a)
#define AIL_quick_load_and_play(fn,lp,wt)     OAL_quick_load_and_play((fn),(lp),(wt))
#define AIL_quick_set_volume(a,v,p)           OAL_quick_set_volume((a),(v),(p))

/* 3-D listener */
#define AIL_enumerate_3D_providers(np,po,no)  OAL_enumerate_3D_providers((np),(po),(no))
#define AIL_open_3D_listener(prov)            OAL_open_3D_listener(prov)
#define AIL_close_3D_listener(lst)            OAL_close_3D_listener(lst)
#define AIL_set_3D_orientation(lst,fx,fy,fz,ux,uy,uz) OAL_set_3D_orientation((lst),(fx),(fy),(fz),(ux),(uy),(uz))
#define AIL_enumerate_filters(np,po,no)       OAL_enumerate_filters((np),(po),(no))

/* 3-D provider / speaker type — keep as successful no-ops */
#define AIL_open_3D_provider(prov)            (0)
#define AIL_close_3D_provider(prov)           ((void)(prov))
#define AIL_set_3D_speaker_type(prov,st)      do { (void)(prov); (void)(st); } while(0)

/* 3-D positional sample — route through the shared 2D/3D sample pool */
#define AIL_allocate_3D_sample_handle(prov)         OAL_allocate_3D_sample(prov)
#define AIL_release_3D_sample_handle(s)             OAL_release_3D_sample(s)
#define AIL_set_3D_sample_file(s,buf)               OAL_set_3D_sample_file((s),(buf))
#define AIL_start_3D_sample(s)                      OAL_start_sample(s)
#define AIL_pause_3D_sample(s)                      OAL_pause_sample(s)
#define AIL_stop_3D_sample(s)                       OAL_stop_sample(s)
#define AIL_resume_3D_sample(s)                     OAL_resume_sample(s)
#define AIL_register_3D_EOS_callback(s,cb)          OAL_register_3D_EOS_callback((s),(AILEOSCALLBACK)(cb))
#define AIL_set_3D_sample_volume(s,v)               OAL_set_sample_volume((s),(v))
#define AIL_set_3D_position(s,x,y,z)               OAL_set_3D_sample_position((void*)(uintptr_t)(s),(float)(x),(float)(y),(float)(z))
#define AIL_set_3D_velocity(s,vx,vy,vz)             do { (void)(s);(void)(vx);(void)(vy);(void)(vz); } while(0)
#define AIL_set_3D_sample_distances(s,mn,mx)        OAL_set_3D_sample_distances((s),(float)(mn),(float)(mx))
#define AIL_set_3D_sample_playback_rate(s,r)        OAL_set_sample_playback_rate((s),(r))
#define AIL_3D_sample_playback_rate(s)              OAL_sample_playback_rate(s)
#define AIL_set_3D_sample_occlusion(s,o)            do { (void)(s); (void)(o); } while(0)
#define AIL_set_3D_user_data(s,i,v)                 OAL_set_sample_user_data((s),(i),(intptr_t)(v))
#define AIL_3D_user_data(s,i)                       ((int32_t)OAL_sample_user_data((s),(i)))

/* Misc stubs */
#define AIL_get_DirectSound_info(s,ds,pr)  do { (void)(s); { void **_ds=(void**)(ds); void **_pr=(void**)(pr); if(_ds)*_ds=NULL; if(_pr)*_pr=NULL; } } while(0)

#ifdef __cplusplus
}
#endif

#endif /* __MSS_H__ */
