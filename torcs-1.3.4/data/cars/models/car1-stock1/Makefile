##############################################################################
#
#    file                 : Makefile
#    created              : Wed Feb 1 14:31:06 CET 2012
#    copyright            : (C) 2012 by Bernhard Wymann                         
#    email                : berniw@bluewin.ch
#    version              : $Id: Makefile,v 1.1.2.2 2012/02/17 07:53:22 berniw Exp $                                  
#
##############################################################################
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
##############################################################################

CAR       = car1-stock1

DATADIR   = cars/$(CAR)

DATA      = $(CAR).xml readme.txt \
            $(CAR).rgb tex-wheel.rgb shadow.rgb driver.rgb carbon-128.rgb grill.rgb\
            $(shell find *.acc -maxdepth 0 -type f -print)

data_PKGFILES	= $(DATA) $(shell find *.xcf -maxdepth 0 -type f -print)

data_PKGDIR	= $(DATADIR)

include ${MAKE_DEFAULT}


