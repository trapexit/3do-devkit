##########################################################################
#   File:       EZQSubscriber.make
#
#	Contains:	make file for building ControlSubscriber
#
#	Copyright (c) 1994 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	09/28/94	gmd		New today.
#
#
##########################################################################


#####################################
#		Symbol definitions
#####################################
Program			=	EZQSubscriber
DebugFlag		=	0
#DebugFlag		=	1
CC				=	armcc
ASM				=	armasm

StreamDir		=	":::"
SubscriberDir	=	"::"

ObjectDir		=	{SubscriberDir}Objects:

#Objects			=	EZQSubscriber.c.o
#Includes		=	EZQSubscriber.h 

MakeFileName	=	{Program}.make

#####################################
#	Default compiler options
#####################################
COptions		=	{GlobalCOptions} -dDEBUG={DebugFlag} -i ":" -i "{SubscriberDir}SubscriberUtilities:" --
					-i "{StreamDir}"

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

OBJECTS			=	"{ObjectDir}EZQSubscriber.c.o"

OBJECTDEPENDS	=	"{ObjectDir}EZQSubscriber.c.depends"

#####################################
#	Default build rules
#####################################
All				A	{OBJECTS}

{ObjectDir}		A	:

.c.o			A	.c
	echo "	compiling {Default}.c with {GlobalCOptions} {RelBranchSwitch}"
	{CC} {COptions} -o {ObjectDir}{Default}.c.o {DepDir}{Default}.c {RelBranchSwitch}

.c.depends		A	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c -M -c --
		| search -q -r "{3DOIncludes}" --
		| StreamEdit -e "1 delete; 1,$ Replace /{ObjectDir}/ '--"--{ObjectDir--}--"'; Replace /A/ '	A'" --
		>> "{MakeFileName}"

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the As, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "Depends".
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
	Find /*[A#]/ "{MakeFileName}"
	Replace AE: deg "--n" "{MakeFileName}"

SaveNewMakefile			A
	Save "{MakeFileName}"
	
#####################################
#	make or build script Dependancies
#####################################
# Artifical target to force build of all subscriber object files
{Program}		A	{Objects}

# Target dependancy to rebuild when makefile or build script changes
#{ObjectDir}{Program}.c.o	A	{Program}.make {SubscriberDir}BuildSubscriberLib
{Objects}					A	{Program}.make {SubscriberDir}BuildSubscriberLib

#####################################
#	Include file dependencies (DonOt change this line or put anything after this section.)
#####################################

"{ObjectDir}"EZQSubscriber.c.o	A	:::DataStreamLib.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::DataStream.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::MsgUtils.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::MemPool.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::SubsChunkCommon.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::HaltChunk.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::DSStreamHeader.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::MsgUtils.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::MemPool.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::ThreadHelper.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::MakeName.h
"{ObjectDir}"EZQSubscriber.c.o	A	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::DataStreamLib.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::SubsChunkCommon.h
"{ObjectDir}"EZQSubscriber.c.o	A	:EZQSubscriber.h
"{ObjectDir}"EZQSubscriber.c.o	A	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"EZQSubscriber.c.o	A	:::DataStreamLib.h
