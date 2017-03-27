/***************************************************************************

    file                 : carstruct.h
    created              : Sun Mar 19 00:06:07 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: carstruct.h,v 1.20 2007/05/02 08:09:51 olethros Exp $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _CAR__H_
#define _CAR__H_

#include <plib/sg.h>
#include <SOLID/solid.h>

#include "aero.h"
#include "susp.h"
#include "brake.h"
#include "axle.h"
#include "steer.h"
#include "wheel.h"
#include "transmission.h"
#include "engine.h"
#include "SimulationOptions.h"

typedef struct
{
    /* driver's interface */
    tCarCtrl	*ctrl;
    void	*params;
    tCarElt	*carElt;

    tCarCtrl	preCtrl;

    /* components */
    tAxle		axle[2];
    tWheel		wheel[4];
    tSteer		steer;
    tBrakeSyst		brkSyst;
    tAero		aero;
    tWing		wing[2];
    tTransmission	transmission;	/* includes clutch, gearbox and driveshaft */
    tEngine		engine;

    /* static */
    t3Dd	dimension;	/* car's mesures */
    tdble	mass;		/* mass with pilot (without fuel) */
    tdble	Minv;		/* 1 / mass with pilot (without fuel) */
    tdble	tank;		/* fuel tank capa */
    t3Dd	statGC;		/* static pos of GC */
    sgQuat      rot_mom;        /* rotational momentum */
    sgVec3      rot_acc;        /* rotational acceleratiom */
    t3Dd	Iinv;		/* inverse of inertial moment along the car's 3 axis */

    /* dynamic */
    tdble	fuel;		/* current fuel load */
    tdble       fuel_consumption; /* average fuel consumption */
    tdble       fuel_prev; /* average fuel consumption */
    tdble       fuel_time; /* average fuel consumption */
    tDynPt	DynGC;		/* GC local data except position */
    tDynPt	DynGCg;		/* GC global data */
    tPosd	VelColl;	/* resulting velocity after collision */
    tDynPt	preDynGC;	/* previous one */
    tTrkLocPos	trkPos;		/* current track position */
    tdble	airSpeed2;	/* current air speed (squared) for aerodynamic forces */

    /* internals */
    tdble	Cosz;
    tdble	Sinz;
    tDynPt	corner[4];	/* x,y,z for static relative pos, ax,ay,az for dyn. world coord */
    int		collision;
    t3Dd	normal;
    t3Dd	collpos;
    tdble	wheelbase;
    tdble	wheeltrack;
    sgMat4	posMat;
    sgMat4      rotMat;
    sgQuat      posQuat;
    DtShapeRef	shape;		/* for collision */
    int		blocked;
    int		dammage;
    
    tdble       upside_down_timer;
    tDynPt	restPos;	/* target rest position after the car is broken */
    tRmInfo     *ReInfo;
    int		collisionAware;
    SimulationOptions* options;
} tCar;

#if 0

#define CHECK_VAR(_var_, _msg_) do {						\
    if (isnan(_var_) || isinf(_var_)) {						\
	printf("%s = %f  in %s line %d\n", _msg_, _var_, __FILE__, __LINE__);	\
        assert (0);								\
	exit(0);								\
    }										\
} while (0)

#define DUMP_CAR(_car_) do {					\
	printf("DynGC.acc.x  = %f\n", (_car_)->DynGC.acc.x);	\
	printf("DynGC.acc.y  = %f\n", (_car_)->DynGC.acc.y);	\
	printf("DynGC.acc.y  = %f\n", (_car_)->DynGC.acc.y);	\
	printf("DynGC.vel.x  = %f\n", (_car_)->DynGC.vel.x);	\
	printf("DynGC.vel.y  = %f\n", (_car_)->DynGC.vel.y);	\
	printf("DynGC.pos.x  = %f\n", (_car_)->DynGC.pos.x);	\
	printf("DynGC.pos.y  = %f\n", (_car_)->DynGC.pos.y);	\
	printf("DynGCg.acc.x = %f\n", (_car_)->DynGCg.acc.x);	\
	printf("DynGCg.acc.y = %f\n", (_car_)->DynGCg.acc.y);	\
	printf("DynGCg.vel.x = %f\n", (_car_)->DynGCg.vel.x);	\
	printf("DynGCg.vel.y = %f\n", (_car_)->DynGCg.vel.y);	\
	printf("DynGCg.pos.x = %f\n", (_car_)->DynGCg.pos.x);	\
	printf("DynGCg.pos.y = %f\n", (_car_)->DynGCg.pos.y);	\
	printf("aero.drag    = %f\n", (_car_)->aero.drag);	\
} while (0)


#define CHECK(_car_) do {									\
    if (isnan((_car_)->DynGC.acc.x) || isinf((_car_)->DynGC.acc.x) ||				\
	isnan((_car_)->DynGC.acc.y) || isinf((_car_)->DynGC.acc.y) ||				\
	isnan((_car_)->DynGC.vel.x) || isinf((_car_)->DynGC.vel.x) ||				\
	isnan((_car_)->DynGC.vel.y) || isinf((_car_)->DynGC.vel.y) ||				\
	isnan((_car_)->DynGC.pos.x) || isinf((_car_)->DynGC.pos.x) ||				\
	isnan((_car_)->DynGC.pos.y) || isinf((_car_)->DynGC.pos.y) ||				\
	isnan((_car_)->DynGC.acc.x) || isinf((_car_)->DynGC.acc.x) ||				\
	isnan((_car_)->DynGCg.acc.y) || isinf((_car_)->DynGCg.acc.y) ||				\
	isnan((_car_)->DynGCg.vel.x) || isinf((_car_)->DynGCg.vel.x) ||				\
	isnan((_car_)->DynGCg.vel.y) || isinf((_car_)->DynGCg.vel.y) ||				\
	isnan((_car_)->DynGCg.pos.x) || isinf((_car_)->DynGCg.pos.x) ||				\
	isnan((_car_)->DynGCg.pos.y) || isinf((_car_)->DynGCg.pos.y) ||				\
	isnan((_car_)->aero.drag) || isinf((_car_)->aero.drag)) {				\
	printf("Problem for %s in %s line %d\n", (_car_)->carElt->_name, __FILE__, __LINE__);	\
	DUMP_CAR(_car_);									\
	assert (0);										\
	/* GfScrShutdown(); */									\
	exit(0);										\
    }												\
} while (0)

#else

#define CHECK_VAR(_var_, _msg_)
#define CHECK(_car_)

#endif

#endif /* _CAR__H_ */ 








