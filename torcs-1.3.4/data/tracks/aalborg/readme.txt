Track

Generate raceline: ~/torcs_bin/bin/trackgen -c road -n aalborg -r
Generate shading: ~/torcs_bin/bin/accc +shad aalborg.ac aalborg-shade.ac
Generate acc: ~/torcs_bin/bin/accc -g aalborg.acc -l0 aalborg.ac -l1 aalborg-shade.ac -l2 aalborg-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

--
Copyright © 2002 Torben Thellefsen, Eric Espie
Copyright © 2005, 2012 Bernhard Wymann

Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

