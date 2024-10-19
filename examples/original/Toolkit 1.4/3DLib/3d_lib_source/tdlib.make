##########################################################################
#   File:       tdlib.make
#
#	Contains:	
#
#	History:
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	tdlib
DebugFlag		=	0
ObjectDir		=	:objects:
CC				=	armcc
ASM				=	armasm
LIB				=   armlib
LIBLIST			=   liblist

#####################################
#	Default compiler options
#####################################

# use the top COptions line to generate debug info
#COptions		= -fa -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag} -gflv
COptions		= -fa -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag} 

SOptions		= -bi -i "{3DOIncludes}"

LIBOptions		=  -c -o 


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}swi.lib"			¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}audio.lib"

# NOTE: Add object files here...
OBJECTS			=   "{ObjectDir}render_3d.c.o"       ¶
					"{ObjectDir}manage_3d.c.o"       ¶
                    "{ObjectDir}miscasm_3d.s.o"      ¶
					"{ObjectDir}process_3d.c.o"      ¶
					"{ObjectDir}misc_3d.c.o"		 ¶
					"{ObjectDir}utils_3d.c.o"		 ¶
					"{ObjectDir}math_3d.s.o"		 ¶
					"{ObjectDir}sound_3d.c.o"

####################################
#	Default build rules
#####################################
All				Ä	{Program}.lib

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {Default}.s


#####################################
#	Target build rules
#####################################
{Program}.lib		ÄÄ	{Program}.make {OBJECTS}
	{LIB}	{LIBOptions}			¶
			{Program}.lib			¶
			-v {LibList}
	rename {Program}.lib 3dlib.lib
	duplicate {3DOAutodup} 3dlib.lib "{3DOLibs}" 

#####################################
#	Include file dependencies
#####################################
3dlib.lib		Ä  tdlib.make
