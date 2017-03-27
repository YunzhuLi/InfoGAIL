Track

Generate raceline: ~/torcs_bin/bin/trackgen -c road -n g-track-3 -r
Generate shading: ~/torcs_bin/bin/accc +shad g-track-3.ac g-track-3-shade.ac
Generate acc: ~/torcs_bin/bin/accc -g g-track-3.acc -l0 g-track-3.ac -l1 g-track-3-shade.ac -l2 g-track-3-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--
Copyright © 2002 Eric Espie
Copyright © 2012 Bernhard Wymann (raceline, adjustments)

Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

