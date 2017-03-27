Track

Generate raceline: ~/torcs_bin/bin/trackgen -c road -n eroad -r
Generate shading: ~/torcs_bin/bin/accc +shad eroad.ac eroad-shade.ac
Generate acc: ~/torcs_bin/bin/accc -g eroad.acc -l0 eroad.ac -l1 eroad-shade.ac -l2 eroad-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--
Copyright © 2002 Eric Espie, reworked © 2005, 2012 Bernhard Wymann

Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

