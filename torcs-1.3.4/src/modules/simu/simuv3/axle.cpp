/***************************************************************************

    file                 : axle.cpp
    created              : Sun Mar 19 00:05:09 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: axle.cpp,v 1.20.2.1 2008/12/31 03:53:56 berniw Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "sim.h"

static const char *AxleSect[2] = {SECT_FRNTAXLE, SECT_REARAXLE};

void
SimAxleConfig(tCar *car, int index)
{
    void	*hdle = car->params;
    tdble	rollCenter;
    
    tAxle *axle = &(car->axle[index]);

    axle->xpos = GfParmGetNum(hdle, AxleSect[index], PRM_XPOS, (char*)NULL, 0);
    axle->I    = GfParmGetNum(hdle, AxleSect[index], PRM_INERTIA, (char*)NULL, 0.15);
    rollCenter = GfParmGetNum(hdle, AxleSect[index], PRM_ROLLCENTER, (char*)NULL, 0.15);
    car->wheel[index*2].rollCenter = car->wheel[index*2+1].rollCenter = rollCenter;

    if (index == 0) {
	SimSuspConfig(hdle, SECT_FRNTARB, &(axle->arbSusp), 0, 0);
    } else {
	SimSuspConfig(hdle, SECT_REARARB, &(axle->arbSusp), 0, 0);
    }
    
    car->wheel[index*2].feedBack.I += axle->I / 2.0;
    car->wheel[index*2+1].feedBack.I += axle->I / 2.0;
}

void
SimAxleUpdate(tCar *car, int index)
{
    tAxle *axle = &(car->axle[index]);
    tdble str, stl, sgn;
    
    str = car->wheel[index*2].susp.x;
    stl = car->wheel[index*2+1].susp.x;
    sgn = SIGN(stl - str);
#if 0
    axle->arbSusp.x = fabs(stl - str);
    SimSuspCheckIn(&(axle->arbSusp));
    SimSuspUpdate(&(axle->arbSusp));
#else
    axle->arbSusp.x = fabs(stl - str);
    if (axle->arbSusp.x > axle->arbSusp.spring.xMax) {
        axle->arbSusp.x = axle->arbSusp.spring.xMax;
    }
    axle->arbSusp.force = - axle->arbSusp.x *axle->arbSusp.spring.K;
    //axle->arbSusp.force = pow (axle->arbSusp.x *axle->arbSusp.spring.K , 4.0);
#endif
    car->wheel[index*2].axleFz =  sgn * axle->arbSusp.force;
    car->wheel[index*2+1].axleFz = - sgn * axle->arbSusp.force;
    //    printf ("%f %f %f ", stl, str, axle->arbSusp.force);
    //    if (index==0) {
    //        printf ("# SUSP\n");
    //    }
}
 
