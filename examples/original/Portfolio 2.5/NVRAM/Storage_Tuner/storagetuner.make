#####################################
##
## Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
## This material contains confidential information that is the property of The 3DO Company.
## Any unauthorized duplication, disclosure or use is prohibited.
## $Id: storagetuner.make,v 1.3 1995/01/19 03:04:23 mattm Exp $
##
#####################################
#   File:       storagetuner.make
#   Target:     storagetuner
#   Sources:    st.c stlists.c stmem.c
#
#   Copyright 3DO Company, 1995
#   All rights reserved.
#

#####################################
#	Symbol definitions
#####################################

App				= storagetuner
DebugFlag		= 1
SourceDir		= {3DOFolder}Examples:NVRAM:Storage_Tuner:
ObjectDir		= :Objects:
Apps_Data		= :Apps_Data:
CC				= armcc
LINK			= armlink
WorkingDisk		=
3DOAutodup		= -y

#####################################
#	Default compiler options
#####################################

COptions			= -fa -zps0 -za1
CDebugOptions		= -g -d DEBUG={DebugFlag}
LOptions			= -aif -r -b 0x00
LStackSize			= 4096
LDebugOptions		= -d
ModbinDebugOptions	= -debug

#####################################
#	Object files
#####################################

OBJECTS			=	 {ObjectDir}st.c.o {ObjectDir}stlists.c.o {ObjectDir}stmem.c.o "{3DOLibs}"cstartup.o

LIBS			=	 �
					"{3DOLibs}Lib3DO.lib" �
					"{3DOLibs}audio.lib" �
					"{3DOLibs}music.lib" �
					"{3DOLibs}operamath.lib" �
					"{3DOLibs}filesystem.lib" �
					"{3DOLibs}graphics.lib" �
					"{3DOLibs}input.lib" �
					"{3DOLibs}clib.lib"

#####################################
#	Default build rules
#####################################

All				�	{App}

{ObjectDir}		�	:

.c.o	�	.c
	{CC} -i "{3DOIncludes}" {COptions} {CDebugOptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

#####################################
#	Target build rules
#####################################

{App} � {App}.make {OBJECTS}
	{LINK} {LOptions} {LDebugOptions} �
		{OBJECTS} �
		{LIBS} �
		-o "{WorkingDisk}"{Targ}
	SetFile "{WorkingDisk}"{Targ} -c 'EaDJ' -t PROJ
	modbin "{WorkingDisk}"{Targ} -stack {LStackSize} {ModbinDebugOptions}
	stripaif "{WorkingDisk}"{Targ} -o {Targ} -s {Targ}.sym
	move {3DOAutodup} {Targ} "{Apps_Data}"
	move {3DOAutodup} {Targ}.sym "{Apps_Data}"

#####################################
#	Additional Target Dependencies
#####################################

{ObjectDir}st.c.o			�	{App}.make
{ObjectDir}stlists.c.o		�	{App}.make
{ObjectDir}stmem.c.o		�	{App}.make

