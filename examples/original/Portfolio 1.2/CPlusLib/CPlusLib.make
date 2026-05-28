#
#	File:		CPlusLib.make
#
#	Contains:	C++ library (makefile)
#
#	Written by:	pab
#
#	Copyright:	(c) 1993-94 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <2>	 1/21/94	CDE		Upgraded to Jumpstart2 style makefile
#		 <1>	10/28/93	pab		Created for examples folder.
#

#####################################
#		Symbol definitions
#####################################
Application			=	CPlusLib
ObjectDir		=	:Objects:
CPLUS			=	armCPlus
CC				=	armcc
ASM				=	armasm
LIB				=   armlib

DebugFlag		=	1
#DebugFlag		=	0

#####################################
#	Default compiler options
#####################################
CPlusOptions	= -i "{3DOIncludes}" -d DEBUG={DebugFlag}

#CDebugOptions	= -g
CDebugOptions	=
COptions		= {CDebugOptions} -fa -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag} 

#SDebugOptions	= -g
SDebugOptions	=
SOptions		= {SDebugOptions} -bi -i "{3DOIncludes}"

LIBOptions		=  -c -o 

#####################################
#		Object files
#####################################
# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}CPlusRuntime.c.o"


#####################################
#	Default build rules
#####################################
{Application}		A	{Application}.lib

{ObjectDir}		A	:


.cp.o			A	.cp
	{CPLUS} {CPlusOptions} {DepDir}{Default}.cp -o {TargDir}{Default}.cp.o -comp {CC} {COptions}

.c.o			A	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			A	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Application}.lib		AA	{Application}.make {OBJECTS}
	{LIB}	{LIBOptions}		--
			{Application}.lib	--
			{OBJECTS}
	duplicate {Application}.lib "{3DOLibs}"{Application}.lib -y 

#####################################
#	Include file dependencies
#####################################
