#
#	File:		3DOOrbit.make
#
#	Contains:	xxx put contents here xxx
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
#		<11>	 8/15/93	JAY		add modbin (a.k.a. Extend_AIF) to link line
#		<10>	 6/25/93	JAY		add workspace 4000 to link options
#		 <9>	 6/23/93	JAY		add stack checking (zps0) to COptions. Needed for Dragon (4B1)
#		 <8>	 6/14/93	JAY		add stack checking (zps0) to COptions. Needed for Dragon Release
#									(4B1)
#		 <7>	  4/7/93	JAY		make the make consistant
#		 <6>	  4/5/93	JAY		adding StripAIF to link actions
#		 <5>	  4/3/93	JAY		adding LIBS to link dependency and added duplicate to remote
#									directory of link output
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
Application		=	3DOOrbit
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink
modbin			=	Extend_AIF


#####################################
#	Default compiler options
#####################################
CDebugOptions	= -g		# turn on symbolic information
COptions		= {CDebugOptions} -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LDebugOptions	= -d		# turn on symbolic information
LOptions		= -aif -r -b 0x00 -workspace 4000 {LDebugOptions}


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
OBJECTS			=	"{ObjectDir}{Application}.c.o"¶
					"{ObjectDir}OrbitSound.c.o"		¶
#					"{ObjectDir}Sprite.c.o"		

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
	modbin {Application} -stack 4000
	stripaif {Application} -o {Application}
	duplicate {Application} {Application}.sym "{3DORemote}" {3DOAutodup}

#####################################
#	Include file dependencies
#####################################
#{Application}.c		Ä	{Application}.h 


3DOOrbit.c	Ä Sprite.h 
OrbitSound.c	Ä OrbitSound.h 
#sprite.c Ä Sprite.h 