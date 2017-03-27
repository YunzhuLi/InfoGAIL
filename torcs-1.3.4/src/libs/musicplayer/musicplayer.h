#ifndef __musicplayer_h__
#define __musicplayer_h__

/***************************************************************************

    file                 : musicplayer.h
    created              : Fri Dec 23 17:35:18 CET 2011
    copyright            : (C) 2011 Bernhard Wymann
    email                : berniw@bluewin.ch
    version              : $Id: musicplayer.h,v 1.1.2.1 2011/12/28 15:11:57 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define MM_SOUND_PARM_CFG			"config/sound.xml"
#define MM_SCT_SOUND				"Menu Music"
#define MM_ATT_SOUND_ENABLE			"enable"
#define MM_VAL_SOUND_ENABLED		"enabled"
#define MM_VAL_SOUND_DISABLED		"disabled"

extern void startMenuMusic();
extern void stopMenuMusic();

#endif //__musicplayer_h__