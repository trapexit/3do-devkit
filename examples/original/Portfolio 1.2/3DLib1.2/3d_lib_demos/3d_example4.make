##########################################################################
#   File:       3d_example4.make
#
#	Contains:	
#
#	History:
##########################################################################

# Remember to set 3DOLibs and 3DOIncludes in your MPW "Startup" file

#####################################
#		Symbol definitions
#####################################
Program			=	3d_example4
DebugFlag		=	1
ObjectDir		=	:objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink


#####################################
#	Default compiler options
#####################################
COptions		= -fa -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag} -gflv

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		=  -d -aif -r -b 0x00 -workspace 0x400 


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}3dlib.lib"		¶
					"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}swi.lib"			¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}audio.lib"

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}3d_example4_core.c.o"	¶
					"{ObjectDir}3d_example4_stuff.c.o"	

#####################################
#	Default build rules
#####################################
All				Ä	{Program}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Program}		ÄÄ	{Program}.make {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {Program}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Program} -c 'EaDJ' -t 'PROJ'
	echo modbinning {Program}
	modbin {Program} -stack 10000  -debug


#####################################
#	Include file dependencies
#####################################
{Program}.c		Ä  3d_example4.make