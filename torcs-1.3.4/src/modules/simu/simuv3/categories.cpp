/***************************************************************************

    file        : categories.cpp
    created     : Sun Dec 15 11:12:56 CET 2002
    copyright   : (C) 2002 by Eric Espié                        
    email       : eric.espie@torcs.org   
    version     : $Id: categories.cpp,v 1.19 2005/04/04 14:56:21 olethros Exp $                                  

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
    @version	$Id: categories.cpp,v 1.19 2005/04/04 14:56:21 olethros Exp $
*/

#include <stdio.h>
#include "sim.h"


tdble simDammageFactor[] = {0.0, 0.5, 0.8, 1.0, 1.0};

tdble simSkidFactor[] = {0.40, 0.35, 0.3, 0.0, 0.0};
