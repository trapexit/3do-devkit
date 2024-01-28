##########################################################################
#   File:       SANMMaker.make
#
#	Contains:	make file for squashsnd
#
#	Copyright © 1993-4 The 3DO Company
#
#   Target:     SANMMaker
#   Sources:    SANMMaker.c, StandardGetFolder.c, SANMSubscriber.h, 
#					rsmsg.c OperaHW.h
#   Created:    Tuesday, June 15, 1993 6:33:20 PM
#
#	History:
#	08/17/94	dtc		Linked with a new set of objectfiles
#						to make it work with MPW 3.4a4.
#	05/31/94	DLD		New today
##########################################################################

#  If you wish to build this tool into it's own directory, then
#		you should comment out the next two lines, instead
#		of the following two lines.

Destination	= :::
COPYTODESTINATION	= Move -y {PROGRAM} "{Destination}"{PROGRAM}
#Destination		= :
#COPYTODESTINATION	= 

PROGRAM				= SANMMaker
OBJECTS = {PROGRAM}.c.o			¶
		  StandardGetFolder.c.o ¶
		  rsmsg.c.o
		  
#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 

CFLAGS	= -r

{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {PROGRAM}.r {OBJECTS}
	Rez  -o {PROGRAM} {PROGRAM}.r -a -ov
	Link -d -c 'SANM' -t APPL ¶
		{DEBUG_FLAGS} ¶
		{OBJECTS} ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}Interface.o" ¶
		#"{Libraries}ToolLibs.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}"IntEnv.o ¶
		-o {PROGRAM}
	{COPYTODESTINATION}

{PROGRAM}.c.o Ä {PROGRAM}.make {PROGRAM}.c
	C  {CFLAGS} {DEBUG_FLAGS} {PROGRAM}.c

StandardGetFolder.c.o Ä  {PROGRAM}.make StandardGetFolder.c StandardGetFolder.h 
	C  {CFLAGS} -mc68881 -elems881 {DEBUG_FLAGS} StandardGetFolder.c

rsmsg.c.o Ä  {PROGRAM}.make rsmsg.c rsmsg.h ImageFile.h OperaHW.h
	C  {CFLAGS} {DEBUG_FLAGS} rsmsg.c

#####################################
#	Include file dependencies
#####################################
{PROGRAM}.c.o		Ä	ImageFile.h OperaHW.h ¶
						SANMSubscriber.h StandardGetFolder.h
