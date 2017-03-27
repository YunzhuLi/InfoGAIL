##############################################################################
#
#    file                 : Makefile
#    created              : Fri Mai 14 10:41:54 CET 2001
#    copyright            : (C) 2010 by Bernhard Wymann
#    email                : berniw@bluewin.ch 
#    version              : $Id: Makefile,v 1.1.2.1 2012/01/27 02:58:23 berniw Exp $
#
##############################################################################
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
##############################################################################


WHEEL     = wheel

DATADIR   = wheels/stock1-1

DATA      = wheel3d.png  readme.txt \
            $(shell find *.acc -maxdepth 0 -type f -print)

data_PKGFILES	= $(DATA) $(shell find *.xcf -maxdepth 0 -type f -print)

data_PKGDIR	= $(DATADIR)

include ${MAKE_DEFAULT}
