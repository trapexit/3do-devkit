##########################################################################
#   File:       DS.lib.make
#
#	Contains:	make file for building DS.lib
#
#	Copyright � 1993 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#
#	 8/15/94	dtc		Add auto-generation of .c.o dependencies
#	 7/07/94	dtc		Make objects dependent on make file.
#						Make DataStream.c, DataStreamDebug.c and DataStreamLib.c
#						dependent on HaltChunk.h
#	 6/30/94	dtc		Added a line to set DebugFlag to 0 and changed the make
#						file to compile without the -g option.
#	6/23/94		rdg		Changes were made but we don't know who did them...
#						 RELATIVE_BRANCHING switch was added to command line
#						 BlockFile.x were removed because they are now in Lib3DO
#						 Symbol definition for ObjectDirectory was added
#	7/24/93		jb		Get rid of 'H' and switch everything to 'DSxxxx'
#	7/10/93		jb		Add SemHelper.c and MakeName.c
#	6/23/93		jb		Turn on stack checking for Dragon O/S release
#	6/15/93		jb		Add DataStreamDebug.c
#	6/8/93		jb		Add ThreadHelper.c
#	5/12/93		jb		Add DataStreamLib.c
#	5/9/93		jb		Fix header file dependencies, remove TimerUtils.c
#	4/3/93		jb		New today.
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f DS.lib.make �� Dev:Null > temp.makeout �� Dev:Null; temp.makeout; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
Library			=	DS
#DebugFlag		=	1
DebugFlag		=	0
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LIBRARIAN		=	armlib
MakeFileName	=	{Library}.lib.make
RelativeBranchSwitch = -dRELATIVE_BRANCHING=0

#####################################
#	Default compiler options
#####################################
#COptions		= -g -zps0 -za1 -i "{3DOIncludes}"  -dDEBUG={DebugFlag} {RelativeBranchSwitch}
COptions		=    -zps0 -za1 -i "{3DOIncludes}"  -dDEBUG={DebugFlag} {RelativeBranchSwitch}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o


#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS			=	"{ObjectDir}DataStream.c.o"			�
					"{ObjectDir}DataStreamLib.c.o"		�
					"{ObjectDir}DataStreamDebug.c.o"	�
					"{ObjectDir}MemPool.c.o"			�
					"{ObjectDir}MsgUtils.c.o"			�
					"{ObjectDir}ThreadHelper.c.o"		�
					"{ObjectDir}SemHelper.c.o"			�
					"{ObjectDir}MakeName.c.o"

OBJECTDEPENDS	=	"{ObjectDir}DataStream.c.depends"		�
					"{ObjectDir}DataStreamLib.c.depends"	�
					"{ObjectDir}DataStreamDebug.c.depends"	�
					"{ObjectDir}MemPool.c.depends"			�
					"{ObjectDir}MsgUtils.c.depends"			�
					"{ObjectDir}ThreadHelper.c.depends"		�
					"{ObjectDir}SemHelper.c.depends"		�
					"{ObjectDir}MakeName.c.depends"


#####################################
#	Default build rules
#####################################
All				�	{Library}.lib

{ObjectDir}		�	:

.c.o			�	.c
	echo "	compiling {Default}.c with {RelativeBranchSwitch}"
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.c.depends		�	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c -M -c �
		| search -q -r "{3DOIncludes}" �
		| StreamEdit -e "1 delete; 1,$ Replace /{ObjectDir}/ '�"�{ObjectDir�}�"'; Replace /�/ '	�'" �
		>> "{MakeFileName}"

.s.o			�	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the �s, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "depends".
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
	Find /�[�#]/  "{MakeFileName}"
	Replace Ƥ:� "�n" "{MakeFileName}"

SaveNewMakefile			�
	Save "{MakeFileName}"

#####################################
#	Target build rules
#####################################
{Library}.lib		��	{Library}.lib.make {OBJECTS}
	{LIBRARIAN}	{LOptions}			�
				{Library}.lib		�
				{OBJECTS}

#####################################
#	make or build script Dependancies
#####################################
# Target dependancy to rebuild when makefile or build script changes
{Library}.c.o	�	{MakeFileName}
 
#####################################
#	Include file dependencies (Don�t change this line or put anything after this section.)
#####################################

"{ObjectDir}"DataStream.c.o	�	:DataStream.h
"{ObjectDir}"DataStream.c.o	�	:MsgUtils.h
"{ObjectDir}"DataStream.c.o	�	:MemPool.h
"{ObjectDir}"DataStream.c.o	�	:SubsChunkCommon.h
"{ObjectDir}"DataStream.c.o	�	:HaltChunk.h
"{ObjectDir}"DataStream.c.o	�	:DSStreamHeader.h
"{ObjectDir}"DataStream.c.o	�	:DataStreamLib.h
"{ObjectDir}"DataStream.c.o	�	:DataStream.h
"{ObjectDir}"DataStream.c.o	�	:ThreadHelper.h
"{ObjectDir}"DataStream.c.o	�	:MakeName.h
"{ObjectDir}"DataStream.c.o	�	:SemHelper.h
"{ObjectDir}"DataStreamLib.c.o	�	:DataStreamLib.h
"{ObjectDir}"DataStreamLib.c.o	�	:DataStream.h
"{ObjectDir}"DataStreamLib.c.o	�	:MsgUtils.h
"{ObjectDir}"DataStreamLib.c.o	�	:MemPool.h
"{ObjectDir}"DataStreamLib.c.o	�	:SubsChunkCommon.h
"{ObjectDir}"DataStreamLib.c.o	�	:HaltChunk.h
"{ObjectDir}"DataStreamLib.c.o	�	:DSStreamHeader.h
"{ObjectDir}"DataStreamDebug.c.o	�	:DataStreamDebug.h
"{ObjectDir}"DataStreamDebug.c.o	�	:DataStream.h
"{ObjectDir}"DataStreamDebug.c.o	�	:MsgUtils.h
"{ObjectDir}"DataStreamDebug.c.o	�	:MemPool.h
"{ObjectDir}"DataStreamDebug.c.o	�	:SubsChunkCommon.h
"{ObjectDir}"DataStreamDebug.c.o	�	:HaltChunk.h
"{ObjectDir}"DataStreamDebug.c.o	�	:DSStreamHeader.h
"{ObjectDir}"MemPool.c.o	�	:MemPool.h
"{ObjectDir}"MsgUtils.c.o	�	:MsgUtils.h
"{ObjectDir}"ThreadHelper.c.o	�	:ThreadHelper.h
"{ObjectDir}"ThreadHelper.c.o	�	:MakeName.h
"{ObjectDir}"SemHelper.c.o	�	:SemHelper.h
"{ObjectDir}"SemHelper.c.o	�	:MakeName.h
"{ObjectDir}"SemHelper.c.o	�	:ThreadHelper.h
"{ObjectDir}"MakeName.c.o	�	:MakeName.h
