/***************************************************************************

    file                 : aero.h
    created              : Sun Mar 19 00:04:59 CET 2000
    copyright            : (C) 2000 by Eric Espie
    email                : torcs@free.fr
    version              : $Id: aero.h,v 1.19 2005/12/15 02:21:37 olethros Exp $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _AERO_H_
#define _AERO_H_

/// air density
#define AIR_DENSITY 1.23
typedef struct
{
    /* dynamic */
    tdble	drag;		/* drag force along car x axis */
    tdble	lift[2];	/* front & rear lift force along car z axis */
    tdble   lateral_drag; /* drag force along car y axis */
    tdble   vertical_drag; /* drag force along car z axis */
	tdble   Mx, My, Mz; /* torques (only with aero damage) */
	sgVec3   rot_front;
	sgVec3   rot_lateral; 
	sgVec3   rot_vertical;
    /* static */
    tdble	SCx2;
    tdble	Clift[2];	/* front & rear lift due to body not wings */
    tdble	Cd;		/* for aspiration */
} tAero;


typedef struct
{
    /* dynamic */
    t3Dd	forces;
    tdble	Kx;
    tdble	Kz;
	tdble	angle;
    tdble efficiency;
    /* static */
    t3Dd	staticPos;
    
} tWing;

/// Get the maximum possible lift coefficient given a drag coefficient
tdble Max_Cl_given_Cd (tdble Cd);

/// Get the maximum possible lift given a drag coefficient and area
tdble Max_Cl_given_Cd (tdble Cd, tdble A);

/** Get the maximum lift given drag.
 * 
 * The equation
 *
 * \f[
 *    F = C/2 \rho u^2 A
 * \f]
 *
 * can be used to calculate \f$F\f$, the exerted force on an object
 * with cross-sectional area \f$A\f$, moving at a speed \f$u\f$
 * through a fluid of density \f$\rho\f$.
 * 
 * For a plane perpendicular to the direction of motion, \f$C=1\$f.
 * In fact, we can seperate it into two components, \f$C_x=1, ~
 * C_y=0\f$, if we wish.
 * 
 * The next part is simple.  Given a drag, we can calculate a maximum lift
 * if we know the cross-sectional area involved.
 *
 * \arg \c drag
 */
tdble MaximumLiftGivenDrag (tdble drag, tdble A = 1.0);


#endif /* _AERO_H_  */ 



