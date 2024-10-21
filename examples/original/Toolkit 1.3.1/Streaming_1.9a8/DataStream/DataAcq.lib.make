##########################################################################
#   File:       DataAcq.make
#
#	Contains:	make file for building DataAcq.lib
#
#	Copyright © 1992 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	6/23/93		jb		Turn on stack checking for Dragon O/S release
#	6/15/93		jb		Add ItemPool.c to dependency list
#	4/5/93		jb		New today.
##########################################################################

#####################################
#		Symbol definitions
#####################################
Library			=	DataAcq
DebugFlag		=	1
ObjectDir		=	:Objects:
StreamDir		=	:
SubscriberDir	=	{StreamDir}Subscribers:
CC				=	armcc
ASM				=	armasm
LIBRARIAN		=	armlib

#####################################
#	Default compiler options
#####################################
#COptions		= -g -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}
COptions		=    -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o


#####################################
#		Object files
#####################################
OBJECTS			=	"{ObjectDir}DataAcq.c.o"		¶
					"{ObjectDir}BlockFile.c.o"		¶
					"{ObjectDir}ItemPool.c.o"		¶
					"{ObjectDir}ThreadHelper.c.o"


#####################################
#	Default build rules
#####################################
All				Ä	{Library}.lib

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s

#####################################
#	Target build rules
#####################################
{Library}.lib		ÄÄ	{Library}.lib.make {OBJECTS}
	{LIBRARIAN}	{LOptions}			¶
				{Library}.lib		¶
				{OBJECTS}

#####################################
#	Include file dependencies
#####################################

DataAcq.c			Ä	DataAcq.h DataStream.h BlockFile.h MsgUtils.h ¶
						MemPool.h ThreadHelper.h
BlockFile.c			Ä	BlockFile.h
ItemPool.c			Ä	ItemPool.h
ThreadHelper.c		Ä	ThreadHelper.h



