#
#	File:		ft_test.make
#
#	Contains:	build the test application that accesses the
#				user folio in the example
#
#	Written by:	Jay Moreland
#
#	Copyright:	© 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <2>	 6/23/93	JAY		add stack checking (zps0) for Dragon
#		 <1>	  4/6/93	JAY		first checked in
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	ft_test
DebugFlag		=	0
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
COptions		= -fa -zps0 -za1 -i "{3DOIncludes}" -gflv -d DEBUG={DebugFlag} 

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -d -aif -r -b 0x00 -workspace 0x4000 


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}music.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}ft_test.c.o"	¶
					"{ObjectDir}ft_lib.c.o"


#####################################
#	Default build rules
#####################################
All				Ä	{Application}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Application}		ÄÄ	{Application}.make {LIBS} {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {Application}			¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -stack 4096
	stripaif "{Application}" -o "{Application}"
	duplicate "{Application}" "{Application}".sym "{3DORemote}" {3DOAutodup} 


#####################################
#	Include file dependencies
#####################################
# {Application}.c		Ä ft_test