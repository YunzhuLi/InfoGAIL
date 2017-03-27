Track

Generate raceline: ~/torcs_bin/bin/trackgen -c road -n g-track-1 -r
Generate shade ac: ~/torcs_bin/bin/accc +shad g-track-1-src.ac g-track-1-shade.ac
Generate acc: ~/torcs_bin/bin/accc -g g-track-1.acc -l0 g-track-1-src.ac -l1 g-track-1-shade.ac -l2 g-track-1-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--
Copyright © 2002 Christophe Guionneau
Copyright © 2005, 2012 Bernhard Wymann

Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

