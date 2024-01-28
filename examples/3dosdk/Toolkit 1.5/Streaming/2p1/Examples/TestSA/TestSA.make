##########################################################################
#   File:       TestSA.make
#
#	Contains:	make file for building TestDS
#
#	Copyright © 1993 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#
#	08/18/94	dtc		Add auto-generation of .c.o dependencies
#	08/17/94	dtc		Fixed compile option to look at Includes folder
#						in SubscriberDir for Subscribers header files.
#	 6/30/94	dtc		Turned DebugFlag off.
#	06/17/94	dtc		Removed link dependency on AppsDir and created
#						'Apps & Data' folder if it doesn't exists.
#						Added stripaif.
#	06/09/94	dtc		Defined RelativeBranchSwitch compile switch
#	05/27/94	DLD		Fixed some dependency problems
#						for the ToolKit directory structure.
#	05/16/94	dtc		Move application to 'Apps & Data:'
#	3/15/94		MPH		Changed location of codec.lib.  Corrected strange link order.
#	9/14/93		rdg		converted from TestDS
#	8/25/93		jb		Switch to using modbin tool to set stack & debug
#	8/10/93		jb		Add Extend_AIF command as required in Dragontail4
#	8/9/93		jb		Add SetDebugInit command as required in Dragontail3
#	7/24/93		jb		Get rid of 'H' in 'DSH'
#	6/18/93		jb		Move 'subscriber.lib' to first library in list
#						so silly linker will load references to Lib3DO.lib
#	6/15/93		jb		Update for release directory structure
#	4/17/93		jb		New today.
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f TestSA.make  ·· Dev:Null > temp.makeout; temp.makeout ·· Dev:Null; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	TestSA
#DebugFlag		=	1
DebugFlag		=	0
AppsDir			=	:Apps & Data:
ObjectDir		=	:Objects:
StreamDir		=	:::DataStream:
SubscriberDir	=	{StreamDir}Subscribers:
AudioSubsDir	= 	{SubscriberDir}SAudioSubscriber:

CC				=	armcc
ASM				=	armasm
LINK			=	armlink
MakeFileName	=	{Program}.make
				
#####################################
#	Default compiler options
#####################################
RelativeBranchSwitch = -dRELATIVE_BRANCHING=0

COptions		=    -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes:" -i "{AudioSubsDir}" -d DEBUG={DebugFlag}
#COptions		= -g -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes:" -i "{AudioSubsDir}" -d DEBUG={DebugFlag}

LOptions		= -aif -r -b 0x00
#LOptions		= -aif -r -b 0x00 -d

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#		Object files
#####################################

LIBS			=	¶
					"{SubscriberDir}subscriber.lib"		¶
					"{StreamDir}dataacq.lib"	¶
					"{StreamDir}ds.lib"			¶
					"{3DOLibs}codec.lib" ¶
					"{3DOLibs}Lib3DO.lib"		¶
					"{3DOLibs}operamath.lib"	¶
					"{3DOLibs}filesystem.lib"	¶
					"{3DOLibs}graphics.lib"		¶
					"{3DOLibs}audio.lib"		¶
					"{3DOLibs}music.lib"		¶
					"{3DOLibs}input.lib"		¶
					"{3DOLibs}clib.lib"			¶
					"{3DOLibs}swi.lib"

OBJECTS			=	"{ObjectDir}{Program}.c.o"

OBJECTDEPENDS	=	"{ObjectDir}{Program}.c.depends"

#####################################
#	Default build rules
#####################################
All				Ä	{Program}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {DepDir}{Default}.c -o {ObjectDir}{Default}.c.o  {COptions}

.c.depends		Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c -M -c ¶
		| search -q -r "{3DOIncludes}" ¶
		| StreamEdit -e "1 delete; 1,$ Replace /{ObjectDir}/ '¶"¶{ObjectDir¶}¶"'; Replace /Ä/ '	Ä'" ¶
		>> "{MakeFileName}"

.s.o			Ä	.s
	{ASM} {SOptions} -o {ObjectDir}{Default}.s.o {DepDir}{Default}.s

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the Äs, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					Ä	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	Ä
	# This is a workaround to make it work with the latest version of Make Tool (MPW V3.4a4).
	# Without the next line, find /.../ will break (MakeFileName) isn't resolved.
	set MakeFileName "{MakeFileName}"
	Open "{MakeFileName}"
	Find ¥ "{MakeFileName}"
	Find /¥#¶tInclude file dependencies ¶(DonÕt change this line or put anything after this section.¶)°/ "{MakeFileName}"
	Find /¥[Â#]/  "{MakeFileName}"
	Replace Æ¤:° "¶n" "{MakeFileName}"

SaveNewMakefile			Ä
	Save "{MakeFileName}"

#####################################
#	Target build rules
#####################################
{Program}		Ä	{Program}.make {OBJECTS} {LIBS}
	{LINK}	{LOptions}					¶
			-o {Program}				¶
			"{3DOLibs}cstartup.o"		¶
			{OBJECTS}					¶
			{LIBS}
	SetFile {Program} -c 'EaDJ' -t 'PROJ'
	modbin {program} -stack 0x4000 -debug
	stripaif {Program} -o {Program}
	if not `exists "{AppsDir}"`
		newFolder "{AppsDir}"
	end
	move -y {Program} "{AppsDir}"{Program}

#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DataStreamDebug.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DataStream.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:MsgUtils.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:MemPool.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:HaltChunk.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DSStreamHeader.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DataStreamLib.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DataStream.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DataAcq.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:ItemPool.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:MarkerChunk.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DataStreamLib.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SAudioSubscriber.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:DataStream.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SAChannel.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:MemPool.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SATemplates.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SAControlMsgs.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:ControlSubscriber.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"TestSA.c.o	Ä	:::DataStream:ItemPool.h
