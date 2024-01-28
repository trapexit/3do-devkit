#
#	File:		JSPlayBgndMusic.make
#
#	Contains:	Background music playing task for JSInteractiveMusic
#				(makefile)
#
#	Written by:	Joe Buczek
#				( Freely adapted for JumpStart by Charlie Eckhaus )
#
#	Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <1>	 1/5/94		CDE		Derived from PlayBkgndMusic.make from old Jumpstart's GameUFO example
#

#####################################
#		Symbol definitions
#####################################
Application		=	JSPlayBgndMusic
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink

DebugFlag		=	1
#DebugFlag		=	0

#####################################
#	Default compiler options
#####################################
CDebugOptions	= -g
#CDebugOptions	=
COptions		= {CDebugOptions} -fa -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SDebugOptions	= -g
#SDebugOptions	=
SOptions		= {SDebugOptions} -bi -i "{3DOIncludes}"

LDebugOptions	= -d
#LDebugOptions	=
LOptions		= {LDebugOptions} -aif -r -b 0x00


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}music.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}clib.lib"			¶
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
{Application}		ÄÄ	{Application}.make {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {Application}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -stack 4000 -debug
#	modbin {Application} -stack 4000
	stripaif "{Application}" -o "{Application}"
	Duplicate {Application} "{3DORemote}"JSData:Sound:{Application}.task {3DOAutodup}
	Duplicate {Application}.sym "{3DORemote}"JSData:Sound: {3DOAutodup}

#####################################
#	Include file dependencies
#####################################
{ObjectDir}{Application}.c.o		Ä "{3DOLibs}"Lib3DO.lib {Application}.make
