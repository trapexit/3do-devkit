##########################################################################
#   File:       TestAM.make
#
#	Contains:	make file for building TestAM
#
#	Copyright © 1993 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	9/20/93		jb		New today.
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	TestAM
DebugFlag		=	1
#DebugFlag		=	0
ObjectDir		=	:Objects:
StreamDir		=	::DataStream:
SubscriberDir	=	{StreamDir}Subscribers:

CC				=	armcc
ASM				=	armasm
LINK			=	armlink
				
#####################################
#	Default compiler options
#####################################
#COptions		=    -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}
COptions		= -g -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}

#LOptions		= -aif -r -b 0x00
LOptions		= -aif -r -b 0x00 -d

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#		Object files
#####################################

LIBS			=	¶
					"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}music.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"

OBJECTS			=	"{ObjectDir}{Program}.c.o"	¶
					"{ObjectDir}BlockFile.c.o"	¶
					"{ObjectDir}AssetMgr.c.o"

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
{Program}		Ä	{Program}.make {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {Program}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Program} -c 'EaDJ' -t 'PROJ'
	modbin {program} -stack 0x4000 -debug

