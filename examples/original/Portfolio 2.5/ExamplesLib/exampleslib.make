
#####################################
##
## Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
## This material contains confidential information that is the property of The 3DO Company.
## Any unauthorized duplication, disclosure or use is prohibited.
## $Id: exampleslib.make,v 1.5 1995/01/20 04:20:45 ceckhaus Exp $
##
#####################################

#   File:       ExamplesLib.lib.make
#   Target:     ExamplesLib.lib
#   Sources:    controlpad.c effectshandler.c vdlutil.c getvideoinfo.c loopstereosoundfile.c
#

#####################################
#	Symbol definitions
#####################################

Library			= ExamplesLib
DebugFlag		= 1
ObjectDir		= :Objects:
SourceDir		= :
CC				= armcc
LIBRARIAN		= armlib
WorkingDisk		=

#####################################
#	Default compiler options
#####################################

COptions			= -fa -zps0 -za1
CDebugOptions		= -g -d DEBUG={DebugFlag}
LOptions			= -c -o

#####################################
#	Object files
#####################################

OBJECTS			=	{ObjectDir}controlpad.c.o --
					{ObjectDir}effectshandler.c.o --
					{ObjectDir}vdlutil.c.o --
					{ObjectDir}getvideoinfo.c.o --
					{ObjectDir}loopstereosoundfile.c.o

#####################################
#	Default build rules
#####################################

All				A	{Library}.lib

{ObjectDir}		A	:

.c.o	A	.c
	{CC} -i "{3DOIncludes}" {COptions} {CDebugOptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

#####################################
#	Target build rules
#####################################

{Library} A {Library}.make {OBJECTS}
	{LIBRARIAN} {LOptions} --
				{Library}.lib	--
				{OBJECTS}

#####################################
#	Additional Target Dependencies
#####################################

{ObjectDir}controlpad.c.o			A	"{SourceDir}"controlpad.h
{ObjectDir}effectshandler.c.o		A	"{SourceDir}"effectshandler.h
{ObjectDir}vdlutil.c.o				A	"{SourceDir}"vdlutil.h
{ObjectDir}getvideoinfo.c.o			A	"{SourceDir}"getvideoinfo.h
{ObjectDir}loopstereosoundfile.c.o	A	"{SourceDir}"loopstereosoundfile.h
