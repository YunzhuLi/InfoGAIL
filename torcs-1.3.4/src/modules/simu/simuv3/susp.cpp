/***************************************************************************

    file                 : susp.cpp
    created              : Sun Mar 19 00:08:41 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: susp.cpp,v 1.20.2.1 2008/12/31 03:53:56 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include "sim.h"

/*
 * b2 and b3 calculus
 */
static void
initDamper(tSuspension *susp)
{
    tDamper *damp;
    
    damp = &(susp->damper);
    
    damp->bump.b2 = (damp->bump.C1 - damp->bump.C2) * damp->bump.v1 + damp->bump.b1;
    damp->rebound.b2 = (damp->rebound.C1 - damp->rebound.C2) * damp->rebound.v1 + damp->rebound.b1;
}

void SimSuspDamage(tSuspension* susp, tdble dmg)
{
	susp->damper.efficiency *= exp(0.1*dmg);
	//printf ("Leak: %f -> %f\n", exp(dmg), susp->damper.efficiency);
}


/*
 * get damper force
 */
static tdble
damperForce(tSuspension *susp)
{
    tDamperDef *dampdef;
    tdble     f;
    tdble     av;
    tdble     v;

    v = susp->v;

    if (fabs(v) > 10.0) {
	v = SIGN(v) * 10.0;
    }
    
    if (v < 0) {
	/* rebound */
	dampdef = &(susp->damper.rebound);
    } else {
	/* bump */
	dampdef = &(susp->damper.bump);
    }
    
    av = fabs(v);
    if (av < dampdef->v1) {
	f = (dampdef->C1 * av + dampdef->b1);
    } else {
	f = (dampdef->C2 * av + dampdef->b2);
    }

    f *= SIGN(v) * susp->damper.efficiency;

    return f;
}

/*
 * get spring force
 */
static tdble
springForce(tSuspension *susp)
{
    tSpring *spring = &(susp->spring);
    tdble f;

    /* K is < 0 */
    f = spring->K * (susp->x - spring->x0) + spring->F0;
#if 0 // NOTE: Why should f be only positive?  Does not make sense.
    if (f < 0) {
	f = 0;
    }
#endif
    if (susp->state & SIM_SUSP_COMP) {
        f *= 2;
    }
    return f;
}


void
SimSuspCheckIn(tSuspension *susp)
{
    susp->state = 0;
    if (susp->x < susp->spring.packers) {
	susp->state = SIM_SUSP_COMP;
        if (susp->x < 0) {
            susp->state |= SIM_SUSP_OVERCOMP;
        }
	susp->x = susp->spring.packers;
    }

    susp->x *= susp->spring.bellcrank;
    if (susp->x > susp->spring.xMax) {
	susp->x = susp->spring.xMax;
	susp->state = SIM_SUSP_EXT;
    }

    switch (susp->type) {
    case Wishbone:
	{
	    //tdble link_u = asin(((susp->x - .5*susp->spring.x0)/susp->spring.bellcrank)/susp->link.y);
	    tdble link_u = asin(((susp->x - .2*susp->spring.x0)/susp->spring.bellcrank)/susp->link.y);
	    tdble x1 = susp->link.y * cos(link_u);
	    tdble y1 = susp->link.y * sin(link_u);
	    tdble r1 = susp->link.z;
	    tdble r0 = susp->link.x;
	    tdble x0 = 0.1;
	    tdble y0 = 0.20;
	    tdble dx = x1 - x0;
	    tdble dy = y1 - y0;
	    tdble d2 =(dx*dx+dy*dy);
	    tdble d = sqrt(d2);
	    if ((d<r0+r1)||(d>fabs(r0-r1))) {
		tdble a = (r0*r0-r1*r1+d2)/(2.0*d);
		tdble h = sqrt (r0*r0-a*a);
		tdble x2 = x0 + a*(x1-x0)/d;
		tdble y2 = y0 + a*(x1-y0)/d;
		tdble x3 = x2 + h*(y1-y0)/d;
		tdble y3 = y2 + h*(x1-x0)/d;
		susp->dynamic_angles.x = atan2(x3-x1, y3-y1);
		//printf ("d:%f sR:%f dR:%f u:%f a:%f\n", d, r0+r1, fabs(r0-r1),link_u,susp->dynamic_angles.x);
	    } else {
		susp->dynamic_angles.x = 0.0;
	    }
	    susp->dynamic_angles.y = 0.0;
	    susp->dynamic_angles.z = 0.0;

	}
	break;
    case Simple:
	susp->dynamic_angles.x = 
	    asin(((susp->x - susp->spring.x0)/susp->spring.bellcrank)/susp->link.y);
        susp->dynamic_angles.y = 0.0;
        susp->dynamic_angles.z = 0.0;
	break;
    case Ideal:
    default:
        susp->dynamic_angles.x = 0.0;
        susp->dynamic_angles.y = 0.0;
        susp->dynamic_angles.z = 0.0;
    }
    susp->fx *= susp->spring.K;
    susp->fy *= susp->spring.K;
}

void
SimSuspUpdate(tSuspension *susp)
{
    susp->force = (springForce(susp) + damperForce(susp)) * susp->spring.bellcrank;
}


void
SimSuspConfig(void *hdle, const char *section, tSuspension *susp, tdble F0, tdble X0)
{
    susp->spring.K          = GfParmGetNum(hdle, section, PRM_SPR, (char*)NULL, 175000);
    susp->spring.xMax       = GfParmGetNum(hdle, section, PRM_SUSPCOURSE, (char*)NULL, 0.5);
    susp->spring.bellcrank  = GfParmGetNum(hdle, section, PRM_BELLCRANK, (char*)NULL, 1.0);
    susp->spring.packers    = GfParmGetNum(hdle, section, PRM_PACKERS, (char*)NULL, 0);
    susp->damper.bump.C1    = GfParmGetNum(hdle, section, PRM_SLOWBUMP, (char*)NULL, 0);
    susp->damper.rebound.C1 = GfParmGetNum(hdle, section, PRM_SLOWREBOUND, (char*)NULL, 0);
    susp->damper.bump.C2    = GfParmGetNum(hdle, section, PRM_FASTBUMP, (char*)NULL, 0);
    susp->damper.rebound.C2 = GfParmGetNum(hdle, section, PRM_FASTREBOUND, (char*)NULL, 0);
	susp->damper.efficiency = 1.0;

	const char* suspension_type = GfParmGetStr(hdle, section, PRM_SUSPENSION_TYPE, "Ideal");

    susp->spring.x0 = susp->spring.bellcrank * X0;
    susp->spring.F0 = F0 / susp->spring.bellcrank;
    susp->spring.K = - susp->spring.K;
    susp->damper.bump.b1 = 0;
    susp->damper.rebound.b1 = 0;
    susp->damper.bump.v1 = 0.5;
    susp->damper.rebound.v1 = 0.5;
	if (!strcmp(suspension_type,"Simple")) {
		susp->type = Simple;
	} else if (!strcmp(suspension_type,"Wishbone")) {
		susp->type = Wishbone;
	} else if (!strcmp(suspension_type,"Ideal")) {
		susp->type = Ideal;
	} else {
		fprintf (stderr, "Warning: unknown suspension type %s\n", suspension_type);
		susp->type = Wishbone;
	}
    susp->dynamic_angles.x = 0.0;
    susp->dynamic_angles.y = 0.0;
    susp->dynamic_angles.z = 0.0;
    susp->link.x = 0.7;//additional fishbone link length
    susp->link.y = 0.8;//suspension+wheel link length
    susp->link.z = 0.2;//space between fishbone links on the wheel
    initDamper(susp);
}

