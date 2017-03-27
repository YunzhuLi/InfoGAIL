/***************************************************************************

    file        : soundconfig.cpp
    created     : Thu Dec 12 15:12:07 CET 2004
    copyright   : (C) 2004 Eric Espi�, Bernhard Wymann, baseed on simuconfig.cpp
    email       : berniw@bluewin.ch
    version     : $Id: soundconfig.cpp,v 1.6.2.4 2011/12/29 16:14:20 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   
    		
    @version	$Id: soundconfig.cpp,v 1.6.2.4 2011/12/29 16:14:20 berniw Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <tgfclient.h>
#include <raceinit.h>
#include <graphic.h>
#include <musicplayer/musicplayer.h>
#include <portability.h>

#include "soundconfig.h"

static float LabelColor[] = {1.0, 0.0, 1.0, 1.0};

// list of options.
static const char *soundOptionList[] = {
	GR_ATT_SOUND_STATE_OPENAL,
	GR_ATT_SOUND_STATE_PLIB,
	GR_ATT_SOUND_STATE_DISABLED
};

static const int nbOptions = sizeof(soundOptionList) / sizeof(soundOptionList[0]);
static int curOption = 0;

static const char *menuMusicList[] = {
	MM_VAL_SOUND_DISABLED,
	MM_VAL_SOUND_ENABLED
};
static int curOptionMenuMusic = 0;

// gui label id.
static int SoundOptionId;
static int MenuMusicOptionId;

// volume
static float VolumeValue = 100.0f;
//static int VolumeValueId;

// gui screen handles.
static void	*scrHandle = NULL;
static void	*prevHandle = NULL;


// Read sound configuration.
static void readSoundCfg(void)
{
	const char *optionName;
	int	i;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), GR_SOUND_PARM_CFG);
	
	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	optionName = GfParmGetStr(paramHandle, GR_SCT_SOUND, GR_ATT_SOUND_STATE, soundOptionList[0]);

	for (i = 0; i < nbOptions; i++) {
		if (strcmp(optionName, soundOptionList[i]) == 0) {
			curOption = i;
			break;
		}
	}

    VolumeValue = GfParmGetNum(paramHandle, GR_SCT_SOUND, GR_ATT_SOUND_VOLUME, "%", 100.0f);
	if (VolumeValue>100.0f) {
		VolumeValue = 100.0f;
	} 
	if (VolumeValue < 0.0f) {
		VolumeValue = 0.0f;
	}

	GfParmReleaseHandle(paramHandle);

	GfuiLabelSetText(scrHandle, SoundOptionId, soundOptionList[curOption]);
	
	// Read Menu music optons
	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), MM_SOUND_PARM_CFG);
	paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	optionName = GfParmGetStr(paramHandle, MM_SCT_SOUND, MM_ATT_SOUND_ENABLE, MM_VAL_SOUND_DISABLED);
	
	if (strcmp(optionName, MM_VAL_SOUND_ENABLED) == 0) {
		GfuiLabelSetText(scrHandle, MenuMusicOptionId, MM_VAL_SOUND_ENABLED);
		curOptionMenuMusic = 1;
	} else {
		GfuiLabelSetText(scrHandle, MenuMusicOptionId, MM_VAL_SOUND_DISABLED);
		curOptionMenuMusic = 0;
	}
	
	GfParmReleaseHandle(paramHandle);
}


// Save the choosen values in the corresponding parameter file.
static void saveSoundOption(void *)
{
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), GR_SOUND_PARM_CFG);
	void *paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	GfParmSetStr(paramHandle, GR_SCT_SOUND, GR_ATT_SOUND_STATE, soundOptionList[curOption]);
	GfParmSetNum(paramHandle, GR_SCT_SOUND, GR_ATT_SOUND_VOLUME, "%", VolumeValue);
	GfParmWriteFile(NULL, paramHandle, "sound");
	GfParmReleaseHandle(paramHandle);

	// Write Menu music optons
	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), MM_SOUND_PARM_CFG);
	paramHandle = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	GfParmSetStr(paramHandle, MM_SCT_SOUND, MM_ATT_SOUND_ENABLE, menuMusicList[curOptionMenuMusic]);
	GfParmWriteFile(NULL, paramHandle, "sound");
	GfParmReleaseHandle(paramHandle);
	
	if (curOptionMenuMusic == 1) {
		startMenuMusic();
	} else {
		stopMenuMusic();
	}
	
	// Return to previous screen.
	GfuiScreenActivate(prevHandle);
	return;
}


// Toggle sound state openal/plib/disabled.
static void changeSoundState(void *vp)
{
	if (vp == 0) {
		curOption--;
		if (curOption < 0) {
	    	curOption = nbOptions - 1;
		}
	} else {
		curOption++;
		if (curOption == nbOptions) {
	    curOption = 0;
	}
    }
    GfuiLabelSetText(scrHandle, SoundOptionId, soundOptionList[curOption]);
}


// Toggle menu music state between enabled and disabled.
static void changeMenuMusicState(void *vp)
{
	if (curOptionMenuMusic == 0) {
		curOptionMenuMusic = 1;
	} else {
		curOptionMenuMusic = 0;
	}

	GfuiLabelSetText(scrHandle, MenuMusicOptionId, menuMusicList[curOptionMenuMusic]);
}


// Volume
/*
static void changeVolume(void * )
{
    char	*val;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];
    val = GfuiEditboxGetString(scrHandle, VolumeValueId);
    sscanf(val, "%g", &VolumeValue);
    snprintf(buf, BUFSIZE, "%g", VolumeValue);
    GfuiEditboxSetString(scrHandle, VolumeValueId, buf);
}
*/

static void onActivate(void * /* dummy */)
{
	readSoundCfg();
}


// Sound menu
void * SoundMenuInit(void *prevMenu)
{
	int x, y, x2, x3, x4, dy;
//	char buf[1024];
	

	// Has screen already been created?
	if (scrHandle) {
		return scrHandle;
	}

	prevHandle = prevMenu;

	scrHandle = GfuiScreenCreateEx((float*)NULL, NULL, onActivate, NULL, (tfuiCallback)NULL, 1);
	GfuiTitleCreate(scrHandle, "Sound Configuration", 0);
	GfuiScreenAddBgImg(scrHandle, "data/img/splash-qrdrv.png");

	x = 20;
	x2 = 200;
	x3 = 340;
	x4 = (x2+x3)/2;
	y = 400;
	dy = 30;

	y -= dy;

	GfuiLabelCreate(scrHandle, "Sound System:", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);
	GfuiGrButtonCreate(scrHandle, "data/img/arrow-left.png", "data/img/arrow-left.png",
			"data/img/arrow-left.png", "data/img/arrow-left-pushed.png",
			x2, y-5, GFUI_ALIGN_HL_VB, 1,
			(void*)-1, changeSoundState,
			NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	GfuiGrButtonCreate(scrHandle, "data/img/arrow-right.png", "data/img/arrow-right.png",
			"data/img/arrow-right.png", "data/img/arrow-right-pushed.png",
			x3, y-5, GFUI_ALIGN_HR_VB, 1,
			(void*)1, changeSoundState,
			NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	SoundOptionId = GfuiLabelCreate(scrHandle, "", GFUI_FONT_MEDIUM_C, x4, y, GFUI_ALIGN_HC_VB, 32);
	GfuiLabelSetColor(scrHandle, SoundOptionId, LabelColor);

	y -= dy;
	
	GfuiLabelCreate(scrHandle, "Menu Music:", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);
	GfuiGrButtonCreate(scrHandle, "data/img/arrow-left.png", "data/img/arrow-left.png",
			"data/img/arrow-left.png", "data/img/arrow-left-pushed.png",
			x2, y-5, GFUI_ALIGN_HL_VB, 1,
			(void*)-1, changeMenuMusicState,
			NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	GfuiGrButtonCreate(scrHandle, "data/img/arrow-right.png", "data/img/arrow-right.png",
			"data/img/arrow-right.png", "data/img/arrow-right-pushed.png",
			x3, y-5, GFUI_ALIGN_HR_VB, 1,
			(void*)1, changeMenuMusicState,
			NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	MenuMusicOptionId = GfuiLabelCreate(scrHandle, "", GFUI_FONT_MEDIUM_C, x4, y, GFUI_ALIGN_HC_VB, 32);
	GfuiLabelSetColor(scrHandle, MenuMusicOptionId, LabelColor);

	/*
    y -= dy;
    GfuiLabelCreate(scrHandle, "Volume:", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);

	
    snprintf(buf, "%f", VolumeValue);
    VolumeValueId = GfuiEditboxCreate(scrHandle, buf, GFUI_FONT_MEDIUM_C,
				    x2+10, y+2, x4-x2+20, 16, NULL, (tfuiCallback)NULL, changeVolume);


*/
	GfuiButtonCreate(scrHandle, "Accept", GFUI_FONT_LARGE, 210, 40, 150, GFUI_ALIGN_HC_VB, GFUI_MOUSE_UP,
	NULL, saveSoundOption, NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	GfuiButtonCreate(scrHandle, "Cancel", GFUI_FONT_LARGE, 430, 40, 150, GFUI_ALIGN_HC_VB, GFUI_MOUSE_UP,
	prevMenu, GfuiScreenActivate, NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	GfuiAddKey(scrHandle, 13, "Save", NULL, saveSoundOption, NULL);
	GfuiAddKey(scrHandle, 27, "Cancel Selection", prevMenu, GfuiScreenActivate, NULL);
	GfuiAddSKey(scrHandle, GLUT_KEY_F12, "Screen-Shot", NULL, GfuiScreenShot, NULL);
	GfuiAddSKey(scrHandle, GLUT_KEY_LEFT, "Previous Option in list", (void*)0, changeSoundState, NULL);
	GfuiAddSKey(scrHandle, GLUT_KEY_RIGHT, "Next Option in list", (void*)1, changeSoundState, NULL);

	readSoundCfg();

	return scrHandle;
}
