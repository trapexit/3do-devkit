#
#	File:		textbox.make
#
#	Contains:	make file for building TextBox
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
#		 <1>	  4/7/93	JAY		first checked in
#				 2/15/93	JML		Updated for Magneto6
#				  2/8/93	JML		New today.
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
App				=	KFontViewer
DebugFlag		=	1
MagnetoFlag		= 	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LINK			=	armlink

#####################################
#	Default compiler options
#####################################
COptions		= -zps0 -za1  -i "::" -i "{3DOIncludes}" -d DEBUG={DebugFlag}

SOptions		= -bi -i "{3DOIncludes}"

LOptions		= -aif -r -b 0x00 -d -workspace 0x10000
#####################################
#		Object files
#####################################
LIBS			=	"::KTextBox.lib"			¶
					"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"			
					


# NOTE: Add object files here...
OBJECTS			=	"{ObjectDir}KFontViewer.c.o"	 ¶
					"{ObjectDir}TimeHelper.c.o"	


#####################################
#	Default build rules
#####################################
All				Ä	{App}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{App}		ÄÄ	{App}.make {OBJECTS}
	{LINK}	{LOptions}					¶
			-o {App}					¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {App} -c 'EaDJ' -t 'PROJ'
	modbin "{App}" -debug -stack 0x10000
	StripAIF {App} -o {App}
	duplicate -y "{App}" "{3DORemote}"

#####################################
#	Include file dependencies
#####################################
