#   File:       makecel.make
#   Target:     makecel
#   Sources:    makecel.c
#   Created:    Friday, April 24, 1992 11:19:05 AM


OBJECTS = makecel.c.o



makecel ÄÄ makecel.make {OBJECTS}
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
		-o makecel

makecel.c.o Ä makecel.make makecel.c 
	 C -warnings off -r makecel.c
