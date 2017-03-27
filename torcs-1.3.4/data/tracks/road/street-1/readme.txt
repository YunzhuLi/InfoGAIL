Track

Generate raceline: ~/torcs_bin/bin/trackgen -c road -n street-1 -r
Generate shading:  ~/torcs_bin/bin/accc +shad street-1.ac street-1-shade.ac 
Generate acc: ~/torcs_bin/bin/accc -g street-1.acc -l0 street-1.ac -l1 street-1-shade.ac -l2 street-1-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--
Copyright © 2005 Andrew Sumner
Copyright © 2012 Bernhard Wymann (raceline)


Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

