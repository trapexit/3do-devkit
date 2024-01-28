#   File:       DumpMF.make
#   Target:     DumpMF
#   Sources:    DumpMF.c MFToText.c ParseMF.c
#
#	10/25/94	ec		move tool to 'apps & data:dumpMF:'.
#	10/11/94	ec		move tool to 'apps & data'.
#						changed to use :Objects: folder
#	9/16/94		rdg		Link with ToolLibs.o for SpinCursor.
#	8/31/94		crm		Replaced Runtime.o with MacRuntime.o and IntEnv.o

#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 
Program			=	DumpMF
AppsDir			=	::'Apps & Data':{Program}:
ObjectDir		=	:Objects:

OBJECTS			= {ObjectDir}{Program}.c.o  {ObjectDir}MFToText.c.o {ObjectDir}ParseMF.c.o

{Program} ÄÄ {Program}.make {OBJECTS}
	Rez  -o {Program} {Program}.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{DEBUG_FLAGS} ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"MacRuntime.o ¶
		"{Libraries}"IntEnv.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o {Program}
	if not `exists {AppsDir}`
		newFolder {AppsDir}
	end
	move -y {Program} {AppsDir}{Program}
	if `exists "{Program}.sym"`
		move -y {Program}.sym {AppsDir}{Program}.sym
	end

{ObjectDir}{Program}.c.o 		Ä		{Program}.make {Program}.c
	 C {DEBUG_FLAGS} -r  {Program}.c -o {ObjectDir}{Program}.c.o

{ObjectDir}MFToText.c.o 	Ä		{Program}.make MFToText.c
	 C {DEBUG_FLAGS} -r  MFToText.c -o {ObjectDir}MFToText.c.o

{ObjectDir}ParseMF.c.o 	Ä		{Program}.make ParseMF.c
	 C {DEBUG_FLAGS} -r  ParseMF.c -o {ObjectDir}ParseMF.c.o
