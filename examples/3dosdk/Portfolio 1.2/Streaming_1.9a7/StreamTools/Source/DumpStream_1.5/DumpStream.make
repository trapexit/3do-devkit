#   File:       DumpStream.make
#   Target:     DumpStream
#   Sources:    DumpStream.c
#   Created:    Monday, May 17, 1993 11:35:23 PM


OBJECTS = DumpStream.c.o
#DEBUGOPTIONS = -sym full
DEBUGOPTIONS = 


DumpStream �� DumpStream.make {OBJECTS}
	Rez  -o DumpStream DumpStream.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST �
		{OBJECTS} �
		#"{CLibraries}"CSANELib.o �
		#"{CLibraries}"Math.o �
		#"{CLibraries}"Complex.o �
		"{CLibraries}"StdClib.o �
		"{Libraries}"Stubs.o �
		"{Libraries}"Runtime.o �
		"{Libraries}"Interface.o �
		"{Libraries}"ToolLibs.o �
		-o DumpStream
DumpStream.c.o � DumpStream.make DumpStream.c
	 C {DEBUGOPTIONS} -r  DumpStream.c
