#
#	File:		bounce.make
#
#	Contains:	make file for building gagSample
#
#	Written by:	Gregg Gorsiski and Darren Gibbs
#
#	Copyright:	� 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <2>	  4/3/93	JAY		change target in duplicate to something more generic and fooled
#									around with Application
#		 <1>	  4/3/93	JAY		first checked in
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	BounceSound
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#
# The workspace setting in the linker command line is REQUIRED... I spent four 
#   days tracking down a "bug" that turned out to be only an insufficiently 
#   large stack.  40000 (determined by trial and error) seems to be plenty. 
#####################################
COptions		= -g -zps1 -za1 -i "{3DOIncludes}" -d{threeDORelease}=1 -d DEBUG={DebugFlag}
#COptions		= -zps1 -za1 -i "{3DOIncludes}" -d{threeDORelease}=1 -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

#LOptions		= -aif -r -b 0x00 
LOptions		= -aif -r -b 0x00 -d -workspace 40000


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
OBJECTS			=	"{ObjectDir}bounce.c.o" �
					"{ObjectDir}bounce_sound.c.o"

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
{Application}		��	bounce.make {LIBS} {OBJECTS}
	{LINK}	{LOptions}					�
			-o {Application}				�
			"{3DOLibs}cstartup.o"		�
			{OBJECTS}					�
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	stripaif {Application}
	Duplicate -y {Application}.nosym "{3DOFolder}3DO_OS:3B1:Remote:{Application}"
	Duplicate -y {Application}.sym "{3DOFolder}3DO_OS:3B1:Remote:{Application}.sym"

#####################################
#	Include file dependencies
#####################################
bounce.c		�	bounce.h 
bouncd_sound.c	� 	bounce_sound.h 
