##########################################################################
#   File:       ProtoSubscriber.make
#
#	Contains:	makefile for building the ProtoSubscriber
#
#	Copyright � 1994 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	 8/15/94	dtc		Add auto-generation of .c.o dependencies
#	6/24/94		rdg		Let each subscriber have its own makefile...
#	6/23/94		rdg		Rename audio trace switches, they must be unique so
#						that other subscriber trace switches can be defined here
#	6/9/94		fyp		Define RelativeBranch switch here instead of at source.
#	6/8/94		fyp		Define SAudio trace switches here instead of in source.
#	4/04/94		rdg		Add ProtoSubscriber and SubscriberTraceUtils 
#	10/19/93	jb		Change to new audio subscriber file organization 
#						with all ".c" source in a separate directory.
#	6/23/93		jb		Turn on stack checking for Dragon O/S release
#	6/17/93		jb		Add JoinSubscriber.c
#	09/06/93	akm		New today.
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f ProtoSubscriber.make �� Dev:Null > temp.makeout; temp.makeout �� Dev:Null; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
Program			=	ProtoSubscriber
DebugFlag		=	0
#DebugFlag		=	1
CC				=	armcc
ASM				=	armasm

StreamDir		=	":::"
SubscriberDir	=	"::"

ObjectDir		=	{SubscriberDir}Objects:

#Objects		=	ProtoSubscriber.c.o ProtoChannels.c.o
#Includes		=	ProtoSubscriber.h ProtoChannels.h ProtoErrors.h ProtoTraceCodes.h

MakeFileName	=	"{Program}.make"
#####################################
# Trace switches, to help with real time debugging.
#
# XXXXX_TRACE_MAIN causes the given subscriber to leave timestamped trace data in a 
# circular buffer which can be examined using the debugger, or dumped with
# XXXXX_DUMP_TRACE_ON_STREAM_CLOSE - which dumps the buffer whenever a StreamClosing
# message is received.
#
# Don't forget link with the trace code utilities if you want to use them..
#
#####################################
 
ProtoTraceSwitches  = -dPROTO_TRACE_MAIN=0 -dPROTO_TRACE_CHANNELS=0 �
					-dPROTO_DUMP_TRACE_ON_STREAM_CLOSE=0 �
					-dPROTO_DUMP_TRACE_ON_STREAM_ABORT=0

#####################################
#	Default compiler options
#####################################
COptions		=	{GlobalCOptions}  -dDEBUG={DebugFlag} -i ":" -i "{StreamDir}" �
					-i "{SubscriberDir}SubscriberUtilities:"

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

OBJECTS			=	"{ObjectDir}ProtoSubscriber.c.o"		�
					"{ObjectDir}ProtoChannels.c.o"

OBJECTDEPENDS	=	"{ObjectDir}ProtoSubscriber.c.depends"	�
					"{ObjectDir}ProtoChannels.c.depends"

#####################################
#	Default build rules
#####################################
All				�	{OBJECTS}

{ObjectDir}		�	:

.c.o			�	.c
	echo "	compiling {Default}.c with {GlobalCOptions} {RelBranchSwitch}"
	{CC} {COptions} -o {ObjectDir}{Default}.c.o  {Default}.c {RelBranchSwitch} {ProtoTraceSwitches}
	
.c.depends		�	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c -M -c �
		| search -q -r "{3DOIncludes}" �
		| StreamEdit -e "1 delete; 1,$ Replace /{ObjectDir}/ '�"�{ObjectDir�}�"'; Replace /�/ '	�'" �
		>> "{MakeFileName}"

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the �s, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "Depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					�	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	�
	# This is a workaround to make it work with the latest version of Make Tool (MPW V3.4a4).
	# Without the next line, find /.../ will break (MakeFileName) isn't resolved.
	set MakeFileName "{MakeFileName}"
	Open "{MakeFileName}"
	Find � "{MakeFileName}"
	Find /�#�tInclude file dependencies �(Don�t change this line or put anything after this section.�)�/ "{MakeFileName}"
	Find /�[�#]/ "{MakeFileName}"
	Replace Ƥ:� "�n" "{MakeFileName}"

SaveNewMakefile			�
	Save "{MakeFileName}"
	
#####################################
#	make or build script Dependancies
#####################################
# Artifical target to force build of all subscriber object files
ProtoSubscriber			�	{Objects}
	
# Target dependancy to rebuild when makefile or build script changes
#{ObjectDir}ProtoSubscriber.c.o		�	ProtoSubscriber.make {SubscriberDir}BuildSubscriberLib
{Objects}					�	{Program}.make {SubscriberDir}BuildSubscriberLib

#####################################
#	Include file dependencies (Don�t change this line or put anything after this section.)
#####################################

"{ObjectDir}"ProtoSubscriber.c.o	�	:::DataStreamLib.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::DataStream.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::MsgUtils.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::MemPool.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::SubsChunkCommon.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::HaltChunk.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::DSStreamHeader.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::MsgUtils.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::MemPool.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::ThreadHelper.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::MakeName.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:ProtoErrors.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:ProtoChannels.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::MemPool.h
"{ObjectDir}"ProtoSubscriber.c.o	�	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::DataStreamLib.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::SubsChunkCommon.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:ProtoSubscriber.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::DataStream.h
"{ObjectDir}"ProtoSubscriber.c.o	�	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:ProtoErrors.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:ProtoTraceCodes.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:ProtoChannels.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:::HaltChunk.h
"{ObjectDir}"ProtoSubscriber.c.o	�	::SubscriberUtilities:SubscriberTraceUtils.h
"{ObjectDir}"ProtoSubscriber.c.o	�	:ProtoTraceCodes.h
"{ObjectDir}"ProtoChannels.c.o	�	:ProtoErrors.h
"{ObjectDir}"ProtoChannels.c.o	�	:ProtoChannels.h
"{ObjectDir}"ProtoChannels.c.o	�	:::MemPool.h
"{ObjectDir}"ProtoChannels.c.o	�	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"ProtoChannels.c.o	�	:::DataStreamLib.h
"{ObjectDir}"ProtoChannels.c.o	�	:::DataStream.h
"{ObjectDir}"ProtoChannels.c.o	�	:::MsgUtils.h
"{ObjectDir}"ProtoChannels.c.o	�	:::SubsChunkCommon.h
"{ObjectDir}"ProtoChannels.c.o	�	:::HaltChunk.h
"{ObjectDir}"ProtoChannels.c.o	�	:::DSStreamHeader.h
"{ObjectDir}"ProtoChannels.c.o	�	:::SubsChunkCommon.h
"{ObjectDir}"ProtoChannels.c.o	�	:ProtoSubscriber.h
"{ObjectDir}"ProtoChannels.c.o	�	:::DataStream.h
"{ObjectDir}"ProtoChannels.c.o	�	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"ProtoChannels.c.o	�	:ProtoErrors.h
"{ObjectDir}"ProtoChannels.c.o	�	:ProtoTraceCodes.h
"{ObjectDir}"ProtoChannels.c.o	�	:ProtoChannels.h
"{ObjectDir}"ProtoChannels.c.o	�	:::HaltChunk.h
"{ObjectDir}"ProtoChannels.c.o	�	::SubscriberUtilities:SubscriberTraceUtils.h
"{ObjectDir}"ProtoChannels.c.o	�	:ProtoTraceCodes.h
