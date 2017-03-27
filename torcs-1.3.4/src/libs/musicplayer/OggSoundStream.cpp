/***************************************************************************

    file                 : OggSoundStream.cpp
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: OggSoundStream.cpp,v 1.1.2.1 2011/12/28 15:11:57 berniw Exp $

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

#include "OggSoundStream.h"
#include <tgf.h>

OggSoundStream::OggSoundStream(char* path):
	SoundStream(path),
	valid(false),
	rateInHz(0),
	format(FORMAT_INVALID)
{
	int result;
 
	if((result = ov_fopen(path, &oggStream)) < 0) {
		GfError("OggSoundStream: Could not open Ogg stream: %s\n", errorString(result));
		return;
	}
	
	// fclose is not required here, the vorbis lib will take care of this on ov_clear.
		
	vorbis_info* vorbisInfo = ov_info(&oggStream, -1);
	rateInHz = vorbisInfo->rate;

    if(vorbisInfo->channels == 1) {
        format = FORMAT_MONO16;
	} else {
        format = FORMAT_STEREO16;
	}

	valid = true;
}




OggSoundStream::~OggSoundStream()
{
	if (isValid()) {
		ov_clear(&oggStream);
	}
}




bool OggSoundStream::read(char* buffer, const int bufferSize, int* resultSize, const char* error)
{
	if (!isValid()) {
		error = "OggSoundStream: Invalid, no data available.";
		return false;
	}

	int section;
	int result;

	while(*resultSize < bufferSize) {
		result = ov_read(&oggStream, buffer + *resultSize, bufferSize - *resultSize, 0, 2, 1, &section);

		if(result > 0) {
			*resultSize += result;
		} else {
			if(result < 0) {
				error = errorString(result);
				return false;
			} else {
				// Loop to the beginning of the stream
				ov_time_seek(&oggStream, 0);
			}
		}
	}

	if(*resultSize == 0) {
		error = "OggSoundStream: Read 0 bytes.";
		return false;
	}
	
	return true;
}




void OggSoundStream::rewind()
{
	if (!isValid()) {
		GfError("OggSoundStream: Invalid, no info available.\n");
		return;
	}

	ov_time_seek(&oggStream, 0);
}




void OggSoundStream::display()
{
	if (!isValid()) {
		GfError("OggSoundStream: Invalid, no info available.\n");
		return;
	}
	
	vorbis_info* vorbisInfo = ov_info(&oggStream, -1);
    vorbis_comment* vorbisComment = ov_comment(&oggStream, -1);
	
	GfOut("version         %d\n", vorbisInfo->version);
	GfOut("channels        %d\n", vorbisInfo->channels);
	GfOut("rate (hz)       %ld\n", vorbisInfo->rate);
	GfOut("bitrate upper   %ld\n", vorbisInfo->bitrate_upper);
	GfOut("bitrate nominal %ld\n", vorbisInfo->bitrate_nominal);
	GfOut("bitrate lower   %ld\n", vorbisInfo->bitrate_lower);
	GfOut("bitrate window  %ld\n\n", vorbisInfo->bitrate_window);
	GfOut("vendor          %s\n", vorbisComment->vendor);

	int i;
    for(i = 0; i < vorbisComment->comments; i++) {
        GfOut("                %s\n", vorbisComment->user_comments[i]);
	}
}




const char* OggSoundStream::errorString(int code)
{
    switch(code)
    {
        case OV_EREAD:
            return "OggSoundStream: Read from media.";
        case OV_ENOTVORBIS:
            return "OggSoundStream: Not Vorbis data.";
        case OV_EVERSION:
            return "OggSoundStream: Vorbis version mismatch.";
        case OV_EBADHEADER:
            return "OggSoundStream: Invalid Vorbis header.";
        case OV_EFAULT:
            return "OggSoundStream: Internal logic fault (bug or heap/stack corruption.";
        default:
            return "OggSoundStream: Unknown Ogg error.";
    }
}
