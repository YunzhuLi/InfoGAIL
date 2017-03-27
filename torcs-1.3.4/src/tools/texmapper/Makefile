##############################################################################
#
#    file                 : Makefile
#    created              : Mon Nov 13 21:48:27 CET 2000
#    copyright            : (C) 2000 by Eric Espie                         
#    email                : torcs@free.fr   
#    version              : $Id: Makefile,v 1.7.2.1 2011/12/28 15:06:07 berniw Exp $                                  
#
##############################################################################
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
##############################################################################


TOOLSDIR	= 

TOOLSCRIPT	= texmapper

PROGRAM   	= texmapper-bin

SOURCES   	= maintexmapper.cpp ssgSaveAC.cpp

SOLIBS    	= -lplibssg -lplibsg -lplibul -ltgf -ltgfclient -lmusicplayer -ltxml -lpng

src_PKGFILES	= $(shell find * -maxdepth 0 -type f -print)

src_PKGDIR	= ${PACKAGE}-${VERSION}/$(subst ${TORCS_BASE},,$(shell pwd))

include ${MAKE_DEFAULT}
