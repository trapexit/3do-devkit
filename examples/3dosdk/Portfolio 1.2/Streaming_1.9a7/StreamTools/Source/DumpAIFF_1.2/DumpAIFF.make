#   File:       DumpAIFF.make
#   Target:     DumpAIFF
#   Sources:    DumpAIFF.c


OBJECTS = DumpAIFF.c.o
#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 


DumpAIFF ÄÄ DumpAIFF.make {OBJECTS}
	Rez  -o DumpAIFF DumpAIFF.r -a -ov
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
		-o DumpAIFF
DumpAIFF.c.o Ä DumpAIFF.make DumpAIFF.c
	 C {DEBUG_FLAGS} -r  DumpAIFF.c
