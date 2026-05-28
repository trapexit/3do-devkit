##########################################################################
#   File:       DataAcq.make
#
#	Contains:	make file for building DataAcq.lib
#
#	Copyright (c) 1992 The 3DO Company
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
Library			=	DataAcqShuttle
DebugFlag		=	1
ObjectDir		=	:Objects:
StreamDir		=	:
SubscriberDir	=	{StreamDir}Subscribers:
CC				=	armcc
ASM				=	armasm
LIBRARIAN		=	armlib
RelativeBranchSwitch = -dRELATIVE_BRANCHING=1

#####################################
#	Default compiler options
#####################################
COptions		= -g -d RELATIVE_BRANCHING  -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag} {RelativeBranchSwitch}
#COptions		=    -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}" -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o


#####################################
#		Object files
#####################################
OBJECTS			=	"{ObjectDir}DataAcq.c.o"		--
					"{ObjectDir}ItemPool.c.o"		--
					"{ObjectDir}ThreadHelper.c.o"


#####################################
#	Default build rules
#####################################
All				A	{Library}.lib

{ObjectDir}		A	:

.c.o			A	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			A	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s

#####################################
#	Target build rules
#####################################
{Library}.lib		AA	{Library}.lib.make {OBJECTS}
	{LIBRARIAN}	{LOptions}			--
				{Library}.lib		--
				{OBJECTS}

#####################################
#	Include file dependencies
#####################################

{ObjectDir}DataAcq.c.o			A	DataAcq.h DataStream.h  MsgUtils.h --
									MemPool.h ThreadHelper.h
{ObjectDir}ItemPool.c.o			A	ItemPool.h
{ObjectDir}ThreadHelper.c.o		A	ThreadHelper.h



