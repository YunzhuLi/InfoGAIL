/***************************************************************************

    file                 : raceline.cpp
    created              : Tue Sep 4 09:54:50 CEST 2012
    copyright            : (C) 2012 by Bernhard Wymann, Remi Coulom
    email                : berniw@bluewin.ch
    version              : $Id: raceline.cpp,v 1.1.2.4 2012/09/05 16:08:04 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tgfclient.h>
#include <track.h>
#include <robottools.h>
#include <portability.h>
#include <math.h>
#include <tmath/linalg_t.h>




struct RacelineSegment {
	vec2d t;	// raceline
	vec2d l;	// left track border
	vec2d r;	// right track border
	double lane;	// 0.0: left, 1.0: right
};

static const double SegLength = 2.0;
static RacelineSegment* rlseg = NULL;
static int nSegments = 0;
static double SideDistExt = 2.0; // Security distance wrt outside
static double SideDistInt = 2.0; // Security distance wrt inside
static double trackWidth = 0.0;
static const double SecurityR = 100.0; // Security radius




static void SplitTrack(tTrack *ptrack)
{
	const tTrackSeg *seg = ptrack->seg;
	const tTrackSeg *first = NULL;
	// Find pointer to start (seems not to start at the "start" all the time, e.g e-track-1)
	do {
		if (seg->lgfromstart == 0.0) {
			first = seg;
			break;
		}
		seg = seg->next;
	} while (ptrack->seg != seg);
	
	trackWidth = seg->width;
	nSegments = (int) floor(ptrack->length/SegLength);
	rlseg = new RacelineSegment[nSegments];

	double lastSegLen = 0.0;
	double curSegLen = 0.0;
	int currentSegId = 0;

	do {
		if (seg->type == TR_STR) {
			double scale = seg->length;
			double dxl = (seg->vertex[TR_EL].x - seg->vertex[TR_SL].x) / scale;
			double dyl = (seg->vertex[TR_EL].y - seg->vertex[TR_SL].y) / scale;
			double dxr = (seg->vertex[TR_ER].x - seg->vertex[TR_SR].x) / scale;
			double dyr = (seg->vertex[TR_ER].y - seg->vertex[TR_SR].y) / scale;
			
			for (int i = 0; curSegLen < seg->length && currentSegId < nSegments; i++) {
				rlseg[currentSegId].l.x = seg->vertex[TR_SL].x + dxl *curSegLen; 
				rlseg[currentSegId].l.y = seg->vertex[TR_SL].y + dyl *curSegLen;
				rlseg[currentSegId].r.x = seg->vertex[TR_SR].x + dxr *curSegLen;
				rlseg[currentSegId].r.y = seg->vertex[TR_SR].y + dyr *curSegLen;
				rlseg[currentSegId].t = (rlseg[currentSegId].l + rlseg[currentSegId].r) / 2.0;
				rlseg[currentSegId].lane = 0.5;

				currentSegId++;
				lastSegLen = curSegLen;
				curSegLen += SegLength;
			}
		} else {
			double dphi = 1.0/seg->radius;
			vec2d c(seg->center.x, seg->center.y);
			dphi = (seg->type == TR_LFT) ? dphi : -dphi;

			vec2d l(seg->vertex[TR_SL].x, seg->vertex[TR_SL].y);
			vec2d r(seg->vertex[TR_SR].x, seg->vertex[TR_SR].y);

			for (int i = 0; curSegLen < seg->length && currentSegId < nSegments; i++) {
				double phi = curSegLen*dphi;
				rlseg[currentSegId].l = l.rotate(c, phi);
				rlseg[currentSegId].r = r.rotate(c, phi);
				rlseg[currentSegId].t = (rlseg[currentSegId].l + rlseg[currentSegId].r) / 2.0;
				rlseg[currentSegId].lane = 0.5;

				currentSegId++;
				lastSegLen = curSegLen;
				curSegLen += SegLength; 
			}
		}

		curSegLen = SegLength - (seg->length - lastSegLen);
		lastSegLen = curSegLen;
		while (curSegLen > SegLength) {
			curSegLen -= SegLength;
		}

		seg = seg->next;
	} while (first != seg);
}




static double GetRInverse(int prev, vec2d p, int next)
{
	
	double x1 = rlseg[next].t.x - p.x;
	double y1 = rlseg[next].t.y - p.y;
	double x2 = rlseg[prev].t.x - p.x;
	double y2 = rlseg[prev].t.y - p.y;
	double x3 = rlseg[next].t.x - rlseg[prev].t.x;
	double y3 = rlseg[next].t.y - rlseg[prev].t.y;

	double det = x1 * y2 - x2 * y1;
	double n1 = x1 * x1 + y1 * y1;
	double n2 = x2 * x2 + y2 * y2;
	double n3 = x3 * x3 + y3 * y3;
	double nnn = sqrt(n1 * n2 * n3);

	return 2 * det / nnn;
}




static void UpdateTxTy(int i)
{
	rlseg[i].t = rlseg[i].lane*rlseg[i].r + (1.0 - rlseg[i].lane)*rlseg[i].l;
}




static void AdjustRadius(int prev, int i, int next, double TargetRInverse, double Security = 0.0)
{
	double OldLane = rlseg[i].lane;

	// Start by aligning points for a reasonable initial lane
	rlseg[i].lane = 
		(-(rlseg[next].t.y - rlseg[prev].t.y) * (rlseg[i].l.x - rlseg[prev].t.x) +
		(  rlseg[next].t.x - rlseg[prev].t.x) * (rlseg[i].l.y - rlseg[prev].t.y)) /
		( (rlseg[next].t.y - rlseg[prev].t.y) * (rlseg[i].r.x - rlseg[i].l.x) -
		(  rlseg[next].t.x - rlseg[prev].t.x) * (rlseg[i].r.y - rlseg[i].l.y));
	if (rlseg[i].lane < -0.2)
		rlseg[i].lane = -0.2;
	else if (rlseg[i].lane > 1.2)
		rlseg[i].lane = 1.2;
	UpdateTxTy(i);

	// Newton-like resolution method
	const double dLane = 0.0001;
	vec2d d = dLane * (rlseg[i].r - rlseg[i].l);
	double dRInverse = GetRInverse(prev, rlseg[i].t + d, next);

	if (dRInverse > 0.000000001) {
		rlseg[i].lane += (dLane / dRInverse) * TargetRInverse;

		double ExtLane = (SideDistExt + Security) / trackWidth;
		double IntLane = (SideDistInt + Security) / trackWidth;
		if (ExtLane > 0.5)
			ExtLane = 0.5;
		if (IntLane > 0.5)
			IntLane = 0.5;

		if (TargetRInverse >= 0.0)
		{
			if (rlseg[i].lane < IntLane)
				rlseg[i].lane = IntLane;
			if (1 - rlseg[i].lane < ExtLane)
			{
				if (1 - OldLane < ExtLane)
					rlseg[i].lane = MIN(OldLane, rlseg[i].lane);
				else
					rlseg[i].lane = 1 - ExtLane;
			}
		}
		else
		{
			if (rlseg[i].lane < ExtLane)
			{
				if (OldLane < ExtLane)
					rlseg[i].lane = MAX(OldLane, rlseg[i].lane);
				else
					rlseg[i].lane = ExtLane;
			}
			if (1 - rlseg[i].lane < IntLane)
				rlseg[i].lane = 1 - IntLane;
		}
	}

	UpdateTxTy(i);
}




static void Smooth(int Step)
{
	int prev = ((nSegments - Step) / Step) * Step;
	int prevprev = prev - Step;
	int next = Step;
	int nextnext = next + Step;

	for (int i = 0; i <= nSegments - Step; i += Step) {
		double ri0 = GetRInverse(prevprev, rlseg[prev].t, i);
		double ri1 = GetRInverse(i, rlseg[next].t, nextnext);
		
		double lPrev = (rlseg[i].t-rlseg[prev].t).len();
		double lNext = (rlseg[i].t-rlseg[next].t).len();
		double TargetRInverse = (lNext * ri0 + lPrev * ri1) / (lNext + lPrev);

		double Security = lPrev * lNext / (8 * SecurityR);
		AdjustRadius(prev, i, next, TargetRInverse, Security);

		prevprev = prev;
		prev = i;
		next = nextnext;
		nextnext = next + Step;
		if (nextnext > nSegments - Step)
			nextnext = 0;
	}
}




static void StepInterpolate(int iMin, int iMax, int Step)
{
	int next = (iMax + Step) % nSegments;
	if (next > nSegments - Step)
		next = 0;

	int prev = (((nSegments + iMin - Step) % nSegments) / Step) * Step;
	if (prev > nSegments - Step)
		prev -= Step;

	double ir0 = GetRInverse(prev, rlseg[iMin].t, iMax % nSegments);
	double ir1 = GetRInverse(iMin,  rlseg[iMax % nSegments].t, next);
	for (int k = iMax; --k > iMin;) {
		double x = double(k - iMin) / double(iMax - iMin);
		double TargetRInverse = x * ir1 + (1 - x) * ir0;
		AdjustRadius(iMin, k, iMax % nSegments, TargetRInverse);
	}
}




static void Interpolate(int Step)
{
	if (Step > 1)
	{
		int i;
		for (i = Step; i <= nSegments - Step; i += Step)
			StepInterpolate(i - Step, i, Step);
		StepInterpolate(i - Step, nSegments, Step);
	}
}




void generateRaceLine(tTrack *pTrack, const double lSideDistExt, const double lSideDistInt)
{
	SideDistExt = lSideDistExt;
	SideDistInt = lSideDistInt;
	
	SplitTrack(pTrack);

	for (int Step = 128; (Step /= 2) > 0;) {
		for (int i = 100 * int(sqrt((double)Step)); --i >= 0;) {
			Smooth(Step);
		}
		Interpolate(Step);
	}
}



double getTexureOffset(double length)
{
	if (!rlseg) {
		return 0.0;
	}

	double seg = length/SegLength;	// normalize
	double rem = seg - floor(seg);	// rem [0..0.999]

	// Average, does not work perfectly on the track end/start transition, but should not matter
	int length1 = ((int) floor(seg)) % nSegments;
	int length2 = (length1 + 1) % nSegments;

	return (rlseg[length1].lane*(1.0 - rem) + rlseg[length2].lane*rem) - 0.5;
}





