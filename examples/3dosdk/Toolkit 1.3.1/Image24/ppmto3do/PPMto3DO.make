#   File:       ppmto3DO.make
#   Target:     ppmto3DO
#   Sources:    ppmto3DO.c
#   Created:    Monday, August 17, 1992 4:57:36 PM


OBJECTS = ppmto3DO.c.o



ppmto3DO ÄÄ ppmto3DO.make {OBJECTS}
	Link -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		#"{Libraries}"ToolLibs.o ¶
		-o ppmto3DO

ppmto3DO.c.o Ä ppmto3DO.make ppmto3DO.c
	 C -warnings off -r  ppmto3DO.c
