#
#	File:		perftest.make
#
#	Contains:	make file for perftest
#
#	Written by:	Jay Moreland
#
#	Copyright:	� 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <7>	 8/15/93	JAY		add duplicate command to build
#		 <6>	 8/11/93	JAY		add modbin to build to set stack size
#		 <5>	 7/30/93	JAY		add -workspace 3000 to link line
#		 <4>	 6/24/93	JAY		add check stack option (zps0) to COptions. Needed for Dragon
#									Release (4B1)
#		 <3>	  4/7/93	JAY		make the make consistant
#		 <2>	  4/5/93	JAY		add StripAIF to link actions
#		 <1>	  4/3/93	JAY		first checked in
#
#	To Do:
#


#####################################
#		Symbol definitions
#####################################
Application		=	perftest
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
CDebugOptions	= -g		# turn on symbolic information
COptions		= {CDebugOptions} -zps0 -za1 -i "{3DOIncludes}" -d{threeDORelease}=1 -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LDebugOptions	= -d		# turn on symbolic information
LOptions		= {LDebugOptions} -aif -workspace 3000 -r -b 0x00 


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}Lib3DO.lib"		�
					"{3DOLibs}operamath.lib"	�
					"{3DOLibs}graphics.lib"	�
					"{3DOLibs}audio.lib"		�
					"{3DOLibs}filesystem.lib"		�
					"{3DOLibs}input.lib"		�
					"{3DOLibs}clib.lib"		�
					"{3DOLibs}swi.lib"

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}{Application}.c.o"�
					"{ObjectDir}Sprite.c.o"		

#####################################
#	Default build rules
#####################################
All				�	{Application}

{ObjectDir}		�	:

.c.o			�	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			�	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Application}		��	{Application}.make {LIBS} {OBJECTS}
	{LINK}	{LOptions}					�
			-o {Application}				�
			"{3DOLibs}cstartup.o"		�
			{OBJECTS}					�
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -stack 3000
	stripaif {Application} -o {Application}
	duplicate -y "{Application}" "{Application}".sym "{3DORemote}" {3DOAutodup}

#####################################
#	Include file dependencies
#####################################
#{Application}.c		�	{Application}.h 


perftest.c	� Sprite.h 
sprite.c � Sprite.h 