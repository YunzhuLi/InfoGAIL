
/* Dump to disk in '.rgb' format */

void ssgaScreenDump ( char *filename,
                      int width, int height,
                      int frontBuffer = TRUE ) ;

/* Put low order 24 bits to disk (R=lsb, G=middle byte, B=msb) */

void ssgaScreenDepthDump ( char *filename,
                      int width, int height,
                      int frontBuffer = TRUE ) ;

/* Dump to a memory buffer - three bytes per pixel */

unsigned char *ssgaScreenDump ( int width, int height,
                                int frontBuffer = TRUE ) ;

unsigned int *ssgaScreenDepthDump ( int width, int height,
                                    int frontBuffer = TRUE ) ;

