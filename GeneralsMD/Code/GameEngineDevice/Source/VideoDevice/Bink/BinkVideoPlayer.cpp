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

////////////////////////////////////////////////////////////////////////////////
//																					//
//  (c) 2001-2003 Electronic Arts Inc.													//
//																					//
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//                                                                          
//                       Westwood Studios Pacific.                          
//                                                                          
//                       Confidential Information                           
//                Copyright (C) 2001 - All Rights Reserved                  
//                                                                          
//----------------------------------------------------------------------------
//
// Project:   Generals
//
// Module:    VideoDevice
//
// File name: BinkDevice.cpp
//
// Created:   10/22/01	TR
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//         Includes                                                      
//----------------------------------------------------------------------------

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wchar.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

#include "Lib/BaseType.h"
#include "VideoDevice/Bink/BinkVideoPlayer.h"
#include "Common/GlobalData.h"
#include "Common/Registry.h"

//----------------------------------------------------------------------------
//         Defines                                                         
//----------------------------------------------------------------------------

#define VIDEO_PATH "Data\\Movies"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) do { if ((x) != NULL) { (x)->Release(); (x) = NULL; } } while (0)
#endif

//----------------------------------------------------------------------------
//         Private Data                                                    
//----------------------------------------------------------------------------

static const char *s_videoExtensions[] =
{
	"wmv",
	"mp4",
	"avi",
	"mov",
	"m4v",
	NULL
};

//----------------------------------------------------------------------------
//         Private Functions                                               
//----------------------------------------------------------------------------

static Bool fileExists( const char *path )
{
	DWORD attributes = GetFileAttributesA( path );
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static Bool hasExplicitExtension( const char *fileName )
{
	const char *dot = strrchr( fileName, '.' );
	const char *slash = strrchr( fileName, '\\' );
	if (slash == NULL) {
		slash = strrchr( fileName, '/' );
	}
	return dot != NULL && (slash == NULL || dot > slash);
}

static void copyMovieBaseName( const char *fileName, char *baseName, size_t baseNameSize )
{
	strcpy_s( baseName, baseNameSize, fileName );
	char *dot = strrchr( baseName, '.' );
	char *slash = strrchr( baseName, '\\' );
	if (slash == NULL) {
		slash = strrchr( baseName, '/' );
	}
	if (dot != NULL && (slash == NULL || dot > slash)) {
		*dot = 0;
	}
}

static Bool tryCopyCandidate( char *resolvedPath, size_t resolvedPathSize, const char *candidate )
{
	if (!fileExists(candidate)) {
		return FALSE;
	}
	strcpy_s( resolvedPath, resolvedPathSize, candidate );
	return TRUE;
}

static Bool appendMoviePath( char *candidate, size_t candidateSize, const char *directoryPrefix, const char *name, const char *extension )
{
	if (extension == NULL) {
		return sprintf_s( candidate, candidateSize, "%s\\%s", directoryPrefix, name ) > 0;
	}
	return sprintf_s( candidate, candidateSize, "%s\\%s.%s", directoryPrefix, name, extension ) > 0;
}

static Bool appendModMoviePath( char *candidate, size_t candidateSize, const char *modDir, const char *name, const char *extension )
{
	if (extension == NULL) {
		return sprintf_s( candidate, candidateSize, "%s%s\\%s", modDir, VIDEO_PATH, name ) > 0;
	}
	return sprintf_s( candidate, candidateSize, "%s%s\\%s.%s", modDir, VIDEO_PATH, name, extension ) > 0;
}

static Bool appendLocalizedMoviePath( char *candidate, size_t candidateSize, const char *language, const char *name, const char *extension )
{
	if (extension == NULL) {
		return sprintf_s( candidate, candidateSize, "Data\\%s\\Movies\\%s", language, name ) > 0;
	}
	return sprintf_s( candidate, candidateSize, "Data\\%s\\Movies\\%s.%s", language, name, extension ) > 0;
}

static Bool resolveMovieFile( const AsciiString& movieBaseName, char *resolvedPath, size_t resolvedPathSize )
{
	if (movieBaseName.isEmpty() || resolvedPath == NULL || resolvedPathSize == 0) {
		return FALSE;
	}

	char candidate[_MAX_PATH];
	const char *name = movieBaseName.str();
	Bool explicitExtension = hasExplicitExtension( name );
	char baseName[_MAX_PATH];
	copyMovieBaseName( name, baseName, sizeof(baseName) );

	if (TheGlobalData->m_modDir.isNotEmpty()) {
		if (explicitExtension) {
			if (appendModMoviePath(candidate, sizeof(candidate), TheGlobalData->m_modDir.str(), name, NULL) &&
				tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
				return TRUE;
			}
			for (Int index = 0; s_videoExtensions[index] != NULL; ++index) {
				if (appendModMoviePath(candidate, sizeof(candidate), TheGlobalData->m_modDir.str(), baseName, s_videoExtensions[index]) &&
					tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
					return TRUE;
				}
			}
		} else {
			for (Int index = 0; s_videoExtensions[index] != NULL; ++index) {
				if (appendModMoviePath(candidate, sizeof(candidate), TheGlobalData->m_modDir.str(), name, s_videoExtensions[index]) &&
					tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
					return TRUE;
				}
			}
		}
	}

	AsciiString language = GetRegistryLanguage();
	if (explicitExtension) {
		if (appendLocalizedMoviePath(candidate, sizeof(candidate), language.str(), name, NULL) &&
			tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
			return TRUE;
		}
		for (Int index = 0; s_videoExtensions[index] != NULL; ++index) {
			if (appendLocalizedMoviePath(candidate, sizeof(candidate), language.str(), baseName, s_videoExtensions[index]) &&
				tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
				return TRUE;
			}
		}
		if (appendMoviePath(candidate, sizeof(candidate), VIDEO_PATH, name, NULL) &&
			tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
			return TRUE;
		}
		for (Int index = 0; s_videoExtensions[index] != NULL; ++index) {
			if (appendMoviePath(candidate, sizeof(candidate), VIDEO_PATH, baseName, s_videoExtensions[index]) &&
				tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
				return TRUE;
			}
		}
	} else {
		for (Int index = 0; s_videoExtensions[index] != NULL; ++index) {
			if (appendLocalizedMoviePath(candidate, sizeof(candidate), language.str(), name, s_videoExtensions[index]) &&
				tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
				return TRUE;
			}
		}
		for (Int index = 0; s_videoExtensions[index] != NULL; ++index) {
			if (appendMoviePath(candidate, sizeof(candidate), VIDEO_PATH, name, s_videoExtensions[index]) &&
				tryCopyCandidate(resolvedPath, resolvedPathSize, candidate)) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

static Bool convertPathToWide( const char *source, wchar_t *destination, size_t destinationCount )
{
	int written = MultiByteToWideChar( CP_ACP, 0, source, -1, destination, (int)destinationCount );
	if (written <= 0) {
		if (destinationCount > 0) {
			destination[0] = 0;
		}
		return FALSE;
	}
	return TRUE;
}

static void copyFrameToVideoBuffer( const std::vector<UnsignedByte>& frameData, UnsignedInt frameStride, Int width, Int height, VideoBuffer *buffer )
{
	if (buffer == NULL || frameData.empty()) {
		return;
	}

	UnsignedByte *destinationBase = (UnsignedByte*)buffer->lock();
	if (destinationBase == NULL) {
		return;
	}

	UnsignedInt xOffsetBytes;
	switch (buffer->format()) {
		case VideoBuffer::TYPE_X8R8G8B8:
			xOffsetBytes = buffer->xPos() * 4;
			break;
		case VideoBuffer::TYPE_R8G8B8:
			xOffsetBytes = buffer->xPos() * 3;
			break;
		default:
			xOffsetBytes = buffer->xPos() * 2;
			break;
	}

	UnsignedByte *destination = destinationBase + (buffer->yPos() * buffer->pitch()) + xOffsetBytes;

	for (Int y = 0; y < height; ++y) {
		const UnsignedByte *source = &frameData[y * frameStride];
		UnsignedByte *destRow = destination + (y * buffer->pitch());

		switch (buffer->format()) {
			case VideoBuffer::TYPE_X8R8G8B8:
				memcpy( destRow, source, width * 4 );
				break;

			case VideoBuffer::TYPE_R8G8B8:
				for (Int x = 0; x < width; ++x) {
					const UnsignedByte *srcPixel = source + (x * 4);
					UnsignedByte *dstPixel = destRow + (x * 3);
					dstPixel[0] = srcPixel[0];
					dstPixel[1] = srcPixel[1];
					dstPixel[2] = srcPixel[2];
				}
				break;

			case VideoBuffer::TYPE_R5G6B5:
				for (Int x = 0; x < width; ++x) {
					const UnsignedByte *srcPixel = source + (x * 4);
					UnsignedShort blue = (UnsignedShort)(srcPixel[0] >> 3);
					UnsignedShort green = (UnsignedShort)(srcPixel[1] >> 2);
					UnsignedShort red = (UnsignedShort)(srcPixel[2] >> 3);
					((UnsignedShort*)destRow)[x] = (UnsignedShort)((red << 11) | (green << 5) | blue);
				}
				break;

			case VideoBuffer::TYPE_X1R5G5B5:
				for (Int x = 0; x < width; ++x) {
					const UnsignedByte *srcPixel = source + (x * 4);
					UnsignedShort blue = (UnsignedShort)(srcPixel[0] >> 3);
					UnsignedShort green = (UnsignedShort)(srcPixel[1] >> 3);
					UnsignedShort red = (UnsignedShort)(srcPixel[2] >> 3);
					((UnsignedShort*)destRow)[x] = (UnsignedShort)(0x8000 | (red << 10) | (green << 5) | blue);
				}
				break;

			default:
				break;
		}
	}

	buffer->unlock();
}

//----------------------------------------------------------------------------
//         Public Functions                                                
//----------------------------------------------------------------------------

BinkVideoPlayer::BinkVideoPlayer()
{
}

BinkVideoPlayer::~BinkVideoPlayer()
{
	deinit();
}

void BinkVideoPlayer::init( void )
{
	VideoPlayer::init();
	HRESULT startupResult = MFStartup( MF_VERSION, MFSTARTUP_LITE );
	if (FAILED(startupResult)) {
		DEBUG_LOG(("BinkVideoPlayer::init() - MFStartup failed 0x%08lx\n", startupResult));
	}
}

void BinkVideoPlayer::deinit( void )
{
	MFShutdown();
	VideoPlayer::deinit();
}

void BinkVideoPlayer::reset( void )
{
	VideoPlayer::reset();
}

void BinkVideoPlayer::update( void )
{
	VideoPlayer::update();
}

void BinkVideoPlayer::loseFocus( void )
{
	VideoPlayer::loseFocus();
}

void BinkVideoPlayer::regainFocus( void )
{
	VideoPlayer::regainFocus();
}

VideoStreamInterface* BinkVideoPlayer::open( AsciiString movieTitle )
{
	const Video *video = getVideo( movieTitle );
	if (video == NULL) {
		return NULL;
	}

	char filePath[_MAX_PATH];
	if (!resolveMovieFile(video->m_filename, filePath, sizeof(filePath))) {
		DEBUG_LOG(("BinkVideoPlayer::open() - no playable movie found for %s (%s). Expected a converted loose file with one of: .mp4 .wmv .avi .mov .m4v\n",
			movieTitle.str(), video->m_filename.str()));
		return NULL;
	}

	BinkVideoStream *stream = NEW BinkVideoStream;
	if (stream == NULL) {
		return NULL;
	}

	if (!stream->openFile(filePath)) {
		delete stream;
		return NULL;
	}

	stream->m_next = m_firstStream;
	stream->m_player = this;
	m_firstStream = stream;
	return stream;
}

VideoStreamInterface* BinkVideoPlayer::load( AsciiString movieTitle )
{
	return open( movieTitle );
}

void BinkVideoPlayer::notifyVideoPlayerOfNewProvider( Bool nowHasValid )
{
	(void)nowHasValid;
}

BinkVideoStream::BinkVideoStream()
	: m_reader(NULL),
	  m_frameStride(0),
	  m_width(0),
	  m_height(0),
	  m_frameCount(0),
	  m_frameIndex(0),
	  m_frameDurationHns(0),
	  m_frameTimeHns(0),
	  m_durationHns(0),
	  m_playbackStartMs(0),
	  m_endOfStream(FALSE),
	  m_hasFrame(FALSE)
{
}

BinkVideoStream::~BinkVideoStream()
{
	SAFE_RELEASE( m_reader );
}

Bool BinkVideoStream::openFile( const char *filePath )
{
	wchar_t widePath[_MAX_PATH];
	if (!convertPathToWide(filePath, widePath, sizeof(widePath) / sizeof(widePath[0]))) {
		DEBUG_LOG(("BinkVideoStream::openFile() - failed to convert path %s\n", filePath));
		return FALSE;
	}

	HRESULT hr = MFCreateSourceReaderFromURL( widePath, NULL, &m_reader );
	if (FAILED(hr) || m_reader == NULL) {
		DEBUG_LOG(("BinkVideoStream::openFile() - MFCreateSourceReaderFromURL failed for %s (0x%08lx)\n", filePath, hr));
		SAFE_RELEASE( m_reader );
		return FALSE;
	}

	IMFMediaType *mediaType = NULL;
	hr = MFCreateMediaType( &mediaType );
	if (SUCCEEDED(hr)) hr = mediaType->SetGUID( MF_MT_MAJOR_TYPE, MFMediaType_Video );
	if (SUCCEEDED(hr)) hr = mediaType->SetGUID( MF_MT_SUBTYPE, MFVideoFormat_RGB32 );
	if (SUCCEEDED(hr)) hr = m_reader->SetCurrentMediaType( MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, mediaType );
	SAFE_RELEASE( mediaType );
	if (FAILED(hr)) {
		DEBUG_LOG(("BinkVideoStream::openFile() - unable to request RGB32 for %s (0x%08lx)\n", filePath, hr));
		SAFE_RELEASE( m_reader );
		return FALSE;
	}

	IMFMediaType *currentType = NULL;
	hr = m_reader->GetCurrentMediaType( MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType );
	if (FAILED(hr) || currentType == NULL) {
		DEBUG_LOG(("BinkVideoStream::openFile() - GetCurrentMediaType failed for %s (0x%08lx)\n", filePath, hr));
		SAFE_RELEASE( currentType );
		SAFE_RELEASE( m_reader );
		return FALSE;
	}

	UINT32 width = 0;
	UINT32 height = 0;
	MFGetAttributeSize( currentType, MF_MT_FRAME_SIZE, &width, &height );
	m_width = (Int)width;
	m_height = (Int)height;
	m_frameStride = width * 4;

	UINT32 frameRateNumerator = 0;
	UINT32 frameRateDenominator = 0;
	if (FAILED(MFGetAttributeRatio(currentType, MF_MT_FRAME_RATE, &frameRateNumerator, &frameRateDenominator)) ||
		frameRateNumerator == 0 || frameRateDenominator == 0) {
		frameRateNumerator = 30;
		frameRateDenominator = 1;
	}
	m_frameDurationHns = (Int64)(((Int64)10000000 * frameRateDenominator) / frameRateNumerator);
	if (m_frameDurationHns <= 0) {
		m_frameDurationHns = 333333;
	}
	SAFE_RELEASE( currentType );

	PROPVARIANT durationValue;
	PropVariantInit( &durationValue );
	hr = m_reader->GetPresentationAttribute( MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &durationValue );
	if (SUCCEEDED(hr) && durationValue.vt == VT_UI8) {
		m_durationHns = (Int64)durationValue.uhVal.QuadPart;
	}
	PropVariantClear( &durationValue );

	if (m_durationHns > 0) {
		m_frameCount = (Int)((m_durationHns + m_frameDurationHns - 1) / m_frameDurationHns);
	}
	if (m_frameCount <= 0) {
		m_frameCount = 1;
	}

	m_playbackStartMs = timeGetTime();
	m_frameIndex = 0;
	m_frameTimeHns = 0;
	m_endOfStream = FALSE;
	m_hasFrame = FALSE;

	if (!readCurrentFrame()) {
		SAFE_RELEASE( m_reader );
		return FALSE;
	}
	m_playbackStartMs = timeGetTime() - (UnsignedInt)(m_frameTimeHns / 10000);
	return TRUE;
}

Int BinkVideoStream::timeToFrameIndex( Int64 sampleTimeHns ) const
{
	if (m_frameDurationHns <= 0) {
		return 0;
	}
	Int index = (Int)(sampleTimeHns / m_frameDurationHns);
	if (index < 0) {
		index = 0;
	}
	if (m_frameCount > 0 && index >= m_frameCount) {
		index = m_frameCount - 1;
	}
	return index;
}

Bool BinkVideoStream::readCurrentFrame( void )
{
	if (m_reader == NULL) {
		return FALSE;
	}

	for (;;) {
		DWORD streamFlags = 0;
		LONGLONG sampleTime = 0;
		IMFSample *sample = NULL;
		HRESULT hr = m_reader->ReadSample( MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &streamFlags, &sampleTime, &sample );
		if (FAILED(hr)) {
			DEBUG_LOG(("BinkVideoStream::readCurrentFrame() - ReadSample failed 0x%08lx\n", hr));
			SAFE_RELEASE( sample );
			return FALSE;
		}

		if ((streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
			m_endOfStream = TRUE;
			if (m_frameCount > 0) {
				m_frameIndex = m_frameCount - 1;
			}
			SAFE_RELEASE( sample );
			return m_hasFrame;
		}

		if (sample == NULL) {
			continue;
		}

		IMFMediaBuffer *buffer = NULL;
		BYTE *lockedData = NULL;
		DWORD currentLength = 0;
		hr = sample->ConvertToContiguousBuffer( &buffer );
		if (SUCCEEDED(hr)) {
			hr = buffer->Lock( &lockedData, NULL, &currentLength );
		}
		if (FAILED(hr) || lockedData == NULL || currentLength == 0) {
			if (buffer != NULL && lockedData != NULL) {
				buffer->Unlock();
			}
			SAFE_RELEASE( buffer );
			SAFE_RELEASE( sample );
			continue;
		}

		m_frameData.resize( currentLength );
		memcpy( &m_frameData[0], lockedData, currentLength );
		buffer->Unlock();
		SAFE_RELEASE( buffer );
		SAFE_RELEASE( sample );

		if (m_height > 0) {
			m_frameStride = currentLength / m_height;
		}
		m_frameTimeHns = sampleTime;
		m_frameIndex = timeToFrameIndex( sampleTime );
		m_hasFrame = TRUE;
		return TRUE;
	}
}

Bool BinkVideoStream::seekToFrame( Int index )
{
	if (m_reader == NULL) {
		return FALSE;
	}
	if (index < 0) {
		index = 0;
	}
	if (m_frameCount > 0 && index >= m_frameCount) {
		index = m_frameCount - 1;
	}

	PROPVARIANT position;
	PropVariantInit( &position );
	position.vt = VT_I8;
	position.hVal.QuadPart = index * m_frameDurationHns;

	HRESULT hr = m_reader->Flush( MF_SOURCE_READER_FIRST_VIDEO_STREAM );
	if (SUCCEEDED(hr)) {
		hr = m_reader->SetCurrentPosition( GUID_NULL, position );
	}
	PropVariantClear( &position );
	if (FAILED(hr)) {
		DEBUG_LOG(("BinkVideoStream::seekToFrame() - failed to seek to frame %d (0x%08lx)\n", index, hr));
		return FALSE;
	}

	m_endOfStream = FALSE;
	m_hasFrame = FALSE;
	if (!readCurrentFrame()) {
		return FALSE;
	}
	m_playbackStartMs = timeGetTime() - (UnsignedInt)(m_frameTimeHns / 10000);
	return TRUE;
}

void BinkVideoStream::update( void )
{
}

Bool BinkVideoStream::isFrameReady( void )
{
	if (!m_hasFrame) {
		return FALSE;
	}
	UnsignedInt elapsedMs = timeGetTime() - m_playbackStartMs;
	Int64 elapsedHns = (Int64)elapsedMs * 10000;
	return elapsedHns + (m_frameDurationHns / 4) >= m_frameTimeHns;
}

void BinkVideoStream::frameDecompress( void )
{
}

void BinkVideoStream::frameRender( VideoBuffer *buffer )
{
	copyFrameToVideoBuffer( m_frameData, m_frameStride, m_width, m_height, buffer );
}

void BinkVideoStream::frameNext( void )
{
	if (!m_endOfStream) {
		readCurrentFrame();
	}
}

Int BinkVideoStream::frameIndex( void )
{
	return m_frameIndex;
}

Int BinkVideoStream::frameCount( void )
{
	return m_frameCount;
}

void BinkVideoStream::frameGoto( Int index )
{
	seekToFrame( index );
}

Int BinkVideoStream::height( void )
{
	return m_height;
}

Int BinkVideoStream::width( void )
{
	return m_width;
}



