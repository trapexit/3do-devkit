#
#	File:		TaskDemo.make
#
#	Contains:	TaskDemo make file
#
#	Written by:	Joe Buczek
#
#	Copyright:	© 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <7>	 8/15/93	JAY		fix a problem with modbin command line
#		 <6>	 8/15/93	JAY		add duplicate command to build
#		<4+>	 8/11/93	JAY		add modbin for build to set stack size
#		 <4>	 6/23/93	JAY		add check stack option (-zps0) to COptions. Needed for Dragon
#									Release (4B1)
#		 <3>	  4/7/93	JAY		make the make consistant
#		 <2>	 3/18/93	JAY		change Program to Application to avoid make errors
#		 <1>	 3/18/93	JAY		first checked in
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
Application		=	TaskDemo
DebugFlag		=	1
ObjectDir		=	:Objects:


#####################################
#	Default compiler options
#####################################
# USE THE FOLLOWING LINE FOR SYMBOLIC DEBUGGING
CDebugOptions	= -g
# USE THE FOLLOWING LINE FOR OPTIMIZED CODE
#CDebugOptions	=

COptions		= {CDebugOptions} -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

# USE THE FOLLOWING LINE FOR SYMBOLIC DEBUGGING
LDebugOptions	= -d		# turn on symbolic information
# USE THE FOLLOWING LINE FOR OPTIMIZED CODE
#LDebugOptions	=			# turn off symbolic information

LOptions		= {LDebugOptions} -aif -r -b 0x00 


#####################################
#		Object files
#####################################
LIBS			=	"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}clib.lib"		¶
					"{3DOLibs}swi.lib"

# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}{Application}.c.o"¶
					"{ObjectDir}SpawnThread.c.o"


#####################################
#	Default build rules
#####################################
All				Ä	{Application}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	ARMcc {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	ARMAsm {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Application}		ÄÄ	{Application}.make {OBJECTS}
	ARMlink {LOptions}					¶
			-o {Application}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Application} -c 'EaDJ' -t 'PROJ'
	modbin {Application} -stack 3000
	stripaif {Application} -o {Application}
	duplicate "{Application}" "{Application}".sym "{3DORemote}" {3DOAutodup}
	
#####################################
#	Include file dependencies
#####################################
#{Application}.c		Ä	{Application}.h 
SpawnThread.c	Ä	SpawnThread.h 
