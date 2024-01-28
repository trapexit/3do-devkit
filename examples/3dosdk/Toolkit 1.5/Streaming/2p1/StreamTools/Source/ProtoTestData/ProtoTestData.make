#   File:       ProtoTestData.make
#   Target:     ProtoTestData
#   Sources:    ProtoTestData.c
#   Created:    Monday, April 4, 1994 2:33:20 PM
#
#	History:
#	09/08/94	dtc		Added PROGRAM and COPYTODESTINATION
#						and fixed some dependency problems
#						for the ToolKit directory structure.
#	09/01/94	dtc		Linked with a new set of objectfiles
#						to make it work with MPW 3.4a4.
#
##########################################################################


#  If you wish to build this tool into it's own directory, then
#		you should comment out the next two lines, instead
#		of the following two lines.

PROGRAM			= ProtoTestData
Destination	= :::
COPYTODESTINATION	= Move -y {PROGRAM} "{Destination}"{PROGRAM}
#Destination		= :
#COPYTODESTINATION	= 

OBJECTS = {PROGRAM}.c.o

{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {OBJECTS}
	Rez  -o ProtoTestData ProtoTestData.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}Interface.o" ¶
		"{Libraries}ToolLibs.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}"IntEnv.o ¶
		-o {PROGRAM}
	{COPYTODESTINATION}

{PROGRAM}.c.o Ä {PROGRAM}.make {PROGRAM}.c
	 C -r  {PROGRAM}.c
