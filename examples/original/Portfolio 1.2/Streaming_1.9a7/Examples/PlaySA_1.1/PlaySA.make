##########################################################################
#   File:       PlaySA.make
#
#	Contains:	make file for building PlaySA   (Play Streamed Audio)
#
#	Copyright � 1993 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	10/25/93	rdg		Convert from TestCpakStream.make
#	10/20/93	jb		Move codec.lib above clib.lib so it can get to
#						malloc() and free()
#	9/20/93		jb		New today
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	PlaySA
DebugFlag		=	1
#DebugFlag		=	0
ObjectDir		=	:Objects:
StreamDir		=	:::DataStream:
SubscriberDir	=	{StreamDir}Subscribers:

CC				=	armcc
ASM				=	armasm
LINK			=	armlink
				
#####################################
#	Default compiler options
#####################################
COptions		=    -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}
#COptions		= -g -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}

LOptions		= -aif -r -b 0x00
#LOptions		= -aif -r -b 0x00 -d

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#		Object files
#####################################

LIBS			=	�
					"{SubscriberDir}subscriber.lib"		�
					"{3DOLibs}Lib3DO.lib"		�
					"{3DOLibs}input.lib"		�
					"{3DOLibs}graphics.lib"		�
					"{3DOLibs}audio.lib"		�
					"{3DOLibs}filesystem.lib"	�
					"{3DOLibs}music.lib"		�
					"{3DOLibs}operamath.lib"	�
					"{SubscriberDir}codec.lib"	�
					"{3DOLibs}clib.lib"			�
					"{3DOLibs}swi.lib" 			�
					"{StreamDir}dataacq.lib"	�
					"{StreamDir}ds.lib"	

OBJECTS			=	"{ObjectDir}{Program}.c.o"	�
					"{ObjectDir}PrepareStream.c.o"	�
					"{ObjectDir}PlaySSNDStream.c.o"  �
					"{ObjectDir}JoyPad.c.o"

#####################################
#	Default build rules
#####################################
All				�	{Program}

{ObjectDir}		�	:

.c.o			�	.c
	{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o  {COptions}

.s.o			�	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Program}		�	{Program}.make {OBJECTS} {LIBS}
	{LINK}	{LOptions}					�
			-o {Program}				�
			"{3DOLibs}cstartup.o"		�
			{OBJECTS}					�
			{LIBS}
	SetFile {Program} -c 'EaDJ' -t 'PROJ'
	modbin {program} -stack 0x4000 -debug

#####################################
#	Include file dependencies
#####################################
{Program}.c.o		�	{Program}.make {Program}.c  {Program}.h
PrepareStream.c.o	�	{Program}.make PrepareStream.c  PrepareStream.h
PlaySSNDStream.c.o	�	{Program}.make PlaySSNDStream.c PlaySSNDStream.h
JoyPad.c.o			�	{Program}.make JoyPad.c JoyPad.h