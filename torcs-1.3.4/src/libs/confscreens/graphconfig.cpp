/***************************************************************************

    file                 : graphconfig.cpp
    created              : Sun Jun  9 16:30:25 CEST 2002
    copyright            : (C) 2001 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: graphconfig.cpp,v 1.5.2.5 2011/12/29 16:14:20 berniw Exp $

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
    		
    @author	<a href=mailto:eric.espie@torcs.org>Eric Espie</a>
    @version	$Id: graphconfig.cpp,v 1.5.2.5 2011/12/29 16:14:20 berniw Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <tgfclient.h>
#include <graphic.h>
#include <portability.h>

#include "graphconfig.h"

static void	*scrHandle = NULL;
static int	FovEditId;
static int	FovFactorValue = 100;
static int	SmokeEditId;
static int	SmokeValue = 300;
static int	SkidEditId;
static int	SkidValue = 20;
static int	LodFactorEditId;
static tdble	LodFactorValue = 1.0;

// Wheel detail.
static const char *wheelDetailOptionList[] = {
	GR_ATT_WHEELRENDERING_DETAILED,
	GR_ATT_WHEELRENDERING_SIMPLE
};

static float LabelColor[] = {1.0, 0.0, 1.0, 1.0};
static const int nbOptionsWheelDetail = sizeof(wheelDetailOptionList) / sizeof(wheelDetailOptionList[0]);
static int curOptionWheelDetail = 0;
static int WheelDetailOptionId;




static void ExitGraphicOptions(void *prevMenu)
{
	GfuiScreenActivate(prevMenu);
}




static void SaveGraphicOptions(void *prevMenu)
{
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];
	
	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), GR_PARAM_FILE);
	void * grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);

	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, "%", FovFactorValue);
	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SMOKENB, NULL, SmokeValue);
	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_MAXSTRIPBYWHEEL, NULL, SkidValue);
	GfParmSetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_LODFACTOR, NULL, LodFactorValue);	
	GfParmSetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_WHEELRENDERING, wheelDetailOptionList[curOptionWheelDetail]);	
	
	GfParmWriteFile(NULL, grHandle, "graph");
	GfParmReleaseHandle(grHandle);
	ExitGraphicOptions(prevMenu);
}




static void ChangeFov(void * /* dummy */)
{
	char *val;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	val = GfuiEditboxGetString(scrHandle, FovEditId);
	FovFactorValue = strtol(val, (char **)NULL, 0);
	snprintf(buf, BUFSIZE, "%d", FovFactorValue);
	GfuiEditboxSetString(scrHandle, FovEditId, buf);
}




static void ChangeLodFactor(void * /* dummy */)
{
	char *val;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	val = GfuiEditboxGetString(scrHandle, LodFactorEditId);
	sscanf(val, "%g", &LodFactorValue);
	snprintf(buf, BUFSIZE, "%g", LodFactorValue);
	GfuiEditboxSetString(scrHandle, LodFactorEditId, buf);
}




static void ChangeSmoke(void * /* dummy */)
{
	char *val;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	val = GfuiEditboxGetString(scrHandle, SmokeEditId);
	SmokeValue = strtol(val, (char **)NULL, 0);
	snprintf(buf, BUFSIZE, "%d", SmokeValue);
	GfuiEditboxSetString(scrHandle, SmokeEditId, buf);
}




static void ChangeSkid(void * /* dummy */)
{
	char *val;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	val = GfuiEditboxGetString(scrHandle, SkidEditId);
	SkidValue = strtol(val, (char **)NULL, 0);
	snprintf(buf, BUFSIZE, "%d", SkidValue);
	GfuiEditboxSetString(scrHandle, SkidEditId, buf);
}




static void changeWheelDetailState(void *vp)
{
	if (vp == 0) {
		curOptionWheelDetail--;
		if (curOptionWheelDetail < 0) {
	    	curOptionWheelDetail = nbOptionsWheelDetail - 1;
		}
	} else {
		curOptionWheelDetail++;
		if (curOptionWheelDetail == nbOptionsWheelDetail) {
	    	curOptionWheelDetail = 0;
		}
	}
	GfuiLabelSetText(scrHandle, WheelDetailOptionId, wheelDetailOptionList[curOptionWheelDetail]);
}




void *GraphMenuInit(void *prevMenu)
{
	const int	x = 50;
	int y = 370;
	const int x2 = 220;
	const int dy = 30;
	const int width = 130;
	const int center = x2 + width/2;
	
	/* screen already created */
	if (scrHandle) {
		return scrHandle;
	}
	
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	scrHandle = GfuiMenuScreenCreate("Graphic Configuration");
	
	GfuiScreenAddBgImg(scrHandle, "data/img/splash-graphconf.png");
	
	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), GR_PARAM_FILE);
	void * grHandle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);	
	
	GfuiLabelCreate(scrHandle, "Visibility (%):", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);
	FovFactorValue = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_FOVFACT, "%", 100.0);
	snprintf(buf, BUFSIZE, "%d", FovFactorValue);
	FovEditId = GfuiEditboxCreate(scrHandle, buf, GFUI_FONT_MEDIUM_C,
					x2+10, y, width-20, 16, NULL, (tfuiCallback)NULL, ChangeFov);
	
	y -= dy;
	GfuiLabelCreate(scrHandle, "Smoke:", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);
	SmokeValue = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_SMOKENB, NULL, 300.0);
	snprintf(buf, BUFSIZE, "%d", SmokeValue);
	SmokeEditId = GfuiEditboxCreate(scrHandle, buf, GFUI_FONT_MEDIUM_C,
					x2+10, y, width-20, 16, NULL, (tfuiCallback)NULL, ChangeSmoke);
	
	y -= dy;
	GfuiLabelCreate(scrHandle, "Skid Marks:", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);
	SkidValue = (int)GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_MAXSTRIPBYWHEEL, NULL, 20.0);
	snprintf(buf, BUFSIZE, "%d", SkidValue);
	SkidEditId = GfuiEditboxCreate(scrHandle, buf, GFUI_FONT_MEDIUM_C,
					x2+10, y, width-20, 16, NULL, (tfuiCallback)NULL, ChangeSkid);
	
	y -= dy;
	GfuiLabelCreate(scrHandle, "LOD factor:", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);
	LodFactorValue = GfParmGetNum(grHandle, GR_SCT_GRAPHIC, GR_ATT_LODFACTOR, NULL, 1.0);
	snprintf(buf, BUFSIZE, "%g", LodFactorValue);
	LodFactorEditId = GfuiEditboxCreate(scrHandle, buf, GFUI_FONT_MEDIUM_C,
					x2+10, y, width-20, 16, NULL, (tfuiCallback)NULL, ChangeLodFactor);
	
					
	// Wheel detail option
	y -= dy;
	GfuiLabelCreate(scrHandle, "Wheel rendering:", GFUI_FONT_MEDIUM, x, y, GFUI_ALIGN_HL_VB, 0);
	
	GfuiGrButtonCreate(scrHandle, "data/img/arrow-left.png", "data/img/arrow-left.png",
		"data/img/arrow-left.png", "data/img/arrow-left-pushed.png",
		x2, y-5, GFUI_ALIGN_HL_VB, 1,
		(void*)-1, changeWheelDetailState,
		NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	GfuiGrButtonCreate(scrHandle, "data/img/arrow-right.png", "data/img/arrow-right.png",
		"data/img/arrow-right.png", "data/img/arrow-right-pushed.png",
		x2+width, y-5, GFUI_ALIGN_HR_VB, 1,
		(void*)1, changeWheelDetailState,
		NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);

	// Read wheel detail parameter.
	int i;
	const char *optionName = GfParmGetStr(grHandle, GR_SCT_GRAPHIC, GR_ATT_WHEELRENDERING, wheelDetailOptionList[0]);
	for (i = 0; i < nbOptionsWheelDetail; i++) {
		if (strcmp(optionName, wheelDetailOptionList[i]) == 0) {
			curOptionWheelDetail = i;
			break;
		}
	}
				
	WheelDetailOptionId = GfuiLabelCreate(scrHandle, wheelDetailOptionList[curOptionWheelDetail], GFUI_FONT_MEDIUM_C, center, y, GFUI_ALIGN_HC_VB, 32);
	GfuiLabelSetColor(scrHandle, WheelDetailOptionId, LabelColor);
										
	// Navigation
	GfuiButtonCreate(scrHandle, "Accept", GFUI_FONT_LARGE, 210, 40, 150, GFUI_ALIGN_HC_VB, GFUI_MOUSE_UP,
				prevMenu, SaveGraphicOptions, NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);
	
	GfuiButtonCreate(scrHandle, "Cancel", GFUI_FONT_LARGE, 430, 40, 150, GFUI_ALIGN_HC_VB, GFUI_MOUSE_UP,
				prevMenu, GfuiScreenActivate, NULL, (tfuiCallback)NULL, (tfuiCallback)NULL);
	
	GfuiAddKey(scrHandle, 27, "Cancel", prevMenu, GfuiScreenActivate, NULL);
	
	GfParmReleaseHandle(grHandle);
	
	return scrHandle;
}
