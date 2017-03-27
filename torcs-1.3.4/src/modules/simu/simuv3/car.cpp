/***************************************************************************

    file                 : car.cpp
    created              : Sun Mar 19 00:05:43 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: car.cpp,v 1.31.2.1 2008/11/09 17:50:23 berniw Exp $

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

void
SimCarConfig(tCar *car)
{
    void	*hdle = car->params;
    tdble	k;
    tdble	w;
    tdble	gcfrl, gcrrl, gcfr;
    tdble	wf0, wr0;
    tdble	overallwidth;
    int		i;
    tCarElt	*carElt = car->carElt;

    car->options = new SimulationOptions;
    car->options->SetFromSkill (carElt->_skillLevel);
    car->options->LoadFromFile (hdle);

    car->fuel_time = 0.0;
    car->fuel_consumption = 0.0;
    car->carElt->_fuelTotal = 0.0;
    car->carElt->_fuelInstant = 10.0;

    car->carElt->priv.collision_state.collision_count = 0;
    for (i=0; i<3; i++) {
        car->carElt->priv.collision_state.pos[0] = 0.0;
        car->carElt->priv.collision_state.force[0] = 0.0;
    }

    car->dimension.x = GfParmGetNum(hdle, SECT_CAR, PRM_LEN, (char*)NULL, 4.7);
    car->dimension.y = GfParmGetNum(hdle, SECT_CAR, PRM_WIDTH, (char*)NULL, 1.9);
    overallwidth     = GfParmGetNum(hdle, SECT_CAR, PRM_OVERALLWIDTH, (char*)NULL, car->dimension.y);
    car->dimension.z = GfParmGetNum(hdle, SECT_CAR, PRM_HEIGHT, (char*)NULL, 1.2);
    car->mass        = GfParmGetNum(hdle, SECT_CAR, PRM_MASS, (char*)NULL, 1500);
    car->Minv        = 1.0 / car->mass;
    gcfr             = GfParmGetNum(hdle, SECT_CAR, PRM_FRWEIGHTREP, (char*)NULL, .5);
    gcfrl            = GfParmGetNum(hdle, SECT_CAR, PRM_FRLWEIGHTREP, (char*)NULL, .5);
    gcrrl            = GfParmGetNum(hdle, SECT_CAR, PRM_RRLWEIGHTREP, (char*)NULL, .5);
    car->statGC.y    = - (gcfr * gcfrl + (1 - gcfr) * gcrrl) * car->dimension.y + car->dimension.y / 2.0;
    car->statGC.z    = GfParmGetNum(hdle, SECT_CAR, PRM_GCHEIGHT, (char*)NULL, .5);
    
    car->tank        = GfParmGetNum(hdle, SECT_CAR, PRM_TANK, (char*)NULL, 80);
    car->fuel        = GfParmGetNum(hdle, SECT_CAR, PRM_FUEL, (char*)NULL, 80);
    k                = GfParmGetNum(hdle, SECT_CAR, PRM_CENTR, (char*)NULL, 1.0);
    carElt->_drvPos_x = GfParmGetNum(hdle, SECT_DRIVER, PRM_XPOS, (char*)NULL, 0.0);
    carElt->_drvPos_y = GfParmGetNum(hdle, SECT_DRIVER, PRM_YPOS, (char*)NULL, 0.0);
    carElt->_drvPos_z = GfParmGetNum(hdle, SECT_DRIVER, PRM_ZPOS, (char*)NULL, 0.0);
    carElt->_bonnetPos_x = GfParmGetNum(hdle, SECT_BONNET, PRM_XPOS, (char*)NULL, carElt->_drvPos_x);
    carElt->_bonnetPos_y = GfParmGetNum(hdle, SECT_BONNET, PRM_YPOS, (char*)NULL, carElt->_drvPos_y);
    carElt->_bonnetPos_z = GfParmGetNum(hdle, SECT_BONNET, PRM_ZPOS, (char*)NULL, carElt->_drvPos_z);

    if (car->fuel > car->tank) {
        car->fuel = car->tank;
    }
    car->fuel_prev = car->fuel;
    k = k * k; 
    car->Iinv.x = 12.0 / (car->mass * (car->dimension.y * car->dimension.y + k *car->dimension.z * car->dimension.z));
    car->Iinv.y = 12.0 / (car->mass * (car->dimension.x * car->dimension.x + k * car->dimension.z * car->dimension.z));
    car->Iinv.z = 12.0 / (car->mass * (car->dimension.y * car->dimension.y + k * car->dimension.x * car->dimension.x));
    
    // initialise rotational momentum
    for (i=0; i<4; i++) {
        car->rot_mom[i] = 0.0;
    }
    car->rot_mom[SG_W] = 1.0;

    /* configure components */
    w = car->mass * G;

    wf0 = w * gcfr;
    wr0 = w * (1 - gcfr);

    car->wheel[FRNT_RGT].weight0 = wf0 * gcfrl;
    car->wheel[FRNT_LFT].weight0 = wf0 * (1 - gcfrl);
    car->wheel[REAR_RGT].weight0 = wr0 * gcrrl;
    car->wheel[REAR_LFT].weight0 = wr0 * (1 - gcrrl);

    for (i = 0; i < 2; i++) {
        SimAxleConfig(car, i);
    }

    for (i = 0; i < 4; i++) {
        SimWheelConfig(car, i);	    
    }


    SimEngineConfig(car);
    SimTransmissionConfig(car);
    SimSteerConfig(car);
    SimBrakeSystemConfig(car);
    SimAeroConfig(car);
    for (i = 0; i < 2; i++) {
        SimWingConfig(car, i);
    }
    
    /* Set the origin to GC */
    car->wheelbase = car->wheeltrack = 0;
    car->statGC.x = car->wheel[FRNT_RGT].staticPos.x * gcfr + car->wheel[REAR_RGT].staticPos.x * (1 - gcfr);

    carElt->_dimension = car->dimension;
    carElt->_statGC = car->statGC;
    carElt->_tank = car->tank;
    for (i = 0; i < 4; i++) {
        carElt->priv.wheel[i].relPos = car->wheel[i].relPos;
    }

    for (i = 0; i < 4; i++) {
        car->wheel[i].staticPos.x -= car->statGC.x;
        car->wheel[i].staticPos.y -= car->statGC.y;
    }
    car->wheelbase = (car->wheel[FRNT_RGT].staticPos.x 
                      + car->wheel[FRNT_LFT].staticPos.x
                      - car->wheel[REAR_RGT].staticPos.x
                      - car->wheel[REAR_LFT].staticPos.x) / 2.0;
    car->wheeltrack = (-car->wheel[REAR_LFT].staticPos.y 
                       - car->wheel[FRNT_LFT].staticPos.y
                       + car->wheel[FRNT_RGT].staticPos.y
                       + car->wheel[REAR_RGT].staticPos.y) / 2.0;

    /* set corners pos */
    car->corner[FRNT_RGT].pos.x = car->dimension.x * .5 - car->statGC.x;
    car->corner[FRNT_RGT].pos.y = - overallwidth * .5 - car->statGC.y;
    car->corner[FRNT_RGT].pos.z = 0;
    car->corner[FRNT_LFT].pos.x = car->dimension.x * .5 - car->statGC.x;
    car->corner[FRNT_LFT].pos.y = overallwidth * .5 - car->statGC.y;
    car->corner[FRNT_LFT].pos.z = 0;
    car->corner[REAR_RGT].pos.x = - car->dimension.x * .5 - car->statGC.x;
    car->corner[REAR_RGT].pos.y = - overallwidth * .5 - car->statGC.y;
    car->corner[REAR_RGT].pos.z = 0;
    car->corner[REAR_LFT].pos.x = - car->dimension.x * .5 - car->statGC.x;
    car->corner[REAR_LFT].pos.y = overallwidth * .5 - car->statGC.y;
    car->corner[REAR_LFT].pos.z = 0;
    car->upside_down_timer = 0.0f;
}


static void
SimCarUpdateForces(tCar *car)
{
    tForces	F;
    int		i;
    tdble	m, w, minv;
    tdble	v, R, Rv, Rm, Rx, Ry, Rz;

    car->preDynGC = car->DynGCg;
 
    /* total mass */
    m = car->mass + car->fuel;
    minv = 1.0 / m;
    w = -m * G;

    /* Weight - Bring weight vector to the car's frame of reference */
    sgVec3 pos = {0.0f, 0.0f, w};
    sgRotateVecQuat (pos, car->posQuat);
    F.F.x = pos[SG_X];
    F.F.y = pos[SG_Y];
    F.F.z = pos[SG_Z];	

    // initial torque 0.
    F.M.x = F.M.y = F.M.z = 0;

    /* Wheels */
    for (i = 0; i < 4; i++) {
        tWheel* wheel = &(car->wheel[i]);
        /* forces */
        tdble susp_pos_y = wheel->staticPos.y - sin(wheel->staticPos.ax)*SIGN(wheel->staticPos.y);
        //printf ("%f %f\n", wheel->staticPos.y, sin(wheel->staticPos.ax)*SIGN(wheel->staticPos.y));

        F.F.x += wheel->forces.x;
        F.F.y += wheel->forces.y;
        F.F.z += wheel->forces.z;

        //printf ("%f\n", car->statGC.z + wheel->rideHeight);
        /* moments */
        t3Dd d;
        d.y = susp_pos_y;
        d.x = wheel->staticPos.x;
        d.z = car->statGC.z + wheel->rideHeight;

        F.M.x += (wheel->forces.z * d.y + //susp_pos_y +
                  wheel->forces.y * d.z);
        //(car->statGC.z + wheel->rideHeight));
        F.M.y -= (wheel->forces.z * d.x + //wheel->staticPos.x + 
                  wheel->forces.x * d.z);
        //(car->statGC.z + wheel->rideHeight));
        F.M.z += (-wheel->forces.x * d.y + //susp_pos_y +
                  wheel->forces.y * d.x); //wheel->staticPos.x);
    }

    F.M.x += car->aero.Mx;
    F.M.y += car->aero.My;
    F.M.z += car->aero.Mz;


    /* Aero Drag */
    F.F.x += car->aero.drag;
    F.F.y += car->aero.lateral_drag;
    F.F.z += car->aero.vertical_drag;

    /* Wings & Aero Downforce */
    for (i = 0; i < 2; i++) {
        /* forces */
        F.F.z += car->wing[i].forces.z + car->aero.lift[i];
        F.F.x += car->wing[i].forces.x;
        /* moments */
        float My = car->wing[i].forces.z* car->wing[i].staticPos.x 
            + car->wing[i].forces.x * car->wing[i].staticPos.z
            + car->aero.lift[i] * car->wing[i].staticPos.x;
        F.M.y -= My;
    }


    /* Rolling Resistance */
	// This method updates rolling resistance using the wheels' resistance.
    if (1) {
        v = sqrt(car->DynGC.vel.x * car->DynGC.vel.x
                 + car->DynGC.vel.y * car->DynGC.vel.y
                 + car->DynGC.vel.z * car->DynGC.vel.z);
	
        R = 0;
        for (i = 0; i < 4; i++) {
            R += car->wheel[i].rollRes;
        }
        if (v > 0.00001) {
            Rv = R / v;
            if ((Rv * minv * SimDeltaTime) > v) {
                Rv = v * m / SimDeltaTime;
            }
        } else {
            Rv = 0;
        }
        Rx = Rv * car->DynGC.vel.x; //car->DynGCg.vel.x; 
        Ry = Rv * car->DynGC.vel.y; //car->DynGCg.vel.x; 
        Rz = Rv * car->DynGC.vel.z; //car->DynGCg.vel.x; 
	
        if ((R * car->wheelbase / 2.0f) > fabs(car->rot_mom[SG_Z])) {
            //car->DynGC.vel.az = -car->rot_mom[SG_Z] * car->Iinv.z;
            //Rm =  -car->rot_mom[SG_Z];//car->DynGCg.vel.az / car->Iinv.z;
            Rm = car->rot_mom[SG_Z];//car->DynGCg.vel.az / car->Iinv.z;
        } else {
            Rm = SIGN(car->rot_mom[SG_Z]) * R * car->wheelbase / 2.0;
        }
    } else {
        Rx = Ry = Rz = 0.0f;
        Rm = 0.0;
    }


    /* compute accelerations */
    if (1) {
        /* This should work as long as all forces have been computed for
           the car's frame of reference */

        car->DynGC.acc.x = (F.F.x - Rx) * minv;
        car->DynGC.acc.y = (F.F.y - Ry) * minv;
        car->DynGC.acc.z = (F.F.z - Rz) * minv;

		sgVec3 accel = {car->DynGC.acc.x,  car->DynGC.acc.y, car->DynGC.acc.z};
		sgRotateCoordQuat (accel, car->posQuat);
		car->DynGCg.acc.x = accel[SG_X];
		car->DynGCg.acc.y = accel[SG_Y];
		car->DynGCg.acc.z = accel[SG_Z];	

        car->rot_acc[0] = F.M.x;
        car->rot_acc[1] = F.M.y;
        car->rot_acc[2] = (F.M.z - Rm); 
    }

    
}


static void
SimCarUpdateSpeed(tCar *car)
{
    t3Dd original;
    t3Dd updated;
    t3Dd angles;
    tdble	mass;
    mass = car->mass + car->fuel;

    
    {
        // fuel consumption
        tdble delta_fuel = car->fuel_prev - car->fuel;
        car->fuel_prev = car->fuel;
        if (delta_fuel > 0) {
            car->carElt->_fuelTotal += delta_fuel;
        }
        tdble fi;
        tdble as = sqrt(car->airSpeed2);
        if (as<0.1) {
            fi = 99.9;
        } else {
            fi = 100000 * delta_fuel / (as*SimDeltaTime);
        }
        tdble alpha = 0.1;
        car->carElt->_fuelInstant = (1.0-alpha)*car->carElt->_fuelInstant + alpha*fi;
    }

    // update angles
    angles.x = car->DynGCg.pos.ax;
    angles.y = car->DynGCg.pos.ay;
    angles.z = car->DynGCg.pos.az;	
   
    // update linear velocity
    car->DynGCg.vel.x += car->DynGCg.acc.x * SimDeltaTime;
    car->DynGCg.vel.y += car->DynGCg.acc.y * SimDeltaTime;
    car->DynGCg.vel.z += car->DynGCg.acc.z * SimDeltaTime;

    /* We need to get the speed on the actual frame of reference
       for the car. Now we don't need to worry about the world's
       coordinates anymore when we calculate stuff. I.E check
       aero.cpp*/
    original.x = car->DynGCg.vel.x;
    original.y = car->DynGCg.vel.y;
    original.z = car->DynGCg.vel.z;
    NaiveRotate (original, angles, &updated);
    car->DynGC.vel.x = updated.x;
    car->DynGC.vel.y = updated.y;
    car->DynGC.vel.z = updated.z;


    // Update angular momentum
    car->rot_mom[SG_X] -= car->rot_acc[0] * SimDeltaTime;
    car->rot_mom[SG_Y] -= car->rot_acc[1] * SimDeltaTime;
    car->rot_mom[SG_Z] -= car->rot_acc[2] * SimDeltaTime;
	
#if 0
    // spin limitation
    if (Rm > fabs(car->rot_mom[SG_Z])) {
        Rm = fabs(car->rot_mom[SG_Z]);
    }
    car->rot_mom[SG_Z] -= Rm * SIGN(car->rot_mom[SG_Z]);
#endif

    // Translate angular momentum to angular velocity
    // NOTE: This translation is done again in SimCarAddAngularVelocity()
    car->DynGCg.vel.ax = car->DynGC.vel.ax = -2.0f*car->rot_mom[SG_X] * car->Iinv.x;
    car->DynGCg.vel.ay = car->DynGC.vel.ay = -2.0f*car->rot_mom[SG_Y] * car->Iinv.y;
    car->DynGCg.vel.az = car->DynGC.vel.az = -2.0f*car->rot_mom[SG_Z] * car->Iinv.z;

}

void
SimCarUpdateWheelPos(tCar *car)
{
    tdble vx = car->DynGC.vel.x;
    tdble vy = car->DynGC.vel.y;
    tdble vz = car->DynGC.vel.z;

    /* Wheels data */
    for (int i = 0; i < 4; i++) {
        tWheel *wheel = &(car->wheel[i]);

        t3Dd pos;
        //t3Dd angles;
        pos.x = wheel->staticPos.x;
        pos.y = wheel->staticPos.y;
        pos.z = -car->statGC.z; // or wheel->staticPos.z; ??

        sgVec3 pos3;
        pos3[SG_X] = wheel->staticPos.x;
        pos3[SG_Y] = wheel->staticPos.y;
        pos3[SG_Z] = -car->statGC.z;

        sgRotateCoordQuat (pos3, car->posQuat);
        wheel->pos.x = pos3[SG_X] + car->DynGC.pos.x;
        wheel->pos.y = pos3[SG_Y] + car->DynGC.pos.y;
        wheel->pos.z = pos3[SG_Z] + car->DynGC.pos.z;


        // these two will be taken into account in wheel.cpp
        //	    angles.x += wheel->relPos.ax;
        //	    angles.z += wheel->steer + wheel->staticPos.az;
	    
        wheel->bodyVel.x = vx
            - car->DynGC.vel.az * wheel->staticPos.y
            + car->DynGC.vel.ay * pos.z;
        wheel->bodyVel.y = vy
            + car->DynGC.vel.az * wheel->staticPos.x
            - car->DynGC.vel.ax * pos.z;
        wheel->bodyVel.z = vz
            + car->DynGC.vel.ax * wheel->staticPos.y
            - car->DynGC.vel.ay * wheel->staticPos.x;

    }
}


static void
SimCarUpdatePos(tCar *car)
{
    tdble vx, vy, vz;

    vx = car->DynGCg.vel.x;
    vy = car->DynGCg.vel.y;
    vz = car->DynGCg.vel.z;
    
    car->DynGCg.pos.x = car->DynGC.pos.x;
    car->DynGCg.pos.y = car->DynGC.pos.y;
    car->DynGCg.pos.z = car->DynGC.pos.z;

    car->DynGCg.pos.x += vx * SimDeltaTime;
    car->DynGCg.pos.y += vy * SimDeltaTime;
    car->DynGCg.pos.z += vz * SimDeltaTime;

    car->DynGC.pos.x = car->DynGCg.pos.x;
    car->DynGC.pos.y = car->DynGCg.pos.y;
    car->DynGC.pos.z = car->DynGCg.pos.z;

    SimCarAddAngularVelocity(car);

    NORM_PI_PI(car->DynGC.pos.ax);
    NORM_PI_PI(car->DynGC.pos.ay);
    NORM_PI_PI(car->DynGC.pos.az);

    car->DynGCg.pos.ax = car->DynGC.pos.ax;
    car->DynGCg.pos.ay = car->DynGC.pos.ay;
    car->DynGCg.pos.az = car->DynGC.pos.az;    
    //printf ("a %f %f %f\n", car->DynGC.pos.ax, car->DynGC.pos.ay, car->DynGC.pos.az);
    RtTrackGlobal2Local(car->trkPos.seg, car->DynGCg.pos.x, car->DynGCg.pos.y, &(car->trkPos), TR_LPOS_MAIN);
}

static void
SimCarUpdateCornerPos(tCar *car)
{
    tdble vx = car->DynGC.vel.x;
    tdble vy = car->DynGC.vel.y;
    tdble vz = car->DynGC.vel.z;
    int i;

    for (i = 0; i < 4; i++) {

        tDynPt *corner = &(car->corner[i]);

        sgVec3 v = {corner->pos.x, corner->pos.y, -car->statGC.z};
        sgRotateCoordQuat (v, car->posQuat);
        corner->pos.ax = car->DynGCg.pos.x + v[SG_X];
        corner->pos.ay = car->DynGCg.pos.y + v[SG_Y];
        corner->pos.az = car->DynGCg.pos.z + v[SG_Z];

        // the following is local - confusing a bit.. vel local is
        // .ax, global is .x contrary to pos [was like that in previous code,
        // might redo it when I have time to look through all
        // potential users of this structure - Christos]
        corner->vel.ax = - car->DynGC.vel.az * corner->pos.y;
        corner->vel.ay = car->DynGC.vel.az * corner->pos.x;
        corner->vel.az = car->DynGC.vel.ax * corner->pos.y
            - car->DynGC.vel.ay * corner->pos.x;


        // calculate global frame velocity.
        sgVec3 vel = {corner->vel.ax, corner->vel.ay, corner->vel.az};
        sgRotateCoordQuat (vel, car->posQuat);
        corner->vel.x = car->DynGCg.vel.x + vel[SG_X];
        corner->vel.y = car->DynGCg.vel.y + vel[SG_Y];
        corner->vel.z = car->DynGCg.vel.z + vel[SG_Z];
		
        // don't forget to add local frame velocity.
        corner->vel.ax += vx;
        corner->vel.ay += vy;
        corner->vel.az += vz;

    }
}

void 
SimTelemetryOut(tCar *car)
{
    tdble Fzf, Fzr;
    int i;
    printf("-----------------------------\nCar: %d %s ---\n", car->carElt->index, car->carElt->_name);
    printf("Seg: %d (%s)  Ts:%f  Tr:%f\n",
           car->trkPos.seg->id, car->trkPos.seg->name, car->trkPos.toStart, car->trkPos.toRight);
    printf("---\nMx: %f  My: %f  Mz: %f (N/m)\n", car->DynGC.acc.ax, car->DynGC.acc.ay, car->DynGC.acc.az);
    printf("Wx: %f  Wy: %f  Wz: %f (rad/s)\n", car->DynGC.vel.ax, car->DynGC.vel.ay, car->DynGC.vel.az);
    printf("Ax: %f  Ay: %f  Az: %f (rad)\n", car->DynGC.pos.ax, car->DynGC.pos.ay, car->DynGC.pos.az);
    printf("---\nAx: %f  Ay: %f  Az: %f (Gs)\n", car->DynGC.acc.x/9.81, car->DynGC.acc.y/9.81, car->DynGC.acc.z/9.81);
    printf("Vx: %f  Vy: %f  Vz: %f (m/s)\n", car->DynGC.vel.x, car->DynGC.vel.y, car->DynGC.vel.z);
    printf("Px: %f  Py: %f  Pz: %f (m)\n---\n", car->DynGC.pos.x, car->DynGC.pos.y, car->DynGC.pos.z);
    printf("As: %f\n---\n", sqrt(car->airSpeed2));

    for (i = 0; i < 4; i++) {
        printf("wheel %d - RH:%f susp:%f zr:%.2f ", i, car->wheel[i].rideHeight, car->wheel[i].susp.x, car->wheel[i].zRoad);
        printf("sx:%f sa:%f w:%f ", car->wheel[i].sx, car->wheel[i].sa, car->wheel[i].spinVel);
        printf("fx:%f fy:%f fz:%f\n", car->wheel[i].forces.x, car->wheel[i].forces.y, car->wheel[i].forces.z);
    }

    Fzf = (car->aero.lift[0] + car->wing[0].forces.z) / 9.81;
    Fzr = (car->aero.lift[1] + car->wing[1].forces.z) / 9.81;
    printf("%f %f %f %f %f\n", car->aero.drag / 9.81, Fzf + Fzr,
           Fzf, Fzr, (Fzf + Fzr) / (car->aero.drag + 0.1) * 9.81);
    //    for (i=0; i<4; i++) {
    //   printf ("%f ", car->wheel[i].spinVel);
    //}
    //printf ("| %f %f\n", car->engine.jointI, car->engine.rads);

}

void
SimCarUpdate(tCar *car, tSituation * /* s */)
{
    SimCarUpdateForces(car);
    CHECK(car);
    SimCarUpdateSpeed(car);
    CHECK(car);
    SimCarUpdateCornerPos(car);
    CHECK(car);
    SimCarUpdatePos(car);
    CHECK(car);
    SimCarCollideZ(car);
    CHECK(car);
    SimCarCollideXYScene(car);
    CHECK(car);
}

void
SimCarUpdate2(tCar *car, tSituation * /* s */)
{
#if 0
    if (SimTelemetry == car->carElt->index) SimTelemetryOut(car);

    static int cnt = 10;

    cnt--;
    if (cnt<=0) {

        for (int i = 0; i < 4; i++) {
            printf("%f %f %f ",
                   car->wheel[i].forces.x,
                   car->wheel[i].forces.y,
                   car->wheel[i].forces.z
                   );
			
        }
        float Fzf = (car->aero.lift[0] + car->wing[0].forces.z) / 9.81f;
        float Fzr = (car->aero.lift[1] + car->wing[1].forces.z) / 9.81f;
        printf("%f %f\n", Fzf, Fzr);
#if 0
        printf(" %f %f %f %f\n",
               car->DynGCg.pos.x,
               car->DynGCg.pos.y,
               car->DynGCg.pos.z,
               car->DynGCg.pos.az);
#endif

        cnt=10;
    }
#endif
}

/* Original coords, angle, new coords - global to local frame*/
/* 
   Given coordinates v in frame [0,0,0], recalculate for frame u.

   This is a naive implementation. It should also work with A(BxC),
   with the angles being represented as a normal vector. That'd be
   faster (6 muls instead of 9) -  angle vector has to be precalculated, 
   but that could be the same overhead as calculating sin/cos for angles*/
void NaiveRotate (t3Dd v, t3Dd u, t3Dd* v0)
{

    tdble cosx = cos(u.x);
    tdble cosy = cos(u.y);
    tdble cosz = cos(u.z);
    tdble sinx = sin(u.x);
    tdble siny = sin(u.y);
    tdble sinz = sin(u.z);
#if 0 //rotation order yaw/pitch/roll
    tdble vx_z = v.x * cosz + v.y * sinz;
    tdble vy_z = v.y * cosz - v.x * sinz;
    tdble vx_0 = vx_z * cosy - v.z * siny;
    tdble vz_y = v.z * cosy + vx_z * siny;
    tdble vy_0 = vy_z * cosx + vz_y * sinx;
    tdble vz_0 = vz_y * cosx - vy_z * sinx;
#else // rotation order yaw/roll/pitch 
    tdble vx_z = v.x * cosz + v.y * sinz;
    tdble vy_z = v.y * cosz - v.x * sinz;
    tdble vy_0 = vy_z * cosx + v.z * sinx;
    tdble vz_x = v.z * cosx - vy_z * sinx;
    tdble vx_0 = vx_z * cosy - vz_x * siny;
    tdble vz_0 = vz_x * cosy + vx_z * siny;
#endif
    v0->x = vx_0;
    v0->y = vy_0;
    v0->z = vz_0;
}


/* Original coords, angle, new coords - local to global frame */
/* Given coordinates v in a frame u, calculate coordinates in
   frame [0,0,0].
*/
void NaiveInverseRotate (t3Dd v, t3Dd u, t3Dd* v0)
{
    tdble cosx = cos(-u.x);
    tdble cosy = cos(-u.y);
    tdble cosz = cos(-u.z);
    tdble sinx = sin(-u.x);
    tdble siny = sin(-u.y);
    tdble sinz = sin(-u.z);
#if 0
    tdble vy_x = v.y * cosx + v.z* sinx;
    tdble vz_x = v.z * cosx - v.y * sinx;
    tdble vx_y = v.x * cosy - vz_x * siny;
    tdble vz_0 = vz_x * cosy + v.x * siny;
    tdble vx_0 = vx_y * cosz + vy_x * sinz;
    tdble vy_0 = vy_x * cosz - vx_y * sinz;
#else
    tdble vx_y = v.x * cosy - v.z * siny;
    tdble vz_y = v.z * cosy + v.x * siny;
    tdble vy_x = v.y * cosx + vz_y* sinx;
    tdble vz_0 = vz_y * cosx - v.y * sinx;
    tdble vx_0 = vx_y * cosz + vy_x * sinz;
    tdble vy_0 = vy_x * cosz - vx_y * sinz;
#endif
    v0->x = vx_0;
    v0->y = vy_0;
    v0->z = vz_0;
    //        printf ("..(%f %f %f)\n..[%f %f %f]\n->[%f %f %f]\n",
    //    	    u.x, u.y, u.z,
    //    	    v.x, v.y, v.z,
    //    	    v0->x, v0->y, v0->z);
}


void EulerToQuat (sgQuat quat, tdble h, tdble p, tdble r)
{
    tdble c1=cos(h/2.0);
    tdble s1=sin(h/2.0);
    tdble c2=cos(p/2.0);
    tdble s2=sin(p/2.0);
    tdble c3=cos(r/2.0);
    tdble s3=sin(r/2.0);
    tdble c1c2 = c1*c2;
    tdble s1s2 = s1*s2;
    tdble c1s2 = c1*s2;
    tdble s1c2 = s1*c2;

    quat[SG_W] = c1c2*s3 + s1s2*s3;
    quat[SG_X] = c1c2*s3 - s1s2*c3;
    quat[SG_Y] = c1s2*c3 + s1c2*s3;
    quat[SG_Z] = s1c2*c3 - c1s2*s3;
}

void QuatToEuler (sgVec3 hpr, const sgQuat quat )
{
    tdble sqw = quat[SG_W] * quat[SG_W];
    tdble sqx = quat[SG_X] * quat[SG_X];
    tdble sqy = quat[SG_Y] * quat[SG_Y];
    tdble sqz = quat[SG_Z] * quat[SG_Z];

    hpr[0] = atan2((tdble)(2.0*(quat[SG_X]*quat[SG_Y] + quat[SG_Z]*quat[SG_W])),
                   (tdble)(sqx - sqy - sqz + sqw));
    hpr[1] = atan2((tdble)(2.0*(quat[SG_Y]*quat[SG_Z] + quat[SG_X]*quat[SG_W])),
                   (tdble)(- sqx - sqy + sqz + sqw));
    hpr[2] = asin(2.0*(quat[SG_X]*quat[SG_Z] - quat[SG_Y]*quat[SG_W]));
}



// quaternions
void SimCarAddAngularVelocity (tCar* car)
{
    // use quaternions for the rotation
    sgQuat w;
    sgVec3 new_position;
    int i;

    // Translate momentum into rotational derivative.
    for (/*int*/ i=0; i<4; i++) {
        w[i] = car->rot_mom[i];
    }
    w[SG_X] *= car->Iinv.x;
    w[SG_Y] *= car->Iinv.y;
    w[SG_Z] *= car->Iinv.z;

    // Transform derivative onto local coordinates
    sgPostMultQuat (w, car->posQuat);

    // Add derivative
    for (/*int*/ i=0; i<4; i++) {
        car->posQuat[i] += w[i] * SimDeltaTime ;
    }

    car->DynGC.vel.ax = -2.0f*car->rot_mom[SG_X] * car->Iinv.x;
    car->DynGC.vel.ay = -2.0f*car->rot_mom[SG_Y] * car->Iinv.y;
    car->DynGC.vel.az = -2.0f*car->rot_mom[SG_Z] * car->Iinv.z;

    //printf ("%f %f %f#AXYZ\n",
    //			car->DynGC.vel.ax, 
    //			car->DynGC.vel.ay, 
    //			car->DynGC.vel.az); 
			

    // Turn quaternion into Euler angles
    sgNormaliseQuat(car->posQuat);
    sgQuat tmpQ;
    sgInvertQuat (tmpQ, car->posQuat);
    sgNormaliseQuat(tmpQ);
    sgQuatToEuler (new_position, tmpQ);
    car->DynGC.pos.ax = DEG2RAD(new_position[0]);
    car->DynGC.pos.ay = DEG2RAD(new_position[1]);
    car->DynGC.pos.az = DEG2RAD(new_position[2]);
}


