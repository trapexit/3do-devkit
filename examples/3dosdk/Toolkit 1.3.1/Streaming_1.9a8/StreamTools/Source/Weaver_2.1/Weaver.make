######################################################################################
#	File:			Weaver.make
#
#	Contains:		make rules to build the Weaver stream data merging tool
#
#	Note:			The Weaver is set up to be built with either THINK C or with
#					MPW. When built as an MPW tool, the file WeaverMain.c is not
#					used as it contains glue code to enable the input of command
#					line arguments when running under THINK C. The only purpose
#					for using THINK C is for its symbolic debugger. The released
#					version of the tool is always expected to be an MPW tool.
#
#	Written by:		Joe Buczek
#
#	History:
#	10/20/93	jb		Change to use standard subscriber includes 
#	6/17/93		jb		Add DEBUGOPTIONS for use with symbolic debugger
#	5/18/93		jb		New today.
#
######################################################################################


OBJECTS = Weaver.c.o WeaveStream.c.o

DEBUGOPTIONS	= 
#DEBUGOPTIONS	= -sym full 

COPTIONS		=	-i "::::DataStream:Subscribers:" ¶
					-i "::::DataStream:" ¶
					{DEBUGOPTIONS} 

Weaver ÄÄ Weaver.make Weaver.r {OBJECTS}
	Rez  -o Weaver Weaver.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o Weaver

Weaver.c.o Ä Weaver.make Weaver.c
	 C {COPTIONS} -r  Weaver.c

WeaveStream.c.o Ä Weaver.make WeaveStream.c
	 C {COPTIONS} -r  WeaveStream.c
