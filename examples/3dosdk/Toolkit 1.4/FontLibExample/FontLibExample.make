#
#	File:		FontLibExample.make
#
#	Contains:	make file for building FontLibExample
#

#####################################
#		Symbol definitions
#####################################
App				=	FontLibExample
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
COptions		= -g -zps0 -za1 -fK -i ":" -i "{3DOIncludes}"  -d DEBUG={DebugFlag} -d DEBUG_MEMORY=1

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -aif -r -b 0x00 -d -workspace 0x10000


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"


# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}FontLibExample.c.o"	¶


#####################################
#	Default build rules
#####################################
All				Ä	{App}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{App}		ÄÄ	{App}.make {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {App}					¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {App} -c 'EaDJ' -t 'PROJ'
	modbin "{App}" -stack 0x10000 -debug
	StripAIF {App} -o {App}
	duplicate -y "{App}" "{3DORemote}" {3DOAutodup}
	duplicate -y "{App}".sym "{3DORemote}" {3DOAutodup}


#####################################
#	Include file dependencies
#####################################

#FontLib.c		Ä FontLib.h 
#textbox.c		Ä textbox.h  TextBox3DO.h

