/***************************************************************************

    file         : track.cpp
    created      : Sun Dec 24 12:14:18 CET 2000
    copyright    : (C) 2000 by Eric Espie
    email        : eric.espie@torcs.org
    version      : $Id: track.cpp,v 1.22.2.16 2012/09/27 09:52:16 berniw Exp $

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
    @version	$Id: track.cpp,v 1.22.2.16 2012/09/27 09:52:16 berniw Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <math.h>

#include <tgfclient.h>
#include <track.h>
#include <robottools.h>
#include <portability.h>

#include "ac3d.h"
#include "util.h"

#include "trackgen.h"

typedef struct texElt {
	char *name;
	char *namebump;
	char *nameraceline;
	unsigned int texid;
	struct texElt *next;
} tTexElt;

typedef struct dispElt {
	int start;
	int nb;
	char *name;
	int id;
	tTexElt *texture;
	struct dispElt *next;
} tDispElt;

typedef struct group {
	int nb;
	tDispElt *dispList;
} tGroup;

typedef struct {
	tdble tx, ty;
	tdble x, y, z;
} tPoint;


static tGroup *Groups;
static int ActiveGroups;
static int GroupNb;

#define LMAX TrackStep

static float *trackvertices;
static float *tracktexcoord;
static unsigned int	*trackindices;

static tdble TrackStep;




static void initPits(tTrackPitInfo *pits)
{
	tTrackSeg *curMainSeg;
	tTrackSeg *curPitSeg = NULL;
	tdble toStart = 0;
	tdble offset = 0;
	tTrkLocPos curPos;
	int changeSeg;
	int i;

	switch (pits->type) {
		case TR_PIT_ON_TRACK_SIDE:
			pits->driversPits = (tTrackOwnPit *)calloc(pits->nMaxPits, sizeof(tTrackOwnPit));
			pits->driversPitsNb = pits->nMaxPits;
			curPos.type = TR_LPOS_MAIN;
			curMainSeg = pits->pitStart->prev;
			changeSeg = 1;
			toStart = 0;
			i = 0;
			while (i < pits->nMaxPits) {
				if (changeSeg) {
					changeSeg = 0;
					offset = 0;
					curMainSeg = curMainSeg->next;
					switch (pits->side) {
						case TR_RGT:
							curPitSeg = curMainSeg->rside;
							if (curPitSeg->rside) {
								offset = curPitSeg->width;
								curPitSeg = curPitSeg->rside;
							}
							break;
						case TR_LFT:
							curPitSeg = curMainSeg->lside;
							if (curPitSeg->lside) {
								offset = curPitSeg->width;
								curPitSeg = curPitSeg->lside;
							}
							break;
					}
					curPos.seg = curMainSeg;
					if (toStart >= curMainSeg->length) {
						toStart -= curMainSeg->length;
						changeSeg = 1;
						continue;
					}
				}
				/* Not the real position but the start and border one instead of center */
				curPos.toStart = toStart;
				switch (pits->side) {
					case TR_RGT:
						curPos.toRight  = -offset - RtTrackGetWidth(curPitSeg, toStart);
						curPos.toLeft   = curMainSeg->width - curPos.toRight;
						curPos.toMiddle = curMainSeg->width / 2.0 - curPos.toRight;
						break;
					case TR_LFT:
						curPos.toLeft   = -offset - RtTrackGetWidth(curPitSeg, toStart);
						curPos.toRight  = curMainSeg->width - curPos.toLeft;
						curPos.toMiddle = curMainSeg->width / 2.0 - curPos.toLeft;
						break;
				}
				memcpy(&(pits->driversPits[i].pos), &curPos, sizeof(curPos));
				toStart += pits->len;
				if (toStart >= curMainSeg->length) {
					toStart -= curMainSeg->length;
					changeSeg = 1;
				}
				i++;
			}
			break;
		case TR_PIT_ON_SEPARATE_PATH:
			break;
		case TR_PIT_NONE:
			break;
	}
}




unsigned int newTextureId()
{
	static unsigned int GenTexId = 1;
	return GenTexId++;
}




void setTexture(tTexElt **texList, tTexElt **curTexElt, unsigned int &curTexId, const char *texname, const char *texnamebump, const char *texnameraceline)
{
	int found = 0;
	*curTexElt = *texList;

	// First time, create first texture
	if (*curTexElt == NULL) {
		*curTexElt = (tTexElt *)calloc(1, sizeof(tTexElt));
		(*curTexElt)->next = *curTexElt;
		*texList = *curTexElt;
		(*curTexElt)->name = strdup(texname);
		(*curTexElt)->namebump = strdup(texnamebump);
		(*curTexElt)->nameraceline = strdup(texnameraceline);

		(*curTexElt)->texid = newTextureId();
	} else {
		// Search for texture, already registered?
		do {
			if (
				strcmp(texname, (*curTexElt)->name) == 0 &&
				strcmp(texnamebump, (*curTexElt)->namebump) == 0 &&
				strcmp(texnameraceline, (*curTexElt)->nameraceline) == 0
			) {
				found = 1;
				break;
			}
			if (*curTexElt == (*curTexElt)->next) {
				break;
			}
			*curTexElt = (*curTexElt)->next;
		} while (true);

		if (!found) {
			// Texture not known, register, insert new element on head
			*curTexElt = (tTexElt *)calloc(1, sizeof(tTexElt));
			(*curTexElt)->next = *texList;
			*texList = *curTexElt;
			(*curTexElt)->name = strdup(texname);
			(*curTexElt)->namebump = strdup(texnamebump);
			(*curTexElt)->nameraceline = strdup(texnameraceline);
			(*curTexElt)->texid = newTextureId();
		}
	}
	curTexId = (*curTexElt)->texid;
}




void newDispList(int texchange, int bump, int nbvert, int &startNeeded, const char *name, int _id, tDispElt **theCurDispElt, tTexElt *curTexElt)
{
	tDispElt *aDispElt;

	if (!bump || (*(curTexElt->namebump) != 0)) {
		if (*theCurDispElt != NULL) {
			startNeeded = texchange;
			if ((*theCurDispElt)->start != nbvert) {
				(*theCurDispElt)->nb = nbvert - (*theCurDispElt)->start;
				*theCurDispElt = aDispElt = (tDispElt *)malloc(sizeof(tDispElt));
				aDispElt->start = nbvert;
				aDispElt->nb = 0;
				aDispElt->name = strdup(name);
				aDispElt->id = _id;
				aDispElt->texture = curTexElt;
				if (Groups[_id].nb == 0) {
					ActiveGroups++;
					aDispElt->next = aDispElt;
					Groups[_id].dispList = aDispElt;
				} else {
					aDispElt->next = Groups[_id].dispList->next;
					Groups[_id].dispList->next = aDispElt;
					Groups[_id].dispList = aDispElt;
				}
				Groups[_id].nb++;
			} else {
				(*theCurDispElt)->texture = curTexElt;
			}
		} else {
			*theCurDispElt = aDispElt = (tDispElt *)malloc(sizeof(tDispElt));
			aDispElt->start = nbvert;
			aDispElt->nb = 0;
			aDispElt->name = strdup(name);
			aDispElt->id = _id;
			aDispElt->texture = curTexElt;
			aDispElt->next = aDispElt;
			Groups[_id].dispList = aDispElt;
			Groups[_id].nb++;
			ActiveGroups++;
		}
	}
}




void checkDispList(
    tTrack *Track,
    void *TrackHandle,
    const char *mat,
    const char *name,
    int id,
    tdble off,
    int bump,
    int nbvert,
    tTexElt **texList,
    tTexElt **curTexElt,
    tDispElt **theCurDispElt,
    unsigned int &curTexId,
    unsigned int &prevTexId,
    int &startNeeded,
    int &curTexType,
    int &curTexLink,
    tdble &curTexOffset,
    tdble &curTexSize
)
{
	const char *texname;
	const char *texnamebump;
	const char *texnameraceline;
	const int BUFSIZE = 256;
	char path_[BUFSIZE];

	if (Track->version < 4) {
		snprintf(path_, BUFSIZE, "%s/%s/%s", TRK_SECT_SURFACES, TRK_LST_SURF, mat);
	} else {
		snprintf(path_, BUFSIZE, "%s/%s", TRK_SECT_SURFACES, mat);
	}
	texnamebump = GfParmGetStr(TrackHandle, path_, TRK_ATT_BUMPNAME, "");
	texnameraceline = GfParmGetStr(TrackHandle, path_, TRK_ATT_RACELINENAME, "");
	texname = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXTURE, "tr-asphalt.rgb");
	setTexture(texList, curTexElt, curTexId, texname, texnamebump, texnameraceline);
	if ((curTexId != prevTexId) || (startNeeded)) {
		const char *textype;
		if (bump) {
			curTexType = 1;
			curTexLink = 1;
			curTexOffset = -off;
			curTexSize = GfParmGetNum(TrackHandle, path_, TRK_ATT_BUMPSIZE, (char *)NULL, 20.0);
		} else {
			textype = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXTYPE, "continuous");
			if (strcmp(textype, "continuous") == 0) {
				curTexType = 1;
			} else {
				curTexType = 0;
			}

			textype = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXLINK, TRK_VAL_YES);
			if (strcmp(textype, TRK_VAL_YES) == 0) {
				curTexLink = 1;
			} else {
				curTexLink = 0;
			}

			textype = GfParmGetStr(TrackHandle, path_, TRK_ATT_TEXSTARTBOUNDARY, TRK_VAL_NO);
			if (strcmp(textype, TRK_VAL_YES) == 0) {
				curTexOffset = -off;
			} else {
				curTexOffset = 0;
			}

			curTexSize = GfParmGetNum(TrackHandle, path_, TRK_ATT_TEXSIZE, (char *)NULL, 20.0);
		}

		prevTexId = curTexId;
		newDispList(1, bump, nbvert, startNeeded, name, id, theCurDispElt, *curTexElt);
	}
}




void checkDispList2(
    const char *texture,
    const char *name,
    int id,
    int bump,
    int nbvert,
    tTexElt **texList,
    tTexElt **curTexElt,
    tDispElt **theCurDispElt,
    unsigned int &curTexId,
    unsigned int &prevTexId,
    int &startNeeded
)
{
	const int TEXNAMESIZE = 256;
	char texname[TEXNAMESIZE];
	snprintf(texname, TEXNAMESIZE, "%s.rgb", texture);
	setTexture(texList, curTexElt, curTexId, texname, "", "");
	if ((curTexId != prevTexId) || (startNeeded)) {
		prevTexId = curTexId;
		newDispList(1, bump, nbvert, startNeeded, name, id, theCurDispElt, *curTexElt);
	}
}




void checkDispList3(
    const char *texture,
    const char *name,
    int id,
    int bump,
    int nbvert,
    tTexElt **texList,
    tTexElt **curTexElt,
    tDispElt **theCurDispElt,
    unsigned int &curTexId,
    unsigned int &prevTexId,
    int &startNeeded
)
{
	setTexture(texList, curTexElt, curTexId, texture, "", "");
	if ((curTexId != prevTexId) || (startNeeded)) {
		prevTexId = curTexId;
		newDispList(1, bump, nbvert, startNeeded, name, id, theCurDispElt, *curTexElt);
	}
}




void setPoint(tTexElt *curTexElt, float t1, float t2, float x, float y, float z, unsigned int &nbvert)
{
	if (*(curTexElt->name) != 0) {
		tracktexcoord[2*nbvert]   = t1;
		tracktexcoord[2*nbvert+1] = t2;
		trackvertices[3*nbvert]   = x;
		trackvertices[3*nbvert+1] = y;
		trackvertices[3*nbvert+2] = z;
		trackindices[nbvert]      = nbvert;
		++nbvert;
	}
}


#define PIT_HEIGHT	5.0f
#define PIT_DEEP	10.0f
#define PIT_TOP		0.2f

/* 
	Start or end of the pit lane building, depending on the side and driving direction.
	The winding of the faces is CCW.
*/
void pitCapCCW(tTexElt *curTexElt, unsigned int &nbvert, tdble x, tdble y, tdble z, const t3Dd &normvec)
{
	tdble x2 = x + PIT_TOP * normvec.x;
	tdble y2 = y + PIT_TOP * normvec.y;
	
	setPoint(curTexElt, 1.0 - PIT_TOP, PIT_HEIGHT - PIT_TOP, x2, y2, z + PIT_HEIGHT - PIT_TOP, nbvert);
	setPoint(curTexElt, 1.0 - PIT_TOP, PIT_HEIGHT, x2, y2, z + PIT_HEIGHT, nbvert);

	x2 = x;
	y2 = y;

	setPoint(curTexElt, 1.0, PIT_HEIGHT - PIT_TOP, x2, y2, z + PIT_HEIGHT - PIT_TOP, nbvert);

	x2 = x - PIT_DEEP * normvec.x;
	y2 = y - PIT_DEEP * normvec.y;

	setPoint(curTexElt, 1.0 + PIT_DEEP, PIT_HEIGHT, x2, y2, z + PIT_HEIGHT, nbvert);

	x2 = x;
	y2 = y;

	setPoint(curTexElt, 1.0, 0, x2, y2, z, nbvert);

	x2 = x - PIT_DEEP * normvec.x;
	y2 = y - PIT_DEEP * normvec.y;

	setPoint(curTexElt, 1.0 + PIT_DEEP, 0, x2, y2, z, nbvert);
}




/* 
	Start or end of the pit lane building, depending on the side and driving direction.
	The winding of the faces is CW.
*/
void pitCapCW(tTexElt *curTexElt, unsigned int &nbvert, tdble x, tdble y, tdble z, const t3Dd &normvec)
{
	tdble x2 = x + PIT_TOP * normvec.x;
	tdble y2 = y + PIT_TOP * normvec.y;

	setPoint(curTexElt, 1.0 - PIT_TOP, PIT_HEIGHT, x2, y2, z + PIT_HEIGHT, nbvert);
	setPoint(curTexElt, 1.0 - PIT_TOP, PIT_HEIGHT - PIT_TOP, x2, y2, z + PIT_HEIGHT - PIT_TOP, nbvert);

	x2 = x - PIT_DEEP * normvec.x;
	y2 = y - PIT_DEEP * normvec.y;

	setPoint(curTexElt, 1.0 + PIT_DEEP, PIT_HEIGHT, x2, y2, z + PIT_HEIGHT, nbvert);

	x2 = x;
	y2 = y;

	setPoint(curTexElt, 1.0, PIT_HEIGHT - PIT_TOP, x2, y2, z + PIT_HEIGHT - PIT_TOP, nbvert);

	x2 = x - PIT_DEEP * normvec.x;
	y2 = y - PIT_DEEP * normvec.y;

	setPoint(curTexElt, 1.0 + PIT_DEEP, 0, x2, y2, z, nbvert);

	x2 = x;
	y2 = y;

	setPoint(curTexElt, 1.0, 0, x2, y2, z, nbvert);
}




int InitScene(tTrack *Track, void *TrackHandle, int bump, int raceline)
{
	int i, j;
	tTrackSeg *seg;
	tTrackSeg *mseg;
	unsigned int nbvert;
	tdble width;
	tdble anz, ts = 0;
	tdble radiusr, radiusl;
	tdble step;
	tTrkLocPos trkpos;
	tdble x, y, z;
	tdble x2, y2, z2;
	tdble x3, y3, z3;
	int startNeeded;

	unsigned int prevTexId;
	unsigned int curTexId = 0;
	int	curTexType = 0;
	int	curTexLink = 0;
	tdble curTexOffset = 0;
	tdble curTexSeg;
	tdble curTexSize = 0;
	tdble curHeight;
	tTexElt *texList = (tTexElt *)NULL;
	tTexElt *curTexElt = NULL;
	tTrackBarrier *curBarrier;
	tdble texLen;
	tdble texStep;
	tdble texMaxT = 0;
	tTrackPitInfo *pits;
	tdble runninglentgh;

	tdble tmWidth  = Track->graphic.turnMarksInfo.width;
	tdble tmHeight = Track->graphic.turnMarksInfo.height;
	tdble tmVSpace = Track->graphic.turnMarksInfo.vSpace;
	tdble tmHSpace = Track->graphic.turnMarksInfo.hSpace;
	int hasBorder;
	tDispElt *theCurDispElt = NULL;
	const int BUFSIZE = 256;
	char sname[BUFSIZE];
	char buf[BUFSIZE];

#define	LG_STEP_MAX	50.0

	printf("++++++++++++ Track ++++++++++++\n");
	printf("name      = %s\n", Track->name);
	printf("author    = %s\n", Track->author);
	printf("filename  = %s\n", Track->filename);
	printf("nseg      = %d\n", Track->nseg);
	printf("version   = %d\n", Track->version);
	printf("length    = %f\n", Track->length);
	printf("width     = %f\n", Track->width);
	printf("pits      = %d\n", Track->pits.nMaxPits);
	printf("XSize     = %f\n", Track->max.x);
	printf("YSize     = %f\n", Track->max.y);
	printf("ZSize     = %f\n", Track->max.z);

	tdble delatx = Track->seg->next->vertex[TR_SL].x - Track->seg->vertex[TR_EL].x;
	tdble delaty = Track->seg->next->vertex[TR_SL].y - Track->seg->vertex[TR_EL].y;
	tdble delatz = Track->seg->next->vertex[TR_SL].z - Track->seg->vertex[TR_EL].z;
	tdble delata = Track->seg->next->angle[TR_ZS] - Track->seg->angle[TR_ZE];
	NORM_PI_PI(delata);

	printf("Delta X   = %f\n", delatx);
	printf("Delta Y   = %f\n", delaty);
	printf("Delta Z   = %f\n", delatz);
	printf("Delta Ang = %f (%f)\n", delata, RAD2DEG(delata));

	Groups = (tGroup *)calloc(Track->nseg, sizeof(tGroup));
	ActiveGroups = 0;
	GroupNb = Track->nseg;
	width = Track->width;

	double rlWidthScale = 1.0;
	double rlOffset = 0.0;

	if (raceline) {
		double SideDistExt = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_RLEXT, (char*)NULL, 2.0);
		double SideDistInt = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_RLINT, (char*)NULL, 2.0);
		rlWidthScale = GfParmGetNum(TrackHandle, TRK_SECT_MAIN, TRK_ATT_RLWIDTHSCALE, (char*)NULL, 1.0);
		rlOffset = (1.0 - 1.0/rlWidthScale)/2.0;

		generateRaceLine(Track, SideDistExt, SideDistInt);
	}

	trkpos.type = TR_LPOS_MAIN;

#define CLOSEDISPLIST() do {					\
	theCurDispElt->nb = nbvert - theCurDispElt->start;	\
    }  while (0)

	/* Count the number of vertices to allocate */
	nbvert = 0;
	for (i = 0, seg = Track->seg->next; i < Track->nseg; i++, seg = seg->next) {
		nbvert++;
		switch (seg->type) {
			case TR_STR:
				nbvert += (int)(seg->length /  LMAX);
				break;
			case TR_LFT:
				nbvert += (int)(seg->arc * (seg->radiusr) / LMAX);
				break;
			case TR_RGT:
				nbvert += (int)(seg->arc * (seg->radiusl) / LMAX);
				break;
		}
	}

	nbvert++;
	// Worst case (walls as barrier plus wall on each side -> (4*4 + 3*2)*2), can be less
	// 4*4: 4 Walls, 4 vertices per slice, and 3 flat surfaces, 2 vertices per slice,
	// *2: beginning and end (in worst case).
	nbvert *= 44;
	nbvert+=58; /* start bridge */
	nbvert+=12 + 10 * Track->pits.driversPitsNb;
	nbvert+=10000; /* safety margin */
	printf("=== Indices Array Size   = %d\n", nbvert);
	printf("=== Vertex Array Size    = %d\n", nbvert * 3);
	printf("=== Tex Coord Array Size = %d\n", nbvert * 2);
	trackindices  = (unsigned int *)malloc(sizeof(unsigned int) * nbvert);
	trackvertices = (float *)malloc(sizeof(GLfloat)*(nbvert * 3));
	tracktexcoord = (float *)malloc(sizeof(GLfloat)*(nbvert * 2));

	nbvert = 0;

	/* Main track */
	prevTexId = 0;
	texLen = 0;
	startNeeded = 1;
	runninglentgh = 0;
	for (i = 0, seg = Track->seg->next; i < Track->nseg; i++, seg = seg->next) {
		checkDispList(Track, TrackHandle, seg->surface->material, "tkMn", i, seg->lgfromstart, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded, curTexType, curTexLink, curTexOffset, curTexSize);
		if (!curTexLink) {
			curTexSeg = 0;
		} else {
			curTexSeg = seg->lgfromstart;
		}
		curTexSeg += curTexOffset;
		
		if (raceline) {
			// Required if some smaller than width tiled texture is used.
			curTexSize = seg->width;
		}

		texLen = curTexSeg / curTexSize;
			
		if (startNeeded || (runninglentgh > LG_STEP_MAX)) {
			newDispList(0, bump, nbvert, startNeeded, "tkMn", i, &theCurDispElt, curTexElt);
			runninglentgh = 0;
			ts = 0;
			if (raceline) {
				// Required if some smaller than width tiled texture is used.
				texMaxT = 1.0;
			} else {
				// Normal case
				texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));
			}
			
			double rlto = getTexureOffset(seg->lgfromstart)/rlWidthScale;
			setPoint(curTexElt, texLen, texMaxT - rlOffset + rlto, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
			setPoint(curTexElt, texLen, 0.0 + rlOffset + rlto, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
		}

		switch (seg->type) {
			case TR_STR:
				ts = LMAX;
				texStep = LMAX / curTexSize;
				trkpos.seg = seg;
				while (ts < seg->length) {
					texLen += texStep;
					trkpos.toStart = ts;

					trkpos.toRight = width;
					RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);

					double rlto = getTexureOffset(seg->lgfromstart + ts)/rlWidthScale;
					setPoint(curTexElt, texLen, texMaxT - rlOffset + rlto, x, y, RtTrackHeightL(&trkpos), nbvert);

					trkpos.toRight = 0;
					RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);

					setPoint(curTexElt, texLen, 0.0 + rlOffset + rlto, x, y, RtTrackHeightL(&trkpos), nbvert);

					ts += LMAX;
				}
				ts = seg->length;
				break;
			case TR_LFT:
				step = LMAX / (seg->radiusr);
				texStep = step * seg->radius / curTexSize;
				anz = seg->angle[TR_ZS] + step;
				ts = step;
				radiusr = seg->radiusr;
				radiusl = seg->radiusl;
				trkpos.seg = seg;
				while (anz < seg->angle[TR_ZE]) {
					texLen += texStep;
					trkpos.toStart = ts;

					double rlto = getTexureOffset(seg->lgfromstart + ts*seg->radius)/rlWidthScale;
					/* left */
					trkpos.toRight = width;
					setPoint(curTexElt, texLen, texMaxT - rlOffset + rlto, seg->center.x + radiusl * sin(anz), seg->center.y - radiusl * cos(anz), RtTrackHeightL(&trkpos), nbvert);

					/* right */
					trkpos.toRight = 0;
					setPoint(curTexElt, texLen, 0.0 + rlOffset + rlto, seg->center.x + radiusr * sin(anz), seg->center.y - radiusr * cos(anz), RtTrackHeightL(&trkpos), nbvert);

					ts += step;
					anz += step;
				}
				ts = seg->arc;
				break;
			case TR_RGT:
				step = LMAX / (seg->radiusl);
				texStep = step * seg->radius / curTexSize;
				anz = seg->angle[TR_ZS] - step;
				ts = step;
				radiusr = seg->radiusr;
				radiusl = seg->radiusl;
				trkpos.seg = seg;
				while (anz > seg->angle[TR_ZE]) {
					texLen += texStep;
					trkpos.toStart = ts;

					double rlto = getTexureOffset(seg->lgfromstart + ts*seg->radius)/rlWidthScale;
					/* left */
					trkpos.toRight = width;
					setPoint(curTexElt, texLen, texMaxT - rlOffset + rlto, seg->center.x - radiusl * sin(anz), seg->center.y + radiusl * cos(anz), RtTrackHeightL(&trkpos), nbvert);

					/* right */
					trkpos.toRight = 0;
					setPoint(curTexElt, texLen, 0 + rlOffset + rlto, seg->center.x - radiusr * sin(anz), seg->center.y + radiusr * cos(anz), RtTrackHeightL(&trkpos), nbvert);

					ts += step;
					anz -= step;
				}
				ts = seg->arc;
				break;
		}
		texLen = (curTexSeg + seg->length) / curTexSize;

		double rlto = getTexureOffset(seg->lgfromstart + seg->length)/rlWidthScale;
		setPoint(curTexElt, texLen, texMaxT - rlOffset + rlto, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
		setPoint(curTexElt, texLen, 0 + rlOffset + rlto, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);

		startNeeded = 0;
		runninglentgh += seg->length;
	}

	if (raceline) {
		CLOSEDISPLIST();
		printf("=== Indices really used = %d\n", nbvert);
		return 0;
	}

	/* Right Border */
	for (j = 0; j < 3; j++) {
		prevTexId = 0;
		texLen = 0;
		startNeeded = 1;
		runninglentgh = 0;
		snprintf(sname, BUFSIZE, "t%dRB", j);
		for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next) {
			if ((mseg->rside != NULL) && (mseg->rside->type2 == TR_RBORDER)) {
				seg = mseg->rside;
				checkDispList(Track, TrackHandle, seg->surface->material, sname, i, mseg->lgfromstart, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded, curTexType, curTexLink, curTexOffset, curTexSize);
				if (!curTexLink) {
					curTexSeg = 0;
				} else {
					curTexSeg = mseg->lgfromstart;
				}
				curTexSeg += curTexOffset;
				texLen = curTexSeg / curTexSize;
				if (startNeeded || (runninglentgh > LG_STEP_MAX)) {
					newDispList(0, bump, nbvert, startNeeded, sname, i, &theCurDispElt, curTexElt);
					runninglentgh = 0;
					ts = 0;

					width = RtTrackGetWidth(seg, ts);
					texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

					switch (seg->style) {
						case TR_PLAN:
							if (j == 0) {
								setPoint(curTexElt, (1.0 - texLen), 0,       seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
								setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
							}
							break;
						case TR_CURB:
							switch (j) {
								case 0:
									if (!mseg->prev->rside || (mseg->prev->rside->type2 != TR_RBORDER) || (mseg->prev->rside->style != TR_CURB)) {
										if (seg->height > 0.0f) {
											setPoint(curTexElt, (1.0 - texLen), 0,       seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z - 0.1, nbvert);
											setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
										}
									}
									setPoint(curTexElt, (1.0 - texLen), 0,       seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
									setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height, nbvert);
									break;
								case 1:
									if (seg->height > 0.0f) {
										setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height, nbvert);
										setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
									}
									break;
								case 2:
									break;

							}
							break;
						case TR_WALL:
							switch (j) {
								case 0:
									setPoint(curTexElt, (1.0 - texLen), 0,   seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
									setPoint(curTexElt, (1.0 - texLen), .33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height, nbvert);
									break;
								case 1:
									setPoint(curTexElt, (1.0 - texLen), 0.33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height, nbvert);
									setPoint(curTexElt, (1.0 - texLen), 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height, nbvert);
									break;
								case 2:
									if (!mseg->prev->rside || (mseg->prev->rside->type2 != TR_RBORDER) || (mseg->prev->rside->style != TR_WALL)) {
										setPoint(curTexElt, 1.0 - (texLen - seg->width/curTexSize), 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height, nbvert);
										setPoint(curTexElt, 1.0 - (texLen - seg->width/curTexSize), 1.00, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
									}
									setPoint(curTexElt, (1.0 - texLen), 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height, nbvert);
									setPoint(curTexElt, (1.0 - texLen), 1.0,  seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
									break;
							}
							break;
					}

				}

				switch (seg->type) {
					case TR_STR:
						ts = LMAX;
						texStep = LMAX / curTexSize;
						texLen += texStep;
						trkpos.seg = seg;
						while (ts < seg->length) {
							trkpos.toStart = ts;
							width = RtTrackGetWidth(seg, ts);
							texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

							switch (seg->style) {
								case TR_PLAN:
									if (j == 0) {
										trkpos.toRight = width;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);
										trkpos.toRight = 0 ;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
									}
									break;
								case TR_CURB:
									switch (j) {
										case 0:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);
											trkpos.toRight = 0 ;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											if (seg->height > 0.0f) {
												trkpos.toRight = 0 ;
												RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
												setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
												setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
											}
											break;
										case 2:
											break;
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0,    x, y, RtTrackHeightL(&trkpos), nbvert);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											trkpos.toRight = 0 ;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 2:
											trkpos.toRight = 0 ;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 1.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
									}
									break;
							}

							ts += LMAX;
							texLen += texStep;
						}
						ts = seg->length;
						break;
					case TR_LFT:
						step = LMAX / (mseg->radiusr);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] + step;
						ts = step;
						texLen += texStep;
						radiusr = seg->radiusr;
						radiusl = seg->radiusl;
						trkpos.seg = seg;
						while (anz < seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							width = RtTrackGetWidth(seg, ts);
							texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

							switch (seg->style) {
								case TR_PLAN:
									if (j == 0) {
										/* left */
										trkpos.toRight = width;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);
										/* right */
										trkpos.toRight = 0;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
									}
									break;
								case TR_CURB:
									switch (j) {
										case 0:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											if (seg->height > 0.0f) {
												trkpos.toRight = 0;
												RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
												setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
												setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
											}
											break;
										case 2:
											break;
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0,    x, y, RtTrackHeightL(&trkpos), nbvert);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 2:
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 1.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
									}
									break;
							}

							ts += step;
							texLen += texStep;
							anz += step;
						}
						ts = seg->arc;
						break;
					case TR_RGT:
						step = LMAX / (mseg->radiusl);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] - step;
						ts = step;
						texLen += texStep;
						radiusr = seg->radiusr;
						radiusl = seg->radiusl;
						trkpos.seg = seg;
						while (anz > seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							width = RtTrackGetWidth(seg, ts);
							texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

							switch (seg->style) {
								case TR_PLAN:
									if (j == 0) {
										/* left */
										trkpos.toRight = width;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);
										/* right */
										trkpos.toRight = 0;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
									}
									break;
								case TR_CURB:
									switch (j) {
										case 0:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											if (seg->height > 0.0f) {
												trkpos.toRight = 0;
												RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
												setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
												setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
											}
											break;
										case 2:
											break;
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0,    x, y, RtTrackHeightL(&trkpos), nbvert);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 2:
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 1.0, x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
									}
									break;
							}

							ts += step;
							texLen += texStep;
							anz -= step;
						}
						ts = seg->arc;
						break;
				}
				texLen = (curTexSeg + mseg->length) / curTexSize;

				switch (seg->style) {
					case TR_PLAN:
						if (j == 0) {
							width = RtTrackGetWidth(seg, ts);
							texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));
							setPoint(curTexElt, (1.0 - texLen), 0,       seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
							setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
						}
						break;
					case TR_CURB:
						switch (j) {
							case 0:
								width = RtTrackGetWidth(seg, ts);
								texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));
								setPoint(curTexElt, (1.0 - texLen), 0,       seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
								setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height, nbvert);
								if (mseg->next->rside && ((mseg->next->rside->type2 != TR_RBORDER) || (mseg->next->rside->style != TR_CURB))) {
									if (seg->height > 0.0f) {
										setPoint(curTexElt, (1.0 - texLen), 0,       seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z - 0.1, nbvert);
										setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
									}
								}
								break;
							case 1:
								if (seg->height > 0.0f) {
									width = RtTrackGetWidth(seg, ts);
									texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));
									setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height, nbvert);
									setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
								}
								break;
							case 2:
								break;
						}
						break;
					case TR_WALL:
						switch (j) {
							case 0:
								setPoint(curTexElt, (1.0 - texLen), 0,    seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
								setPoint(curTexElt, (1.0 - texLen), 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height, nbvert);
								break;
							case 1:
								setPoint(curTexElt, (1.0 - texLen), 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height, nbvert);
								setPoint(curTexElt, (1.0 - texLen), 0.66, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height, nbvert);
								break;
							case 2:
								setPoint(curTexElt, (1.0 - texLen), 0.66, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height, nbvert);
								setPoint(curTexElt, (1.0 - texLen), 1.0,  seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
								if (mseg->next->rside && ((mseg->next->rside->type2 != TR_RBORDER) || (mseg->next->rside->style != TR_WALL))) {
									setPoint(curTexElt, 1.0 - (texLen + seg->width/curTexSize), 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height, nbvert);
									setPoint(curTexElt, 1.0 - (texLen + seg->width/curTexSize), 1.00, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
								}
								break;
						}
						break;
				}

				startNeeded = 0;
				runninglentgh += seg->length;
			} else {
				startNeeded = 1;
			}
		}
	}


	/* Right Side */
	prevTexId = 0;
	texLen = 0;
	startNeeded = 1;
	runninglentgh = 0;
	hasBorder = 0;
	for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next) {
		if ((mseg->rside != NULL) &&
		        ((mseg->rside->type2 == TR_RSIDE) || (mseg->rside->rside != NULL))) {
			seg = mseg->rside;
			if (seg->rside != NULL) {
				seg = seg->rside;
				if (hasBorder == 0) {
					startNeeded = 1;
					hasBorder = 1;
				} else if (mseg->rside->width != mseg->prev->rside->width) {
					// If border width changes we need a new start as well
					startNeeded = 1;
				}
			} else {
				if (hasBorder) {
					startNeeded = 1;
					hasBorder = 0;
				}
			}
			checkDispList(Track, TrackHandle, seg->surface->material, "tkRS", i, mseg->lgfromstart, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded, curTexType, curTexLink, curTexOffset, curTexSize);

			if (!curTexLink) {
				curTexSeg = 0;
			} else {
				curTexSeg = mseg->lgfromstart;
			}
			curTexSeg += curTexOffset;
			texLen = curTexSeg / curTexSize;
			if (startNeeded || (runninglentgh > LG_STEP_MAX)) {
				newDispList(0, bump, nbvert, startNeeded, "tkRS", i, &theCurDispElt, curTexElt);

				runninglentgh = 0;
				ts = 0;

				width = RtTrackGetWidth(seg, ts);
				texMaxT = (curTexType == 1 ? width / curTexSize : 1.0 + floor(width / curTexSize));

				setPoint(curTexElt, (1.0 - texLen), 0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
				setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
			}

			switch (seg->type) {
				case TR_STR:
					ts = LMAX;
					texStep = LMAX / curTexSize;
					texLen += texStep;
					trkpos.seg = seg;
					while (ts < seg->length) {
						trkpos.toStart = ts;

						width = RtTrackGetWidth(seg, ts);
						texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

						trkpos.toRight = width;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);

						trkpos.toRight = 0 ;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);

						ts += LMAX;
						texLen += texStep;
					}
					ts = seg->length;
					break;
				case TR_LFT:
					step = LMAX / (mseg->radiusr);
					texStep = step * mseg->radius / curTexSize;
					anz = seg->angle[TR_ZS] + step;
					ts = step;
					texLen += texStep;
					radiusr = seg->radiusr;
					radiusl = seg->radiusl;
					trkpos.seg = seg;
					while (anz < seg->angle[TR_ZE]) {
						trkpos.toStart = ts;
						width = RtTrackGetWidth(seg, ts);
						texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

						/* left */
						trkpos.toRight = width;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);

						/* right */
						trkpos.toRight = 0;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);

						ts += step;
						texLen += texStep;
						anz += step;
					}
					ts = seg->arc;
					break;
				case TR_RGT:
					step = LMAX / (mseg->radiusl);
					texStep = step * mseg->radius / curTexSize;
					anz = seg->angle[TR_ZS] - step;
					ts = step;
					texLen += texStep;
					radiusr = seg->radiusr;
					radiusl = seg->radiusl;
					trkpos.seg = seg;
					while (anz > seg->angle[TR_ZE]) {
						trkpos.toStart = ts;
						width = RtTrackGetWidth(seg, ts);
						texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

						/* left */
						trkpos.toRight = width;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, (1.0 - texLen), 0, x, y, RtTrackHeightL(&trkpos), nbvert);

						/* right */
						trkpos.toRight = 0;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, (1.0 - texLen), texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);

						ts += step;
						texLen += texStep;
						anz -= step;
					}
					ts = seg->arc;
					break;
			}
			texLen = (curTexSeg + mseg->length) / curTexSize;

			width = RtTrackGetWidth(seg, ts);
			texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

			setPoint(curTexElt, (1.0 - texLen), 0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
			setPoint(curTexElt, (1.0 - texLen), texMaxT, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);

			startNeeded = 0;
			runninglentgh += seg->length;
		} else {
			startNeeded = 1;
		}
	}


	/* Left Border */
	for (j = 0; j < 3; j++) {
		prevTexId = 0;
		texLen = 0;
		startNeeded = 1;
		runninglentgh = 0;
		snprintf(sname, BUFSIZE, "t%dLB", j);
		for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next) {
			if ((mseg->lside != NULL) && (mseg->lside->type2 == TR_LBORDER)) {
				seg = mseg->lside;
				checkDispList(Track, TrackHandle, seg->surface->material, sname, i, mseg->lgfromstart, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded, curTexType, curTexLink, curTexOffset, curTexSize);

				if (!curTexLink) {
					curTexSeg = 0;
				} else {
					curTexSeg = mseg->lgfromstart;
				}
				curTexSeg += curTexOffset;
				texLen = curTexSeg / curTexSize;
				if (startNeeded || (runninglentgh > LG_STEP_MAX)) {
					newDispList(0, bump, nbvert, startNeeded, sname, i, &theCurDispElt, curTexElt);
					runninglentgh = 0;
					ts = 0;
					width = RtTrackGetWidth(seg, ts);
					texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));
					switch (seg->style) {
						case TR_PLAN:
							if (j == 0) {
								setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
								setPoint(curTexElt, texLen, 0,       seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
							}
							break;
						case TR_CURB:
							switch (j) {
								case 0:
									if (seg->height > 0.0f) {
										setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
										setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height, nbvert);
									}
									break;
								case 1:
									if (!mseg->prev->lside || (mseg->prev->lside->type2 != TR_LBORDER) || (mseg->prev->lside->style != TR_CURB)) {
										if (seg->height > 0.0f) {
											setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
											setPoint(curTexElt, texLen, 0,       seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z - 0.1, nbvert);
										}
									}
									setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height, nbvert);
									setPoint(curTexElt, texLen, 0,       seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
									break;
								case 2:
									break;
							}
							break;
						case TR_WALL:
							switch (j) {
								case 0:
									if (!mseg->prev->lside || (mseg->prev->lside->type2 != TR_LBORDER) || (mseg->prev->lside->style != TR_WALL)) {
										setPoint(curTexElt, texLen - seg->width/curTexSize, 1.00, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
										setPoint(curTexElt, texLen - seg->width/curTexSize, 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height, nbvert);
									}
									setPoint(curTexElt, texLen, 1.0,  seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
									setPoint(curTexElt, texLen, 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height, nbvert);
									break;
								case 1:
									setPoint(curTexElt, texLen, 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + seg->height, nbvert);
									setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height, nbvert);
									break;
								case 2:
									setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + seg->height, nbvert);
									setPoint(curTexElt, texLen, 0.0,  seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
									break;
							}
							break;
					}

				}

				switch (seg->type) {
					case TR_STR:
						ts = LMAX;
						texStep = LMAX / curTexSize;
						texLen += texStep;
						trkpos.seg = seg;
						while (ts < seg->length) {
							trkpos.toStart = ts;
							width = RtTrackGetWidth(seg, ts);
							texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

							switch (seg->style) {
								case TR_PLAN:
									if (j == 0) {
										trkpos.toRight = width;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
										trkpos.toRight = 0;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);
									}
									break;
								case TR_CURB:
									switch (j) {
										case 0:
											if (seg->height > 0.0f) {
												trkpos.toRight = width;
												RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
												setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
												setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											}
											break;
										case 1:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
										case 2:
											break;
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 1.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											setPoint(curTexElt, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 2:
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											setPoint(curTexElt, texLen, 0.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
									}
									break;
							}

							ts += LMAX;
							texLen += texStep;
						}
						ts = seg->length;
						break;
					case TR_LFT:
						step = LMAX / (mseg->radiusr);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] + step;
						ts = step;
						texLen += texStep;
						radiusr = seg->radiusr;
						radiusl = seg->radiusl;
						trkpos.seg = seg;
						while (anz < seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							width = RtTrackGetWidth(seg, ts);
							texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

							switch (seg->style) {
								case TR_PLAN:
									if (j == 0) {
										/* left */
										trkpos.toRight = width;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
										/* right */
										trkpos.toRight = 0;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);
									}
									break;
								case TR_CURB:
									switch (j) {
										case 0:
											if (seg->height > 0.0f) {
												trkpos.toRight = width;
												RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
												setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
												setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											}
											break;
										case 1:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
										case 2:
											break;
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 1.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											setPoint(curTexElt, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 2:
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											setPoint(curTexElt, texLen, 0.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
									}
									break;
							}


							ts += step;
							texLen += texStep;
							anz += step;
						}
						ts = seg->arc;
						break;
					case TR_RGT:
						step = LMAX / (mseg->radiusl);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] - step;
						ts = step;
						texLen += texStep;
						radiusr = seg->radiusr;
						radiusl = seg->radiusl;
						trkpos.seg = seg;
						while (anz > seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							width = RtTrackGetWidth(seg, ts);
							texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

							switch (seg->style) {
								case TR_PLAN:
									if (j == 0) {
										/* left */
										trkpos.toRight = width;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
										/* right */
										trkpos.toRight = 0;
										RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
										setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);
									}
									break;
								case TR_CURB:
									switch (j) {
										case 0:
											if (seg->height > 0.0f) {
												trkpos.toRight = width;
												RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
												setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);
												setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											}
											break;
										case 1:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
										case 2:
											break;
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 1.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											setPoint(curTexElt, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 1:
											/* left */
											trkpos.toRight = width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.66, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											/* right */
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											break;
										case 2:
											trkpos.toRight = 0;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, RtTrackHeightL(&trkpos) + seg->height, nbvert);
											setPoint(curTexElt, texLen, 0.0,  x, y, RtTrackHeightL(&trkpos), nbvert);
											break;
									}
									break;
							}

							ts += step;
							texLen += texStep;
							anz -= step;
						}
						ts = seg->arc;
						break;
				}
				texLen = (curTexSeg + mseg->length) / curTexSize;

				width = RtTrackGetWidth(seg, ts);
				texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

				switch (seg->style) {
					case TR_PLAN:
						if (j == 0) {
							setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
							setPoint(curTexElt, texLen, 0,       seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
						}
						break;
					case TR_CURB:
						switch (j) {
							case 0:
								if (seg->height > 0.0f) {
									setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
									setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height, nbvert);
								}
								break;
							case 1:
								setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height, nbvert);
								setPoint(curTexElt, texLen, 0,       seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
								if (mseg->next->lside && ((mseg->next->lside->type2 != TR_LBORDER) || (mseg->next->lside->style != TR_CURB))) {
									if (seg->height > 0.0f) {
										setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
										setPoint(curTexElt, texLen, 0,       seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z - 0.1, nbvert);
									}
								}
								break;
							case 2:
								break;
						}
						break;
					case TR_WALL:
						switch (j) {
							case 0:
								setPoint(curTexElt, texLen, 1.0,  seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
								setPoint(curTexElt, texLen, 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height, nbvert);
								if (mseg->next->lside && ((mseg->next->lside->type2 != TR_LBORDER) || (mseg->next->lside->style != TR_WALL))) {
									setPoint(curTexElt, texLen + seg->width/curTexSize, 1.00, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
									setPoint(curTexElt, texLen + seg->width/curTexSize, 0.66,  seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height, nbvert);
								}
								break;
							case 1:
								setPoint(curTexElt, texLen, 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + seg->height, nbvert);
								setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height, nbvert);
								break;
							case 2:
								setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + seg->height, nbvert);
								setPoint(curTexElt, texLen, 0.0,  seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
								break;
						}
						break;
				}

				startNeeded = 0;
				runninglentgh += seg->length;
			} else {
				startNeeded = 1;
			}
		}
	}

	/* Left Side */
	prevTexId = 0;
	texLen = 0;
	startNeeded = 1;
	runninglentgh = 0;
	hasBorder = 0;
	for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next) {
		if ((mseg->lside != NULL) &&
		        ((mseg->lside->type2 == TR_LSIDE) || (mseg->lside->lside != NULL))) {
			seg = mseg->lside;
			if (seg->lside) {
				seg = seg->lside;
				if (hasBorder == 0) {
					startNeeded = 1;
					hasBorder = 1;
				} else if (mseg->lside->width != mseg->prev->lside->width) {
					// If border width changes we need a new start as well
					startNeeded = 1;
				}
				
			} else {
				if (hasBorder) {
					startNeeded = 1;
					hasBorder = 0;
				}
			}
			checkDispList(Track, TrackHandle, seg->surface->material, "tkLS", i, mseg->lgfromstart, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded, curTexType, curTexLink, curTexOffset, curTexSize);

			if (!curTexLink) {
				curTexSeg = 0;
			} else {
				curTexSeg = mseg->lgfromstart;
			}
			curTexSeg += curTexOffset;
			texLen = curTexSeg / curTexSize;
			if (startNeeded || (runninglentgh > LG_STEP_MAX)) {
				newDispList(0, bump, nbvert, startNeeded, "tkLS", i, &theCurDispElt, curTexElt);
				runninglentgh = 0;

				ts = 0;
				width = RtTrackGetWidth(seg, ts);
				texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));
				setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
				setPoint(curTexElt, texLen, 0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
			}

			switch (seg->type) {
				case TR_STR:
					ts = LMAX;
					texStep = LMAX / curTexSize;
					texLen += texStep;
					trkpos.seg = seg;
					while (ts < seg->length) {
						trkpos.toStart = ts;

						width = RtTrackGetWidth(seg, ts);
						texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

						trkpos.toRight = width;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);

						trkpos.toRight = 0;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);

						ts += LMAX;
						texLen += texStep;
					}
					ts = seg->length;
					break;
				case TR_LFT:
					step = LMAX / (mseg->radiusr);
					texStep = step * mseg->radius / curTexSize;
					anz = seg->angle[TR_ZS] + step;
					ts = step;
					texLen += texStep;
					radiusr = seg->radiusr;
					radiusl = seg->radiusl;
					trkpos.seg = seg;
					while (anz < seg->angle[TR_ZE]) {
						trkpos.toStart = ts;
						width = RtTrackGetWidth(seg, ts);
						texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

						/* left */
						trkpos.toRight = width;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);

						/* right */
						trkpos.toRight = 0;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);

						ts += step;
						texLen += texStep;
						anz += step;
					}
					ts = seg->arc;
					break;
				case TR_RGT:
					step = LMAX / (mseg->radiusl);
					texStep = step * mseg->radius / curTexSize;
					anz = seg->angle[TR_ZS] - step;
					ts = step;
					texLen += texStep;
					radiusr = seg->radiusr;
					radiusl = seg->radiusl;
					trkpos.seg = seg;
					while (anz > seg->angle[TR_ZE]) {
						trkpos.toStart = ts;
						width = RtTrackGetWidth(seg, ts);
						texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

						/* left */
						trkpos.toRight = width;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, texLen, texMaxT, x, y, RtTrackHeightL(&trkpos), nbvert);

						/* right */
						trkpos.toRight = 0;
						RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
						setPoint(curTexElt, texLen, 0, x, y, RtTrackHeightL(&trkpos), nbvert);

						ts += step;
						texLen += texStep;
						anz -= step;
					}
					ts = seg->arc;
					break;
			}
			texLen = (curTexSeg + mseg->length) / curTexSize;

			width = RtTrackGetWidth(seg, ts);
			texMaxT = (curTexType == 1 ?  width / curTexSize : 1.0 + floor(width / curTexSize));

			setPoint(curTexElt, texLen, texMaxT, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
			setPoint(curTexElt, texLen, 0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);

			startNeeded = 0;
			runninglentgh += seg->length;
		} else {
			startNeeded = 1;
		}
	}


	/* Right barrier */
	for (j = 0; j < 3; j++) {
		prevTexId = 0;
		texLen = 0;
		startNeeded = 1;
		runninglentgh = 0;
		snprintf(sname, BUFSIZE, "B%dRt", j);
		for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next) {
			if ((mseg->rside != NULL) && (mseg->rside->raceInfo & TR_PIT)) {
				runninglentgh = 0;
				newDispList(1, bump, nbvert, startNeeded, sname, i, &theCurDispElt, curTexElt);
			} else {
				curBarrier = mseg->barrier[0];
				checkDispList(Track, TrackHandle, curBarrier->surface->material, sname, i, 0, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded, curTexType, curTexLink, curTexOffset, curTexSize);

				if (!curTexLink) {
					curTexSeg = 0;
				} else {
					curTexSeg = mseg->lgfromstart;
				}
				texLen = curTexSeg / curTexSize;
				if (mseg->rside) {
					seg = mseg->rside;
					if (seg->rside) {
						seg = seg->rside;
					}
				} else {
					seg = mseg;
				}
				trkpos.seg = seg;
				if (startNeeded || (runninglentgh > LG_STEP_MAX)) {
					newDispList(0, bump, nbvert, startNeeded, sname, i, &theCurDispElt, curTexElt);
					if (curTexType == 0) texLen = 0;
					runninglentgh = 0;

					switch (curBarrier->style) {
						case TR_FENCE:
							if (j == 0) {
								setPoint(curTexElt, (1.0 - texLen), 0,   seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
								setPoint(curTexElt, (1.0 - texLen), 1.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height, nbvert);
							} else if (j == 1) {
								setPoint(curTexElt, (1.0 - texLen), 1.0, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height, nbvert);
								setPoint(curTexElt, (1.0 - texLen), 0,   seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
							}
							break;
						case TR_WALL:
							switch (j) {
								case 0:
									setPoint(curTexElt, (1.0 - texLen), 0,    seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
									setPoint(curTexElt, (1.0 - texLen), 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height, nbvert);
									break;
								case 1:
									trkpos.toStart = 0;
									trkpos.toRight = -curBarrier->width;
									RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
									setPoint(curTexElt, (1.0 - texLen), 0.33, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height, nbvert);
									setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, seg->vertex[TR_SR].z + curBarrier->height, nbvert);
									break;
								case 2:
									trkpos.toStart = 0;
									trkpos.toRight = -curBarrier->width;
									RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
									if ((mseg->prev->barrier[0]->style != TR_WALL) || (mseg->prev->barrier[0]->height != curBarrier->height)) {
										setPoint(curTexElt, 1.0 - (texLen - curBarrier->width/curTexSize), 0.66, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z + curBarrier->height, nbvert);
										setPoint(curTexElt, 1.0 - (texLen - curBarrier->width/curTexSize), 1.00, seg->vertex[TR_SR].x, seg->vertex[TR_SR].y, seg->vertex[TR_SR].z, nbvert);
									}
									setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, seg->vertex[TR_SR].z + curBarrier->height, nbvert);
									setPoint(curTexElt, (1.0 - texLen), 1.0,  x, y, seg->vertex[TR_SR].z, nbvert);
									break;
							}
							break;
					}
				}
				switch (seg->type) {
					case TR_STR:
						ts = LMAX;
						texStep = LMAX / curTexSize;
						texLen += texStep;
						trkpos.seg = seg;
						while (ts < seg->length) {
							trkpos.toStart = ts;
							trkpos.toRight = 0;
							RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
							curHeight = RtTrackHeightL(&trkpos);

							switch (curBarrier->style) {
								case TR_FENCE:
									if (j == 0) {
										setPoint(curTexElt, (1.0 - texLen), 0,   x, y, curHeight, nbvert);
										setPoint(curTexElt, (1.0 - texLen), 1.0, x, y, curHeight + curBarrier->height, nbvert);
									} else if (j == 1) {
										setPoint(curTexElt, (1.0 - texLen), 1.0, x, y, curHeight + curBarrier->height, nbvert);
										setPoint(curTexElt, (1.0 - texLen), 0,   x, y, curHeight, nbvert);
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											setPoint(curTexElt, (1.0 - texLen), 0.0,  x, y, curHeight, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 1:
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, curHeight + curBarrier->height, nbvert);
											trkpos.toRight = -curBarrier->width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 2:
											trkpos.toRight = -curBarrier->width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, curHeight + curBarrier->height, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 1.0,  x, y, curHeight, nbvert);
											break;
									}
									break;
							}

							ts += LMAX;
							texLen += texStep;
						}
						ts = seg->length;
						break;
					case TR_LFT:
						step = LMAX / (mseg->radiusr);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] + step;
						ts = step;
						texLen += texStep;
						radiusr = seg->radiusr;
						trkpos.seg = seg;
						while (anz < seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							trkpos.toRight = 0;
							RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
							curHeight = RtTrackHeightL(&trkpos);

							switch (curBarrier->style) {
								case TR_FENCE:
									if (j == 0) {
										setPoint(curTexElt, (1.0 - texLen), 0.0, x, y, curHeight, nbvert);
										setPoint(curTexElt, (1.0 - texLen), 1.0, x, y, curHeight + curBarrier->height, nbvert);
									} else if (j == 1) {
										setPoint(curTexElt, (1.0 - texLen), 1.0, x, y, curHeight + curBarrier->height, nbvert);
										setPoint(curTexElt, (1.0 - texLen), 0.0, x, y, curHeight, nbvert);
									} 
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											setPoint(curTexElt, (1.0 - texLen), 0.0,  x, y, curHeight, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 1:
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, curHeight + curBarrier->height, nbvert);
											trkpos.toRight = -curBarrier->width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 2:
											trkpos.toRight = -curBarrier->width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, curHeight + curBarrier->height, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 1.0,  x, y, curHeight, nbvert);
											break;
									}
									break;
							}

							ts += step;
							texLen += texStep;
							anz += step;
						}
						ts = seg->arc;
						break;
					case TR_RGT:
						step = LMAX / (mseg->radiusl);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] - step;
						ts = step;
						texLen += texStep;
						radiusr = seg->radiusr;
						trkpos.seg = seg;
						while (anz > seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							trkpos.toRight = 0;
							RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
							curHeight = RtTrackHeightL(&trkpos);

							switch (curBarrier->style) {
								case TR_FENCE:
									if (j == 0) {
										setPoint(curTexElt, (1.0 - texLen), 0.0, x, y, curHeight, nbvert);
										setPoint(curTexElt, (1.0 - texLen), 1.0, x, y, curHeight + curBarrier->height, nbvert);
									} else if (j == 1) {
										setPoint(curTexElt, (1.0 - texLen), 1.0, x, y, curHeight + curBarrier->height, nbvert);
										setPoint(curTexElt, (1.0 - texLen), 0.0, x, y, curHeight, nbvert);
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											setPoint(curTexElt, (1.0 - texLen), 0.0,  x, y, curHeight, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 1:
											setPoint(curTexElt, (1.0 - texLen), 0.33, x, y, curHeight + curBarrier->height, nbvert);
											trkpos.toRight = -curBarrier->width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 2:
											trkpos.toRight = -curBarrier->width;
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, curHeight + curBarrier->height, nbvert);
											setPoint(curTexElt, (1.0 - texLen), 1.0,  x, y, curHeight, nbvert);
											break;
									}
									break;
							}
							ts += step;
							texLen += texStep;
							anz -= step;
						}
						ts = seg->arc;
						break;
				}
				texLen = (curTexSeg + mseg->length) / curTexSize;
				switch (curBarrier->style) {
					case TR_FENCE:
						if (j == 0) {
							setPoint(curTexElt, (1.0 - texLen), 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
							setPoint(curTexElt, (1.0 - texLen), 1.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height, nbvert);
						} else if (j == 1) {
							setPoint(curTexElt, (1.0 - texLen), 1.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height, nbvert);
							setPoint(curTexElt, (1.0 - texLen), 0.0, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
						}
						break;
					case TR_WALL:
						switch (j) {
							case 0:
								setPoint(curTexElt, (1.0 - texLen), 0.0,  seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
								setPoint(curTexElt, (1.0 - texLen), 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height, nbvert);
								break;
							case 1:
								setPoint(curTexElt, (1.0 - texLen), 0.33, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height, nbvert);
								trkpos.toStart = ts;
								trkpos.toRight = -curBarrier->width;
								RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
								setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, seg->vertex[TR_ER].z + curBarrier->height, nbvert);
								break;
							case 2:
								trkpos.toStart = ts;
								trkpos.toRight = -curBarrier->width;
								RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
								setPoint(curTexElt, (1.0 - texLen), 0.66, x, y, seg->vertex[TR_ER].z + curBarrier->height, nbvert);
								setPoint(curTexElt, (1.0 - texLen), 1.00, x, y, seg->vertex[TR_ER].z, nbvert);
								if ((mseg->next->barrier[0]->style != TR_WALL) || (mseg->next->barrier[0]->height != curBarrier->height)) {
									setPoint(curTexElt, 1.0 - (texLen + curBarrier->width/curTexSize), 0.66, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z + curBarrier->height, nbvert);
									setPoint(curTexElt, 1.0 - (texLen + curBarrier->width/curTexSize), 1.00, seg->vertex[TR_ER].x, seg->vertex[TR_ER].y, seg->vertex[TR_ER].z, nbvert);
								}
								break;
						}
						break;
				}
				startNeeded = 0;
				runninglentgh += seg->length;
			}
		}
	}

	/* Left Barrier */
	for (j = 0; j < 3; j++) {
		prevTexId = 0;
		texLen = 0;
		startNeeded = 1;
		runninglentgh = 0;
		snprintf(sname, BUFSIZE, "B%dLt", j);
		for (i = 0, mseg = Track->seg->next; i < Track->nseg; i++, mseg = mseg->next) {
			if ((mseg->lside != NULL) && (mseg->lside->raceInfo & TR_PIT)) {
				runninglentgh = 0;
				newDispList(1, bump, nbvert, startNeeded, sname, i, &theCurDispElt, curTexElt);
			} else {
				curBarrier = mseg->barrier[1];
				checkDispList(Track, TrackHandle, curBarrier->surface->material, sname, i, 0, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded, curTexType, curTexLink, curTexOffset, curTexSize);

				if (!curTexLink) {
					curTexSeg = 0;
				} else {
					curTexSeg = mseg->lgfromstart;
				}
				texLen = curTexSeg / curTexSize;
				if (mseg->lside) {
					seg = mseg->lside;
					if (seg->lside) {
						seg = seg->lside;
					}
				} else {
					seg = mseg;
				}
				if (startNeeded || (runninglentgh > LG_STEP_MAX)) {
					newDispList(0, bump, nbvert, startNeeded, sname, i, &theCurDispElt, curTexElt);
					runninglentgh = 0;
					if (curTexType == 0) texLen = 0;


					switch (curBarrier->style) {
						case TR_FENCE:
							if (j == 0) {
								setPoint(curTexElt, texLen, 1.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height, nbvert);
								setPoint(curTexElt, texLen, 0.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
							} else if (j == 1) {
								setPoint(curTexElt, texLen, 0.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
								setPoint(curTexElt, texLen, 1.0, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height, nbvert);
							}
							break;
						case TR_WALL:
							switch (j) {
								case 0:
									trkpos.toStart = 0;
									trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, 0);
									trkpos.seg = seg;
									RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
									if ((mseg->prev->barrier[1]->style != TR_WALL) || (mseg->prev->barrier[1]->height != curBarrier->height)) {
										setPoint(curTexElt, texLen - curBarrier->width/curTexSize, 1.00, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
										setPoint(curTexElt, texLen - curBarrier->width/curTexSize, 0.66, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height, nbvert);
									}
									setPoint(curTexElt, texLen, 1.0,  x, y, seg->vertex[TR_SL].z, nbvert);
									setPoint(curTexElt, texLen, 0.66, x, y, seg->vertex[TR_SL].z + curBarrier->height, nbvert);
									break;
								case 1:
									trkpos.toStart = 0;
									trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, 0);
									trkpos.seg = seg;
									RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
									setPoint(curTexElt, texLen, 0.66, x, y, seg->vertex[TR_SL].z + curBarrier->height, nbvert);
									setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height, nbvert);
									break;
								case 2:
									setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z + curBarrier->height, nbvert);
									setPoint(curTexElt, texLen, 0.0,  seg->vertex[TR_SL].x, seg->vertex[TR_SL].y, seg->vertex[TR_SL].z, nbvert);
									break;
							}
							break;
					}
				}

				switch (seg->type) {
					case TR_STR:
						ts = LMAX;
						texStep = LMAX / curTexSize;
						texLen += texStep;
						trkpos.seg = seg;
						while (ts < seg->length) {
							trkpos.toStart = ts;
							trkpos.toRight = RtTrackGetWidth(seg, ts);
							RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
							curHeight = RtTrackHeightL(&trkpos);

							switch (curBarrier->style) {
								case TR_FENCE:
									if (j == 0) {
										setPoint(curTexElt, texLen, 1.0, x, y, curHeight + curBarrier->height, nbvert);
										setPoint(curTexElt, texLen, 0,   x, y, curHeight, nbvert);
									} else if (j == 1) {
										setPoint(curTexElt, texLen, 0,   x, y, curHeight, nbvert);
										setPoint(curTexElt, texLen, 1.0, x, y, curHeight + curBarrier->height, nbvert);
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 1.0,  x, y, curHeight, nbvert);
											setPoint(curTexElt, texLen, 0.66, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 1:
											trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.66, x, y, curHeight + curBarrier->height, nbvert);
											trkpos.toRight = RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 2:
											setPoint(curTexElt, texLen, 0.33, x, y, curHeight + curBarrier->height, nbvert);
											setPoint(curTexElt, texLen, 0,    x, y, curHeight, nbvert);
											break;
									}
									break;
							}

							ts += LMAX;
							texLen += texStep;
						}
						ts = seg->length;
						break;
					case TR_LFT:
						step = LMAX / (mseg->radiusr);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] + step;
						ts = step;
						texLen += texStep;
						radiusl = seg->radiusl;
						trkpos.seg = seg;
						while (anz < seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							trkpos.toRight = RtTrackGetWidth(seg, ts);
							RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
							curHeight = RtTrackHeightL(&trkpos);

							switch (curBarrier->style) {
								case TR_FENCE:
									if (j == 0) {
										setPoint(curTexElt, texLen, 1.0, x, y, curHeight + curBarrier->height, nbvert);
										setPoint(curTexElt, texLen, 0,   x, y, curHeight, nbvert);
									} else if (j == 1) {
										setPoint(curTexElt, texLen, 0,   x, y, curHeight, nbvert);
										setPoint(curTexElt, texLen, 1.0, x, y, curHeight + curBarrier->height, nbvert);
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 1.0,  x, y, curHeight, nbvert);
											setPoint(curTexElt, texLen, 0.66, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 1:
											trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.66, x, y, curHeight + curBarrier->height, nbvert);
											trkpos.toRight = RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 2:
											setPoint(curTexElt, texLen, 0.33, x, y, curHeight + curBarrier->height, nbvert);
											setPoint(curTexElt, texLen, 0,    x, y, curHeight, nbvert);
											break;
									}
									break;
							}

							ts += step;
							texLen += texStep;
							anz += step;
						}
						ts = seg->arc;
						break;
					case TR_RGT:
						step = LMAX / (mseg->radiusl);
						texStep = step * mseg->radius / curTexSize;
						anz = seg->angle[TR_ZS] - step;
						ts = step;
						texLen += texStep;
						radiusl = seg->radiusl;
						trkpos.seg = seg;
						while (anz > seg->angle[TR_ZE]) {
							trkpos.toStart = ts;
							trkpos.toRight = RtTrackGetWidth(seg, ts);
							RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
							curHeight = RtTrackHeightL(&trkpos);

							switch (curBarrier->style) {
								case TR_FENCE:
									if (j == 0) {
										setPoint(curTexElt, texLen, 1.0, x, y, curHeight + curBarrier->height, nbvert);
										setPoint(curTexElt, texLen, 0,   x, y, curHeight, nbvert);
									} else if (j == 1) {
										setPoint(curTexElt, texLen, 0,   x, y, curHeight, nbvert);
										setPoint(curTexElt, texLen, 1.0, x, y, curHeight + curBarrier->height, nbvert);
									}
									break;
								case TR_WALL:
									switch (j) {
										case 0:
											trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 1.0,  x, y, curHeight, nbvert);
											setPoint(curTexElt, texLen, 0.66, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 1:
											trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.66, x, y, curHeight + curBarrier->height, nbvert);
											trkpos.toRight = RtTrackGetWidth(seg, ts);
											RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
											setPoint(curTexElt, texLen, 0.33, x, y, curHeight + curBarrier->height, nbvert);
											break;
										case 2:
											setPoint(curTexElt, texLen, 0.33, x, y, curHeight + curBarrier->height, nbvert);
											setPoint(curTexElt, texLen, 0,    x, y, curHeight, nbvert);
											break;
									}
									break;
							}

							ts += step;
							texLen += texStep;
							anz -= step;
						}
						ts = seg->arc;
						break;
				}
				texLen = (curTexSeg + mseg->length) / curTexSize;

				switch (curBarrier->style) {
					case TR_FENCE:
						if (j == 0) {
							setPoint(curTexElt, texLen, 1.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height, nbvert);
							setPoint(curTexElt, texLen, 0,   seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
						} else if (j == 1) {
							setPoint(curTexElt, texLen, 0,   seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
							setPoint(curTexElt, texLen, 1.0, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height, nbvert);
						}
						break;
					case TR_WALL:
						switch (j) {
							case 0:
								trkpos.toStart = ts;
								trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
								RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
								setPoint(curTexElt, texLen, 1.0,  x, y, seg->vertex[TR_EL].z, nbvert);
								setPoint(curTexElt, texLen, 0.66, x, y, seg->vertex[TR_EL].z + curBarrier->height, nbvert);
								if ((mseg->next->barrier[1]->style != TR_WALL) || (mseg->next->barrier[1]->height != curBarrier->height)) {
									setPoint(curTexElt, texLen + curBarrier->width/curTexSize, 1.00, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
									setPoint(curTexElt, texLen + curBarrier->width/curTexSize, 0.66, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height, nbvert);
								}
								break;
							case 1:
								trkpos.toStart = ts;
								trkpos.toRight = curBarrier->width + RtTrackGetWidth(seg, ts);
								RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
								setPoint(curTexElt, texLen, 0.66, x, y, seg->vertex[TR_EL].z + curBarrier->height, nbvert);
								setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height, nbvert);
								break;
							case 2:
								setPoint(curTexElt, texLen, 0.33, seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z + curBarrier->height, nbvert);
								setPoint(curTexElt, texLen, 0.0,  seg->vertex[TR_EL].x, seg->vertex[TR_EL].y, seg->vertex[TR_EL].z, nbvert);
								break;
						}
						break;
				}

				startNeeded = 0;
				runninglentgh += seg->length;
			}
		}
	}

	if (!bump) {

		/* Turn Marks */
		for (i = 0, seg = Track->seg->next; i < Track->nseg; i++, seg = seg->next) {
			if (seg->ext) {
				t3Dd	normvec;
				int 	nbMarks = seg->ext->nbMarks;
				int 	*marks  = seg->ext->marks;
				int		j, k;

				for (j = 0; j < nbMarks; j++) {
					/* find the segment */
					tdble lgfs = seg->lgfromstart - (tdble)marks[j];
					if (lgfs < 0) {
						lgfs += Track->length;
					}
					for (k = 0, mseg = Track->seg->next; k < Track->nseg; k++, mseg = mseg->next) {
						if ((lgfs >= mseg->lgfromstart) && (lgfs < (mseg->lgfromstart + mseg->length))) {
							break;
						}
					}

					if (seg->type == TR_RGT) {
						snprintf(buf, BUFSIZE, "turn%dR", marks[j]);
						trkpos.toRight = Track->width + tmHSpace + tmWidth;
					} else {
						snprintf(buf, BUFSIZE, "turn%dL", marks[j]);
						trkpos.toRight = -tmHSpace;
					}
					trkpos.toStart = lgfs - mseg->lgfromstart;
					if (mseg->type != TR_STR) {
						trkpos.toStart = trkpos.toStart / mseg->radius;
					}
					trkpos.seg = mseg;
					RtTrackLocal2Global(&trkpos, &x, &y, TR_TORIGHT);
					z = tmVSpace + RtTrackHeightL(&trkpos);
					if (seg->type == TR_LFT) {
						RtTrackSideNormalG(mseg, x, y, TR_RGT, &normvec);
						normvec.x = -normvec.x;
						normvec.y = -normvec.y;
					} else {
						RtTrackSideNormalG(mseg, x, y, TR_LFT, &normvec);
					}
					checkDispList2(buf, "TuMk", mseg->id, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);

					setPoint(curTexElt, 0.0, 0.0, x, y, z, nbvert);
					setPoint(curTexElt, 1.0, 0.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z, nbvert);
					setPoint(curTexElt, 0.0, 1.0, x, y, z + tmHeight, nbvert);
					setPoint(curTexElt, 1.0, 1.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z + tmHeight, nbvert);

					checkDispList2("back-sign", "TuMk", mseg->id, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);

					setPoint(curTexElt, 0.0, 0.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z, nbvert);
					setPoint(curTexElt, 1.0, 0.0, x, y, z, nbvert);
					setPoint(curTexElt, 0.0, 1.0, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z + tmHeight, nbvert);
					setPoint(curTexElt, 1.0, 1.0, x, y, z + tmHeight, nbvert);

					printf("(%f, %f, %f), (%f, %f, %f)\n", x, y, z, x + tmWidth * normvec.x, y + tmWidth * normvec.y, z + tmHeight);

				}
			}
		}



		/* Start Bridge */
		checkDispList2("pylon1", "S0Bg", 0, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);

#define BR_HEIGHT_1	8.0
#define BR_HEIGHT_2	6.0
#define BR_WIDTH_0	2.0
#define BR_WIDTH_1	2.0
		mseg = Track->seg->next;
		if (mseg->rside) {
			seg = mseg->rside;
			if (seg->rside) {
				seg = seg->rside;
			}
		} else {
			seg = mseg;
		}

		x = seg->vertex[TR_SR].x;
		y = seg->vertex[TR_SR].y - 0.1;
		z = seg->vertex[TR_SR].z;

		setPoint(curTexElt, 0, 0, x, y, z, nbvert);
		setPoint(curTexElt, 0, 1, x, y, z + BR_HEIGHT_2, nbvert);

		x += BR_WIDTH_0;

		setPoint(curTexElt, 1, 0, x, y, z, nbvert);
		setPoint(curTexElt, 1, 1, x, y, z + BR_HEIGHT_2, nbvert);

		y -= BR_WIDTH_1;

		setPoint(curTexElt, 2, 0, x, y, z, nbvert);
		setPoint(curTexElt, 2, 1, x, y, z + BR_HEIGHT_2, nbvert);

		x -= BR_WIDTH_0;

		setPoint(curTexElt, 3, 0, x, y, z, nbvert);
		setPoint(curTexElt, 3, 1, x, y, z + BR_HEIGHT_2, nbvert);

		y += BR_WIDTH_1;

		setPoint(curTexElt, 4, 0, x, y, z, nbvert);
		setPoint(curTexElt, 4, 1, x, y, z + BR_HEIGHT_2, nbvert);

		newDispList(0, bump, nbvert, startNeeded, "S1Bg", 0, &theCurDispElt, curTexElt);

		if (mseg->lside) {
			seg = mseg->lside;
			if (seg->lside) {
				seg = seg->lside;
			}
		} else {
			seg = mseg;
		}
		x2 = seg->vertex[TR_SL].x;
		y2 = seg->vertex[TR_SL].y + 0.1;
		z2 = seg->vertex[TR_SL].z;

		setPoint(curTexElt, 0, 1, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 0, 0, x2, y2, z2, nbvert);

		x2 += BR_WIDTH_0;

		setPoint(curTexElt, 1, 1, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 1, 0, x2, y2, z2, nbvert);

		y2 += BR_WIDTH_1;

		setPoint(curTexElt, 2, 1, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 2, 0, x2, y2, z2, nbvert);

		x2 -= BR_WIDTH_0;

		setPoint(curTexElt, 3, 1, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 3, 0, x2, y2, z2, nbvert);

		y2 -= BR_WIDTH_1;

		setPoint(curTexElt, 4, 1, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 4, 0, x2, y2, z2, nbvert);

		checkDispList2("pylon2", "S2Bg", 0, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);


		setPoint(curTexElt, 0, 1, x, y, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 0, 0, x, y, z + BR_HEIGHT_2, nbvert);

		y -= BR_WIDTH_1;

		setPoint(curTexElt, 1, 1, x, y, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 1, 0, x, y, z + BR_HEIGHT_2, nbvert);

		x += BR_WIDTH_0;

		setPoint(curTexElt, 2, 1, x, y, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 2, 0, x, y, z + BR_HEIGHT_2, nbvert);

		y += BR_WIDTH_1;

		setPoint(curTexElt, 3, 1, x, y, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 3, 0, x, y, z + BR_HEIGHT_2, nbvert);

		x -= BR_WIDTH_0;

		setPoint(curTexElt, 3, 1, x + BR_WIDTH_0, y, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 3, 0, x, y, z + BR_HEIGHT_1, nbvert);

		y -= BR_WIDTH_1;

		setPoint(curTexElt, 4, 1, x + BR_WIDTH_0, y, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 4, 0, x, y, z + BR_HEIGHT_1, nbvert);

		y += BR_WIDTH_1;	/* back to origin */

		newDispList(0, bump, nbvert, startNeeded, "S3Bg", 0, &theCurDispElt, curTexElt);

		y2 += BR_WIDTH_1;

		setPoint(curTexElt, 0, 1, x2 + BR_WIDTH_0, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 0, 0, x2, y2, z + BR_HEIGHT_1, nbvert);

		y2 -= BR_WIDTH_1;

		setPoint(curTexElt, 1, 1, x2 + BR_WIDTH_0, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 1, 0, x2, y2, z + BR_HEIGHT_1, nbvert);

		x2 += BR_WIDTH_0;

		setPoint(curTexElt, 1, 1, x2, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 1, 0, x2, y2, z + BR_HEIGHT_2, nbvert);

		y2 += BR_WIDTH_1;

		setPoint(curTexElt, 2, 1, x2, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 2, 0, x2, y2, z + BR_HEIGHT_2, nbvert);

		x2 -= BR_WIDTH_0;

		setPoint(curTexElt, 3, 1, x2, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 3, 0, x2, y2, z + BR_HEIGHT_2, nbvert);

		y2 -= BR_WIDTH_1;

		setPoint(curTexElt, 4, 1, x2, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 4, 0, x2, y2, z + BR_HEIGHT_2, nbvert);

		/* Middle on the bridge */
		checkDispList2("pylon3", "S4Bg", 2, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);

		setPoint(curTexElt, 0, 0, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 1, 0, x, y, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 0, 0.25, x2, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 1, 0.25, x, y, z + BR_HEIGHT_1, nbvert);

		x += BR_WIDTH_0;
		x2 += BR_WIDTH_0;

		setPoint(curTexElt, 0, 0.5, x2, y2, z + BR_HEIGHT_1, nbvert);
		setPoint(curTexElt, 1, 0.5, x, y, z + BR_HEIGHT_1, nbvert);


		setPoint(curTexElt, 0, 0.75, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 1, 0.75, x, y, z + BR_HEIGHT_2, nbvert);

		x -= BR_WIDTH_0;
		x2 -= BR_WIDTH_0;

		setPoint(curTexElt, 0, 1, x2, y2, z + BR_HEIGHT_2, nbvert);
		setPoint(curTexElt, 1, 1, x, y, z + BR_HEIGHT_2, nbvert);




		/* draw the pits */

		pits = &(Track->pits);
		initPits(pits);

		if (pits->type == TR_PIT_ON_TRACK_SIDE) {
			int		uid = 1;
			t3Dd	normvec;

			startNeeded = 1;
			snprintf(sname, BUFSIZE, "P%dts", uid++);
			checkDispList3("concrete2.rgb", sname, pits->driversPits[0].pos.seg->id, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);

			RtTrackLocal2Global(&(pits->driversPits[0].pos), &x, &y, pits->driversPits[0].pos.type);
			RtTrackSideNormalG(pits->driversPits[0].pos.seg, x, y, pits->side, &normvec);
			z2 = RtTrackHeightG(pits->driversPits[0].pos.seg, x, y);

			// To get the normal right choose the right winding.
			if (pits->side == TR_RGT) {
				pitCapCW(curTexElt, nbvert, x, y, z2, normvec);
			} else {
				pitCapCCW(curTexElt, nbvert, x, y, z2, normvec);
			}

			// Pit front, roof and back.
			for (i = 0; i < pits->driversPitsNb; i++) {

				startNeeded = 1;
				snprintf(sname, BUFSIZE, "P%dts", uid++);
				checkDispList3("concrete.rgb", sname, pits->driversPits[i].pos.seg->id, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);

				RtTrackLocal2Global(&(pits->driversPits[i].pos), &x, &y, pits->driversPits[i].pos.type);
				RtTrackSideNormalG(pits->driversPits[i].pos.seg, x, y, pits->side, &normvec);
				x2 = x;
				y2 = y;
				z2 = RtTrackHeightG(pits->driversPits[i].pos.seg, x2, y2);

				// Different ordering for left and right to get correct face normals
				int leftpoints[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};	// ordering for pit on the left
				int rightpoints[10] = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8};	// ordering for pit on the right
				int *pindex;

				if (pits->side == TR_RGT) {
					x3 = x + pits->len * normvec.y;
					y3 = y - pits->len * normvec.x;
					pindex = rightpoints;
				} else {
					x3 = x - pits->len * normvec.y;
					y3 = y + pits->len * normvec.x;
					pindex = leftpoints;
				}

				z3 = RtTrackHeightG(pits->driversPits[i].pos.seg, x3, y3);

				tPoint parray[10] = {
					{pits->len,	0.0f,									x2,							y2,							z2 + PIT_HEIGHT - PIT_TOP},
					{0.0f,		0.0f,									x3,							y3,							z3 + PIT_HEIGHT - PIT_TOP},
					{pits->len, PIT_TOP,								x2 + PIT_TOP * normvec.x,	y2 + PIT_TOP * normvec.y,	z2 + PIT_HEIGHT - PIT_TOP},
					{0.0f,		PIT_TOP,								x3 + PIT_TOP * normvec.x,	y3 + PIT_TOP * normvec.y,	z3 + PIT_HEIGHT - PIT_TOP},
					{pits->len, 2.0f * PIT_TOP,							x2 + PIT_TOP * normvec.x,	y2 + PIT_TOP * normvec.y,	z2 + PIT_HEIGHT},
					{0.0f,		2.0f * PIT_TOP,							x3 + PIT_TOP * normvec.x,	y3 + PIT_TOP * normvec.y,	z3 + PIT_HEIGHT},
					{pits->len, 2.0f * PIT_TOP + PIT_DEEP,				x2 - PIT_DEEP * normvec.x,	y2 - PIT_DEEP * normvec.y,	z2 + PIT_HEIGHT},
					{0.0f,		2.0f * PIT_TOP + PIT_DEEP,				x3 - PIT_DEEP * normvec.x,	y3 - PIT_DEEP * normvec.y,	z3 + PIT_HEIGHT},
					{pits->len, 2.0f * PIT_TOP + PIT_DEEP + PIT_HEIGHT, x2 - PIT_DEEP * normvec.x,	y2 - PIT_DEEP * normvec.y,	z2},
					{0.0f,		2.0f * PIT_TOP + PIT_DEEP + PIT_HEIGHT, x3 - PIT_DEEP * normvec.x,	y3 - PIT_DEEP * normvec.y,	z3}
				};

				int pp;

				for (pp = 0; pp < 10; pp++) {
					tPoint &p = parray[pindex[pp]]; 
					setPoint(curTexElt, p.tx, p.ty, p.x, p.y, p.z, nbvert);
				} 
			}
			startNeeded = 1;
			i--;
			snprintf(sname, BUFSIZE, "P%dts", uid++);
			checkDispList3("concrete2.rgb", sname, pits->driversPits[i].pos.seg->id, bump, nbvert, &texList, &curTexElt, &theCurDispElt, curTexId, prevTexId, startNeeded);

			RtTrackLocal2Global(&(pits->driversPits[i].pos), &x, &y, pits->driversPits[i].pos.type);
			RtTrackSideNormalG(pits->driversPits[i].pos.seg, x, y, pits->side, &normvec);

			if (pits->side == TR_RGT) {
				x = x + pits->len * normvec.y;
				y = y - pits->len * normvec.x;
			} else {
				x = x - pits->len * normvec.y;
				y = y + pits->len * normvec.x;
			}

			z2 = RtTrackHeightG(pits->driversPits[i].pos.seg, x, y);

			// To get the normal right choose the right winding.
			if (pits->side == TR_RGT) {
				pitCapCCW(curTexElt, nbvert, x, y, z2, normvec);
			} else {
				pitCapCW(curTexElt, nbvert, x, y, z2, normvec);
			}
		}
	}

	CLOSEDISPLIST();
	printf("=== Indices really used = %d\n", nbvert);
	return 0;
}


static void
saveObject(FILE *curFd, int nb, int start, char *texture, char *name)
{
	int	i, index;

	fprintf(curFd, "OBJECT poly\n");
	fprintf(curFd, "name \"%s\"\n", name);
	fprintf(curFd, "texture \"%s\"\n", texture);
	fprintf(curFd, "numvert %d\n", nb);

	for (i = 0; i < nb; i++) {
		index = 3 * (start + i);
		fprintf(curFd, "%f %f %f\n", trackvertices[index], trackvertices[index+2], -trackvertices[index+1]);
	}

	fprintf(curFd, "numsurf %d\n", nb - 2);
	fprintf(curFd, "SURF 0x10\n");
	fprintf(curFd, "mat 0\n");
	fprintf(curFd, "refs 3\n");
	fprintf(curFd, "%d %f %f\n", 0, tracktexcoord[2*start], tracktexcoord[2*start+1]);
	fprintf(curFd, "%d %f %f\n", 1, tracktexcoord[2*(start+1)], tracktexcoord[2*(start+1)+1]);
	fprintf(curFd, "%d %f %f\n", 2, tracktexcoord[2*(start+2)], tracktexcoord[2*(start+2)+1]);

	/* triangle strip conversion to triangles */
	for (i = 2; i < nb-1; i++) {
		fprintf(curFd, "SURF 0x10\n");
		fprintf(curFd, "mat 0\n");
		fprintf(curFd, "refs 3\n");
		if ((i % 2) == 0) {
			index = i;
			fprintf(curFd, "%d %f %f\n", index, tracktexcoord[2*(start+index)], tracktexcoord[2*(start+index)+1]);
			index = i - 1;
			fprintf(curFd, "%d %f %f\n", index, tracktexcoord[2*(start+index)], tracktexcoord[2*(start+index)+1]);
			index = i + 1;
			fprintf(curFd, "%d %f %f\n", index, tracktexcoord[2*(start+index)], tracktexcoord[2*(start+index)+1]);
		} else {
			index = i - 1;
			fprintf(curFd, "%d %f %f\n", index, tracktexcoord[2*(start+index)], tracktexcoord[2*(start+index)+1]);
			index = i;
			fprintf(curFd, "%d %f %f\n", index, tracktexcoord[2*(start+index)], tracktexcoord[2*(start+index)+1]);
			index = i + 1;
			fprintf(curFd, "%d %f %f\n", index, tracktexcoord[2*(start+index)], tracktexcoord[2*(start+index)+1]);
		}
	}
	fprintf(curFd, "kids 0\n");
}


static void
SaveMainTrack(FILE *curFd, int bump, int raceline)
{
	tDispElt		*aDispElt;
	const int BUFSIZE = 256;
	char		buf[BUFSIZE];
	int			i;

	for (i = 0; i < GroupNb; i++) {
		if (Groups[i].nb != 0) {
			aDispElt = Groups[i].dispList;
			snprintf(buf, BUFSIZE, "TKMN%d", i);
			Ac3dGroup(curFd, buf, Groups[i].nb);
			do {
				aDispElt = aDispElt->next;
				if (aDispElt->nb != 0) {
					snprintf(buf, BUFSIZE, "%s%d", aDispElt->name, aDispElt->id);
					if (bump) {
						saveObject(curFd, aDispElt->nb, aDispElt->start, aDispElt->texture->namebump, buf);
					} else if (raceline) { 
						saveObject(curFd, aDispElt->nb, aDispElt->start, aDispElt->texture->nameraceline, buf);
					} else {
						saveObject(curFd, aDispElt->nb, aDispElt->start, aDispElt->texture->name, buf);
					}
				}
			} while (aDispElt != Groups[i].dispList);
		}
	}
}


/** Calculate track parameters and exit without any file creation
    It is for information only, mainly for use from TrackEditor.
    @param	Track	track structure
    @param	TrackHandle	handle on the track description
    @return	none
*/
void CalculateTrack(tTrack *Track, void *TrackHandle)
{
	TrackStep = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_TSTEP, NULL, TrackStep);
	GfOut("Track step: %.2f ", TrackStep);
	InitScene(Track, TrackHandle, 0, 0);
	printf("Calculation finished\n");
}


/** Generate the track AC3D file(s).
    @param	Track	track structure
    @param	TrackHandle	handle on the track description
    @param	outFile	output file name for track only
    @param	AllFd	fd of the merged file
    @return	none
*/
void
GenerateTrack(tTrack *Track, void *TrackHandle, char *outFile, FILE *AllFd, int bump, int raceline)
{
	FILE *curFd;

	TrackStep = GfParmGetNum(TrackHandle, TRK_SECT_TERRAIN, TRK_ATT_TSTEP, NULL, TrackStep);
	GfOut("Track step: %.2f ", TrackStep);

	InitScene(Track, TrackHandle, bump, raceline);

	if (outFile) {
		curFd = Ac3dOpen(outFile, 1);
		Ac3dGroup(curFd, "track", ActiveGroups);
		SaveMainTrack(curFd, bump, raceline);
		Ac3dClose(curFd);
	}

	if (AllFd) {
		Ac3dGroup(AllFd, "track", ActiveGroups);
		SaveMainTrack(AllFd, bump, raceline);
	}

}
