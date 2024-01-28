#   File:       Chunkify.make
#   Target:     Chunkify
#   Sources:    Chunkify.c
#   Created:    June 16, 1993

OBJECTS			= Chunkify.c.o
#DEBUGOPTIONS	= -sym full 
DEBUGOPTIONS	=

Chunkify ÄÄ Chunkify.make {OBJECTS}
	Rez  -o Chunkify Chunkify.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o Chunkify
Chunkify.c.o Ä Chunkify.make Chunkify.c
	 C {DEBUGOPTIONS} -r Chunkify.c
