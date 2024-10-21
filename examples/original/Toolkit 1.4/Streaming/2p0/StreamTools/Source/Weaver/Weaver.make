######################################################################################
#	File:			Weaver.make
#
#	Contains:		make rules to build the Weaver stream data merging tool
#
#	Copyright © 1993-4 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
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
#	06/06/94	dtc		Defined USE_MAC_IO to allow toggling between standard I/O
#						and MAC_IO. If you don't have to weave more than 15 files, 
#						portability will be maintained by setting USE_MAC_IO to 0.
#	05/31/94	DLD		Fixed some dependency problems
#						for the ToolKit directory structure.
#	05/04/94	dtc		Version 2.2
#						Added the generation of 68020 code.
#	10/20/93	jb		Change to use standard subscriber includes 
#	6/17/93		jb		Add DEBUGOPTIONS for use with symbolic debugger
#	5/18/93		jb		New today.
#
######################################################################################

#  If you wish to build this tool into it's own directory, then
#		you should comment out the next two lines, instead
#		of the following two lines.

Destination	= :::
COPYTODESTINATION	= Move -y {PROGRAM} "{Destination}"{PROGRAM}
#Destination		= :
#COPYTODESTINATION	= 

PROGRAM				= Weaver
OBJECTS = {PROGRAM}.c.o WeaveStream.c.o

DEBUGOPTIONS	= 
# Use the following to generate symbols for symbolic debugging
#DEBUGOPTIONS	= -sym full 

# Use the following if you wish to use Standard I/O 
COPTIONS		= -mc68020 -i "::::DataStream:Subscribers:" -i "::::DataStream:"
 
# Use the following if you wish to use MAC_IO (this allows you to weave more than 20 files)
# COPTIONS		= -mc68020 -d USE_MAC_IO -i "::::DataStream:Subscribers:" -i "::::DataStream:"


{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {PROGRAM}.r {OBJECTS}
	Rez  -o {PROGRAM} {PROGRAM}.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o {PROGRAM}
	{COPYTODESTINATION}

{PROGRAM}.c.o Ä {PROGRAM}.make {PROGRAM}.c
	 C {COPTIONS} {DEBUGOPTIONS} -r  {PROGRAM}.c {PROGRAM}.h

WeaveStream.c.o Ä {PROGRAM}.make WeaveStream.c
	 C {COPTIONS} {DEBUGOPTIONS} -r  WeaveStream.c WeaveStream.h
