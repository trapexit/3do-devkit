#   File:       DumpStreamCCBs.make
#   Target:     DumpStreamCCBs
#   Sources:    DumpStreamCCBs.c
#   Created:    9/14/93

OBJECTS			= DumpStreamCCBs.c.o
DEBUGOPTIONS	= 
#DEBUGOPTIONS	= -sym full


DumpStreamCCBs ÄÄ DumpStreamCCBs.make {OBJECTS}
	Rez  -o DumpStreamCCBs DumpStreamCCBs.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o DumpStreamCCBs
DumpStreamCCBs.c.o Ä DumpStreamCCBs.make DumpStreamCCBs.c
	 C {DEBUGOPTIONS} -r  DumpStreamCCBs.c
