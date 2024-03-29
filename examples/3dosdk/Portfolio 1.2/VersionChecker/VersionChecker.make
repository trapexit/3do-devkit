#
#	File:		VersionChecker.make
#
#	Contains:	Version checker (Makefile).
#				Finds a task by name and displays its version number. 
#
#	Written by:	Charlie Eckhaus
#
#	Copyright:	� 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <1>	 7/1/93	CDE		Derived from TaskDemon makefile
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application			=	VersionChecker
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink
OSVersion		=	:4B1:


#####################################
#	Default compiler options
#####################################

#Defines			=
Defines			=  -d USE_4B1 -d USE_STRCASECMP

COptions		= -g -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag} {Defines}

SOptions		= -bi -g -i "{3DOIncludes}"


LOptions		= -aif -r -b 0x00 -d -workspace 0x10000

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
					
OBJECTS			=	"{ObjectDir}{Application}.c.o" �
					"{ObjectDir}baseUtils.c.o"


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
{Application}		��	{Application}.make {OBJECTS}
	{LINK}	{LOptions}					�
			-o {Application}				�
			"{3DOLibs}cstartup.o"		�
			{OBJECTS}					�
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -stack 0x1000 -debug
	stripaif {Application} -o {Application}
	duplicate {Application} "{3DORemote}" {3DOAutodup}
	duplicate {Application}.sym "{3DORemote}" {3DOAutodup}


#####################################
#	Include file dependencies
#####################################
{Application}.c		�	baseUtils.h
baseUtils.c		�	baseUtils.h


