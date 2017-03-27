1. Generate raceline: ~/torcs_bin/bin/trackgen -c road -n brondehach -r
2. Edit raceline to match hierarchy in brondehach-src.ac (TKMN0, TKMN1)
3. Generate shading: ~/torcs_bin/bin/accc +shad brondehach-src.ac brondehach-shade.ac
4. Edit brondehach-shade.ac to use shadow.png
5. Generate acc: ~/torcs_bin/bin/accc -g brondehach.acc -l0 brondehach-src.ac -l1 brondehach-shade.ac -l2 brondehach-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

Brondehach (Brands Hatch Circuit)
------------------------------------------------------------------------------------
...is largely a conversion to TORCS of a Brands Hatch track, created for
a game called SBK2001 by "Milestone".

His work was released without any license (so far as I can tell) and as such has
been ported to several other games, including SCGT, Racer and VDrift.

With this conversion I have redesigned the track layout to accurately reflect that
of today's Brands Hatch circuit - although I've had to guess at road heights and
cambers based on various descriptions of the track.

As such, it has some challenging off-camber corners, interesting slope changes and
a difficult chicane which sends cars airborne if they approach it too quickly or 
on the wrong line. Nevertheless its quite a fast track that can be very enjoyable
once you've mastered it.

The name Brondehach is Gaelic, meaning both "wooded slope" and "forest entrance".

Copyright (C) 2001, "Milestone"
Copyright (C) 2007, Andrew Sumner
Copyright (C) 2010, Eckhard M. Jäger (texture rework)
Copyright (C) 2012, Bernhard Wymann (fixed graph hierarchy, raceline, z-fighting)


The following applies to the parts of the work created by Andrew Sumner...

Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.
