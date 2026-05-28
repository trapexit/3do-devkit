##########################################################################
#   File:       PlaySAnim.make
#
#	Contains:	make file for building PlaySAnim
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
#	9/20/93		jb		New today
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f PlaySAnim.make ** Dev:Null > temp.makeout; temp.makeout ** Dev:Null; delete -i temp.makeout

#
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	PlaySAnim
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

COptions		=   -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes:" -d DEBUG={DebugFlag}
#COptions		= -g -zps0 -za1 {RelativeBranchSwitch} -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubscriberDir}Includes:" -d DEBUG={DebugFlag}

LOptions		= -aif -r -b 0x00
#LOptions		= -aif -r -b 0x00 -d

SOptions		= -bi -g -i "{3DOIncludes}"

#####################################
#		Object files
#####################################

LIBS			=	--
					"{SubscriberDir}subscriber.lib"		--
					"{3DOLibs}Lib3DO.lib"		--
					"{3DOLibs}input.lib"		--
					"{3DOLibs}graphics.lib"		--
					"{3DOLibs}audio.lib"		--
					"{3DOLibs}filesystem.lib"	--
					"{3DOLibs}music.lib"		--
					"{3DOLibs}operamath.lib"	--
					"{3DOLibs}clib.lib"			--
					"{3DOLibs}swi.lib" 			--
					"{StreamDir}dataacq.lib"	--
					"{StreamDir}ds.lib"	

OBJECTS			=	"{ObjectDir}{Program}.c.o"		--
					"{ObjectDir}PrepareStream.c.o"	--
					"{ObjectDir}Utils.c.o"

OBJECTDEPENDS	=	"{ObjectDir}{Program}.c.depends"		--
					"{ObjectDir}PrepareStream.c.depends"	--
					"{ObjectDir}Utils.c.depends"

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
	stripaif "{program}" -o "{program}"
	modbin {program} -stack 0x4000 -debug
	if not `exists "{AppsDir}"`
		newFolder "{AppsDir}"
	end
	move -y {Program} "{AppsDir}"{Program}


#####################################
#	Include file dependencies (DonOt change this line or put anything after this section.)
#####################################

"{ObjectDir}"PlaySAnim.c.o	A	:PlaySAnim.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStreamDebug.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:MsgUtils.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:HaltChunk.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataAcq.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:MarkerChunk.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SAnimSubscriber.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SAudioSubscriber.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SAChannel.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SATemplates.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SAControlMsgs.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:ControlSubscriber.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:ItemPool.h
"{ObjectDir}"PlaySAnim.c.o	A	:PrepareStream.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"PlaySAnim.c.o	A	:PrepareStream.h
"{ObjectDir}"PlaySAnim.c.o	A	:::DataStream:Subscribers:Includes:SAnimSubscriber.h
"{ObjectDir}"PrepareStream.c.o	A	:PrepareStream.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:MsgUtils.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:HaltChunk.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DSStreamHeader.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SAnimSubscriber.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:SubsChunkCommon.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DataStreamLib.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SAudioSubscriber.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:DataStream.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SAChannel.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:MemPool.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SATemplates.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SAStreamChunks.h
"{ObjectDir}"PrepareStream.c.o	A	:::DataStream:Subscribers:Includes:SAControlMsgs.h
