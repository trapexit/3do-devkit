#
#	File:		Makefile
#
#	Contains:	Make file for building StorageTuner Utility
#
#				StorageTunerExample		-	an example program which links in the 
#											StorageTuner.lib and then calls the Storage Tuner
#				StorageTuner.lib 		-	library for calling the Storage Tuner
#											from within your program
#				StorageTuner	 		-	Stand-Alone version of the Storage Tuner
#											this can be launched as an application
#				StorageTunerDebug 		-	Debug version of same
#
#				StorageTunerSubroutine	-	Subroutine version of the Storage Tuner
#											this can be called as a Subroutine
#											of your application
#											
#
#	Written by:	Francis Stanbach
#
#	Copyright:	© 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <1>	 1/7/94		FJS		first stolen
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################

ObjDir			=	:Objects:
DObjDir			=	:ObjectsDebug:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink
LIBRARIAN		=	armlib

#####################################
#	Default compiler options
#####################################


COptions		= {CDebugOptions} -zps0 -za1 -i "{3DOIncludes}" -i :Library: -i :Tuner: -d DEBUG={DebugFlag}
SOptions		= -bi -g -i "{3DOIncludes}"
LDebugOptions	= -d 
LOptions		= -aif -r -b 0x00 -workspace 4096
LinkOptions		= -o

#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}Lib3DO.lib"			¶
					"{3DOLibs}operamath.lib"		¶
					"{3DOLibs}graphics.lib"			¶
					"{3DOLibs}filesystem.lib"		¶
					"{3DOLibs}input.lib"			¶
					"{3DOLibs}clib.lib"				¶
					"{3DOLibs}swi.lib"

OBJECTS			=	"{ObjDir}st.c.o"				¶
					"{ObjDir}stlists.c.o"			¶
					"{ObjDir}stmem.c.o"
					
#
# debug stuff
#

DebugLIBS		=	"{3DOLibs}Lib3DO.lib"			¶
					"{3DOLibs}operamath.lib"		¶
					"{3DOLibs}graphics.lib"			¶
					"{3DOLibs}filesystem.lib"		¶
					"{3DOLibs}input.lib"			¶
					"{3DOLibs}clib.lib"				¶
					"{3DOLibs}swi.lib"

DebugOBJECTS	=	"{DObjDir}st.c.o"				¶
					"{DObjDir}stlists.c.o"			¶
					"{DObjDir}stmem.c.o"
					


#####################################
#	Default build rules
#####################################

{ObjDir}		Ä	: 
{ObjDir}		Ä	:Tuner:
{ObjDir}		Ä	:Example:
{ObjDir}		Ä	:Library:


{DObjDir}		Ä	: 
{DObjDir}		Ä	:Tuner:
{DObjDir}		Ä	:Examples:
{DObjDir}		Ä	:Library:


.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################

All				Ä	StorageTunerSubroutine StorageTunerExample StorageTuner StorageTunerDebug

NoDebug Ä
	Set DebugFlag 0

StorageTunerExample  Ä {ObjDir}Storagetunerexample.c.o StorageTuner.lib
	{LINK}	{LOptions}						¶
			-o {targ}						¶
			"{3DOLibs}"cstartup.o				¶
			{ObjDir}StorageTunerExample.c.o	¶
			StorageTuner.lib				¶
			{LIBS}
	SetFile {targ} -c 'EaDJ' -t 'PROJ'
	modbin {targ} -stack 4096
	duplicate -y "{targ}" "{3DORemote}"StorageTuner:

StorageTuner.lib  Ä {ObjDir}storagetuner.c.o
	{LIBRARIAN}	-c {LinkOptions} 	¶
				{targ}				¶
				{ObjDir}storagetuner.c.o

StorageTuner	Ä	Makefile NoDebug {LIBS} {OBJECTS}
	{LINK}	{LOptions}						¶
			-o {targ}						¶
			"{3DOLibs}"cstartup.o				¶
			{OBJECTS}						¶
			{LIBS}
	SetFile {targ} -c 'EaDJ' -t 'PROJ'
	modbin {targ} -stack 4096
#	stripaif "{targ}" -o Dev:Null -s "{targ}".sym
	duplicate -y "{targ}" "{3DORemote}"StorageTuner:

StorageTunerSubroutine	Ä	Makefile NoDebug {LIBS} {OBJECTS}
	{LINK}	{LOptions}						¶
			-o {targ}						¶
			"{3DOLibs}"subroutinestartup.o	¶
			{OBJECTS}						¶
			{LIBS}
	SetFile {targ} -c 'EaDJ' -t 'PROJ'
	modbin {targ} -stack 4096
#	stripaif "{targ}" -o Dev:Null -s "{targ}".sym
	duplicate -y "{targ}" "{3DORemote}"StorageTuner:

#####################################
#	Debug build rules
#####################################

StorageTunerDebug	Ä	Makefile DoDebugOptions {DebugLIBS} {DebugOBJECTS}
	{LINK}	{LDebugOptions} {LOptions}		¶
			-o {targ}						¶
			"{3DOLibs}"cstartup.o				¶
			{DebugOBJECTS}					¶
			{DebugLIBS}
	SetFile {targ} -c 'EaDJ' -t 'PROJ'
	modbin {targ} -debug -stack 4096
	stripaif "{targ}" -o "{targ}" -s "{targ}".sym
	duplicate -y "{targ}" "{3DORemote}"StorageTuner:
	duplicate -y "{targ}".sym "{3DORemote}"StorageTuner:


# the Weasel dependency is a trick that I have used here to avoid
# creating separate build commands for debug and non-debug objects
# its not a good way to do it

DoDebugOptions	Ä
	set CDebugOptions -g
	set DebugFlag 1
