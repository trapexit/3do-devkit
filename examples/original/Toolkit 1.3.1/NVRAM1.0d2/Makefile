#
#	File:		Makefile
#
#	Contains:	Make file for building NVRAM Utility
#
#	Written by:	Francis Stanbach
#
#	Copyright:	© 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <1>	 1/7/94		FJS		first stolen
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	NVRAM
DebugFlag		=	1
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
OBJECTS			=	"{ObjectDir}{Application}.c.o" ¶
					"{ObjectDir}FontLib.c.o" ¶
					"{ObjectDir}TextLib.c.o" ¶
					"{ObjectDir}ListHandler.c.o" ¶
					"{ObjectDir}ListUtilities.c.o" ¶
					"{ObjectDir}Failure.c.o" ¶
					"{ObjectDir}FontBlit3To8_.s.o"

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
{Application}	Ä	Makefile {LIBS} {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {Application}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -debug -stack 4096
	stripaif "{Application}" -o "{Application}" -s "{Application}".sym
	duplicate -y "{Application}" "{3DORemote}"NVRAM: 
	duplicate -y "{Application}".sym "{3DORemote}"NVRAM:


#####################################
#	Include file dependencies
#####################################
#{Application}.c		Ä	{Application}.h 
