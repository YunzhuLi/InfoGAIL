Track

Generate raceline: ~/torcs_bin/bin/trackgen -c oval -n michigan -r
Generate shading: ~/torcs_bin/bin/accc +shad michigan-src.ac michigan-shade.ac
Generate acc: ~/torcs_bin/bin/accc -g michigan.acc -l0 michigan-src.ac -l1 michigan-shade.ac -l2 michigan-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--

Copyright  2002 Eric Espie
Copyright  2006, 2012 Bernhard Wymann (totally reworked)


Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

