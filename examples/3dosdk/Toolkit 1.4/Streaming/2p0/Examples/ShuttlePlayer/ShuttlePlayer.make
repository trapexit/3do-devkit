##########################################################################
#   File:       ShuttlePlayer.make
#
#	Contains:	make file for building ShuttlePlayer
#
#	Copyright © 1993 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	06/17/94	dtc		Removed link dependency on AppsDir and created
#						'Apps & Data' folder if it doesn't exists.
#						Added stripaif.
#	06/09/94	dtc		Defined RelativeBranchSwitch compile switch.
#						Fixed to look for codec.lib in the {3DOLibs}
#						instead of {SubscriberDir}.
#	11/12/93	jb		Add JoyPad.c
#	10/20/93	jb		Move codec.lib above clib.lib so it can get to
#						malloc() and free()
#	9/20/93		jb		New today
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	ShuttlePlayer
DebugFlag		=	1
#DebugFlag		=	0
AppsDir			=	:Apps & Data:
ObjectDir		=	:Objects:
StreamDir		=	:::DataStream:
SubscriberDir	=	{StreamDir}Subscribers:

CC				=	armcc
ASM				=	armasm
LINK			=	armlink

RelativeBranchSwitch = -dRELATIVE_BRANCHING=1
				
#####################################
#	Default compiler options
#####################################

COptions		= -g -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag} {RelativeBranchSwitch}
#COptions		=    -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag} {RelativeBranchSwitch}

LOptions		= -aif -r -b 0x00 -d
#LOptions		= -aif -r -b 0x00

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#		Object files
#####################################

LIBS			=	¶
					"{SubscriberDir}subscriberShuttle.lib"		¶
					"{StreamDir}dataacqShuttle.lib"	¶
					"{StreamDir}dsShuttle.lib"	¶
					"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}music.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}codec.lib"		¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib" 			

OBJECTS			=	"{ObjectDir}{Program}.c.o"		¶
					"{ObjectDir}PlayCPakStream.c.o"	¶
					"{ObjectDir}PrepareStream.c.o"	¶
					"{ObjectDir}PlaySSNDStream.c.o"	¶
					"{ObjectDir}JoyPad.c.o"			¶
					"{ObjectDir}UserControls.c.o"	¶
					"{ObjectDir}Screens.c.o"		¶
					"{ObjectDir}elkabong.c.o"		¶
					"{ObjectDir}Menus.c.o"

#####################################
#	Default build rules
#####################################
All				Ä	{Program}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o  {COptions}

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Program}		Ä	{Program}.make {OBJECTS} {LIBS}
	{LINK}	{LOptions}					¶
			-o {Program}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Program} -c 'EaDJ' -t 'PROJ'
	modbin {program} -stack 0x4000 -debug
	stripaif {Program} -o {Program}
	if not `exists "{AppsDir}"`
		newFolder "{AppsDir}"
	end
	move -y {Program} "{AppsDir}"{Program}

#####################################
#	Include file dependencies
#####################################
{ObjectDir}PlayCPakStream.c.o		Ä  "PlayCPakStream.h"
{ObjectDir}PlaySSNDStream.c.o		Ä  "PlaySSNDStream.h"
{ObjectDir}PrepareStream.c.o		Ä  "PrepareStream.h"
{ObjectDir}JoyPad.c.o				Ä  "JoyPad.h"
