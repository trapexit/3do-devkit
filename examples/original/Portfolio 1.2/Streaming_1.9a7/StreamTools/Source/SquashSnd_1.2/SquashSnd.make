#   File:       squashsnd.make
#   Target:     squashsnd
#   Sources:    squashsnd.c, twotoone.c, macfileio.c, iff_tools.c
#   Created:    Tuesday, June 15, 1993 6:33:20 PM

OBJECTS = squashsnd.c.o			¶
		  twotoone.c.o			¶
		  macfileio.c.o			¶
		  iff_tools.c.o
		  
#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 

CFLAGS	= -r

SquashSnd ÄÄ squashsnd.make {OBJECTS}
	Rez  -o SquashSnd squashsnd.r -a -ov
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
		-o SquashSnd

squashsnd.c.o Ä squashsnd.make squashsnd.c
	C  {CFLAGS} {DEBUG_FLAGS} squashsnd.c

twotoone.c.o Ä  squashsnd.make twotoone.c
	C  {CFLAGS} -mc68881 -elems881 {DEBUG_FLAGS} twotoone.c

macfileio.c.o Ä  squashsnd.make macfileio.c
	C  {CFLAGS} {DEBUG_FLAGS} macfileio.c

iff_tools.c.o Ä  squashsnd.make iff_tools.c
	C  {CFLAGS} {DEBUG_FLAGS} iff_tools.c


#####################################
#	Include file dependencies
#####################################
squashsnd.c		Ä	squashsnd.h 
twotoone.c		Ä	twotoone.h 
macfileio.c		Ä	macfileio.h 
iff_tools.c		Ä	iff_tools.h 
