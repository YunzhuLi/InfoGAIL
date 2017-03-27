#ifndef __OggSoundStream_h__
#define __OggSoundStream_h__

/***************************************************************************

    file                 : OggSoundStream.h
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: OggSoundStream.h,v 1.1.2.2 2011/12/29 09:23:24 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Concrete implementation for ogg sound streams */

#define OV_EXCLUDE_STATIC_CALLBACKS

#include <vorbis/vorbisfile.h>
#include "SoundStream.h"

class OggSoundStream : public SoundStream
{
	public:
		OggSoundStream(char* path);
		virtual ~OggSoundStream();
		
		virtual int getRateInHz() { return rateInHz; }
		virtual SoundFormat getSoundFormat() { return format; }
		
		virtual bool read(char* buffer, const int bufferSize, int* resultSize, const char* error);
		virtual void rewind();
		virtual void display();
		virtual bool isValid() { return valid; }

	protected:
		
	private:
		const char* errorString(int code);
		
		OggVorbis_File	oggStream;
		bool			valid;
		int				rateInHz;
		SoundFormat		format;
};

#endif // __OggSoundStream_h__