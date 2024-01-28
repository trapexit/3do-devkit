#   File:       CtlMaker.make
#   Target:     CtlMaker
#   Sources:    CtlMaker.c
#   Created:    Saturday, July 10, 1993 10:56:34 PM


OBJECTS = CtlMaker.c.o
#DEBUGOPTIONS	= -sym full 
DEBUGOPTIONS = 


CtlMaker ÄÄ CtlMaker.make {OBJECTS}
	Rez  -o CtlMaker CtlMaker.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		#"{Libraries}"ToolLibs.o ¶
		-o CtlMaker
CtlMaker.c.o Ä CtlMaker.make CtlMaker.c
	 C {DEBUGOPTIONS} -r  CtlMaker.c
