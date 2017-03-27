Track

Generate raceline: ~/torcs_bin/bin/trackgen -n dirt-2 -c dirt -r
Generate shading: ~/torcs_bin/bin/accc +shad dirt-2-shade-src.ac dirt-2-shade.ac (edited shade src)
Generate accc: ~/torcs_bin/bin/accc -g dirt-2.acc -l0 dirt-2-src.ac -l1 dirt-2-shade.ac -l2 dirt-2-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--
Copyright © 2002 Eric Espie
Copyright © 2012 Bernhard Wymann (totally reworked)

Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

