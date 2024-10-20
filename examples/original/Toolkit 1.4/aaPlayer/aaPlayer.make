#
#	File:		aaPlayer.make
#
#	Contains:	make file for building aaPlayer.
#
#	Written by:	Jay Moreland
#
#	Copyright:	� 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <3>	 8/15/93	JAY		add duplicate command to build
#		<1+>	 8/10/93	JAY		add modbin to build procedure
#		 <1>	 6/25/93	JAY		first checked in
#
#	To Do:
#


#####################################
#		Symbol definitions
#####################################
Application		=	aaPlayer
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
COptions		= -g -zps0 -za1 -i "{3DOIncludes}" -d{threeDORelease}=1 -d DEBUG={DebugFlag} -d kBuildDate="�"`date -a`�""
#COptions		= -zps0 -za1 -i "{3DOIncludes}" -d{threeDORelease}=1 -d DEBUG={DebugFlag} -d kBuildDate="�"`date -a`�""

SOptions		= -bi -g -i "{3DOIncludes}"

#LOptions		= -aif -r -b 0x00 
LOptions		= -aif -r -b 0x00 -d -workspace 0x1000


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}Lib3DO.lib" �
					"{3DOLibs}input.lib" �
					"{3DOLibs}music.lib" �
					"{3DOLibs}filesystem.lib"	�
					"{3DOLibs}graphics.lib"		�
					"{3DOLibs}clib.lib"			�
					"{3DOLibs}audio.lib"		�
					"{3DOLibs}operamath.lib" 	�
					"{3DOLibs}swi.lib"  

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}{Application}.c.o"

#####################################
#	Default build rules
#####################################
All				�	{Application}

{ObjectDir}		�	:

.c.o			�	.c
	{CC} {DepDir}{Default}.c {COptions} -o {TargDir}{Default}.c.o 

.s.o			�	.s
	{ASM} {DepDir}{Default}.s {SOptions} -o {TargDir}{Default}.s.o 


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
	modbin {Application} -stack 4096
	stripaif {Application} -o {Application}
	duplicate {Application} {Application}.sym "{3DORemote}" {3DOAutodup} 

#####################################
#	Include file dependencies
#####################################
#{Application}.c		�	{Application}.h 


