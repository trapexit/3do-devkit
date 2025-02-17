##########################################################################
#   File:       BasicUFO.make
#
#	Contains:	make file for Jumpstart's BasicUFO example
#
#	History:
#	03/23/93	smb		Update for Red release
#	11/16/92	jb		Update for release 1B1
#	7/30/92		jb		New today.
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	MessageSample
DebugFlag		=	1
3DORelease		=	1P01
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
COptions		= -g -fa -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -aif -r -b 0x00 -workspace 0x4000 -d


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}Lib3DO.lib"		�
					"{3DOLibs}operamath.lib"	�
					"{3DOLibs}filesystem.lib"	�
					"{3DOLibs}graphics.lib"		�
					"{3DOLibs}audio.lib"		�
					"{3DOLibs}music.lib"		�
					"{3DOLibs}input.lib"		�
					"{3DOLibs}clib.lib"			�
					"{3DOLibs}swi.lib"

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}{Program}.c.o"


#####################################
#	Default build rules
#####################################
All				�	{Program}

{ObjectDir}		�	:

.c.o			�	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			�	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Program}		��	{Program}.make {OBJECTS}
	{LINK}	{LOptions}					�
			-o {Program}				�
			"{3DOLibs}cstartup.o"		�
			{OBJECTS}					�
			{LIBS}
	SetFile {Program} -c 'EaDJ' -t 'PROJ'
	modbin {program} -debug -stack 0x2000
	Duplicate {Program} "{3DOremote}" {3DOAutodup}


#####################################
#	Include file dependencies
#####################################
{Program}.c		�	"{3DOLibs}"Lib3DO.lib {Program}.make