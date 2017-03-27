/***************************************************************************

    file                 : util.cpp
    created              : Wed May 29 22:20:24 CEST 2002
    copyright            : (C) 2001 by Eric Espie
    email                : eric.espie@torcs.org
    version              : $Id: util.cpp,v 1.3.2.4 2012/02/09 22:36:25 berniw Exp $

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
    @version	$Id: util.cpp,v 1.3.2.4 2012/02/09 22:36:25 berniw Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <plib/ul.h>
#include <tgfclient.h>
#include <portability.h>

#include "util.h"

int
GetFilename(const char *filename, const char *filepath, char *buf, const int BUFSIZE)
{
	const char	*c1, *c2;
	int		found = 0;
	int		lg;
	int flen = strlen(filename);
	
	if (filepath) {
		c1 = filepath;
		c2 = c1;
		while ((!found) && (c2 != NULL)) {
			c2 = strchr(c1, ';');
			if (c2 == NULL) {
				snprintf(buf, BUFSIZE, "%s/%s", c1, filename);
			} else {
				lg = c2 - c1;
				if (lg + flen + 2 < BUFSIZE) {
					strncpy(buf, c1, lg);
					buf[lg] = '/';
					strcpy(buf + lg + 1, filename);
				} else {
					buf[0] = '\0';
				}
			}

			if (ulFileExists(buf)) {
				found = 1;
			}
			c1 = c2 + 1;
		}
	} else {
		strncpy(buf, filename, BUFSIZE);
		if (ulFileExists(buf)) {
			found = 1;
		}
	}

	if (!found) {
		printf("File %s not found\n", filename);
		printf("File Path was %s\n", filepath);
		return 0;
	}
	
	return 1;
}



float
getHOT(ssgRoot *root, float x, float y)
{
  sgVec3 test_vec;
  sgMat4 invmat;
  sgMakeIdentMat4(invmat);

  invmat[3][0] = -x;
  invmat[3][1] = -y;
  invmat[3][2] =  0.0f         ;

  test_vec [0] = 0.0f;
  test_vec [1] = 0.0f;
  test_vec [2] = 100000.0f;

  ssgHit *results;
  int num_hits = ssgHOT (root, test_vec, invmat, &results);

  float hot = -1000000.0f;

  for (int i = 0; i < num_hits; i++)
  {
    ssgHit *h = &results[i];

    float hgt = - h->plane[3] / h->plane[2];

    if (hgt >= hot)
      hot = hgt;
  }

  return hot;
}
