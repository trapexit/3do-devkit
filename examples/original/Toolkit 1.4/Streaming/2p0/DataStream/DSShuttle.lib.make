##########################################################################
#   File:       DS.lib.make
#
#	Contains:	make file for building DS.lib
#
#	Copyright (c) 1993 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	7/24/93		jb		Get rid of 'H' and switch everything to 'DSxxxx'
#	7/10/93		jb		Add SemHelper.c and MakeName.c
#	6/23/93		jb		Turn on stack checking for Dragon O/S release
#	6/15/93		jb		Add DataStreamDebug.c
#	6/8/93		jb		Add ThreadHelper.c
#	5/12/93		jb		Add DataStreamLib.c
#	5/9/93		jb		Fix header file dependencies, remove TimerUtils.c
#	4/3/93		jb		New today.
##########################################################################

#####################################
#		Symbol definitions
#####################################
Library			=	DSShuttle
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LIBRARIAN		=	armlib
RelativeBranchSwitch = -dRELATIVE_BRANCHING=1

#####################################
#	Default compiler options
#####################################
COptions		= -g -zps0 -za1 -d RELATIVE_BRANCHING -i "{3DOIncludes}"  -dDEBUG={DebugFlag} {RelativeBranchSwitch}
#COptions		=    -zps0 -za1 -i "{3DOIncludes}"  -dDEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o


#####################################
#		Object files
#####################################
OBJECTS			=	"{ObjectDir}DataStream.c.o"		--
					"{ObjectDir}DataStreamLib.c.o"	--
					"{ObjectDir}DataStreamDebug.c.o"	--
					"{ObjectDir}MemPool.c.o"		--
					"{ObjectDir}MsgUtils.c.o"		--
					"{ObjectDir}ThreadHelper.c.o"	--
					"{ObjectDir}SemHelper.c.o"		--
					"{ObjectDir}MakeName.c.o"


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

{ObjectDir}DataStream.c.o		A	DataStream.h MsgUtils.h MemPool.h SemHelper.h ThreadHelper.h
{ObjectDir}DataStreamLib.c.o	A	DataStreamLib.h DataStream.h MsgUtils.h MemPool.h
{ObjectDir}DataStreamDebug.c.o	A	DataStreamDebug.h DataStream.h
{ObjectDir}MemPool.c.o			A	MemPool.h
{ObjectDir}MsgUtils.c.o			A	MsgUtils.h
{ObjectDir}ThreadHelper.c.o		A	ThreadHelper.h MakeName.h
{ObjectDir}SemHelper.c.o		A	SemHelper.h ThreadHelper.h MakeName.h
{ObjectDir}MakeName.c.o			A	MakeName.h