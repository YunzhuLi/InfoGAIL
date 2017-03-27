/***************************************************************************

    file                 : aero.cpp
    created              : Sun Mar 19 00:04:50 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: aero.cpp,v 1.25.2.1 2008/12/31 03:53:56 berniw Exp $

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

void 
SimAeroConfig(tCar *car)
{
    void *hdle = car->params;
    tdble Cx, FrntArea;
    
    Cx       = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_CX, (char*)NULL, 0.4);
    FrntArea = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_FRNTAREA, (char*)NULL, 2.5);
    car->aero.Clift[0] = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_FCL, (char*)NULL, 0.0);
    car->aero.Clift[1] = GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_RCL, (char*)NULL, 0.0);
    float aero_factor = car->options->aero_factor;

    car->aero.SCx2 = 0.5f * AIR_DENSITY * Cx * FrntArea;
    car->aero.Clift[0] *= aero_factor / 4.0f;
    car->aero.Clift[1] *= aero_factor / 4.0f;
    float max_lift = MaximumLiftGivenDrag (car->aero.SCx2, FrntArea);
    float current_lift = 2.0f * (car->aero.Clift[0] + car->aero.Clift[1]);
    if (current_lift > max_lift) {
        fprintf (stderr, "Warning: car %s, driver %s: lift coefficients (%f, %f), generate a lift of %f, while maximum theoretical value is %f\n",
                 car->carElt->_carName,
                 car->carElt->_name,
                 car->aero.Clift[0],
                 car->aero.Clift[1],
                 current_lift,
                 max_lift);
    }

    GfParmSetNum(hdle, SECT_AERODYNAMICS, PRM_FCL, (char*)NULL, car->aero.Clift[0]);
    GfParmSetNum(hdle, SECT_AERODYNAMICS, PRM_RCL, (char*)NULL, car->aero.Clift[1]);
    //printf ("%f %f\n", GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_FCL, (char*)NULL, 0.0), GfParmGetNum(hdle, SECT_AERODYNAMICS, PRM_RCL, (char*)NULL, 0.0));
    //printf ("cl: %f\n", car->aero.Clift[0]+car->aero.Clift[1]);
    car->aero.Cd += car->aero.SCx2;
    car->aero.rot_front[0] = 0.0;
    car->aero.rot_front[1] = 0.0;
    car->aero.rot_front[2] = 0.0;
    car->aero.rot_lateral[0] = 0.0;
    car->aero.rot_lateral[1] = 0.0;
    car->aero.rot_lateral[2] = 0.0;
    car->aero.rot_vertical[0] = 0.0;
    car->aero.rot_vertical[1] = 0.0;
    car->aero.rot_vertical[2] = 0.0;

}


void 
SimAeroUpdate(tCar *car, tSituation *s)
{
    //tdble	hm;
    int		i;	    
    tdble	airSpeed;
    tdble	dragK = 1.0;

    airSpeed = car->DynGC.vel.x;

    if (airSpeed > 10.0) {
	tdble x = car->DynGC.pos.x;
	tdble y = car->DynGC.pos.y;
	//	tdble x = car->DynGC.pos.x + cos(yaw)*wing->staticPos.x;
	//	tdble y = car->DynGC.pos.y + sin(yaw)*wing->staticPos.x;
	tdble yaw = car->DynGC.pos.az;
	tdble spdang = atan2(car->DynGCg.vel.y, car->DynGCg.vel.x);
	for (i = 0; i < s->_ncars; i++) {
	    if (i == car->carElt->index) {
		continue;
	    }


	    tdble tmpas = 1.00;

	    tCar* otherCar = &(SimCarTable[i]);
	    tdble otherYaw = otherCar->DynGC.pos.az;
	    tdble tmpsdpang = spdang - atan2(y - otherCar->DynGC.pos.y, x - otherCar->DynGC.pos.x);
	    NORM_PI_PI(tmpsdpang);
	    tdble dyaw = yaw - otherYaw;
	    NORM_PI_PI(dyaw);

	    if ((otherCar->DynGC.vel.x > 10.0) &&
		(fabs(dyaw) < 0.1396)) {
		if (fabs(tmpsdpang) > 2.9671) {	    /* 10 degrees */
		    /* behind another car - reduce overall airflow */
                    tdble factor = (fabs(tmpsdpang)-2.9671)/(M_PI-2.9671);

		    tmpas = 1.0 - factor * exp(- 2.0 * DIST(x, y, otherCar->DynGC.pos.x, otherCar->DynGC.pos.y)/(otherCar->aero.Cd * otherCar->DynGC.vel.x));
		    airSpeed = airSpeed * tmpas;
		} else if (fabs(tmpsdpang) < 0.1396f) {	    /* 8 degrees */
                    tdble factor = 0.5f * (0.1396f-fabs(tmpsdpang))/(0.1396f);
		    /* before another car - breaks down rear eddies, reduces only drag*/
		    tmpas = 1.0f - factor * exp(- 8.0 * DIST(x, y, otherCar->DynGC.pos.x, otherCar->DynGC.pos.y) / (car->aero.Cd * car->DynGC.vel.x));
		    dragK = dragK * tmpas;
		}
	    }
	}
    }

    car->airSpeed2 = airSpeed * airSpeed;
    
    tdble v2 = car->airSpeed2;
    tdble dmg_coef = ((tdble)car->dammage / 10000.0);

    car->aero.drag = -SIGN(car->DynGC.vel.x) * car->aero.SCx2 * v2 * (1.0 + dmg_coef) * dragK * dragK;


    // Since we have the forces ready, we just multiply. 
    // Should insert constants here.
    // Also, no torque is produced since the effect can be
    // quite dramatic. Interesting idea to make all drags produce
    // torque when the car is damaged.
    car->aero.Mx = car->aero.drag * dmg_coef * car->aero.rot_front[0];
    car->aero.My = car->aero.drag * dmg_coef * car->aero.rot_front[1];
    car->aero.Mz = car->aero.drag * dmg_coef * car->aero.rot_front[2];


    v2 = car->DynGC.vel.y;
    car->aero.lateral_drag = -SIGN(v2)*v2*v2*0.7;
    car->aero.Mx += car->aero.lateral_drag * dmg_coef * car->aero.rot_lateral[0];
    car->aero.My += car->aero.lateral_drag * dmg_coef * car->aero.rot_lateral[1];
    car->aero.Mz += car->aero.lateral_drag * dmg_coef * car->aero.rot_lateral[2];

    v2 = car->DynGC.vel.z;
    car->aero.vertical_drag = -SIGN(v2)*v2*v2*1.5;
    car->aero.Mx += car->aero.vertical_drag * dmg_coef * car->aero.rot_vertical[0];
    car->aero.My += car->aero.vertical_drag * dmg_coef * car->aero.rot_vertical[1];
    car->aero.Mz += car->aero.vertical_drag * dmg_coef * car->aero.rot_vertical[2];



    

}

void SimAeroDamage(tCar *car, sgVec3 poc, tdble F)
{
    tAero* aero = &car->aero;
    tdble dmg = F*0.0001;

    aero->rot_front[0] += dmg*(urandom()-.5);
    aero->rot_front[1] += dmg*(urandom()-.5);
    aero->rot_front[2] += dmg*(urandom()-.5);
    if (sgLengthVec3(car->aero.rot_front) > 1.0) {
        sgNormaliseVec3 (car->aero.rot_front);
    }
    aero->rot_lateral[0] += dmg*(urandom()-.5);
    aero->rot_lateral[1] += dmg*(urandom()-.5);
    aero->rot_lateral[2] += dmg*(urandom()-.5);
    if (sgLengthVec3(car->aero.rot_lateral) > 1.0) {
        sgNormaliseVec3 (car->aero.rot_lateral);
    }
    aero->rot_vertical[0] += dmg*(urandom()-.5);
    aero->rot_vertical[1] += dmg*(urandom()-.5);
    aero->rot_vertical[2] += dmg*(urandom()-.5);
    if (sgLengthVec3(car->aero.rot_vertical) > 1.0) {
        sgNormaliseVec3 (car->aero.rot_vertical);
    }

    //printf ("aero damage:%f (->%f %f %f)\n", dmg, sgLengthVec3(car->aero.rot_front),
    //			sgLengthVec3(car->aero.rot_lateral), sgLengthVec3(car->aero.rot_vertical));



}

static const char *WingSect[2] = {SECT_FRNTWING, SECT_REARWING};

void
SimWingConfig(tCar *car, int index)
{
    void   *hdle = car->params;
    tWing  *wing = &(car->wing[index]);
    tdble area;

    area              = GfParmGetNum(hdle, WingSect[index], PRM_WINGAREA, (char*)NULL, 0);
    // we need also the angle
    wing->angle       = GfParmGetNum(hdle, WingSect[index], PRM_WINGANGLE, (char*)NULL, 0);
    wing->staticPos.x = GfParmGetNum(hdle, WingSect[index], PRM_XPOS, (char*)NULL, 0);
    wing->staticPos.z = GfParmGetNum(hdle, WingSect[index], PRM_ZPOS, (char*)NULL, 0);
    
    switch (car->options->aeroflow_model) {
    case SIMPLE:
        wing->Kx = -AIR_DENSITY * area; ///< \bug: there should be a 1/2 here.
        //wing->Kz = 4.0 * wing->Kx;
        wing->Kz = car->options->aero_factor * wing->Kx;
        break;
    case PLANAR:
        wing->Kx = -AIR_DENSITY * area * 16.0f;
        wing->Kz = wing->Kx;
        break;
    case OPTIMAL:
		fprintf (stderr, "Using optimal wings\n");
        wing->Kx = -AIR_DENSITY * area; ///< \bug: there should be a 1/2 here.
        wing->Kz = car->options->aero_factor * wing->Kx;
        break;
    default:
        fprintf (stderr, "Unimplemented option %d for aeroflow model\n", car->options->aeroflow_model);
    }


    if (index == 1) {
	car->aero.Cd -= wing->Kx*sin(wing->angle);
    }
}


void
SimWingUpdate(tCar *car, int index, tSituation* s)
{
    tWing  *wing = &(car->wing[index]);
    tdble vt2 = car->DynGC.vel.x;
    tdble i_flow = 1.0;
    // rear wing should not get any flow.

    // compute angle of attack 
    // we don't  add ay anymore since DynGC.vel.x,z are now in the correct frame of reference (see car.cpp)
    tdble aoa = atan2(car->DynGC.vel.z, car->DynGC.vel.x); //+ car->DynGC.pos.ay;
    // The flow to the rear wing can get cut off at large negative
    // angles of attack.  (so it won't produce lift because it will be
    // completely shielded by the car's bottom)
    // The value -0.4 should depend on the positioning of the wing. 
    // we also make this be like that.
    if (index==1) {
        i_flow = PartialFlowSmooth (-0.4, aoa);
    } 
    // Flow to the wings gets cut off by other cars.
    tdble airSpeed = car->DynGC.vel.x;

    if (airSpeed > 10.0) {
	tdble yaw = car->DynGC.pos.az;
	tdble x = car->DynGC.pos.x + cos(yaw)*wing->staticPos.x;
	tdble y = car->DynGC.pos.y + sin(yaw)*wing->staticPos.x;
	tdble spdang = atan2(car->DynGCg.vel.y, car->DynGCg.vel.x);

	int i;
	for (i = 0; i < s->_ncars; i++) {
	    if (i == car->carElt->index) {
		continue;
	    }
	    tdble tmpas = 1.00;
	    tCar* otherCar = &(SimCarTable[i]);
	    tdble otherYaw = otherCar->DynGC.pos.az;
	    tdble tmpsdpang = spdang - atan2(y - otherCar->DynGC.pos.y, x - otherCar->DynGC.pos.x);
	    NORM_PI_PI(tmpsdpang);
	    tdble dyaw = yaw - otherYaw;
	    NORM_PI_PI(dyaw);
	    if ((otherCar->DynGC.vel.x > 10.0) &&
		(fabs(dyaw) < 0.1396)) {
		if (fabs(tmpsdpang) > 2.9671) {	    /* 10 degrees */
		    /* behind another car - reduce overall airflow */
                    tdble factor = (fabs(tmpsdpang)-2.9671)/(M_PI-2.9671);
		    tmpas = 1.0 - factor*exp(- 2.0 * DIST(x, y, otherCar->DynGC.pos.x, otherCar->DynGC.pos.y) /
                                             (otherCar->aero.Cd * otherCar->DynGC.vel.x));
		    i_flow = i_flow * tmpas;
		} 
	    }
	}
    }
    //if (index==1) { -- thrown away so that we have different downforce
    // reduction for front and rear parts.
    if (1) {
        // downforce due to body and ground effect.
        tdble alpha = 0.0f;
        tdble vt2b = vt2 * (alpha+(1-alpha)*i_flow);
        vt2b = vt2b * vt2b;
        tdble hm = 1.5 * (car->wheel[0].rideHeight + car->wheel[1].rideHeight + car->wheel[2].rideHeight + car->wheel[3].rideHeight);
        hm = hm*hm;
        hm = hm*hm;
        hm = 1.0 + exp(-3.0*hm);
        car->aero.lift[index] = - car->aero.Clift[index] * vt2b * hm;
        //car->aero.lift[1] = - car->aero.Clift[1] * vt2b *  hm;
        //printf ("%f\n", car->aero.lift[0]+car->aero.lift[1]);
    }


    vt2=vt2*i_flow;
    vt2=vt2*vt2;
	
    aoa += wing->angle;
	
    // the sinus of the angle of attack
    tdble sinaoa = sin(aoa);
    tdble cosaoa = cos(aoa);


    if (car->DynGC.vel.x > 0.0f) {
        switch (car->options->aeroflow_model) {
        case SIMPLE:
            wing->forces.x = wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0f) * sinaoa;
            wing->forces.z = wing->Kz * vt2 * sinaoa;
            break;
        case PLANAR:
            wing->forces.x = wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0f) * sinaoa * sinaoa * sinaoa;
            wing->forces.z = wing->Kz * vt2 * sinaoa * sinaoa * cosaoa;
            break;
        case OPTIMAL:
            wing->forces.x = wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0f) * (1.0f - cosaoa);
            wing->forces.x = wing->Kx * vt2 * (1.0f + (tdble)car->dammage / 10000.0f) * sinaoa;
            break;
	default:
            fprintf (stderr, "Unimplemented option %d for aeroflow model\n", car->options->aeroflow_model);
        }
    } else {
        wing->forces.x = wing->forces.z = 0.0f;
    }
}

tdble PartialFlowRectangle(tdble theta, tdble psi)
{
    if (psi>0)
        return 1.0;
    if (fabs(psi)>fabs(2.0*theta))
        return 0.0;
    return (1.0-(1.0-sin(psi)/sin(2.0*theta)));
}
 
tdble PartialFlowSmooth(tdble theta, tdble psi)
{
    if (psi>0)
        return 1.0;
    if (fabs(psi)>fabs(2.0*theta))
        return 0.0;
    return (0.5*(1.0+tanh((theta-psi)/(fabs(1.0-(psi/theta))-1.0))));
}
 
tdble PartialFlowSphere(tdble theta, tdble psi)
{
    if (psi>0)
        return 1.0;
    if (fabs(psi)>fabs(2.0*theta))
        return 0.0;
    return (0.5*(1.0+sin((theta-psi)*M_PI/(2.0*theta))));
}
           
tdble Max_Cl_given_Cd (tdble Cd)
{
    // if Cd = 1, then all air hitting the surface is stopped.
    // In any case, horizontal speed of air particles is given by
    tdble ux = 1 - Cd;
    
    // We assume no energy lost and thus can calculate the maximum
    // possible vertical speed imparted to the paticles
    tdble uy = sqrt(1 - ux*ux);

    // So now Cl is just the imparted speed
    return uy;
}

tdble Max_SCl_given_Cd (tdble Cd, tdble A)
{
    tdble Cl = Max_Cl_given_Cd (Cd);
    return A * Cl * AIR_DENSITY / 2.0f;
}

tdble MaximumLiftGivenDrag (tdble drag, tdble A)
{
    // We know the drag, C/2 \rho A.
    // We must calculate the drag coefficient
    tdble Cd = (drag / A) * 2.0f / AIR_DENSITY;
    return Max_SCl_given_Cd (Cd, A);
}
