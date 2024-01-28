##########################################################################
#   File:       EZQSubscriber.make
#
#	Contains:	make file for building ControlSubscriber
#
#	Copyright © 1994 The 3DO Company
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
COptions		=	{GlobalCOptions} -dDEBUG={DebugFlag} -i ":" -i "{SubscriberDir}SubscriberUtilities:" ¶
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
All				Ä	{OBJECTS}

{ObjectDir}		Ä	:

.c.o			Ä	.c
	echo "	compiling {Default}.c with {GlobalCOptions} {RelBranchSwitch}"
	{CC} {COptions} -o {ObjectDir}{Default}.c.o {DepDir}{Default}.c {RelBranchSwitch}

.c.depends		Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c -M -c ¶
		| search -q -r "{3DOIncludes}" ¶
		| StreamEdit -e "1 delete; 1,$ Replace /{ObjectDir}/ '¶"¶{ObjectDir¶}¶"'; Replace /Ä/ '	Ä'" ¶
		>> "{MakeFileName}"

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the Äs, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "Depends".
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
	Find /¥[Â#]/ "{MakeFileName}"
	Replace Æ¤:° "¶n" "{MakeFileName}"

SaveNewMakefile			Ä
	Save "{MakeFileName}"
	
#####################################
#	make or build script Dependancies
#####################################
# Artifical target to force build of all subscriber object files
{Program}		Ä	{Objects}

# Target dependancy to rebuild when makefile or build script changes
#{ObjectDir}{Program}.c.o	Ä	{Program}.make {SubscriberDir}BuildSubscriberLib
{Objects}					Ä	{Program}.make {SubscriberDir}BuildSubscriberLib

#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

"{ObjectDir}"EZQSubscriber.c.o	Ä	:::DataStreamLib.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::DataStream.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::MsgUtils.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::MemPool.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::SubsChunkCommon.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::HaltChunk.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::DSStreamHeader.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::MsgUtils.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::MemPool.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::ThreadHelper.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::MakeName.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::DataStreamLib.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::SubsChunkCommon.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:EZQSubscriber.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"EZQSubscriber.c.o	Ä	:::DataStreamLib.h
