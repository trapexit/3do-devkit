#
#	File:		ApplTest.make
#
#	Contains:	Test application class (makefile)
#
#	Written by:	pab
#
#	Copyright:	© 1994 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		<2>	  1/3/94	CDE		Upgraded to Jumpstart2-style.
#		<1>	10/28/93	pab		Derived from Makefile.
#

#####################################
#		Symbol definitions
#####################################
Application		=	ApplTest
ObjectDir		=	:Objects:
CPLUS			=	armCPlus
CC				=	armcc
ASM				=	armasm
LINK			=	armlink

DebugFlag		=	1
#DebugFlag		=	0

#####################################
#	Default options
#####################################

CPlusOptions	= -i "{3DOIncludes}" -d DEBUG={DebugFlag}

CDebugOptions	= -g
#CDebugOptions	=
COptions		= {CDebugOptions} -fa -za1 -zps0 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SDebugOptions	= -g
#SDebugOptions	=
SOptions		= {SDebugOptions} -bi -i "{3DOIncludes}"

LDebugOptions	= -d
#LDebugOptions	=
LOptions		= {LDebugOptions} -aif -r -b 0x00

#####################################
#		Object files
#####################################

#NOTE: Add any other required library files here:
LIBS			=	"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}CPlusLib.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"			

#NOTE: Add any other required object files here:
OBJECTS			=	"{ObjectDir}CApplication.cp.o" 		¶
					"{ObjectDir}{Application}.cp.o"

#####################################
#	Default build rules
#####################################
All				Ä	{Application}

{ObjectDir}		Ä	:

.cp.o			Ä	.cp
	{CPLUS} {CPlusOptions} {DepDir}{Default}.cp -o {TargDir}{Default}.cp.o -comp {CC} {COptions}

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
	modbin {Application} -stack 4000 -debug
#	modbin {Application} -stack 4000
	stripaif "{Application}" -o "{Application}"
	duplicate "{Application}" "{Application}".sym "{3DORemote}" {3DOAutodup}
#	duplicate "{Application}" "{3DORemote}" {3DOAutodup}

#####################################
#	Include file dependencies
#####################################
{ObjectDir}{Application}.cp.o		Ä	"{3DOLibs}"Lib3DO.lib {Application}.make CApplication.h
{ObjectDir}CApplication.cp.o		Ä	"{3DOLibs}"Lib3DO.lib {Application}.make CApplication.h

