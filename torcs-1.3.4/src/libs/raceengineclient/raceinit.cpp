/***************************************************************************

    file        : raceinit.cpp
    created     : Sat Nov 16 10:34:35 CET 2002
    copyright   : (C) 2002 by Eric Espi�                       
    email       : eric.espie@torcs.org   
    version     : $Id: raceinit.cpp,v 1.18.2.10 2012/06/11 11:28:01 berniw Exp $                                  

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
    @version	$Id: raceinit.cpp,v 1.18.2.10 2012/06/11 11:28:01 berniw Exp $
*/


/* Race initialization routines */

#include <stdlib.h>
#include <stdio.h>
#include <tgfclient.h>
#include <raceman.h>
#include <robot.h>
#include <racescreens.h>
#include <robottools.h>
#include <portability.h>
#include <string>
#include <map>
#include <portability.h>
#include <musicplayer/musicplayer.h>

#include "raceengine.h"
#include "racemain.h"
#include "racestate.h"
#include "racegl.h"
#include "raceresults.h"

#include "raceinit.h"

static const char *level_str[] = { ROB_VAL_ROOKIE, ROB_VAL_AMATEUR, ROB_VAL_SEMI_PRO, ROB_VAL_PRO };

static tModList *reEventModList = 0;
tModList *ReRaceModList = 0;

typedef struct 
{
	int index;
	tModList *curmod;
} tMainMod;

/* Race Engine Initialization */
void
ReInit(void)
{
	tRmMovieCapture *capture;

	ReShutdown();

	ReInfo = (tRmInfo *)calloc(1, sizeof(tRmInfo));
	ReInfo->s = (tSituation *)calloc(1, sizeof(tSituation));
	ReInfo->modList = &ReRaceModList;

	const int BUFSIZE = 1024;
	char buf[BUFSIZE];
	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), RACE_ENG_CFG);

	ReInfo->_reParam = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);

	GfOut("Loading Track Loader...\n");
	const char* dllname = GfParmGetStr(ReInfo->_reParam, "Modules", "track", "");
	snprintf(buf, BUFSIZE, "%smodules/track/%s.%s", GetLibDir (), dllname, DLLEXT);
	if (GfModLoad(0, buf, &reEventModList)) return;
	reEventModList->modInfo->fctInit(reEventModList->modInfo->index, &ReInfo->_reTrackItf);

	GfOut("Loading Graphic Engine...\n");
	dllname = GfParmGetStr(ReInfo->_reParam, "Modules", "graphic", "");
	snprintf(buf, BUFSIZE, "%smodules/graphic/%s.%s", GetLibDir (), dllname, DLLEXT);
	if (GfModLoad(0, buf, &reEventModList)) return;
	reEventModList->modInfo->fctInit(reEventModList->modInfo->index, &ReInfo->_reGraphicItf);

	capture = &(ReInfo->movieCapture);
	if (strcmp(GfParmGetStr(ReInfo->_reParam, RM_SECT_MOVIE_CAPTURE, RM_ATT_CAPTURE_ENABLE, "no"), "no") == 0){
		capture->enabled = 0;
	} else {
		capture->enabled = 1;
		capture->state = 0;
		capture->deltaFrame = 1.0 / GfParmGetNum(ReInfo->_reParam, RM_SECT_MOVIE_CAPTURE, RM_ATT_CAPTURE_FPS, NULL, 1.0);
		capture->outputBase = GfParmGetStr(ReInfo->_reParam, RM_SECT_MOVIE_CAPTURE, RM_ATT_CAPTURE_OUT_DIR, "/tmp");
		capture->deltaSimu = RCM_MAX_DT_SIMU;
	}

	ReInfo->_reGameScreen = ReHookInit();
}


/* Race Engine Exit */
void ReShutdown(void)
{
	/* Free previous situation */
	if (ReInfo) {
		ReInfo->_reTrackItf.trkShutdown();

		GfModUnloadList(&reEventModList);

		if (ReInfo->results) {
	    	GfParmReleaseHandle(ReInfo->results);
		}
		if (ReInfo->_reParam) {
			GfParmReleaseHandle(ReInfo->_reParam);
		}
		FREEZ(ReInfo->s);
		FREEZ(ReInfo->carList);
		FREEZ(ReInfo->rules);
		FREEZ(ReInfo->_reFilename);
		FREEZ(ReInfo);
    }
}


void
ReStartNewRace(void * /* dummy */)
{
	ReInitResults();
	ReStateManage();
}


/* Launch a race manager */
static void reSelectRaceman(void *params)
{
	char *s, *e, *m;

	ReInfo->params = params;
	FREEZ(ReInfo->_reFilename);

	s = GfParmGetFileName(params);
	while ((m = strstr(s, "/")) != 0) {
		s = m + 1;
	}

	e = strstr(s, PARAMEXT);
	ReInfo->_reFilename = strndup(s, e-s+1);
	ReInfo->_reFilename[e-s] = '\0';
	ReInfo->_reName = GfParmGetStr(params, RM_SECT_HEADER, RM_ATTR_NAME, "");
	ReStateApply(RE_STATE_CONFIG);
}


void ReRunRaceOnConsole(const char* raceconfig)
{
	ReInfo = (tRmInfo *)calloc(1, sizeof(tRmInfo));
	ReInfo->s = (tSituation *)calloc(1, sizeof(tSituation));
	ReInfo->modList = &ReRaceModList;

	const int BUFSIZE = 1024;
	char buf[BUFSIZE];
	snprintf(buf, BUFSIZE, "%s%s", GetLocalDir(), RACE_ENG_CFG);

	ReInfo->_reParam = GfParmReadFile(buf, GFPARM_RMODE_REREAD | GFPARM_RMODE_CREAT);
	ReInfo->_displayMode = RM_DISP_MODE_CONSOLE;

	GfOut("Loading Track Loader...\n");
	const char* dllname = GfParmGetStr(ReInfo->_reParam, "Modules", "track", "");
	snprintf(buf, BUFSIZE, "%smodules/track/%s.%s", GetLibDir (), dllname, DLLEXT);
	if (GfModLoad(0, buf, &reEventModList)) return;
	reEventModList->modInfo->fctInit(reEventModList->modInfo->index, &ReInfo->_reTrackItf);

	ReInfo->movieCapture.enabled = 0;

	const char *s, *e, *m;

	ReInfo->params = GfParmReadFile(raceconfig, GFPARM_RMODE_STD);
	if (ReInfo->params == 0) {
		GfError("Could not open file: %s\n", raceconfig);
		exit(1);
	}

	s = GfParmGetFileName(ReInfo->params);
	while ((m = strstr(s, "/")) != 0) {
		s = m + 1;
	}

	e = strstr(s, PARAMEXT);
	ReInfo->_reFilename = strndup(s, e-s+1);
	ReInfo->_reFilename[e-s] = '\0';
	ReInfo->_reName = GfParmGetStr(ReInfo->params, RM_SECT_HEADER, RM_ATTR_NAME, "");

	ReInitResults();

	ReStateApply((void *) RE_STATE_EVENT_INIT);	
	GfParmReleaseHandle(ReInfo->params);
	ReShutdown();
}


/* Register a race manager */
static void
reRegisterRaceman(tFList *racemanCur)
{
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];
	
	snprintf(buf, BUFSIZE, "%sconfig/raceman/%s", GetLocalDir(), racemanCur->name);
	racemanCur->userData = GfParmReadFile(buf, GFPARM_RMODE_STD);
	racemanCur->dispName = strndup(GfParmGetStr(racemanCur->userData, RM_SECT_HEADER, RM_ATTR_NAME, 0), 1024);
}

/* Sort race managers by priority */
static void
reSortRacemanList(tFList **racemanList)
{
	tFList *head;
	tFList *cur;
	tFList *tmp;

	head = *racemanList;
	cur = head;
	while (cur->next != head) {
		if (GfParmGetNum(cur->userData, RM_SECT_HEADER, RM_ATTR_PRIO, NULL, 10000) >
			GfParmGetNum(cur->next->userData, RM_SECT_HEADER, RM_ATTR_PRIO, NULL, 10000)) {
			/* swap cur and cur->next */
			tmp = cur->next;
			if (tmp->next != cur) {
				cur->next = tmp->next;
				tmp->next = cur;
				tmp->prev = cur->prev;
				cur->prev = tmp;
				cur->next->prev = cur;
				tmp->prev->next = tmp;
			}
			if (cur == head) {
				head = tmp;
			} else {
				cur = tmp->prev;
			}
		} else {
			cur = cur->next;
		}
	}
	*racemanList = head;
}


/* Load race managers selection menu */
void ReAddRacemanListButton(void *menuHandle)
{
	tFList *racemanList;
	tFList *racemanCur;

	racemanList = GfDirGetListFiltered("config/raceman", "xml");
	if (!racemanList) {
		GfOut("No race manager available\n");
		return;
	}

	racemanCur = racemanList;
	do {
		reRegisterRaceman(racemanCur);
		racemanCur = racemanCur->next;
	} while (racemanCur != racemanList);

	reSortRacemanList(&racemanList);

	racemanCur = racemanList;
	do {
		GfuiMenuButtonCreate(menuHandle,
				racemanCur->dispName,
				GfParmGetStr(racemanCur->userData, RM_SECT_HEADER, RM_ATTR_DESCR, ""),
				racemanCur->userData,
				reSelectRaceman);
		racemanCur = racemanCur->next;
	} while (racemanCur != racemanList);

	// The list contains at least one element, checked above.
	tFList *rl = racemanList;
	do {
		tFList *tmp = rl;
		rl = rl->next;
		// Do not free userData and dispName, is in use.
		freez(tmp->name);
		free(tmp);
	} while (rl != racemanList);
}



/*
 * Function
 *	initStartingGrid
 *
 * Description
 *	Place the cars on the starting grid
 *
 * Parameters
 *	Race Information structure initialized
 *
 * Return
 *	none
 */
static void
initStartingGrid(void)
{
	int i;
	tTrackSeg *curseg;
	int rows;
	tdble a, b; //, wi2;
	tdble d1, d2,d3;
	tdble startpos, tr, ts;
	tdble speedInit;
	tdble heightInit;
	tCarElt *car;
	const char *pole;
	void *trHdle = ReInfo->track->params;
	void *params = ReInfo->params;
	const int BUFSIZE = 1024;
	char path[BUFSIZE];

	snprintf(path, BUFSIZE, "%s/%s", ReInfo->_reRaceName, RM_SECT_STARTINGGRID);

	/* Search for the first turn for find the pole side */
	curseg = ReInfo->track->seg->next;
	while (curseg->type == TR_STR) {
		/* skip the straight segments */
		curseg = curseg->next;
	}
	/* Set the pole for the inside of the first turn */
	if (curseg->type == TR_LFT) {
		pole = GfParmGetStr(params, path, RM_ATTR_POLE, "left");
	} else {
		pole = GfParmGetStr(params, path, RM_ATTR_POLE, "right");
	}
	/* Tracks definitions can force the pole side */
	pole = GfParmGetStr(trHdle, RM_SECT_STARTINGGRID, RM_ATTR_POLE, pole);

	if (strcmp(pole, "left") == 0) {
		a = ReInfo->track->width;
		b = -a;
	} else {
		a = 0;
		b = ReInfo->track->width;
	}
	//wi2 = ReInfo->track->width * 0.5;

	rows = (int)GfParmGetNum(params, path, RM_ATTR_ROWS, (char*)NULL, 2);
	rows = (int)GfParmGetNum(trHdle, RM_SECT_STARTINGGRID, RM_ATTR_ROWS, (char*)NULL, rows);
	d1 = GfParmGetNum(params, path, RM_ATTR_TOSTART, (char*)NULL, 10);
	d1 = GfParmGetNum(trHdle, RM_SECT_STARTINGGRID, RM_ATTR_TOSTART, (char*)NULL, d1);
	d2 = GfParmGetNum(params, path, RM_ATTR_COLDIST, (char*)NULL, 10);
	d2 = GfParmGetNum(trHdle, RM_SECT_STARTINGGRID, RM_ATTR_COLDIST, (char*)NULL, d2);
	d3 = GfParmGetNum(params, path, RM_ATTR_COLOFFSET, (char*)NULL, 5);
	d3 = GfParmGetNum(trHdle, RM_SECT_STARTINGGRID, RM_ATTR_COLOFFSET, (char*)NULL, d3);
	speedInit = GfParmGetNum(params, path, RM_ATTR_INITSPEED, (char*)NULL, 0.0);
	heightInit = GfParmGetNum(params, path, RM_ATTR_INITHEIGHT, (char*)NULL, 0.3);
	heightInit = GfParmGetNum(trHdle, RM_SECT_STARTINGGRID, RM_ATTR_INITHEIGHT, (char*)NULL, heightInit);

	if (rows < 1) {
		rows = 1;
	}
	for (i = 0; i < ReInfo->s->_ncars; i++) {
		car = &(ReInfo->carList[i]);
		car->_speed_x = speedInit;
		car->_commitBestLapTime = true;
		startpos = ReInfo->track->length - (d1 + (i / rows) * d2 + (i % rows) * d3);
		tr = a + b * ((i % rows) + 1) / (rows + 1);
		curseg = ReInfo->track->seg;  /* last segment */
		while (startpos < curseg->lgfromstart) {
			curseg = curseg->prev;
		}
		ts = startpos - curseg->lgfromstart;
		car->_trkPos.seg = curseg;
		car->_trkPos.toRight = tr;
		switch (curseg->type) {
			case TR_STR:
				car->_trkPos.toStart = ts;
				RtTrackLocal2Global(&(car->_trkPos), &(car->_pos_X), &(car->_pos_Y), TR_TORIGHT);
				car->_yaw = curseg->angle[TR_ZS];
				break;
			case TR_RGT:
				car->_trkPos.toStart = ts / curseg->radius;
				RtTrackLocal2Global(&(car->_trkPos), &(car->_pos_X), &(car->_pos_Y), TR_TORIGHT);
				car->_yaw = curseg->angle[TR_ZS] - car->_trkPos.toStart;
				break;
			case TR_LFT:
				car->_trkPos.toStart = ts / curseg->radius;
				RtTrackLocal2Global(&(car->_trkPos), &(car->_pos_X), &(car->_pos_Y), TR_TORIGHT);
				car->_yaw = curseg->angle[TR_ZS] + car->_trkPos.toStart;
				break;
		}
		car->_pos_Z = RtTrackHeightL(&(car->_trkPos)) + heightInit;

		NORM0_2PI(car->_yaw);
		ReInfo->_reSimItf.config(car, ReInfo);
	}
}


static void
initPits(void)
{
	tTrackPitInfo *pits;
	int i, j;

	/*
	typedef std::map<std::string, int> tTeamsMap;
	typedef tTeamsMap::const_iterator tTeamsMapIterator;
	tTeamsMap teams;
	tTeamsMapIterator teamsIterator;

	// create a list with the teams, a pit can just be used by one team.
	for (i = 0; i < ReInfo->s->_ncars; i++) {
		tCarElt *car = &(ReInfo->carList[i]);
		teams[car->_teamname] = teams[car->_teamname] + 1; 
	}	

	for (teamsIterator = teams.begin(); teamsIterator != teams.end(); ++teamsIterator) {
		printf("----------------- %s\t%d\n", (teamsIterator->first).c_str(), teamsIterator->second); 
	}
	*/

	// How many cars are sharing a pit?
	int carsPerPit = (int) GfParmGetNum(ReInfo->params, ReInfo->_reRaceName, RM_ATTR_CARSPERPIT, NULL, 1.0f);
	if (carsPerPit < 1) {
		carsPerPit = 1;
	} else if (carsPerPit > TR_PIT_MAXCARPERPIT) {
		carsPerPit = TR_PIT_MAXCARPERPIT;
	}
	GfOut("Cars per pit: %d\n", carsPerPit);

	switch (ReInfo->track->pits.type) {
		case TR_PIT_ON_TRACK_SIDE:
			pits = &(ReInfo->track->pits);
			pits->driversPitsNb = ReInfo->s->_ncars;
			pits->carsPerPit = carsPerPit;

			// Initialize pit data for every pit, necessary because of restarts
			// (track gets not reloaded, no calloc).
			for (i = 0; i < pits->nMaxPits; i++) {
				tTrackOwnPit *pit = &(pits->driversPits[i]);
				pit->freeCarIndex = 0;
				pit->pitCarIndex = TR_PIT_STATE_FREE;
				for (j = 0; j < TR_PIT_MAXCARPERPIT; j++) {
					pit->car[j] = NULL;
				}
			}

			// Assign cars to pits. Inefficient (O(n*n)), but just at initialization, so do not care.
			// One pit can just host cars of one team (this matches with the reality) 
			for (i = 0; i < ReInfo->s->_ncars; i++) {
				// Find pit for the cars team.
				tCarElt *car = &(ReInfo->carList[i]);
				for (j = 0; j < pits->nMaxPits; j++) {
					tTrackOwnPit *pit = &(pits->driversPits[j]);
					// Put car in this pit if the pit is unused or used by a teammate and there is
					// space left.
					if (pit->freeCarIndex <= 0 ||
						(strcmp(pit->car[0]->_teamname, car->_teamname) == 0 && pit->freeCarIndex < carsPerPit))
					{
						// Assign car to pit.
						pit->car[pit->freeCarIndex] = car;
						// If this is the first car set up more pit values, assumtion: the whole team
						// uses the same car. If not met it does not matter much, but the car might be
						// captured a bit too easy or too hard.
						if (pit->freeCarIndex == 0) {
							pit->pitCarIndex = TR_PIT_STATE_FREE;
							pit->lmin = pit->pos.seg->lgfromstart + pit->pos.toStart - pits->len / 2.0 + car->_dimension_x / 2.0;
							if (pit->lmin > ReInfo->track->length) {
								pit->lmin -= ReInfo->track->length;
							}
							pit->lmax = pit->pos.seg->lgfromstart + pit->pos.toStart + pits->len / 2.0 - car->_dimension_x / 2.0;
							if (pit->lmax > ReInfo->track->length) {
								pit->lmax -= ReInfo->track->length;
							}
						}
						(pit->freeCarIndex)++;
						ReInfo->carList[i]._pit = pit;
						// Assigned, continue with next car.
						break;
					}
				}
			}

			// Print out assignments.
			for (i = 0; i < pits->nMaxPits; i++) {
				tTrackOwnPit *pit = &(pits->driversPits[i]);
				for (j = 0; j < pit->freeCarIndex; j++) {
					if (j == 0) {
						GfOut("Pit %d, Team: %s, ", i, pit->car[j]->_teamname); 
					}
					GfOut("%d: %s ", j, pit->car[j]->_name);
				}
				if (j > 0) {
					GfOut("\n");
				}
			}

			break;
		case TR_PIT_ON_SEPARATE_PATH:
			break;
		case TR_PIT_NONE:
			break;
	}
}

/** Initialize the cars for a race.
    The car are positionned on the starting grid.
    @return	0 Ok
		-1 Error
 */
int
ReInitCars(void)
{
	int nCars;
	int index;
	int i, j, k;
	int robotIdx;
	tModInfo *curModInfo;
	tRobotItf *curRobot;
	void *handle;
	//char *category;
	void *cathdle;
	void *carhdle;
	void *robhdle;
	tCarElt *elt;
	//char *str;
	//int focusedIdx;
	void *params = ReInfo->params;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE], path[BUFSIZE];

	/* Get the number of cars racing */
	nCars = GfParmGetEltNb(params, RM_SECT_DRIVERS_RACING);
	GfOut("loading %d cars\n", nCars);

	FREEZ(ReInfo->carList);
	ReInfo->carList = (tCarElt*)calloc(nCars, sizeof(tCarElt));
	FREEZ(ReInfo->rules);
	ReInfo->rules = (tRmCarRules*)calloc(nCars, sizeof(tRmCarRules));
	//focusedIdx = (int)GfParmGetNum(ReInfo->params, RM_SECT_DRIVERS, RM_ATTR_FOCUSEDIDX, NULL, 0);
	index = 0;

	for (i = 1; i < nCars + 1; i++) {
		/* Get Shared library name */
		snprintf(path, BUFSIZE, "%s/%d", RM_SECT_DRIVERS_RACING, i);
		const char* cardllname = GfParmGetStr(ReInfo->params, path, RM_ATTR_MODULE, "");
		robotIdx = (int)GfParmGetNum(ReInfo->params, path, RM_ATTR_IDX, NULL, 0);
		snprintf(path, BUFSIZE, "%sdrivers/%s/%s.%s", GetLibDir (), cardllname, cardllname, DLLEXT);

		/* load the robot shared library */
		if (GfModLoad(CAR_IDENT, path, ReInfo->modList)) {
			GfTrace("Pb with loading %s driver\n", path);
			break;
		}

		/* search for corresponding index */
		for (j = 0; j < MAX_MOD_ITF; j++) {
			if ((*(ReInfo->modList))->modInfo[j].index == robotIdx) {
				/* good robot found */
				curModInfo = &((*(ReInfo->modList))->modInfo[j]);
				GfOut("Driver's name: %s\n", curModInfo->name);
				/* retrieve the robot interface (function pointers) */
				curRobot = (tRobotItf*)calloc(1, sizeof(tRobotItf));
				curModInfo->fctInit(robotIdx, (void*)(curRobot));
				snprintf(buf, BUFSIZE, "%sdrivers/%s/%s.xml", GetLocalDir(), cardllname, cardllname);
				robhdle = GfParmReadFile(buf, GFPARM_RMODE_STD);
				if (!robhdle) {
					snprintf(buf, BUFSIZE, "drivers/%s/%s.xml", cardllname, cardllname);
					robhdle = GfParmReadFile(buf, GFPARM_RMODE_STD);
				}
				if (robhdle != NULL) {
					elt = &(ReInfo->carList[index]);
					GF_TAILQ_INIT(&(elt->_penaltyList));

					elt->index = index;
					elt->robot = curRobot;
					elt->_paramsHandle = robhdle;
					elt->_driverIndex = robotIdx;
					strncpy(elt->_modName, cardllname, MAX_NAME_LEN - 1);
					elt->_modName[MAX_NAME_LEN - 1] = 0;

					snprintf(path, BUFSIZE, "%s/%s/%d", ROB_SECT_ROBOTS, ROB_LIST_INDEX, robotIdx);
					strncpy(elt->_name, GfParmGetStr(robhdle, path, ROB_ATTR_NAME, "<none>"), MAX_NAME_LEN - 1);
					elt->_name[MAX_NAME_LEN - 1] = 0;
					strncpy(elt->_teamname, GfParmGetStr(robhdle, path, ROB_ATTR_TEAM, "<none>"), MAX_NAME_LEN - 1);
					elt->_teamname[MAX_NAME_LEN - 1] = 0;
					
					strncpy(elt->_carName, GfParmGetStr(robhdle, path, ROB_ATTR_CAR, ""), MAX_NAME_LEN - 1);
					elt->_carName[MAX_NAME_LEN - 1] = 0;
					elt->_raceNumber = (int)GfParmGetNum(robhdle, path, ROB_ATTR_RACENUM, (char*)NULL, 0);
					if (strcmp(GfParmGetStr(robhdle, path, ROB_ATTR_TYPE, ROB_VAL_ROBOT), ROB_VAL_ROBOT)) {
						elt->_driverType = RM_DRV_HUMAN;
						if (ReInfo->_displayMode == RM_DISP_MODE_CONSOLE) {
							GfError("Human drivers not allowed in console race, fix race setup.\n");
							exit(1);
						}
					} else {
						elt->_driverType = RM_DRV_ROBOT;
					}
					elt->_skillLevel = 0;
					const char* str = GfParmGetStr(robhdle, path, ROB_ATTR_LEVEL, ROB_VAL_SEMI_PRO);
					for(k = 0; k < (int)(sizeof(level_str)/sizeof(char*)); k++) {
						if (strcmp(level_str[k], str) == 0) {
							elt->_skillLevel = k;
							break;
						}
					}
					elt->_startRank  = index;
					elt->_pos        = index+1;
					elt->_remainingLaps = ReInfo->s->_totLaps;

					/* handle contains the drivers modifications to the car */
					/* Read Car model specifications */
					snprintf(buf, BUFSIZE, "cars/%s/%s.xml", elt->_carName, elt->_carName);
					GfOut("Car Specification: %s\n", buf);
					carhdle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
					const char* category = GfParmGetStr(carhdle, SECT_CAR, PRM_CATEGORY, NULL);
					snprintf(buf, BUFSIZE, "Loading Driver %-20s... Car: %s", curModInfo->name, elt->_carName);
					RmLoadingScreenSetText(buf);
					if (category != 0) {
						strncpy(elt->_category, category, MAX_NAME_LEN - 1);
						elt->_category[MAX_NAME_LEN - 1] = '\0';

						/* Read Car Category specifications */
						snprintf(buf, BUFSIZE, "categories/%s.xml", category);
						GfOut("Category Specification: %s\n", buf);
						cathdle = GfParmReadFile(buf, GFPARM_RMODE_STD | GFPARM_RMODE_CREAT);
						if (GfParmCheckHandle(cathdle, carhdle)) {
							GfTrace("Car %s not in Category %s (driver %s) !!!\n", elt->_carName, category, elt->_name);
							break;
						}
						carhdle = GfParmMergeHandles(cathdle, carhdle,
										GFPARM_MMODE_SRC | GFPARM_MMODE_DST | GFPARM_MMODE_RELSRC | GFPARM_MMODE_RELDST);
										curRobot->rbNewTrack(robotIdx, ReInfo->track, carhdle, &handle, ReInfo->s);
						if (handle != NULL) {
							if (GfParmCheckHandle(carhdle, handle)) {
								GfTrace("Bad Car parameters for driver %s\n", elt->_name);
								break;
							}
							handle = GfParmMergeHandles(carhdle, handle,
										GFPARM_MMODE_SRC | GFPARM_MMODE_DST | GFPARM_MMODE_RELSRC | GFPARM_MMODE_RELDST);
						} else {
							handle = carhdle;
						}
						elt->_carHandle = handle;
						//GfParmWriteFile("toto.xml", handle, "toto");
					} else {
						elt->_category[0] = '\0';
						GfTrace("Bad Car category for driver %s\n", elt->_name);
						break;
					}
					index ++;
				} else {
					GfTrace("Pb No description file for driver %s\n", cardllname);
				}
				break;
			}
		}
    }

	nCars = index; /* real number of cars */
	if (nCars == 0) {
		GfTrace("No driver for that race...\n");
		return -1;
	} else {
		GfOut("%d drivers ready to race\n", nCars);
	}

	ReInfo->s->_ncars = nCars;
	FREEZ(ReInfo->s->cars);
	ReInfo->s->cars = (tCarElt **)calloc(nCars, sizeof(tCarElt *));
	for (i = 0; i < nCars; i++) {
		ReInfo->s->cars[i] = &(ReInfo->carList[i]);
	}

	// TODO: reconsider splitting the call into one for cars, track and maybe other objects.
	// I stuff for now anything into one call because collision detection works with the same
	// library on all objects, so it is a bit dangerous to distribute the handling to various
	// locations (because the library maintains global state like a default collision handler etc.).
    ReInfo->_reSimItf.init(nCars, ReInfo->track);

    initStartingGrid();

    initPits();

    return 0;
}

/** Dump the track segments on screen
    @param	track	track to dump
    @param	verbose	if set to 1 all the segments are described (long)
    @ingroup	racemantools
 */
static void
reDumpTrack(tTrack *track, int verbose)
{
	int i;
	tTrackSeg *seg;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];
	
#ifdef DEBUG
    const char *stype[4] = { "", "RGT", "LFT", "STR" };
#endif

    RmLoadingScreenSetText("Loading Track Geometry...");
    snprintf(buf, BUFSIZE, ">>> Track Name    %s", track->name);
    RmLoadingScreenSetText(buf);
    snprintf(buf, BUFSIZE, ">>> Track Author  %s", track->author);
    RmLoadingScreenSetText(buf);
    snprintf(buf, BUFSIZE, ">>> Track Length  %.2f m", track->length);
    RmLoadingScreenSetText(buf);
    snprintf(buf, BUFSIZE, ">>> Track Width   %.2f m", track->width);
    RmLoadingScreenSetText(buf);

    GfOut("++++++++++++ Track ++++++++++++\n");
    GfOut("name     = %s\n", track->name);
    GfOut("author   = %s\n", track->author);
    GfOut("filename = %s\n", track->filename);
    GfOut("nseg     = %d\n", track->nseg);
    GfOut("version  = %d\n", track->version);
    GfOut("length   = %f\n", track->length);
    GfOut("width    = %f\n", track->width);
    GfOut("XSize    = %f\n", track->max.x);
    GfOut("YSize    = %f\n", track->max.y);
    GfOut("ZSize    = %f\n", track->max.z);
    switch (track->pits.type) {
    case TR_PIT_NONE:
	GfOut("Pits     = none\n");
	break;
    case TR_PIT_ON_TRACK_SIDE:
	GfOut("Pits     = present on track side\n");
	break;
    case TR_PIT_ON_SEPARATE_PATH:
	GfOut("Pits     = present on separate path\n");
	break;
    }
    if (verbose) {
	for (i = 0, seg = track->seg->next; i < track->nseg; i++, seg = seg->next) {
	    GfOut("	segment %d -------------- \n", seg->id);
#ifdef DEBUG
	    GfOut("        type    %s\n", stype[seg->type]);
#endif
	    GfOut("        length  %f\n", seg->length);
	    GfOut("	radius  %f\n", seg->radius);
	    GfOut("	arc	%f   Zs %f   Ze %f   Zcs %f\n", RAD2DEG(seg->arc),
		   RAD2DEG(seg->angle[TR_ZS]),
		   RAD2DEG(seg->angle[TR_ZE]),
		   RAD2DEG(seg->angle[TR_CS]));
	    GfOut("	Za	%f\n", RAD2DEG(seg->angle[TR_ZS]));
	    GfOut("	vertices: %-8.8f %-8.8f %-8.8f ++++ ",
		   seg->vertex[TR_SR].x,
		   seg->vertex[TR_SR].y,
		   seg->vertex[TR_SR].z);
	    GfOut("%-8.8f %-8.8f %-8.8f\n",
		   seg->vertex[TR_SL].x,
		   seg->vertex[TR_SL].y,
		   seg->vertex[TR_SL].z);
	    GfOut("	vertices: %-8.8f %-8.8f %-8.8f ++++ ",
		   seg->vertex[TR_ER].x,
		   seg->vertex[TR_ER].y,
		   seg->vertex[TR_ER].z);
	    GfOut("%-8.8f %-8.8f %-8.8f\n",
		   seg->vertex[TR_EL].x,
		   seg->vertex[TR_EL].y,
		   seg->vertex[TR_EL].z);
	    GfOut("	prev    %d\n", seg->prev->id);
	    GfOut("	next    %d\n", seg->next->id);
	}
	GfOut("From Last To First\n");
	GfOut("Dx = %-8.8f  Dy = %-8.8f Dz = %-8.8f\n",
	       track->seg->next->vertex[TR_SR].x - track->seg->vertex[TR_ER].x,
	       track->seg->next->vertex[TR_SR].y - track->seg->vertex[TR_ER].y,
	       track->seg->next->vertex[TR_SR].z - track->seg->vertex[TR_ER].z);

    }
}


/** Initialize the track for a race manager.
    @return	<tt>0 ... </tt>Ok<br>
		<tt>-1 .. </tt>Error
*/
int
ReInitTrack(void)
{
	int curTrkIdx;
	void *params = ReInfo->params;
	void *results = ReInfo->results;
	const int BUFSIZE = 1024;
	char buf[BUFSIZE];

	
	curTrkIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_TRACK, NULL, 1);
	snprintf(buf, BUFSIZE, "%s/%d", RM_SECT_TRACKS, curTrkIdx);
	const char* trackName = GfParmGetStr(params, buf, RM_ATTR_NAME, 0);
	if (!trackName) return -1;

	const char* catName = GfParmGetStr(params, buf, RM_ATTR_CATEGORY, 0);
	if (!catName) return -1;
	
	snprintf(buf, BUFSIZE, "Loading Track %s...", trackName);
	RmLoadingScreenSetText(buf);
	snprintf(buf, BUFSIZE, "tracks/%s/%s/%s.%s", catName, trackName, trackName, TRKEXT);
	ReInfo->track = ReInfo->_reTrackItf.trkBuild(buf);
	reDumpTrack(ReInfo->track, 0);
	
	return 0;
}

void
ReRaceCleanup(void)
{
	ReInfo->_reGameScreen = ReHookInit();
	ReInfo->_reSimItf.shutdown();
	if (ReInfo->_displayMode == RM_DISP_MODE_NORMAL) {
		ReInfo->_reGraphicItf.shutdowncars();
		startMenuMusic();
	}
	ReStoreRaceResults(ReInfo->_reRaceName);
	ReRaceCleanDrivers();
}


void
ReRaceCleanDrivers(void)
{
	int i;
	tRobotItf *robot;
	int nCars;

	nCars = ReInfo->s->_ncars;
	for (i = 0; i < nCars; i++) {
		robot = ReInfo->s->cars[i]->robot;
		if (robot->rbShutdown) {
			robot->rbShutdown(robot->index);
		}
		GfParmReleaseHandle(ReInfo->s->cars[i]->_paramsHandle);
		GfParmReleaseHandle(ReInfo->s->cars[i]->_carHandle);
		free(robot);
	}

	FREEZ(ReInfo->s->cars);
	ReInfo->s->cars = 0;
	ReInfo->s->_ncars = 0;
	GfModUnloadList(&ReRaceModList);
}


const char *
ReGetCurrentRaceName(void)
{
	int curRaceIdx;
	void *params = ReInfo->params;
	void *results = ReInfo->results;
	const int BUFSIZE = 1024;
	char path[BUFSIZE];
	
	curRaceIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1);
	snprintf(path, BUFSIZE, "%s/%d", RM_SECT_RACES, curRaceIdx);
	return GfParmGetStr(params, path, RM_ATTR_NAME, 0);
}

const char *
ReGetPrevRaceName(void)
{
	int curRaceIdx;
	void *params = ReInfo->params;
	void *results = ReInfo->results;
	const int BUFSIZE = 1024;
	char path[BUFSIZE];	
	
	curRaceIdx = (int)GfParmGetNum(results, RE_SECT_CURRENT, RE_ATTR_CUR_RACE, NULL, 1) - 1;
	snprintf(path, BUFSIZE, "%s/%d", RM_SECT_RACES, curRaceIdx);
	return GfParmGetStr(params, path, RM_ATTR_NAME, 0);
}
