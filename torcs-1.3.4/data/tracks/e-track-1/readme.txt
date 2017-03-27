Track

Generate raceline: ~/torcs_bin/bin/trackgen -c road -n e-track-1 -r
Generate shading: ~/torcs_bin/bin/accc +shad e-track-1.ac e-track-1-shade.ac | grep unknown
Generate acc: ~/torcs_bin/bin/accc -g e-track-1.acc -l0 e-track-1.ac -l1 e-track-1-shade.ac -l2 e-track-1-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--
Copyright © 2002 Eric Espie,© 2005, 2012 Bernhard Wymann
  
Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

