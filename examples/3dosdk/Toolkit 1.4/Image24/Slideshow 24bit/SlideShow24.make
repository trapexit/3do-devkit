#
#	File:		SlideShow24.make
#
#	Contains:	make file for SlideShow24
#
#	Written by:	Joe Buczek
#
#	Copyright:	© 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <11>	 8/19/93	dsm		added support for 24 bit images
#		 <10>	 8/19/93	ec		add modbin for stack space and debug
#		 <9>	 8/11/93	JAY		add -workspace 3000 to link line
#		 <8>	 6/23/93	JAY		merge ec's Dragon changes (4B1)
#		 <7>	 6/23/93	JAY		add check stack option (zps0) to COptions. Needed for Dragon
#									Release (4B1)
#		 <6>	  4/7/93	JAY		make the make consistant
#		 <5>	  4/5/93	JAY		add stripAIF to link actions
#		 <4>	 3/31/93	JAY		changing ordering of link line modules
#		 <3>	 3/31/93	JAY		add input.lib to link line for 3B1
#		 <2>	 3/18/93	JAY		change Program to Application to avoid make errors
#		 <1>	 3/18/93	JAY		first checked in
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	SlideShow24
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
CDebugOptions	= -g
#CDebugOptions	=			# turn off symbolic information
COptions		= {CDebugOptions} -zps0 -za1 -i "{3DOIncludes}" ¶
					-d DEBUG={DebugFlag} ¶
					-d GREEN_HARDWARE=1 ¶
					-d RED_HARDWARE=0 ¶
					-d kBuildDate="¶"`date -a`¶""

SOptions		= -bi -g -i "{3DOIncludes}"

LDebugOptions	= -d		# turn on symbolic information
LOptions		= -aif -r -b 0x00 {LDebugOptions}


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}{Application}.c.o"	¶
					"{ObjectDir}loadfile24.c.o"	¶
					"{ObjectDir}ourVDL.c.o"

#####################################
#	Default build rules
#####################################
All				Ä	{Application}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {DepDir}{Default}.c {COptions} -o {TargDir}{Default}.c.o 

.s.o			Ä	.s
	{ASM} {DepDir}{Default}.s {SOptions} -o {TargDir}{Default}.s.o


#####################################
#	Target build rules
#####################################
{Application}		ÄÄ	{Application}.make {LIBS} {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {Application}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	stripaif {Application} -o {Application}
	modbin {Application} -stack 3000 -debug
	duplicate -y {Application} "{3DORemote}"
	duplicate -y {Application}.sym "{3DORemote}"

#####################################
#	Include file dependencies
#####################################
#{Application}.c		Ä	{Application}.h 

{ObjectDir}loadfile24.c.o	Ä loadfile24.c loadfile24.h
{ObjectDir}ourVDL.c.o	Ä ourVDL.c ourVDL.h
{Application}.c	Ä {Application}.h ¶
				"{3DOIncludes}Utils3DO.h" ¶
				"{3DOIncludes}Init3DO.h" ¶
				"{3DOIncludes}Parse3DO.h"¶
				ourVDL.h
