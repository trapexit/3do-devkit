##########################################################################
#   File:       squashsnd.make
#
#	Contains:	make file for squashsnd
#
#	Copyright © 1993-4 The 3DO Company
#
#   Target:     squashsnd
#   Sources:    squashsnd.c, twotoone.c, macfileio.c, iff_tools.c
#   Created:    Tuesday, June 15, 1993 6:33:20 PM
#
#	History:
#	05/31/94	DLD		Fixed some dependency problems
#						for the ToolKit directory structure.
##########################################################################

#  If you wish to build this tool into it's own directory, then
#		you should comment out the next two lines, instead
#		of the following two lines.

Destination	= :::
COPYTODESTINATION	= Move -y {PROGRAM} "{Destination}"{PROGRAM}
#Destination		= :
#COPYTODESTINATION	= 

PROGRAM				= SquashSnd
OBJECTS = {PROGRAM}.c.o			¶
		  twotoone.c.o			¶
		  macfileio.c.o			¶
		  iff_tools.c.o
		  
#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 

CFLAGS	= -r

{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {PROGRAM}.r {OBJECTS}
	Rez  -o {PROGRAM} {PROGRAM}.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{DEBUG_FLAGS} ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o {PROGRAM}
	{COPYTODESTINATION}

{PROGRAM}.c.o Ä {PROGRAM}.make {PROGRAM}.c
	C  {CFLAGS} {DEBUG_FLAGS} {PROGRAM}.c

twotoone.c.o Ä  {PROGRAM}.make twotoone.c twotoone.h 
	C  {CFLAGS} -mc68881 -elems881 {DEBUG_FLAGS} twotoone.c

macfileio.c.o Ä  {PROGRAM}.make macfileio.c macfileio.h
	C  {CFLAGS} {DEBUG_FLAGS} macfileio.c

iff_tools.c.o Ä  {PROGRAM}.make iff_tools.c iff_tools.h
	C  {CFLAGS} {DEBUG_FLAGS} iff_tools.c


#####################################
#	Include file dependencies
#####################################
{PROGRAM}.c.o		Ä	{PROGRAM}.h 
