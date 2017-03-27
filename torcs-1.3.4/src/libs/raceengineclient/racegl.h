/***************************************************************************

    file        : racegl.h
    created     : Sat Nov 16 19:02:56 CET 2002
    copyright   : (C) 2002 by Eric Espi�                        
    email       : eric.espie@torcs.org   
    version     : $Id: racegl.h,v 1.3.2.1 2008/12/31 03:53:55 berniw Exp $                                  

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
    		
    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
    @version	$Id: racegl.h,v 1.3.2.1 2008/12/31 03:53:55 berniw Exp $
*/

#ifndef _RACEGL_H_
#define _RACEGL_H_

extern void *ReScreenInit(void);
extern void  ReScreenShutdown(void);
extern void *ReHookInit(void);
extern void ReHookShutdown(void);
extern void ReSetRaceMsg(const char *msg);
extern void ReSetRaceBigMsg(const char *msg);

extern void *ReResScreenInit(void);
extern void ReResScreenSetTitle(char *title);
extern void ReResScreenAddText(char *text);
extern void ReResScreenSetText(const char *text, int line, int clr);
extern void ReResScreenRemoveText(int line);
extern void ReResShowCont(void);
extern int  ReResGetLines(void);
extern void ReResEraseScreen(void);

#endif /* _RACEGL_H_ */ 



