##########################################################################
#   File:       ProtoPlayer.make
#
#	Contains:	make file for building ProtoPlayer 
#
#	Copyright (c) 1994 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	09/01/94	dtc		Add auto-generation of .c.o dependencies
#	7/07/94		rdg		modify to use new subscriber folder system
#	4/04/94		rdg		New today
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f ProtoPlayer.make  ** Dev:Null > temp.makeout; temp.makeout ** Dev:Null; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program				=	ProtoPlayer
#DebugFlag			=	1
DebugFlag			=	0
AppsDir				=	:Apps & Data:
ObjectDir			=	:Objects:
StreamDir			=	:::DataStream:
SubscriberDir		=	{StreamDir}Subscribers:

CC				=	armcc
ASM				=	armasm
LINK			=	armlink
				
MakeFileName		=	{Program}.make				
RelativeBranchSwitch = -dRELATIVE_BRANCHING=0
#####################################
#	Default compiler options
#####################################
COptions		=    -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes" -d DEBUG={DebugFlag}
#COptions		= -g -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes" -d DEBUG={DebugFlag}

LOptions		= -aif -r -b 0x00
#LOptions		= -aif -r -b 0x00 -d

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

LIBS			=	--
					"{SubscriberDir}subscriber.lib"			--
					"{StreamDir}dataacq.lib"				--
					"{StreamDir}ds.lib"						--
					"{3DOLibs}Lib3DO.lib"					--
					"{3DOLibs}input.lib"					--
					"{3DOLibs}graphics.lib"					--
					"{3DOLibs}audio.lib"					--
					"{3DOLibs}filesystem.lib"				--
					"{3DOLibs}music.lib"					--
					"{3DOLibs}operamath.lib"				--
					"{3DOLibs}codec.lib"					--
					"{3DOLibs}clib.lib"						--
					"{3DOLibs}swi.lib" 			

OBJECTS			=	"{ObjectDir}{Program}.c.o"				--
					"{ObjectDir}JoyPad.c.o"

OBJECTDEPENDS	=	"{ObjectDir}{Program}.c.depends"		--
					"{ObjectDir}JoyPad.c.depends"

#####################################
#	Default build rules
#####################################
All				A	{Program}

{ObjectDir}		A	:

.c.o			A	.c
	{CC} {DepDir}{Default}.c -o {ObjectDir}{Default}.c.o  {COptions}

.c.depends		A	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c -M -c --
		| search -q -r "{3DOIncludes}" --
		| StreamEdit -e "1 delete; 1,$ Replace /{ObjectDir}/ '--"--{ObjectDir--}--"'; Replace /A/ '	A'" --
		>> "{MakeFileName}"

.s.o			A	.s
	{ASM} {SOptions} -o {ObjectDir}{Default}.s.o {DepDir}{Default}.s

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the As, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					A	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	A
	# This is a workaround to make it work with the latest version of Make Tool (MPW V3.4a4).
	# Without the next line, find /.../ will break (MakeFileName) isn't resolved.
	set MakeFileName "{MakeFileName}"
	Open "{MakeFileName}"
	Find * "{MakeFileName}"
	Find /*#--tInclude file dependencies --(DonOt change this line or put anything after this section.--) deg/ "{MakeFileName}"
	Find /*[A#]/  "{MakeFileName}"
	Replace AE: deg "--n" "{MakeFileName}"

SaveNewMakefile			A
	Save "{MakeFileName}"

#####################################
#	Target build rules
#####################################
{Program}		A	{Program}.make {OBJECTS} {LIBS}
	{LINK}	{LOptions}					--
			-o {Program}				--
			"{3DOLibs}cstartup.o"		--
			{OBJECTS}					--
			{LIBS}
	SetFile {Program} -c 'EaDJ' -t 'PROJ'
	modbin {Program} -stack 0x4000 -debug
	stripaif {Program} -o {Program}
	if not `exists "{AppsDir}"`
		newFolder "{AppsDir}"
	end
	duplicate -y {Program} 	"{3DORemote}"
	if `exists "{Program}.sym"`
		duplicate -y {Program}.sym 	"{3DORemote}"
	end
	move -y {Program} "{AppsDir}"{Program}

#####################################
#	make or build script Dependancies
#####################################
# Target dependancy to rebuild when makefile or build script changes
{ObjectDir}{Program}.c.o	A	{MakeFileName}
 
#####################################
#	Include file dependencies (DonOt change this line or put anything after this section.)
#####################################

"{ObjectDir}"ProtoPlayer.c.o	A	:JoyPad.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DataStreamDebug.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:MsgUtils.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:HaltChunk.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DataAcq.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:MarkerChunk.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:ProtoSubscriber.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:ProtoErrors.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:ProtoTraceCodes.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:ProtoChannels.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:HaltChunk.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:ControlSubscriber.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"ProtoPlayer.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"JoyPad.c.o	A	:JoyPad.h
