#####################################
##
## Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
## This material contains confidential information that is the property of The 3DO Company.
## Any unauthorized duplication, disclosure or use is prohibited.
## $Id: colorecho.make,v 1.7 1995/01/16 19:46:30 gyld Exp $
##
#####################################

#####################################
#	Symbol definitions
#####################################

App				= colorecho
DebugFlag		= 1
AppsDir			= :Apps_Data:
ObjectDir		= :Objects:
CC				= armcc
LINK			= armlink
WorkingDisk		=

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

OBJECTS			=	 {ObjectDir}colorecho.c.o {ObjectDir}colorecho_tools.c.o "{3DOLibs}"cstartup.o

LIBS			=	 �
					"{3DOLibs}Lib3DO.lib" �
					"{3DOLibs}music.lib" �
					"{3DOLibs}audio.lib" �
					"{3DOLibs}operamath.lib" �
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
	if not `exists "{AppsDir}"`
		newFolder "{AppsDir}"
	end
	move -y {Targ} "{AppsDir}"
	move -y {Targ}.sym "{AppsDir}"

#####################################
#	Additional Target Dependencies
#####################################

{ObjectDir}colorecho.c.o			�	{App}.make
{ObjectDir}colorecho_tools.c.o			�	{App}.make
