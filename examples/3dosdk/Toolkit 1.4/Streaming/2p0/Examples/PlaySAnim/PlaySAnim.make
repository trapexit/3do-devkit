##########################################################################
#   File:       PlaySAnim.make
#
#	Contains:	make file for building PlaySAnim
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
#	06/09/94	dtc		Defined RelativeBranchSwitch compile switch
#	05/27/94	DLD		Fixed some dependency problems
#						for the ToolKit directory structure.
#	9/20/93		jb		New today
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	PlaySAnim
DebugFlag		=	1
#DebugFlag		=	0
AppsDir			=	:Apps & Data:
ObjectDir		=	:Objects:
StreamDir		=	:::DataStream:
SubscriberDir	=	{StreamDir}Subscribers:

CC				=	armcc
ASM				=	armasm
LINK			=	armlink
				
#####################################
#	Default compiler options
#####################################
RelativeBranchSwitch = -dRELATIVE_BRANCHING=0

#COptions		=   -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}
COptions		= -g -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}

#LOptions		= -aif -r -b 0x00
LOptions		= -aif -r -b 0x00 -d

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#		Object files
#####################################

LIBS			=	¶
					"{SubscriberDir}subscriber.lib"		¶
					"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}music.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib" 			¶
					"{StreamDir}dataacq.lib"	¶
					"{StreamDir}ds.lib"	

OBJECTS			=	"{ObjectDir}{Program}.c.o"		¶
					"{ObjectDir}PrepareStream.c.o"	¶
					"{ObjectDir}Utils.c.o"

#####################################
#	Default build rules
#####################################
All				Ä	{Program}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {DepDir}{Default}.c -o {ObjectDir}{Default}.c.o  {COptions}

.s.o			Ä	.s
	{ASM} {SOptions} -o {ObjectDir}{Default}.s.o {DepDir}{Default}.s


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
	stripaif "{program}" -o "{program}"
	modbin {program} -stack 0x4000 -debug
	if not `exists "{AppsDir}"`
		newFolder "{AppsDir}"
	end
	move -y {Program} "{AppsDir}"{Program}


#####################################
#	Include file dependencies
#####################################
{ObjectDir}{Program}.c.o		Ä	"{Program}.h"
{ObjectDir}PrepareStream.c.o	Ä	"PrepareStream.h"
