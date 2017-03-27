/***************************************************************************

    file                 : musicplayer.cpp
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: musicplayer.cpp,v 1.1.2.2 2011/12/29 09:23:24 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "musicplayer.h"

#include <GL/glut.h>
#include <string.h>
#include <tgf.h>
#include <portability.h>

#include "OggSoundStream.h"
#include "OpenALMusicPlayer.h"


static bool isEnabled()
{
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];
	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), MM_SOUND_PARM_CFG);
	bool enabled = false;
	
	void *handle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
	const char* s = GfParmGetStr(handle, MM_SCT_SOUND, MM_ATT_SOUND_ENABLE, MM_VAL_SOUND_DISABLED);
	
	if (strcmp(s, MM_VAL_SOUND_ENABLED) == 0) {
		enabled = true;
	}
	
	GfParmReleaseHandle(handle);
	return enabled;
}


// Path relative to CWD, e.g "data/music/torcs1.ogg"
static SoundStream* getMenuSoundStream(char* oggFilePath)
{
	static OggSoundStream stream(oggFilePath);
	return &stream;
}


static OpenALMusicPlayer* getMusicPlayer()
{
	const int BUFSIZE = 1024;
	char oggFilePath[BUFSIZE];
	strncpy(oggFilePath, "data/music/torcs1.ogg", BUFSIZE);

	static OpenALMusicPlayer player(getMenuSoundStream(oggFilePath));
	return &player;
}


static void playMenuMusic(int /* value */)
{
	const int nextcallinms = 100;
	
	OpenALMusicPlayer* player = getMusicPlayer();
	if (player->playAndManageBuffer()) {
		glutTimerFunc(nextcallinms, playMenuMusic, 0);
	}
}


void startMenuMusic()
{
	if (isEnabled()) {
		OpenALMusicPlayer* player = getMusicPlayer();
		player->start();
		playMenuMusic(0);
	}
}


void stopMenuMusic()
{
	OpenALMusicPlayer* player = getMusicPlayer();
	player->stop();
	player->rewind();
}