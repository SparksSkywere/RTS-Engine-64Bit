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
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//                                                                          
//                       Westwood Studios Pacific.                          
//                                                                          
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
// Project:    Generals
//
// File name:  GameClient/VideoPlayer.h
//
// Created:    10/22/01
//
//----------------------------------------------------------------------------

#pragma once

#ifndef __VIDEODEVICE_BINKDEVICE_H_
#define __VIDEODEVICE_BINKDEVICE_H_


//----------------------------------------------------------------------------
//           Includes                                                      
//----------------------------------------------------------------------------

#include <vector>

#include "GameClient/VideoPlayer.h"

struct IMFSourceReader;

//----------------------------------------------------------------------------
//           Forward References
//----------------------------------------------------------------------------

class BinkVideoPlayer;

//----------------------------------------------------------------------------
//           Type Defines
//----------------------------------------------------------------------------

class BinkVideoStream : public VideoStream
{
	friend class BinkVideoPlayer;

	protected:

		IMFSourceReader			*m_reader;
		std::vector<UnsignedByte>	m_frameData;
		UnsignedInt				m_frameStride;
		Int						m_width;
		Int						m_height;
		Int						m_frameCount;
		Int						m_frameIndex;
		Int64					m_frameDurationHns;
		Int64					m_frameTimeHns;
		Int64					m_durationHns;
		UnsignedInt				m_playbackStartMs;
		Bool					m_endOfStream;
		Bool					m_hasFrame;

		BinkVideoStream();
		virtual ~BinkVideoStream();

		Bool openFile( const char *filePath );
		Bool readCurrentFrame( void );
		Bool seekToFrame( Int index );
		Int timeToFrameIndex( Int64 sampleTimeHns ) const;

	public:

		virtual void update( void );
		virtual Bool	isFrameReady( void );
		virtual void	frameDecompress( void );
		virtual void	frameRender( VideoBuffer *buffer );
		virtual void	frameNext( void );
		virtual Int		frameIndex( void );
		virtual Int		frameCount( void );
		virtual void	frameGoto( Int index );
		virtual Int		height( void );
		virtual Int		width( void );
};

class BinkVideoPlayer : public VideoPlayer
{
	public:

		virtual void	init( void );
		virtual void	reset( void );
		virtual void	update( void );
		virtual void	deinit( void );

		BinkVideoPlayer();
		~BinkVideoPlayer();

		virtual void	loseFocus( void );
		virtual void	regainFocus( void );

		virtual VideoStreamInterface*	open( AsciiString movieTitle );
		virtual VideoStreamInterface*	load( AsciiString movieTitle );

		virtual void notifyVideoPlayerOfNewProvider( Bool nowHasValid );
};


//----------------------------------------------------------------------------
//           Inlining                                                       
//----------------------------------------------------------------------------


#endif // __VIDEODEVICE_BINKDEVICE_H_
