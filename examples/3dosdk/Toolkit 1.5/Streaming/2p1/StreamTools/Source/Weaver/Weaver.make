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
#	07/20/94	dtc		Fixed :
#							-dFORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY=1
#							-dUSE_BIGGER_BUFFERS=1
#							-dUSE_MAC_IO=1
#						to:
#							-d FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY
#							-d USE_BIGGER_BUFFERS
#							-d USE_MAC_IO
#	06/09/94	dtc		Added the following compile switches to Weaver.make:
#							FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY
#							USE_BIGGER_BUFFERS
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

# The following defines the various compile switches
#
# FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY compile switch determines whether or
# not the beginning of the stream data will be forced onto a stream block boundary.
# Setting this switch results in wasting space in the stream file, but may be useful
# if the application does a considerable amount of stream looping, whereby the header
# and marker table data, if present, could cause performance problems. It is
# recommended that this be left OFF.
#
# USE_MAC_IO compile switch determines whether or not to allow unlimited number of
# input streams through USE_MAC_IO. If you don't have to weave more than 15 files, 
# portability will be maintained by setting USE_MAC_IO to 0.
#
# USE_BIGGER_BUFFERS compile switch determines whether or not larger I/O buffers are used
# for stream I/O. This SIGNIFICANTLY improves performance, and makes an ANSI compliant
# library call to set the buffer size. It is recommended that this option be left ON.
#
# Use the following if you wish to use Standard I/O 
SWITCHOPTIONS	=	-d FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY -d USE_BIGGER_BUFFERS

# Use the following if you wish to use MAC_IO (this allows you to weave more than 20 files)
#SWITCHOPTIONS	= -d FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY -d USE_MAC_IO -d USE_BIGGER_BUFFERS
 
COPTIONS		= -mc68020 {SWITCHOPTIONS} -i "::::DataStream:Subscribers:Includes:" -i "::::DataStream:"


{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {PROGRAM}.r {OBJECTS}
	Rez  -o {PROGRAM} {PROGRAM}.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{Libraries}Interface.o" ¶
		"{Libraries}ToolLibs.o" ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}"IntEnv.o ¶
		-o {PROGRAM}
	{COPYTODESTINATION}

{PROGRAM}.c.o Ä {PROGRAM}.make {PROGRAM}.c
	 C {COPTIONS} {DEBUGOPTIONS} -r  {PROGRAM}.c {PROGRAM}.h

WeaveStream.c.o Ä {PROGRAM}.make WeaveStream.c
	 C {COPTIONS} {DEBUGOPTIONS} -r  WeaveStream.c WeaveStream.h
