#
#	File:		AccessTest.Make
#
#	Contains:	xxx put contents here xxx
#
#	Written by:	Jay London
#
#	Copyright:	© 1993 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 <3>	 8/10/93	JML		Update 8/10/93 730am.
#
#	To Do:
#

AppName		= AccessTest
DebugFlag	= 1



### SOURCE DEBUGGING ON ###
COptions	= -g -fa -zps0 -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

### SOURCE DEBUGGING OFF ###
# COptions	= -zps0 -ff -za1 -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SOptions	= -bi -g -i "{3DOIncludes}"
LOptions	= -aif -r -b 0x00 -d

xC			= armcc
xAsm		= armasm {SOptions}

SrcApp=:
ObjApp=:Objects:

Libs			=	"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}music.lib"		¶
					"{3DOLibs}codec.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"


CLibs=

OtherLibs=

OBJFiles= ¶
	"{ObjApp}"JoyPad.c.o				¶
	"{ObjApp}"AccessLib.c.o			¶
	"{ObjApp}"{AppName}.c.o				¶


##################################

"{AppName}" Ä	{OBJFiles} {OtherLibs} {Libs} {CLibs}
	armlink -o "{AppName}" {LOptions} ¶
		"{3DOLibs}cstartup.o"		¶
		{OBJFiles}¶
		{OtherLibs}¶
		{Libs} {PLibs} {CLibs}
	ModBin {AppName} -stack 4000 -debug
	SetFile {AppName} -c 'EaDJ' -t 'PROJ'
	stripaif "{AppName}" -o "{AppName}"
	duplicate {AppName} "{3DORemote}" {3DOAutodup}
	duplicate {AppName}.sym "{3DORemote}" {3DOAutodup}

##################################

"{ObjApp}"AccessLib.c.o Ä "{SrcApp}"AccessLib.c¶
	"{SrcApp}"AccessLib.h
 {xC} -o "{ObjApp}"AccessLib.c.o "{SrcApp}"AccessLib.c {COptions}

"{ObjApp}"JoyPad.c.o Ä "{SrcApp}"JoyPad.c¶
	"{SrcApp}"JoyPad.h
 {xC} -o "{ObjApp}"JoyPad.c.o "{SrcApp}"JoyPad.c {COptions}

"{ObjApp}"{AppName}.c.o Ä "{SrcApp}"{AppName}.c¶
	"{SrcApp}"AccessLib.h¶
	"{SrcApp}"JoyPad.h
 {xC} -o "{ObjApp}"{AppName}.c.o "{SrcApp}"{AppName}.c {COptions}

