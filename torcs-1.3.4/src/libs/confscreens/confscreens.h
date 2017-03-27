/***************************************************************************

    file                 : racemantools.h
    created              : Sat Mar 18 23:33:01 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: confscreens.h,v 1.2.2.1 2008/12/31 03:53:55 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
/**
    @defgroup	racemantools	Tools for race managers.
    This is a collection of useful functions for programming a race manager.
*/

#ifndef __RACEMANTOOLS_H__
#define __RACEMANTOOLS_H__

#include <car.h>
#include <raceman.h>
#include <track.h>
#include <simu.h>

typedef struct
{
    void        *param;		/* Race manager parameters where to set the selected track */
    void        *prevScreen;	/* Race manager screen to go back */
    void        *nextScreen;	/* Race manager screen to go after select */
    tTrackItf	trackItf;	/* Track module interface */
} tRmTrackSelect;

typedef struct
{
    void        *param;
    void        *prevScreen;	/* Race manager screen to go back */
    void        *nextScreen;	/* Race manager screen to go after select */
} tRmDrvSelect;

typedef struct
{
    void        	*param;
    void        	*prevScreen;
    void        	*nextScreen;	/* Race manager screen to go after select */
    const char *title;
    unsigned int	confMask;	/* Tell what to configure */
#define RM_CONF_RACE_LEN	0x00000001
#define RM_CONF_DISP_MODE	0x00000002
} tRmRaceParam;

typedef void (*tfSelectFile) (char *);

typedef struct
{
    const char *title;
    char		*path;
    void        	*prevScreen;
    tfSelectFile	select;
} tRmFileSelect;


extern void RmTrackSelect(void * /* vs */);
extern char *RmGetTrackName(char * /* category */, char * /* trackName */);

extern void RmDriversSelect(void * /* vs */);
extern void RmDriverSelect(void * /* vs */);

extern void RmPitMenuStart(tCarElt * /* car */, void * /* userdata */, tfuiCallback /* callback */);

extern void RmLoadingScreenStart(const char * /* text */, const char * /* bgimg */);
extern void RmLoadingScreenSetText(const char * /* text */);
extern void RmShutdownLoadingScreen(void);

extern void RmShowResults(void * /* prevHdle */, tRmInfo * /* info */);

extern void *RmTwoStateScreen(const char *title,
			      const char *label1, const char *tip1, void *screen1,
			      const char *label2, const char *tip2, void *screen2);

extern void *RmTriStateScreen(const char *title,
			      const char *label1, const char *tip1, void *screen1,
			      const char *label2, const char *tip2, void *screen2,
			      const char *label3, const char *tip3, void *screen3);

extern void RmDisplayStartRace(tRmInfo *info, void *startScr, void *abortScr);


extern void RmRaceParamMenu(void *vrp);

extern void RmShowStandings(void *prevHdle, tRmInfo *info);

extern void RmFileSelect(void *vs);

#endif /* __RACEMANTOOLS_H__ */

