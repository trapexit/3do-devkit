#   File:       SFtoStream.make
#   Target:     SFtoStream
#   Sources:    SFtoStream.c
#   Created:    Saturday, April 17, 1993 2:33:20 AM


OBJECTS = SFtoStream.c.o
#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 


SFtoStream ÄÄ SFtoStream.make {OBJECTS}
	Rez  -o SFtoStream SFtoStream.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{DEBUG_FLAGS} ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o SFtoStream
SFtoStream.c.o Ä SFtoStream.make SFtoStream.c
	 C {DEBUG_FLAGS} -r  SFtoStream.c
