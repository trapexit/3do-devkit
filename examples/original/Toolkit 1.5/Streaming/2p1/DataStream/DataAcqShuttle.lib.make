##########################################################################
#   File:       DataAcqShuttle.make
#
#	Contains:	make file for building DataAcqShuttle.lib
#
#	Copyright © 1992 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#
#	 7/07/94	dtc		Make objects dependent on make file.
#						Make DataAcq.c dependent on MarkerChunk.h.
#	 6/30/94	dtc		Changes were made but we don't know who did
#						them...
#						This is a copy of Subscriber.lib.make modified
#						to make it work for ShuttlePlayer.
#						Changed file name to reflect the correct name.
#						Added a line to set DebugFlag to 0 and changed
#						the make file to compile without the -g option.
#						Deleted duplicate Relative Branch Switch.
#	6/23/93		jb		Turn on stack checking for Dragon O/S release
#	6/15/93		jb		Add ItemPool.c to dependency list
#	4/5/93		jb		New today.
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f DataAcqShuttle.lib.make ·· Dev:Null > temp.makeout; temp.makeout ·· Dev:Null; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
Library			=	DataAcqShuttle
#DebugFlag			=	1
DebugFlag			=	0
ObjectDir			=	:Objects:
StreamDir			=	:
SubsIncludesDir		=	{StreamDir}Subscribers:Includes:
CC					=	armcc
ASM					=	armasm
LIBRARIAN			=	armlib
MakeFileName		=	{Library}.lib.make
RelativeBranchSwitch = -dRELATIVE_BRANCHING=1

#####################################
#	Default compiler options
#####################################
#COptions		= -g -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubsIncludesDir}" -d DEBUG={DebugFlag} {RelativeBranchSwitch}
COptions		=    -zps0 -za1 -i "{3DOIncludes}" -i "{StreamDir}" -i "{SubsIncludesDir}" -d DEBUG={DebugFlag} {RelativeBranchSwitch}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS			=	"{ObjectDir}DataAcq.c.o"		¶
					"{ObjectDir}ItemPool.c.o"		¶
					"{ObjectDir}ThreadHelper.c.o"

OBJECTDEPENDS	=	"{ObjectDir}DataAcq.c.depends"	¶
					"{ObjectDir}ItemPool.c.depends"	¶
					"{ObjectDir}ThreadHelper.c.depends"

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
#	Default build rules
#####################################
All				Ä	{Library}.lib

{ObjectDir}		Ä	:

.c.o			Ä	.c
	echo "	compiling {Default}.c with {RelativeBranchSwitch}"
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.c.depends		Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c -M -c ¶
		| search -q -r "{3DOIncludes}" ¶
		| StreamEdit -e "1 delete; 1,$ Replace /{ObjectDir}/ '¶"¶{ObjectDir¶}¶"'; Replace /Ä/ '	Ä'" ¶
		>> "{MakeFileName}"

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s

#####################################
#	Target build rules
#####################################
{Library}.lib		ÄÄ	{Library}.lib.make {OBJECTS}
	{LIBRARIAN}	{LOptions}			¶
				{Library}.lib		¶
				{OBJECTS}

#####################################
#	make or build script Dependancies
#####################################
# Target dependancy to rebuild when makefile or build script changes
{Library}.c.o	Ä	{MakeFileName}
 
#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

"{ObjectDir}"DataAcq.c.o	Ä	:DataAcq.h
"{ObjectDir}"DataAcq.c.o	Ä	:DataStream.h
"{ObjectDir}"DataAcq.c.o	Ä	:MsgUtils.h
"{ObjectDir}"DataAcq.c.o	Ä	:MemPool.h
"{ObjectDir}"DataAcq.c.o	Ä	:SubsChunkCommon.h
"{ObjectDir}"DataAcq.c.o	Ä	:HaltChunk.h
"{ObjectDir}"DataAcq.c.o	Ä	:DSStreamHeader.h
"{ObjectDir}"DataAcq.c.o	Ä	:ItemPool.h
"{ObjectDir}"DataAcq.c.o	Ä	:MarkerChunk.h
"{ObjectDir}"DataAcq.c.o	Ä	:Subscribers:Includes:SubscriberUtils.h
"{ObjectDir}"DataAcq.c.o	Ä	:DataStreamLib.h
"{ObjectDir}"DataAcq.c.o	Ä	:DataStream.h
"{ObjectDir}"DataAcq.c.o	Ä	:SubsChunkCommon.h
"{ObjectDir}"DataAcq.c.o	Ä	:MsgUtils.h
"{ObjectDir}"DataAcq.c.o	Ä	:MemPool.h
"{ObjectDir}"DataAcq.c.o	Ä	:ItemPool.h
"{ObjectDir}"DataAcq.c.o	Ä	:ThreadHelper.h
"{ObjectDir}"DataAcq.c.o	Ä	:MakeName.h
"{ObjectDir}"DataAcq.c.o	Ä	:MarkerChunk.h
"{ObjectDir}"ItemPool.c.o	Ä	:ItemPool.h
"{ObjectDir}"ThreadHelper.c.o	Ä	:ThreadHelper.h
"{ObjectDir}"ThreadHelper.c.o	Ä	:MakeName.h
