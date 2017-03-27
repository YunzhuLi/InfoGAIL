/***************************************************************************

    file                 : collide.cpp
    created              : Sun Mar 19 00:06:19 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: collide.cpp,v 1.26 2007/05/01 14:32:34 olethros Exp $

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
extern tRmInfo ReInfo;

#define CAR_DAMMAGE     0.1

void SimCarCollideAddDeformation(tCar* car, sgVec3 pos, sgVec3 force)
{
    // just compute values, gr does deformation later.
    // must average position and add up force.
    tCollisionState* collision_state = &car->carElt->priv.collision_state;
    collision_state->collision_count++;
    //tdble k = (tdble) cnt;
    if (sgLengthVec3(collision_state->force) < sgLengthVec3(force)) {
        for (int i=0; i<3; i++) {
            collision_state->pos[i] = pos[i];// + k*collision_state->pos[i])/(k+1.0);
            collision_state->force[i] = 0.0001*force[i];// + k*collision_state->force[i])/(k+1.0);
            //printf ("F:%f\n", collision_state->force[i]);
        }
    }
}

void
SimCarCollideZ(tCar *car)
{
    int         i;
    t3Dd        normal;
    t3Dd        rel_normal;
    tdble       dotProd;
    tWheel      *wheel;
    bool upside_down = false;

    if (car->carElt->_state & RM_CAR_STATE_NO_SIMU) {
        return;
    }

    
    t3Dd angles;

    RtTrackSurfaceNormalL(&(car->trkPos), &normal);
    
    angles.x = car->DynGCg.pos.ax;
    angles.y = car->DynGCg.pos.ay;
    angles.z = car->DynGCg.pos.az;
    NaiveRotate (normal, angles, &rel_normal);
    
    if (rel_normal.z > 0) {
        upside_down = false;
        car->upside_down_timer = 0.0f;
    } else {
        upside_down = true;
        car->upside_down_timer += SimDeltaTime;
    }

    // TODO: possibly use this for a better implementation.
    //float corner_reactions[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    for (i = 0; i < 4; i++) {
        wheel = &(car->wheel[i]);
        if ((wheel->state & SIM_SUSP_COMP)
            ||(rel_normal.z < 0)){
            t3Dd angles;
            t3Dd orig;
            t3Dd delta;

            t3Dd corner;

            bool flag = false;
            corner.z = -0.5;
            if (rel_normal.z <= 0) {
                orig.x = car->corner[i].pos.x;
                orig.y = car->corner[i].pos.y;
                orig.z = corner.z;
            } else {
                orig.x = 0.0;
                orig.y = 0.0;
                orig.z = wheel->susp.spring.packers - wheel->rideHeight;
                corner.z = orig.z;//wheel->susp.spring.packers - wheel->rideHeight;
                flag = true;
            }
            angles.x = car->DynGCg.pos.ax;
            angles.y = car->DynGCg.pos.ay;
            angles.z = car->DynGCg.pos.az;
            NaiveInverseRotate (orig, angles, &delta);
            if (((rel_normal.z <=0)
                 && (car->DynGCg.pos.z - delta.z < wheel->zRoad))
                || (rel_normal.z > 0)) {
                        
                //printf ("C[%d] %f (%f)\n", i, rel_normal.z, delta.z);
                if (rel_normal.z <= 0) {
                    delta.z =  -(car->DynGCg.pos.z - delta.z - wheel->zRoad);
                }
                car->DynGCg.pos.z += delta.z;

                RtTrackSurfaceNormalL(&(wheel->trkPos), &normal);
                tdble cvx = (car->DynGCg.vel.x);
                tdble cvy = (car->DynGCg.vel.y);
                tdble cvz = (car->DynGCg.vel.z);
                //cvx = car->corner[i].vel.x;
                //cvy = car->corner[i].vel.y;
                //cvz = car->corner[i].vel.z;

                dotProd = (cvx * normal.x + cvy * normal.y + cvz * normal.z) * wheel->trkPos.seg->surface->kRebound;
                if (rel_normal.z < 0) {
                    dotProd *=1.0;
                }
                if (dotProd < 0) {
                    if (dotProd <-5.0) {
                        car->collision |= 16;
                        wheel->rotational_damage_x -= dotProd*0.01*urandom();
                        wheel->rotational_damage_z -= dotProd*0.01*urandom();
                        wheel->bent_damage_x += 0.1*urandom()-0.05;
                        wheel->bent_damage_z += 0.1*urandom()-0.05;
                        if (wheel->rotational_damage_x>0.25) {
                            wheel->rotational_damage_x = 0.25;
                        }
                        if (wheel->rotational_damage_z>0.25) {
                            wheel->rotational_damage_z = 0.25;
                        }
                        if (car->options->suspension_damage) {
                            SimSuspDamage (&wheel->susp, dotProd+5);
                        }
                        car->collision |= 1;
                    }
                    car->collision |= 8;
                    if (wheel->susp.state & SIM_SUSP_OVERCOMP) {
                        car->collision |= 1;
                    }

                    {
                        t3Dd N;
                                
                        N.x = - normal.x * dotProd;
                        N.y = - normal.y * dotProd;
                        N.z = - normal.z * dotProd;

                        // reaction
                        car->DynGCg.vel.x += N.x;
                        car->DynGCg.vel.y += N.y;
                        car->DynGCg.vel.z += N.z;

                        // friction
                        tdble c;
                        if (rel_normal.z < 0) {
                            c = 0.5;
                        } else {
                            c = 0.25;
                        }
                        car->DynGCg.vel.x = ConstantFriction (car->DynGCg.vel.x, c*dotProd*SIGN(car->DynGCg.vel.x));
                        car->DynGCg.vel.y = ConstantFriction (car->DynGCg.vel.y, c*dotProd*SIGN(car->DynGCg.vel.y));
                        car->DynGCg.vel.z = ConstantFriction (car->DynGCg.vel.z, c*dotProd*SIGN(car->DynGCg.vel.z));
                                
                    }
                        
                    if ((car->carElt->_state & RM_CAR_STATE_FINISH) == 0) {
                        car->dammage += (int)(wheel->trkPos.seg->surface->kDammage * fabs(dotProd) * simDammageFactor[car->carElt->_skillLevel]);
                    }

                    NaiveRotate (normal, angles, &rel_normal);
                    // dotProd = (car->DynGC.vel.x * rel_normal.x + car->DynGC.vel.y * rel_normal.y + car->DynGC.vel.z * rel_normal.z) * wheel->trkPos.seg->surface->kRebound;
                    //          printf ("%f\n", dotProd);
#if 1
                    if (1) {//rel_normal.z <= 0) {
                        t3Dd forces;
                        forces.x = -0.25 * car->mass * dotProd * rel_normal.x;
                        forces.y = -0.25 * car->mass * dotProd * rel_normal.y;
                        forces.z = -0.25 * car->mass * dotProd * rel_normal.z;
                        t3Dd friction;
                        cvx = car->corner[i].vel.x;
                        cvy = car->corner[i].vel.y;
                        cvz = car->corner[i].vel.z;

                        tdble sum_v = sqrt(cvx*cvx + cvy*cvy + cvz*cvz);
                        if (sum_v>1.0) {
                            t3Dd fr_loc;
                            friction.x = cvx/sum_v;
                            friction.y = cvy/sum_v;
                            friction.z = cvz/sum_v;
                            NaiveRotate(friction, angles, &fr_loc);
                            tdble C = 0.5f * dotProd;

                            forces.x += C*fr_loc.x;
                            forces.y += C*fr_loc.y;
                            forces.z += C*fr_loc.z;
                                                        
                            car->DynGC.vel.x += C*fr_loc.x;//*car->mass;
                            car->DynGC.vel.y += C*fr_loc.y;//*car->mass;
                            car->DynGC.vel.z += C*fr_loc.z;//*car->mass;

                        }

                        car->rot_mom[SG_X] -=
                            (forces.z * wheel->relPos.y
                             + forces.y * corner.z);

                        car->rot_mom[SG_Y] +=
                            (forces.z * wheel->relPos.x
                             + forces.x * corner.z);

                        car->rot_mom[SG_Z] -=
                            (-forces.x * wheel->relPos.y +
                             forces.y * wheel->relPos.x);

                        for (int i=0; i<3; i++) {
                            if (fabs(car->rot_mom[i])>2.0*car->mass) {
                                car->rot_mom[i] = 2.0*car->mass * SIGN(car->rot_mom[i]);
                            }
                        }
                        car->rot_mom[SG_X]*=.999;
                        car->rot_mom[SG_Y]*=.999;
                        car->rot_mom[SG_Z]*=.999;
                        car->DynGC.vel.ax = car->DynGCg.vel.ax = -2.0f*car->rot_mom[SG_X] * car->Iinv.x;
                        car->DynGC.vel.ay = car->DynGCg.vel.ay = -2.0f*car->rot_mom[SG_Y] * car->Iinv.y;
                        car->DynGC.vel.az = car->DynGCg.vel.az = -2.0f*car->rot_mom[SG_Z] * car->Iinv.z;
                    }
#endif
                }
            }
        }
    }
}

const tdble BorderFriction = 0.00;

void
SimCarCollideXYScene(tCar *car)
{
    tTrackSeg   *seg = car->trkPos.seg;
    tTrkLocPos  trkpos;
    int         i;
    tDynPt      *corner;
    //t3Dd      normal;
    tdble       initDotProd;
    tdble       dotProd, cx, cy, dotprod2;
    tTrackBarrier *curBarrier;
    tdble       dmg;
    
    if (car->carElt->_state & RM_CAR_STATE_NO_SIMU) {
        return;
    }

    corner = &(car->corner[0]);
    for (i = 0; i < 4; i++, corner++) {
        cx = corner->pos.ax - car->DynGCg.pos.x;
        cy = corner->pos.ay - car->DynGCg.pos.y;
        seg = car->trkPos.seg;
        RtTrackGlobal2Local(seg, corner->pos.ax, corner->pos.ay, &trkpos, TR_LPOS_TRACK);
        seg = trkpos.seg;
        tdble toSide;

        if (trkpos.toRight < 0.0) {
            // collision with right border.
            curBarrier = seg->barrier[TR_SIDE_RGT];
            toSide = trkpos.toRight;
        } else if (trkpos.toLeft < 0.0) {
            // collision with left border.
            curBarrier = seg->barrier[TR_SIDE_LFT];
            toSide = trkpos.toLeft;
        } else {
            continue;
        }

        const tdble& nx = curBarrier->normal.x;
        const tdble& ny = curBarrier->normal.y;
        t3Dd normal = {curBarrier->normal.x,
                       curBarrier->normal.y,
                       0.0f};
        car->DynGCg.pos.x -= nx * toSide;
        car->DynGCg.pos.y -= ny * toSide;

        // Corner position relative to center of gravity.
        cx = corner->pos.ax - car->DynGCg.pos.x;
        cy = corner->pos.ay - car->DynGCg.pos.y;

        car->blocked = 1;
        car->collision |= SEM_COLLISION;

        // Impact speed perpendicular to barrier (of corner).
        initDotProd = nx * corner->vel.x + ny * corner->vel.y;

        // Compute dmgDotProd (base value for later damage) with a heuristic.
        tdble absvel = MAX(1.0, sqrt(car->DynGCg.vel.x*car->DynGCg.vel.x + car->DynGCg.vel.y*car->DynGCg.vel.y));
        tdble GCgnormvel = car->DynGCg.vel.x*nx + car->DynGCg.vel.y*ny;
        tdble cosa = GCgnormvel/absvel;
        tdble dmgDotProd = GCgnormvel*cosa;


        dotProd = initDotProd * curBarrier->surface->kFriction;
        car->DynGCg.vel.x -= nx * dotProd;
        car->DynGCg.vel.y -= ny * dotProd;
        dotprod2 = (nx * cx + ny * cy);
                

        // Angular velocity change caused by friction of collisding car part with wall.
        static tdble VELSCALE = 10.0f;
        //static tdble VELMAX = 6.0f;           
        {

            sgVec3 v = {normal.x, normal.y, normal.z};
            tdble d2 = dotProd / SimDeltaTime;
            sgRotateVecQuat (v, car->posQuat);
            car->DynGC.acc.x -= v[SG_X] * d2;
            car->DynGC.acc.y -= v[SG_Y] * d2;
            car->carElt->_accel_x -= v[SG_X] * d2;
            car->carElt->_accel_y -= v[SG_Y] * d2;          

            car->rot_mom[SG_Z] +=  0.5f*dotprod2 * dotProd / (VELSCALE * car->Iinv.z);
            car->DynGCg.vel.az = car->DynGC.vel.az = -2.0f*car->rot_mom[SG_Z] * car->Iinv.z;
        }

        {
            sgVec3 v = {normal.x, normal.y, normal.z};
            sgRotateVecQuat (v, car->posQuat);
            car->DynGC.acc.x -= v[SG_X] * dotProd/SimDeltaTime;
            car->DynGC.acc.y -= v[SG_Y] * dotProd/SimDeltaTime;
            car->carElt->_accel_x -= v[SG_X] * dotProd/SimDeltaTime;
            car->carElt->_accel_y -= v[SG_Y] * dotProd/SimDeltaTime;
        }
                
        dotprod2 = (nx * cx + ny * cy);
        {
            sgVec3 v = {normal.x, normal.y, normal.z};
            sgRotateVecQuat (v, car->posQuat);
            car->DynGC.acc.x -= v[SG_X] * dotProd/SimDeltaTime;
            car->DynGC.acc.y -= v[SG_Y] * dotProd/SimDeltaTime;
            car->carElt->_accel_x -= v[SG_X] * dotProd/SimDeltaTime;
            car->carElt->_accel_y -= v[SG_Y] * dotProd/SimDeltaTime;
        }


        // Dammage.
        dotProd = initDotProd;
        if (dotProd < 0.0f && (car->carElt->_state & RM_CAR_STATE_FINISH) == 0) {
            dmg = curBarrier->surface->kDammage * fabs(0.5*dmgDotProd*dmgDotProd) * simDammageFactor[car->carElt->_skillLevel];
            car->dammage += (int)dmg;
        } else {
            dmg = 0.0f;
        }

        dotProd *= curBarrier->surface->kRebound;

        // If the car moves toward the barrier, rebound.
        if (dotProd < 0.0f) {
            car->collision |= SEM_COLLISION_XYSCENE;
            car->normal.x = nx * dmg;
            car->normal.y = ny * dmg;
            car->collpos.x = corner->pos.ax;
            car->collpos.y = corner->pos.ay;
            car->DynGCg.vel.x -= nx * dotProd;
            car->DynGCg.vel.y -= ny * dotProd;
        }
#if 0
        static tdble DEFORMATION_THRESHOLD = 0.01f;
        if (car->options->aero_damage
            || sgLengthVec3(force) > DEFORMATION_THRESHOLD) {
            sgVec3 poc;
            poc[0] = corner->pos.x;
            poc[1] = corner->pos.y;
            poc[2] = (urandom()-0.5)*2.0;
            sgRotateVecQuat (force, car->posQuat);
            sgNormaliseVec3(force);
            for (int i=0; i<3; i++) {
                force[i]*=dmg;
            }
            // just compute values, gr does deformation later.
            // must average position and add up force.
            SimCarCollideAddDeformation(car, poc, force);

            // add aero damage if applicable
            if (car->options->aero_damage) {
                SimAeroDamage (car, poc, sgLengthVec3(force));
            }
        }
#endif
    }

}



static void SimCarCollideResponse(void * /*dummy*/, DtObjectRef obj1, DtObjectRef obj2, const DtCollData *collData)
{
    sgVec2 n;               // Collision normal delivered by solid: Global(point1) - Global(point2)
    tCar *car[2];   // The cars.
    sgVec2 p[2];    // Collision points delivered by solid, in body local coordinates.
    sgVec2 r[2];    // Collision point relative to center of gravity.
    sgVec2 vp[2];   // Speed of collision point in world coordinate system.
    sgVec3 pt[2];   // Collision points in global coordinates.

    int i;

    car[0] = (tCar*)obj1;
    car[1] = (tCar*)obj2;

    if ((car[0]->carElt->_state & RM_CAR_STATE_NO_SIMU) ||
        (car[1]->carElt->_state & RM_CAR_STATE_NO_SIMU))
        {
            return;
        }

    if (car[0]->carElt->index < car[1]->carElt->index) {
        // vector conversion from double to float.
        p[0][0] = (float)collData->point1[0];
        p[0][1] = (float)collData->point1[1];
        p[1][0] = (float)collData->point2[0];
        p[1][1] = (float)collData->point2[1];
        n[0]  = (float)collData->normal[0];
        n[1]  = (float)collData->normal[1];
    } else {
        // swap the cars (not the same for the simu).
        car[0] = (tCar*)obj2;
        car[1] = (tCar*)obj1;
        p[0][0] = (float)collData->point2[0];
        p[0][1] = (float)collData->point2[1];
        p[1][0] = (float)collData->point1[0];
        p[1][1] = (float)collData->point1[1];
        n[0]  = -(float)collData->normal[0];
        n[1]  = -(float)collData->normal[1];
    }

    // TODO: find out if that (the isnan tests and length test) is still needed. If yes, add
    // it as well in the wall collision code.
    // HYPOTHESIS: Perhaps this code was needed before dtProcees has been guarded by dtTest?
    // Remember dtProceed just works correct if all objects are disjoint.
    // I will let people test this wihout the guards, see what happens...
    /*
      if ((isnan(p[0][0]) ||
      isnan(p[0][1]) ||
      isnan(p[0][0]) ||
      isnan(p[0][1]) ||
      isnan(n[0])  ||
      isnan(n[1]))) {
      // I really don't know where the problem is...
      GfOut ("Collide failed 1 (%s - %s)\n", car[0]->carElt->_name, car[1]->carElt->_name);
      return;
      }

      if (sgLengthVec2 (n) == 0.0f) {
      // I really don't know where the problem is...
      GfOut ("Collide failed 2 (%s - %s)\n", car[0]->carElt->_name, car[1]->carElt->_name);
      return;
      }
    */
    sgNormaliseVec2(n);

    sgVec2 rg[2];   // raduis oriented in global coordinates, still relative to CG (rotated aroung CG).
    tCarElt *carElt;

    for (i = 0; i < 2; i++) {
        // vector GP (Center of gravity to collision point). p1 and p2 are delivered from solid as
        // points in the car coordinate system.
        sgSubVec2(r[i], p[i], (const float*)&(car[i]->statGC));

        // Speed of collision points, linear motion of center of gravity (CG) plus rotational
        // motion around the CG.
        carElt = car[i]->carElt;
        float sina = sin(carElt->_yaw);
        float cosa = cos(carElt->_yaw);
        rg[i][0] = r[i][0]*cosa - r[i][1]*sina;
        rg[i][1] = r[i][0]*sina + r[i][1]*cosa;

        vp[i][0] = car[i]->DynGCg.vel.x - car[i]->DynGCg.vel.az * rg[i][1];
        vp[i][1] = car[i]->DynGCg.vel.y + car[i]->DynGCg.vel.az * rg[i][0];
    }

    // Relative speed of collision points.
    sgVec2 v1ab;
    sgSubVec2(v1ab, vp[0], vp[1]);

    // try to separate the cars. The computation is necessary because dtProceed is not called till
    // the collision is resolved. 
    for (i = 0; i < 2; i++) {
        sgCopyVec2(pt[i], r[i]);
        pt[i][2] = 0.0f;
        // Transform points relative to cars local coordinate system into global coordinates.
        sgFullXformPnt3(pt[i], car[i]->carElt->_posMat);
    }

    // Compute distance of collision points.
    sgVec3 pab;
    sgSubVec2(pab, pt[1], pt[0]);
    float distpab = sgLengthVec2(pab);

    sgVec2 tmpv;
    /*
      static const float CAR_MIN_MOVEMENT = 0.02f;
      static const float CAR_MAX_MOVEMENT = 0.05f;
      sgScaleVec2(tmpv, n, MIN(MAX(distpab, CAR_MIN_MOVEMENT), CAR_MAX_MOVEMENT));
    */
    sgScaleVec2(tmpv, n, MIN(distpab, 0.05));
    if (car[0]->blocked == 0) {
        sgAddVec2((float*)&(car[0]->DynGCg.pos), tmpv);
        car[0]->blocked = 1;
    }
    if (car[1]->blocked == 0) {
        sgSubVec2((float*)&(car[1]->DynGCg.pos), tmpv);
        car[1]->blocked = 1;
    }

    // Doing no dammage and correction if the cars are moving out of each other.
    if (sgScalarProductVec2(v1ab, n) > 0) {
        return;
    }

    // impulse.
    float rpn[2];
    rpn[0] = sgScalarProductVec2(rg[0], n);
    rpn[1] = sgScalarProductVec2(rg[1], n);

    // Pesudo cross product to find out if we are left or right.
    // TODO: SIGN, scrap value?
    float rpsign[2];
    rpsign[0] =  n[0]*rg[0][1] - n[1]*rg[0][0];
    rpsign[1] = -n[0]*rg[1][1] + n[1]*rg[1][0];

    const float e = 1.0f;   // energy restitution

    float j = -(1.0f + e) * sgScalarProductVec2(v1ab, n) /
        ((car[0]->Minv + car[1]->Minv) +
         rpn[0] * rpn[0] * car[0]->Iinv.z + rpn[1] * rpn[1] * car[1]->Iinv.z);

    // TODO: check that, eventually remove assert... should not go to production, IMHO.
    //assert (!isnan(j));

    for (i = 0; i < 2; i++) {
        // Damage.
        tdble damFactor, atmp;
        atmp = atan2(r[i][1], r[i][0]);
        if (fabs(atmp) < (PI / 3.0)) {
            // Front collision gives more damage.
            damFactor = 1.5f;
        } else {
            // Rear collision gives less damage.
            damFactor = 1.0f;
        }

        if ((car[i]->carElt->_state & RM_CAR_STATE_FINISH) == 0) {
            car[i]->dammage += (int)(CAR_DAMMAGE * fabs(j) * damFactor * simDammageFactor[car[i]->carElt->_skillLevel]);
        }

        // Compute collision velocity.
        const float ROT_K = 1.0f;

        float js = (i == 0) ? j : -j;
        sgScaleVec2(tmpv, n, js * car[i]->Minv);
        sgVec2 v2a;

        if (car[i]->collision & SEM_COLLISION_CAR) {
            sgAddVec2(v2a, (const float*)&(car[i]->VelColl.x), tmpv);
            car[i]->VelColl.az = car[i]->VelColl.az + js * rpsign[i] * rpn[i] * car[i]->Iinv.z * ROT_K;
        } else {
            sgAddVec2(v2a, (const float*)&(car[i]->DynGCg.vel), tmpv);
            car[i]->VelColl.az = car[i]->DynGCg.vel.az + js * rpsign[i] * rpn[i] * car[i]->Iinv.z * ROT_K;
        }

        static float VELMAX = 3.0f;
        if (fabs(car[i]->VelColl.az) > VELMAX) {
            car[i]->VelColl.az = SIGN(car[i]->VelColl.az) * VELMAX;
        }

        sgCopyVec2((float*)&(car[i]->VelColl.x), v2a);

        // Move the car for the collision lib.
        tCarElt *carElt = car[i]->carElt;
        sgMakeCoordMat4(carElt->pub.posMat, car[i]->DynGCg.pos.x, car[i]->DynGCg.pos.y,
                        car[i]->DynGCg.pos.z - carElt->_statGC_z, RAD2DEG(carElt->_yaw),
                        RAD2DEG(carElt->_roll), RAD2DEG(carElt->_pitch));
        dtSelectObject(car[i]);
        dtLoadIdentity();
        dtTranslate(-carElt->_statGC_x, -carElt->_statGC_y, 0.0f);
        dtMultMatrixf((const float *)(carElt->_posMat));

        car[i]->collision |= SEM_COLLISION_CAR;
    }
}


void
SimCarCollideShutdown(int nbcars)
{
    int  i;
    
    for (i = 0; i < nbcars; i++) {
        dtDeleteShape(SimCarTable[i].shape);
        dtDeleteObject(&(SimCarTable[i]));
    }
    dtClearDefaultResponse();
}


void 
SimCarCollideConfig(tCar *car)
{
    tCarElt *carElt;
    
    carElt = car->carElt;
    // The current car shape is a box...
    car->shape = dtBox(carElt->_dimension_x, carElt->_dimension_y, carElt->_dimension_z);
    dtCreateObject(car, car->shape);

    car->collisionAware = 1;
}

void
SimCarCollideInit(void)
{
    dtSetDefaultResponse(SimCarCollideResponse, DT_SMART_RESPONSE, NULL);
    dtDisableCaching();
    dtSetTolerance(0.001);
}


#if 0
void
SimCarCollideCars(tSituation *s)
{
    tCar        *car;
    tCarElt     *carElt;
    int         i;
    
    for (i = 0; i < s->_ncars; i++) {
        carElt = s->cars[i];
        car = &(SimCarTable[carElt->index]);
        dtSelectObject(car);
        dtLoadIdentity();
        dtTranslate(-carElt->_statGC_x, -carElt->_statGC_y, 0);
        dtMultMatrixf((const float *)(carElt->_posMat));
        memset(&(car->VelColl), 0, sizeof(tPosd));
    }

    if (dtTest() == 0) {
        dtProceed();
    }

    for (i = 0; i < s->_ncars; i++) {
        carElt = s->cars[i];
        if (carElt->_state & RM_CAR_STATE_NO_SIMU) {
            continue;
        }
        car = &(SimCarTable[carElt->index]);
        if (car->collision & 4) {
            car->DynGCg.vel.x = car->VelColl.x;
            car->DynGCg.vel.y = car->VelColl.y;
            //tdble delta = car->VelColl.az - car->DynGCg.vel.az;
            //car->DynGCg.vel.az = car->VelColl.az;
            car->rot_mom[SG_Z] = -car->VelColl.az/car->Iinv.z;
            car->DynGC.vel.az = car->DynGCg.vel.az = -2.0f*car->rot_mom[SG_Z] * car->Iinv.z;
            //car->rot_mom[SG_Z] -= delta/car->Iinv.z;
        }
    }
}
#else
void
SimCarCollideCars(tSituation *s)
{
    tCar *car;
    tCarElt *carElt;
    int i;

    for (i = 0; i < s->_ncars; i++) {
        carElt = s->cars[i];
        if (carElt->_state & RM_CAR_STATE_NO_SIMU) {
            continue;
        }

        car = &(SimCarTable[carElt->index]);
        dtSelectObject(car);
        // Fit the bounding box around the car, statGC's are the static offsets.
        dtLoadIdentity();
        dtTranslate(-carElt->_statGC_x, -carElt->_statGC_y, 0.0f);
        // Place the bounding box such that it fits the car in the world.
        dtMultMatrixf((const float *)(carElt->_posMat));
        memset(&(car->VelColl), 0, sizeof(tPosd));
    }

    // Running the collision detection. If no collision is detected, call dtProceed.
    // dtProceed just works if all objects are disjoint.
    if (dtTest() == 0) {
        dtProceed();
    }

    for (i = 0; i < s->_ncars; i++) {
        carElt = s->cars[i];
        if (carElt->_state & RM_CAR_STATE_NO_SIMU) {
            continue;
        }
        car = &(SimCarTable[carElt->index]);
        if (car->collision & SEM_COLLISION_CAR) {
            car->DynGCg.vel.x = car->VelColl.x;
            car->DynGCg.vel.y = car->VelColl.y;
            car->rot_mom[SG_Z] = -car->VelColl.az/car->Iinv.z;
            car->DynGC.vel.az = car->DynGCg.vel.az = -2.0f*car->rot_mom[SG_Z] * car->Iinv.z;
        }
    }
}

#endif
extern tdble ConstantFriction (tdble u, tdble du)
{
    tdble u2 = u + du;
    if (u*du > 0.0) {
        return u;
    }
    if (u2*u <= 0.0)
        return 0.0;
    return u2;
}
extern tdble ConstantFriction (tdble u, tdble a, tdble dt)
{
    return ConstantFriction (u, a*dt);
}
