#   File:       DumpAIFF.make
#   Target:     DumpAIFF
#   Sources:    DumpAIFF.c
#
#	Copyright © 1993-4 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	09/08/94	dtc		Added PROGRAM and COPYTODESTINATION.
#	08/17/94	dtc		Linked with a new set of objectfiles
#						to make it work with MPW 3.4a4.
#	05/27/94	DLD		Fixed some dependency problems
#						for the ToolKit directory structure.


#  If you wish to build this tool into it's own directory, then
#		you should comment out the next two lines, instead
#		of the following two lines.

PROGRAM			= DumpAIFF
Destination	= :::
COPYTODESTINATION	= Move -y {PROGRAM} "{Destination}"{PROGRAM}
#Destination		= :
#COPYTODESTINATION	= 

OBJECTS = {PROGRAM}.c.o
#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 


{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {PROGRAM}.r {OBJECTS}
	Rez  -o  {PROGRAM}  {PROGRAM}.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{DEBUG_FLAGS} ¶
		{OBJECTS} ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}Interface.o" ¶
		"{Libraries}ToolLibs.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}"IntEnv.o ¶
		-o {PROGRAM}
	{COPYTODESTINATION}

{PROGRAM}.c.o Ä {PROGRAM}.make {PROGRAM}.c
	 C {DEBUG_FLAGS} -r   {PROGRAM}.c
