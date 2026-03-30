#pragma once

#ifndef __BINK_STUB_H__
#define __BINK_STUB_H__

typedef unsigned int U32;

typedef struct BINK
{
	int FrameNum;
	int Frames;
	int Height;
	int Width;
} BINK;

typedef BINK* HBINK;

#ifndef BINKPRELOADALL
#define BINKPRELOADALL 0
#endif

#ifndef BINKSURFACE32
#define BINKSURFACE32 0
#endif
#ifndef BINKSURFACE24
#define BINKSURFACE24 1
#endif
#ifndef BINKSURFACE565
#define BINKSURFACE565 2
#endif
#ifndef BINKSURFACE555
#define BINKSURFACE555 3
#endif

inline HBINK BinkOpen(const char*, U32)
{
	return 0;
}

inline void BinkClose(HBINK)
{
}

inline void BinkSetVolume(HBINK, U32, int)
{
}

inline int BinkSoundUseDirectSound(void*)
{
	return 0;
}

inline void BinkSetSoundTrack(U32, U32*)
{
}

inline int BinkWait(HBINK)
{
	return 0;
}

inline void BinkDoFrame(HBINK)
{
}

inline void BinkCopyToBuffer(HBINK, void*, int, int, int, int, U32)
{
}

inline void BinkNextFrame(HBINK)
{
}

inline void BinkGoto(HBINK, int, void*)
{
}

#endif