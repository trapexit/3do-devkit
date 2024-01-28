#
#	File:		Example2.make
#
#	Contains:	make file for building Example2
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
#		 <7>	 8/15/93	JAY		add duplication command to build
#		<5+>	 8/10/93	JAY		add modbin to build in order to specify stack size
#		 <5>	 6/27/93	dsm		Add -workspace 4096
#		 <4>	 6/23/93	JAY		add stack checking option (-zps0) to COptions. Needed for Dragon
#									Release (4B1)
#		 <3>	  4/7/93	JAY		make the make consistant
#		 <2>	 3/31/93	JAY		added input.lib to link line for 3B1
#		 <1>	 3/18/93	JAY		first checked in
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	Example2
DebugFlag		=	0
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
CDebugOptions	= -g
COptions		= {CDebugOptions} -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LDebugOptions	= -d
LOptions		= -aif -r -b 0x00 -workspace 4096 {LDebugOptions}


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}graphics.lib"	¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}filesystem.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}clib.lib"		¶
					"{3DOLibs}swi.lib"

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}{Application}.c.o"

#####################################
#	Default build rules
#####################################
All				Ä	{Application}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Application}		ÄÄ	{Application}.make {LIBS} {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {Application}			¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -stack 4096
	stripaif "{Application}" -o "{Application}"
	duplicate "{Application}" "{Application}".sym "{3DORemote}" {3DOAutodup} 

#####################################
#	Include file dependencies
#####################################
#{Application}.c		Ä	{Application}.h 


