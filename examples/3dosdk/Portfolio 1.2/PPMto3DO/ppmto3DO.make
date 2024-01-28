#   File:       ppmto3do.make
#   Target:     ppmto3do
#   Sources:    ppmto3DO.c
#   Created:    Friday, January 21, 1994 1:28:38 PM


OBJECTS = ppmto3DO.c.o



ppmto3do ÄÄ ppmto3do.make {OBJECTS}
	Link -d -c 'MPS ' -t MPST -sym on -mf ¶
		{OBJECTS} ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		-o ppmto3do
ppmto3DO.c.o Ä ppmto3do.make ppmto3DO.c
	 C -r -sym on   ppmto3DO.c
