#
#	File:		eventJoy.make
#
#	Contains:	control pad demo make file
#
#	Written by:	Jay Moreland
#
#	Copyright:	© 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		<1+>	 8/10/93	JAY		add modbin to build for setting stack size
#		 <1>	 6/28/93	JAY		first checked in
#		 <4>	 3/31/93	JAY		added input.lib to the link line for 3B1
#		 <3>	 3/17/93	JAY		turning on debug flag
#		 <2>	 3/17/93	JAY		turned on the debugger
#		 <1>	 3/17/93	JAY		first checked in
#		 <0>	 7/30/92	jb		New today
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	eventJoy
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
COptions		= -g -zps0 -za1 -i "{3DOIncludes}" -d{threeDORelease}=1 -d DEBUG={DebugFlag}
#COptions		= -zps0 -za1 -i "{3DOIncludes}" -d{threeDORelease}=1 -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -aif -r -b 0x00 -d
#LOptions		= -aif -r -b 0x00


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
					"{ObjectDir}JoyPad.c.o"


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
			-o {Application}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -stack 3000
	StripAIF {Application} -o {Application}
	Duplicate {Application} {Application}.sym {3DORemote} {3DOAutodup} 


#####################################
#	Include file dependencies
#####################################
