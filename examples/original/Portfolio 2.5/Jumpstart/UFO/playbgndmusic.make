#####################################
##
## Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
## This material contains confidential information that is the property of The 3DO Company.
## Any unauthorized duplication, disclosure or use is prohibited.
## $Id: playbgndmusic.make,v 1.11 1995/02/06 19:46:58 limes Exp $
##
#####################################
#
#   File:       playbgndmusic.make
#   Target:     playbgndmusic
#   Sources:    playbgndmusic.c
#
#   Copyright (c) 1995, The 3DO Company
#   All rights reserved.
#

#####################################
#	Symbol definitions
#####################################

App				=	playbgndmusic
DebugFlag		=	1
ObjectDir		=	:Objects:
SourceDir		=	:
AppsDir			=	:Apps_Data:
ExamplesLibDir	=	{3DOFolder}Examples:ExamplesLib:
CC				=	armcc
LINK			=	armlink
WorkingDisk		=

#####################################
#	Default compiler options
#####################################

COptions			= -fa -zps0 -za1 -i "{ExamplesLibDir}"
CDebugOptions		= -g -d DEBUG={DebugFlag}
LOptions			= -aif -r -b 0x00
LStackSize			= 4096
LDebugOptions		= -d
ModbinDebugOptions	= -debug

#####################################
#	Object files
#####################################

OBJECTS			=	{ObjectDir}{App}.c.o �
					"{3DOLibs}"cstartup.o

LIBS			=	 �
					"{ExamplesLibDir}ExamplesLib.lib"  �
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
	SetFile "{WorkingDisk}"{Targ} -c 'EaDJ' -t 'PROJ'
	modbin "{WorkingDisk}"{Targ} -stack {LStackSize} {ModbinDebugOptions}
	stripaif "{WorkingDisk}"{Targ} -o {Targ} -s {Targ}.sym
	SetVersion "{WorkingDisk}"{Targ} -t vers -i 1 -sv 2 -sr 0  -verstring ' ^, � 3DO Company 1994'
	if not `exists "{AppsDir}"`
		newFolder "{AppsDir}"
	end
	move -y {Targ} "{AppsDir}"
	move -y {Targ}.sym "{AppsDir}"


#####################################
#	Additional Target Dependencies
#####################################

{ObjectDir}{App}.c.o			�	{App}.make

