/***************************************************************************

    file                 : simu.cpp
    created              : Sun Mar 19 00:07:53 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: simu.cpp,v 1.24.2.1 2008/12/31 03:53:56 berniw Exp $

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#ifdef WIN32
#include <windows.h>
#include <float.h>
#define isnan _isnan
#endif

#include <tgf.h>
#include <robottools.h>
#include "sim.h"

extern double timer_coordinate_transform;
extern double timer_reaction;
extern double timer_angles;
extern double timer_friction;
extern double timer_temperature;
extern double timer_force_calculation;
extern double timer_wheel_to_car;
extern double access_times;

tCar *SimCarTable = 0;

tdble SimDeltaTime;

int SimTelemetry;

static int SimNbCars = 0;
static double simu_total_time = 0.0f;
static double simu_init_time = 0.0f;

t3Dd vectStart[16];
t3Dd vectEnd[16];

#define MEANNB 0
#define MEANW  1



/*
 * Check the input control from robots
 */
static void
ctrlCheck(tCar *car)
{
    tTransmission	*trans = &(car->transmission);
    tClutch		*clutch = &(trans->clutch);

    /* sanity check */
#ifndef WIN32
    if (isnan(car->ctrl->accelCmd) || isinf(car->ctrl->accelCmd)) car->ctrl->accelCmd = 0;
    if (isnan(car->ctrl->brakeCmd) || isinf(car->ctrl->brakeCmd)) car->ctrl->brakeCmd = 0;
    if (isnan(car->ctrl->clutchCmd) || isinf(car->ctrl->clutchCmd)) car->ctrl->clutchCmd = 0;
    if (isnan(car->ctrl->steer) || isinf(car->ctrl->steer)) car->ctrl->steer = 0;
    if (isnan(car->ctrl->gear) || isinf(car->ctrl->gear)) car->ctrl->gear = 0;
#else
    if (isnan(car->ctrl->accelCmd)) car->ctrl->accelCmd = 0;
    if (isnan(car->ctrl->brakeCmd)) car->ctrl->brakeCmd = 0;
    if (isnan(car->ctrl->clutchCmd)) car->ctrl->clutchCmd = 0;
    if (isnan(car->ctrl->steer)) car->ctrl->steer = 0;
    if (isnan(car->ctrl->gear)) car->ctrl->gear = 0;
#endif

    /* When the car is broken try to send it on the track side */
    if (car->carElt->_state & RM_CAR_STATE_BROKEN) {
		car->ctrl->accelCmd = 0.0;
		car->ctrl->brakeCmd = 0.1;
		car->ctrl->gear = 0;
		if (car->trkPos.toRight >  car->trkPos.seg->width / 2.0) {
			car->ctrl->steer = 0.1;
		} else {
			car->ctrl->steer = -0.1;
		}
    } else if (car->carElt->_state & RM_CAR_STATE_ELIMINATED) {
		car->ctrl->accelCmd = 0.0;
		car->ctrl->brakeCmd = 0.1;
		car->ctrl->gear = 0;
		if (car->trkPos.toRight >  car->trkPos.seg->width / 2.0) {
			car->ctrl->steer = 0.1;
		} else {
			car->ctrl->steer = -0.1;
		}
    } else if (car->carElt->_state & RM_CAR_STATE_FINISH) {
		/* when the finish line is passed, continue at "slow" pace */
		car->ctrl->accelCmd = MIN(car->ctrl->accelCmd, 0.20);
		if (car->DynGC.vel.x > 30.0) {
			car->ctrl->brakeCmd = MAX(car->ctrl->brakeCmd, 0.05);
		}
    }

    /* check boundaries */
    if (car->ctrl->accelCmd > 1.0) {
		car->ctrl->accelCmd = 1.0;
    } else if (car->ctrl->accelCmd < 0.0) {
		car->ctrl->accelCmd = 0.0;
    }
    if (car->ctrl->brakeCmd > 1.0) {
		car->ctrl->brakeCmd = 1.0;
    } else if (car->ctrl->brakeCmd < 0.0) {
		car->ctrl->brakeCmd = 0.0;
    }
    if (car->ctrl->clutchCmd > 1.0) {
		car->ctrl->clutchCmd = 1.0;
    } else if (car->ctrl->clutchCmd < 0.0) {
		car->ctrl->clutchCmd = 0.0;
    }
    if (car->ctrl->steer > 1.0) {
		car->ctrl->steer = 1.0;
    } else if (car->ctrl->steer < -1.0) {
		car->ctrl->steer = -1.0;
    }

    clutch->transferValue = 1.0 - car->ctrl->clutchCmd;
}

/* Initial configuration */
void
SimConfig(tCarElt *carElt, tRmInfo* ReInfo)
{
    tCar *car = &(SimCarTable[carElt->index]);

    memset(car, 0, sizeof(tCar));

    car->carElt = carElt;
    car->DynGCg = car->DynGC = carElt->_DynGC;

    car->trkPos = carElt->_trkPos;
    car->ctrl   = &carElt->ctrl;
    car->params = carElt->_carHandle;
    car->ReInfo = ReInfo;
    SimCarConfig(car);

    SimCarCollideConfig(car);
    sgMakeCoordMat4(carElt->pub.posMat, carElt->_pos_X, carElt->_pos_Y, carElt->_pos_Z - carElt->_statGC_z,
					RAD2DEG(carElt->_yaw), RAD2DEG(carElt->_roll), RAD2DEG(carElt->_pitch));

	sgEulerToQuat (car->posQuat, -RAD2DEG(carElt->_yaw), RAD2DEG(carElt->_pitch), RAD2DEG(carElt->_roll));
	sgQuatToMatrix (car->posMat, car->posQuat);

	simu_total_time = 0.0f;
	simu_init_time = GfTimeClock();
	//sgMakeRotMat4 (car->posMat, RAD2DEG(carElt->_yaw), RAD2DEG(carElt->_roll), RAD2DEG(carElt->_pitch));

}

/* After pit stop */
void
SimReConfig(tCarElt *carElt)
{
    tCar *car = &(SimCarTable[carElt->index]);
    if (carElt->pitcmd.fuel > 0) {
		car->fuel += carElt->pitcmd.fuel;
		if (car->fuel > car->tank) car->fuel = car->tank;
    }
    if (carElt->pitcmd.repair > 0) {
		for (int i=0; i<4; i++) {
			carElt->_tyreCondition(i) = 1.01;
			carElt->_tyreT_in(i) = 50.0;
			carElt->_tyreT_mid(i) = 50.0;
			carElt->_tyreT_out(i) = 50.0;
			car->wheel[i].bent_damage_x = urandom();
			car->wheel[i].bent_damage_z = urandom();
			car->wheel[i].rotational_damage_x = 0.0;
			car->wheel[i].rotational_damage_z = 0.0;
			car->wheel[i].susp.damper.efficiency = 1.0;
		}
		
		// (no need to repair wings because effect depends on damage).
		car->dammage -= carElt->pitcmd.repair;
		if (car->dammage < 0) car->dammage = 0;
    }
}

static void
RemoveCar(tCar *car, tSituation *s)
{
    int		i;
    tCarElt 	*carElt;
    tTrkLocPos	trkPos;
    int		trkFlag;
    tdble	travelTime;
    tdble	dang;

#define PULL_Z_OFFSET 3.0
#define PULL_SPD      0.5

    carElt = car->carElt;

    if (carElt->_state & RM_CAR_STATE_PULLUP) {
		carElt->_pos_Z += car->restPos.vel.z * SimDeltaTime;
		carElt->_yaw += car->restPos.vel.az * SimDeltaTime;
		carElt->_roll += car->restPos.vel.ax * SimDeltaTime;
		carElt->_pitch += car->restPos.vel.ay * SimDeltaTime;
		sgMakeCoordMat4(carElt->pub.posMat, carElt->_pos_X, carElt->_pos_Y, carElt->_pos_Z - carElt->_statGC_z,
						RAD2DEG(carElt->_yaw), RAD2DEG(carElt->_roll), RAD2DEG(carElt->_pitch));

		if (carElt->_pos_Z > (car->restPos.pos.z + PULL_Z_OFFSET)) {
			carElt->_state &= ~RM_CAR_STATE_PULLUP;
			carElt->_state |= RM_CAR_STATE_PULLSIDE;

			travelTime = DIST(car->restPos.pos.x, car->restPos.pos.y, carElt->_pos_X, carElt->_pos_Y) / PULL_SPD;
			car->restPos.vel.x = (car->restPos.pos.x - carElt->_pos_X) / travelTime;
			car->restPos.vel.y = (car->restPos.pos.y - carElt->_pos_Y) / travelTime;
		}
		return;
    }
    

    if (carElt->_state & RM_CAR_STATE_PULLSIDE) {
		carElt->_pos_X += car->restPos.vel.x * SimDeltaTime;
		carElt->_pos_Y += car->restPos.vel.y * SimDeltaTime;
		sgMakeCoordMat4(carElt->pub.posMat, carElt->_pos_X, carElt->_pos_Y, carElt->_pos_Z - carElt->_statGC_z,
						RAD2DEG(carElt->_yaw), RAD2DEG(carElt->_roll), RAD2DEG(carElt->_pitch));

		if ((fabs(car->restPos.pos.x - carElt->_pos_X) < 0.5) && (fabs(car->restPos.pos.y - carElt->_pos_Y) < 0.5)) {
			carElt->_state &= ~RM_CAR_STATE_PULLSIDE;
			carElt->_state |= RM_CAR_STATE_PULLDN;
		}
		return;
    }

    if (carElt->_state & RM_CAR_STATE_PULLDN) {
		carElt->_pos_Z -= car->restPos.vel.z * SimDeltaTime;
		sgMakeCoordMat4(carElt->pub.posMat, carElt->_pos_X, carElt->_pos_Y, carElt->_pos_Z - carElt->_statGC_z,
						RAD2DEG(carElt->_yaw), RAD2DEG(carElt->_roll), RAD2DEG(carElt->_pitch));

		if (carElt->_pos_Z < car->restPos.pos.z) {
			carElt->_state &= ~RM_CAR_STATE_PULLDN;
			carElt->_state |= RM_CAR_STATE_OUT;
		}
		return;
    }

    if (carElt->_state & RM_CAR_STATE_NO_SIMU) {
		return;
    }

    if ((s->_maxDammage) && (car->dammage > s->_maxDammage)) {
		carElt->_state |= RM_CAR_STATE_BROKEN;
    } else {
		carElt->_state |= RM_CAR_STATE_OUTOFGAS;
    }
    carElt->_gear = car->transmission.gearbox.gear = 0;
    carElt->_enginerpm = car->engine.rads = 0;
  
    if (!(carElt->_state & RM_CAR_STATE_DNF)) {
		if (fabs(carElt->_speed_x) > 1.0) {
			return;
		}
    }
    carElt->_state |= RM_CAR_STATE_PULLUP;

    carElt->priv.collision = car->collision = 0;
    for(i = 0; i < 4; i++) {
		carElt->_skid[i] = 0;
		carElt->_wheelSpinVel(i) = 0;
		carElt->_brakeTemp(i) = 0;
    }
    carElt->pub.DynGC = car->DynGC;
    carElt->_speed_x = 0;

    /* compute the target zone for the wrecked car */
    trkPos = car->trkPos;
    if (trkPos.toRight >  trkPos.seg->width / 2.0) {
		while (trkPos.seg->lside != 0) {
			trkPos.seg = trkPos.seg->lside;
		}
		trkPos.toLeft = -3.0;
		trkFlag = TR_TOLEFT;
    } else {
		while (trkPos.seg->rside != 0) {
			trkPos.seg = trkPos.seg->rside;
		}
		trkPos.toRight = -3.0;
		trkFlag = TR_TORIGHT;
    }

    trkPos.type = TR_LPOS_SEGMENT;
    RtTrackLocal2Global(&trkPos, &(car->restPos.pos.x), &(car->restPos.pos.y), trkFlag);
    car->restPos.pos.z = RtTrackHeightL(&trkPos) + carElt->_statGC_z;
    car->restPos.pos.az = RtTrackSideTgAngleL(&trkPos);
    car->restPos.pos.ax = 0;
    car->restPos.pos.ay = 0;

    car->restPos.vel.z = PULL_SPD;
    travelTime = (car->restPos.pos.z + PULL_Z_OFFSET - carElt->_pos_Z) / car->restPos.vel.z;
    dang = car->restPos.pos.az - carElt->_yaw;
    NORM_PI_PI(dang);
    car->restPos.vel.az = dang / travelTime;
    dang = car->restPos.pos.ax - carElt->_roll;
    NORM_PI_PI(dang);
    car->restPos.vel.ax = dang / travelTime;
    dang = car->restPos.pos.ay - carElt->_pitch;
    NORM_PI_PI(dang);
    car->restPos.vel.ay = dang / travelTime;

}



void
SimUpdate(tSituation *s, double deltaTime, int telemetry)
{
    int		i;
    int		ncar;
    tCarElt 	*carElt;
    tCar 	*car;
    sgVec3	P;
	static const float UPSIDE_DOWN_TIMEOUT = 5.0f;

	double timestamp_start = GfTimeClock();
    SimDeltaTime = deltaTime;
    SimTelemetry = 0;//telemetry;
    for (ncar = 0; ncar < s->_ncars; ncar++) {
		SimCarTable[ncar].collision = 0;
		SimCarTable[ncar].blocked = 0;
    }
    
    for (ncar = 0; ncar < s->_ncars; ncar++) {
		car = &(SimCarTable[ncar]);
		carElt = car->carElt;

		if (carElt->_state & RM_CAR_STATE_NO_SIMU) {
			RemoveCar(car, s);
			continue;
		} else if (((s->_maxDammage) && (car->dammage > s->_maxDammage)) ||
				   (car->fuel == 0) ||
				   (car->upside_down_timer > UPSIDE_DOWN_TIMEOUT) ||
				   (car->carElt->_state & RM_CAR_STATE_ELIMINATED))  {
			RemoveCar(car, s);
			if (carElt->_state & RM_CAR_STATE_NO_SIMU) {
				continue;
			}
		}
	
		if (s->_raceState & RM_RACE_PRESTART) {
			car->ctrl->gear = 0;
		}
	



		CHECK(car);
		ctrlCheck(car);
		CHECK(car);
		SimSteerUpdate(car);
		CHECK(car);
		SimGearboxUpdate(car);
		CHECK(car);
		SimEngineUpdateTq(car);
		CHECK(car);


		if (!(s->_raceState & RM_RACE_PRESTART)) {

				SimCarUpdateWheelPos(car);
			CHECK(car);
				SimBrakeSystemUpdate(car);
			CHECK(car);
				SimAeroUpdate(car, s);
			CHECK(car);
				for (i = 0; i < 2; i++){
				SimWingUpdate(car, i, s);
			}
			CHECK(car);
				for (i = 0; i < 4; i++){
				SimWheelUpdateRide(car, i);
			}
			CHECK(car);
				for (i = 0; i < 2; i++){
				SimAxleUpdate(car, i);
			}
			CHECK(car);
				for (i = 0; i < 4; i++){
				SimWheelUpdateForce(car, i);
			}
			CHECK(car);
		}
		SimTransmissionUpdate(car);
		CHECK(car);

		if (!(s->_raceState & RM_RACE_PRESTART)) {
				SimWheelUpdateRotation(car);
			CHECK(car);
				SimCarUpdate(car, s);
			CHECK(car);
		}
    }

    SimCarCollideCars(s);

    /* printf ("%f - ", s->currentTime); */
    
    for (ncar = 0; ncar < s->_ncars; ncar++) {
		car = &(SimCarTable[ncar]);
		CHECK(car);
		carElt = car->carElt;

		if (carElt->_state & RM_CAR_STATE_NO_SIMU) {
			continue;
		}

		CHECK(car);
		SimCarUpdate2(car, s);

		/* copy back the data to carElt */

		carElt->pub.DynGC = car->DynGC;
		carElt->pub.DynGCg = car->DynGCg;
#if 0
		sgMakeCoordMat4(carElt->pub.posMat, carElt->_pos_X, carElt->_pos_Y, carElt->_pos_Z - carElt->_statGC_z,
						RAD2DEG(carElt->_yaw), RAD2DEG(carElt->_roll), RAD2DEG(carElt->_pitch));
#else
		sgQuatToMatrix (carElt->pub.posMat, car->posQuat);
		carElt->pub.posMat[3][0] = car->DynGCg.pos.x;
		carElt->pub.posMat[3][1] = car->DynGCg.pos.y;
		carElt->pub.posMat[3][2] = car->DynGCg.pos.z - carElt->_statGC_z;
		carElt->pub.posMat[0][3] =  SG_ZERO ;
		carElt->pub.posMat[1][3] =  SG_ZERO ;
		carElt->pub.posMat[2][3] =  SG_ZERO ;
		carElt->pub.posMat[3][3] =  SG_ONE ;
#endif
		carElt->_trkPos = car->trkPos;
		for (i = 0; i < 4; i++) {
			carElt->priv.wheel[i].relPos = car->wheel[i].relPos;
			carElt->_wheelSeg(i) = car->wheel[i].trkPos.seg;
			carElt->_brakeTemp(i) = car->wheel[i].brake.temp;
			carElt->pub.corner[i] = car->corner[i].pos;
		}
		carElt->_gear = car->transmission.gearbox.gear;
		carElt->_enginerpm = car->engine.rads;
		carElt->_fuel = car->fuel;
		carElt->priv.collision |= car->collision;
		carElt->_dammage = car->dammage;

		P[0] = -carElt->_statGC_x;
		P[1] = -carElt->_statGC_y;
		P[2] = -carElt->_statGC_z;
		sgXformPnt3(P, carElt->_posMat);
		carElt->_pos_X = P[0];
		carElt->_pos_Y = P[1];
		carElt->_pos_Z = P[2];
	}
	simu_total_time +=  GfTimeClock() - timestamp_start;
}


void
SimInit(int nbcars, tTrack* track)
{
    SimNbCars = nbcars;
    SimCarTable = (tCar*)calloc(nbcars, sizeof(tCar));
    SimCarCollideInit();
}

void
SimShutdown(void)
{
    tCar *car;
    int	 ncar;
#if 0
	double elapsed_time = GfTimeClock() - simu_init_time;
	printf ("delta_time: %f\n", SimDeltaTime);
	printf ("simu time: %fs (%f%% of %fs)\n", simu_total_time,
			100.0f * simu_total_time/elapsed_time, elapsed_time);
	printf ("\
			 timer_coordinate_transform:%f\n\
			 timer_reaction:%f\n\
			 timer_angles:%f\n\
			 timer_friction:%f\n\
			 timer_temperature:%f\n\
			 timer_force_calculation:%f\n\
			 timer_wheel_to_car:%f\n\
			 access_times:%f\n",
			 timer_coordinate_transform,
			 timer_reaction,
			 timer_angles,
			 timer_friction,
			 timer_temperature,
			 timer_force_calculation,
			timer_wheel_to_car,
			access_times);
#endif
    SimCarCollideShutdown(SimNbCars);
    if (SimCarTable) {
		for (ncar = 0; ncar < SimNbCars; ncar++) {
			car = &(SimCarTable[ncar]);
			delete car->options;
			SimEngineShutdown(car);
		}
		free(SimCarTable);
		SimCarTable = 0;
    }
}

