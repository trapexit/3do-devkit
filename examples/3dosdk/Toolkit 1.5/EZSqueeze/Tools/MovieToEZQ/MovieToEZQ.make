#   File:       MovieToEZQ.make
#   Target:     MovieToEZQ
#   Sources:    MovieToEZQ.c
#   Created:    Sunday, June 27, 1993 4:41:00 PM
#  If you wish to build this tool into it's own directory, then
#		you should comment out the next two lines, instead
#		of the following two lines.

PROGRAM 			= MovieToEZQ
Destination	= :::
COPYTODESTINATION	= Duplicate -y {PROGRAM} "{Destination}"{PROGRAM}
#Destination		= :
#COPYTODESTINATION	= 


#OBJECTS = MovieToEZQ.c.o Display.c.o GWorlds.a.o writeVideo.c.o ¶
	
OBJECTS = MovieToEZQ.c.o Display.c.o GWorlds.a.o writeVideo.c.o 



COptions = -mc68020 -mc68881 -r -d makeformac

MathLibs = "{CLibraries}"Math881.o "{CLibraries}"Clib881.o "{CLibraries}"CSANELib881.o 
#MathLibs = "{CLibraries}"CSANELib.o "{CLibraries}"Math.o

	
{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {PROGRAM}.r {OBJECTS}
	Rez  -o MovieToEZQ MovieToEZQ.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		{MathLibs}¶
		Squeeze.lib  ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"MacRunTime.o ¶
		"{Libraries}"IntEnv.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o MovieToEZQ
	{COPYTODESTINATION}


MovieToEZQ.c.o Ä MovieToEZQ.make MovieToEZQ.c EZQ_Defs.h
	 C {COptions}  MovieToEZQ.c
Display.c.o Ä MovieToEZQ.make Display.c myGWorld.h
	 C {COptions}  Display.c
writeVideo.c.o Ä MovieToEZQ.make writeVideo.c myGWorld.h
	 C {COptions}  writeVideo.c
GWorlds.a.o Ä GWorlds.a
	Asm GWorlds.a

