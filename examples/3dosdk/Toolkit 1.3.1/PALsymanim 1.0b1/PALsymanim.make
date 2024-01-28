#
#	File:		symanim.make
#
#	Contains:	make for building symanim
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
#		 <8>	 8/15/93	JAY		add modbin and duplicate command to build
#		 <7>	 6/24/93	dsm		Add increase stacksize -w 4096
#		 <6>	 6/14/93	JAY		add stack checking option to COptions. Necessary for Dragon
#									Release.
#		 <5>	  4/7/93	JAY		make the make consistant
#		 <4>	  4/5/93	JAY		add stripAIF to link actions
#		 <3>	 3/31/93	JAY		added input.lib to link line for 3B1
#		 <1>	 3/18/93	JAY		first checked in for Cardinal build
#				 7/30/92	jb		New today.
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	PALsymanim
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
# USE THE FOLLOWING LINE FOR SYMBOLIC DEBUGGING
CDebugOptions	= -g
# USE THE FOLLOWING LINE FOR OPTIMIZED CODE
#CDebugOptions	=

COptions		= {CDebugOptions} -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

# USE THE FOLLOWING LINE FOR SYMBOLIC DEBUGGING
LDebugOptions	= -d		# turn on symbolic information
# USE THE FOLLOWING LINE FOR OPTIMIZED CODE
#LDebugOptions	=			# turn off symbolic information

LOptions		= {LDebugOptions} -aif -r -b 0x00 -workspace 4096 

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
					"{ObjectDir}Sprite.c.o"		

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
	modbin {Application} -stack 0x2000 -debug
	stripaif {Application} -o {Application}
	duplicate {Application} "{3DORemote}" {3DOAutodup}
	duplicate {Application}.sym "{3DORemote}" {3DOAutodup}
#####################################
#	Include file dependencies
#####################################
#{Application}.c		Ä	{Application}.h 


PALsymanim.c	Ä Sprite.h 
sprite.c Ä Sprite.h 