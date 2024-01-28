#####################################
# Tweakable Symbol definitions
#####################################

Application		=	FontView

DebugFlag		=	1
CDebugOptions	=	-g -d DEBUG_MEMORY=1 
LDebugOptions	=	-d
StackSize		=	4096

#####################################
#		Object files
#####################################

OBJECTS			=	¶
					"{ObjectDir}{Application}.c.o"		¶
					"{ObjectDir}JoyPad.c.o"				¶


LIBS			=	¶
					"{3DOLibs}Lib3DO.lib"				¶
					"{3DOLibs}graphics.lib"				¶
					"{3DOLibs}audio.lib"				¶
					"{3DOLibs}input.lib"				¶
					"{3DOLibs}filesystem.lib"			¶
					"{3DOLibs}operamath.lib"			¶
					"{3DOLibs}clib.lib"					¶
					"{3DOLibs}swi.lib"					¶

					
#####################################
#	Default compiler options
#####################################

COptions		= -zps0 -zpv1 -za1 -fK -i "{3DOIncludes}" -d DEBUG={DebugFlag} {CDebugOptions} 
LOptions		= -aif -r -b 0x00 {LDebugOptions}

#####################################
#	Default build rules
#####################################

C			= armcc
LINK		= armlink 

ObjectDir	= :Objects:

"{ObjectDir}"	Ä	:

#####################################
#	Target build rules
#####################################

{Application} Ä	{LIBS} {OBJECTS}
	{LINK}	{LOptions}					¶
			-o "{Application}"			¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	setfile 	 "{Application}" 	-c 'EaDJ' -t 'PROJ'
	stripaif 	 "{Application}" 	-o "{Application}" -s "{3DORemote}{Application}.sym" || Set status 0
	modbin 		 "{Application}" -stack {StackSize} -debug
	duplicate -y "{Application}"  "{3DORemote}{Application}"
	
