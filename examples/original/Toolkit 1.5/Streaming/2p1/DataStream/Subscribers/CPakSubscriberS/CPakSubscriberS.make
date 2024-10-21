##########################################################################
#   File:       CPakSubscriberS.make
#
#	Contains:	makefile for building ControlSubscriber for Shuttle Player
#
#	Copyright © 1994 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	 8/22/94	dtc		Re-generated new dependencies list to include codec
#						from SubscriberUtilities.
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
#	make Depends -f CPakSubscriberS.make ·· Dev:Null> temp.makeout; temp.makeout ·· Dev:Null; delete -i temp.makeout
#
##########################################################################


#####################################
#		Symbol definitions
#####################################
Program			=	CPakSubscriberS
DebugFlag		=	0
#DebugFlag		=	1
CC				=	armcc
ASM				=	armasm

StreamDir		=	":::"
SubscriberDir	=	"::"

ObjectDir		=	{SubscriberDir}Objects:

#Objects			=	CPakSubscriberS.c.o
#Includes		=	CPakSubscriberS.h 

MakeFileName	=	"{Program}.make"

#####################################
#	Default compiler options
#####################################
COptions		=	{GlobalCOptions} -dDEBUG={DebugFlag} -i ":" -i "{SubscriberDir}SubscriberUtilities:" ¶
					-i "{StreamDir}"
#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

OBJECTS			=	"{ObjectDir}CPakSubscriberS.c.o"

OBJECTDEPENDS	=	"{ObjectDir}CPakSubscriberS.c.depends"

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

"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::DataStreamLib.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::DataStream.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::MsgUtils.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::MemPool.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::SubsChunkCommon.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::HaltChunk.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::DSStreamHeader.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::MsgUtils.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::MemPool.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::ThreadHelper.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::MakeName.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	::SubscriberUtilities:codec.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::DataStreamLib.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:::SubsChunkCommon.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	:CPakSubscriberS.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	::SubscriberUtilities:SubscriberUtils.h
"{ObjectDir}"CPakSubscriberS.c.o	Ä	::SubscriberUtilities:codec.h
