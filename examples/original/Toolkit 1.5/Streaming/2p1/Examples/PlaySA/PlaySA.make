##########################################################################
#   File:       PlaySA.make
#
#	Contains:	make file for building PlaySA   (Play Streamed Audio)
#
#	Copyright (c) 1993 The 3DO Company
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
#	3/15/94		MPH		Changed link order & home of codec.lib.
#	10/25/93	rdg		Convert from TestCpakStream.make
#	10/20/93	jb		Move codec.lib above clib.lib so it can get to
#						malloc() and free()
#	9/20/93		jb		New today
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f PlaySA.make  ** Dev:Null > temp.makeout; temp.makeout ** Dev:Null; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	PlaySA
#DebugFlag		=	1
DebugFlag		=	0
AppsDir			=	:Apps & Data:
ObjectDir		=	:Objects:
StreamDir		=	:::DataStream:
SubscriberDir	=	{StreamDir}Subscribers:

CC				=	armcc
ASM				=	armasm
LINK			=	armlink
				
MakeFileName	=	{Program}.make
#####################################
#	Default compiler options
#####################################
RelativeBranchSwitch = -dRELATIVE_BRANCHING=0

COptions		=    -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes:" -d DEBUG={DebugFlag}
#COptions		= -g -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes:" -d DEBUG={DebugFlag}

LOptions		= -aif -r -b 0x00
#LOptions		= -aif -r -b 0x00 -d

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#		Object files
#####################################

LIBS			=	--
					"{SubscriberDir}subscriber.lib"		--
					"{StreamDir}dataacq.lib"	--
					"{StreamDir}ds.lib"			--
					"{3DOLibs}codec.lib" --
					"{3DOLibs}Lib3DO.lib"		--
					"{3DOLibs}operamath.lib"	--
					"{3DOLibs}filesystem.lib"	--
					"{3DOLibs}graphics.lib"		--
					"{3DOLibs}audio.lib"		--
					"{3DOLibs}music.lib"		--
					"{3DOLibs}input.lib"			--
					"{3DOLibs}clib.lib"			--
					"{3DOLibs}swi.lib"

OBJECTS			=	"{ObjectDir}{Program}.c.o"	--
					"{ObjectDir}PrepareStream.c.o"	--
					"{ObjectDir}PlaySSNDStream.c.o"  --
					"{ObjectDir}JoyPad.c.o"

OBJECTDEPENDS		=	"{ObjectDir}{Program}.c.depends"	--
					"{ObjectDir}PrepareStream.c.depends"	--
					"{ObjectDir}PlaySSNDStream.c.depends"  --
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
	move -y {Program} "{AppsDir}"{Program}

#####################################
#	Include file dependencies (DonOt change this line or put anything after this section.)
#####################################

"{ObjectDir}"PlaySA.c.o	A	:JoyPad.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SAudioSubscriber.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:MsgUtils.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:HaltChunk.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SAChannel.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SATemplates.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SAControlMsgs.h
"{ObjectDir}"PlaySA.c.o	A	:PlaySSNDStream.h
"{ObjectDir}"PlaySA.c.o	A	:PrepareStream.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:DataAcq.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:MarkerChunk.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SAudioSubscriber.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:ControlSubscriber.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"PlaySA.c.o	A	:::DataStream:DataStreamDebug.h
"{ObjectDir}"PrepareStream.c.o	A	:PrepareStream.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:MsgUtils.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:HaltChunk.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStreamDebug.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:MsgUtils.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:HaltChunk.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataAcq.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:MarkerChunk.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SAudioSubscriber.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SAChannel.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SATemplates.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SAControlMsgs.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:ControlSubscriber.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:PlaySSNDStream.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:PrepareStream.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataAcq.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:SAudioSubscriber.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:Subscribers:Includes:ControlSubscriber.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DataStreamDebug.h
"{ObjectDir}"PlaySSNDStream.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"JoyPad.c.o	A	:JoyPad.h
