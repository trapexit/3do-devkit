##########################################################################
#   File:       AssetMgr.make
#
#	Contains:	make file for building AssetMgr.lib
#
#	Copyright © 1992 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	9/15/93		jb		New today.
##########################################################################

#####################################
#		Symbol definitions
#####################################
Library			=	AssetMgr
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LIBRARIAN		=	armlib

#####################################
#	Default compiler options
#####################################
#COptions		= -g -zps0 -za1 -i "{3DOIncludes}"  -dDEBUG={DebugFlag}
COptions		=    -zps0 -za1 -i "{3DOIncludes}"  -dDEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o


#####################################
#		Object files
#####################################
OBJECTS			=	"{ObjectDir}AssetMgr.c.o"		¶
					"{ObjectDir}BlockFile.c.o"


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

AssetMgr.c			Ä	AssetMgr.h BlockFile.h 
BlockFile.c			Ä	BlockFile.h



