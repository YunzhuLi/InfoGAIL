Generate raceline: ~/torcs_bin/bin/trackgen -c road -n ruudskogen -r
Generate shading: ~/torcs_bin/bin/accc +shad ruudskogen.ac ruudskogen-shade.ac
Generate acc: ~/torcs_bin/bin/accc -g ruudskogen.acc -l0 ruudskogen.ac -l1 ruudskogen-shade.ac -l2 ruudskogen-trk-raceline.ac -d3 1000 -d2 500 -d1 300 -S 300 -es

---
Copyright © 2003 Tor Arne Hustvedt
Converted to Torcs by Andrew Sumner.
Copyright © 2012 Bernhard Wymann (raceline)

Copyleft: this work of art is free, you can redistribute
it and/or modify it according to terms of the Free Art license.
You will find a specimen of this license on the site
Copyleft Attitude http://artlibre.org as well as on other sites.

