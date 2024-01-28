#
#	File:		Lib3DO.lib.make
#
#	Contains:	Make file for Lib3DOLib
#
#	Written by:	Joe Buczek
#
#	Copyright:	© 1992 by The 3DO Company. All rights reserved.
#				This material constitutes confidential and proprietary
#				information of the 3DO Company and shall not be used by
#				any Person or for any purpose except as expressly
#				authorized in writing by the 3DO Company.
#
#	Change History (most recent first):
#
#		 		 10/10/93	ec		added leaks
#		 <5>	 8/13/93	JAY		clean up for 1p01 release
#		 <3>	  4/7/93	JAY		make the make consistant
#		 <2>	 3/18/93	JAY		turning debug on
#		 <1>	 3/17/93	JAY		This is the 2B1 release being checked in for the Cardinal build
#		<10>	 1/28/93	dsm		removed the d{threeDORelease}=1 define in the make file.
#		 <9>	  1/2/93	pgb		Modify Make to compile without symbols. Also switched include
#									order to look in local headers first.
#		 <8>	12/27/92	dsm		Modify Make to compile without symbols.
#		 <6>	12/14/92	JAY		remove -W compiler switch (suppress warnings)
#		 <5>	12/14/92	JAY		adding compiler switch threeDORelease to aid in
#									upgrading/downgrading between releases
#		 <4>	12/10/92	JML		Update for dependency on Parse3DO.h
#		 <3>	12/10/92	JML		Update for dependency on Portfolio.h
#		 <2>	12/10/92	JML		Updated to find interfaces and copy library to new location
#				 12/9/92	JML		For projector
#
#	To Do:
#

#####################################
#		Symbol definitions
#####################################
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	armcc
ASM				=	armasm
LIBRARIAN		=	armlib

#####################################
#	Default compiler options
#####################################
CDebugOptions	= -g		# turn on symbolic information
COptions		= {CDebugOptions} -zps0 -za1 -W ¶
					-i "{3DOIncludes}" ¶
					-d CHECK_FOR_MEMORY_LEAKS=1 ¶
					-dDEBUG={DebugFlag}

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o


#####################################
#		Object files
#####################################
OBJECTS			=	"{ObjectDir}OpenGraphics.c.o" ¶
			"{ObjectDir}OpenMacLink.c.o" ¶
			"{ObjectDir}OpenSPORT.c.o" ¶
			"{ObjectDir}OpenAudio.c.o" ¶
			"{ObjectDir}ShutDown.c.o" ¶
		"{ObjectDir}GetFileSize.c.o" ¶
		"{ObjectDir}ReadFile.c.o" ¶
		"{ObjectDir}FindChunk.c.o" ¶
		"{ObjectDir}GetChunk.c.o" ¶
		"{ObjectDir}setCCBPLUTPtr.c.o" ¶
		"{ObjectDir}setCCBDataPtr.c.o" ¶
		"{ObjectDir}LoadCel.c.o" ¶
		"{ObjectDir}OldLoadCel.c.o" ¶
		"{ObjectDir}FreeAnimFrames.c.o" ¶
		"{ObjectDir}LoadAnim.c.o" ¶
		"{ObjectDir}OldLoadAnim.c.o" ¶
		"{ObjectDir}LoadImage.c.o" ¶
		"{ObjectDir}LoadSoundFX.c.o" ¶
		"{ObjectDir}LoadSoundEffect.c.o" ¶
					"{ObjectDir}BlockFile.c.o" ¶
					"{ObjectDir}CenterCelOnScreen.c.o" ¶
					"{ObjectDir}CenterRectf16.c.o" ¶
					"{ObjectDir}DrawAnimCel.c.o" ¶
					"{ObjectDir}DrawImage.c.o" ¶
					"{ObjectDir}FadeFromBlack.c.o" ¶
					"{ObjectDir}FadeInCel.c.o" ¶
					"{ObjectDir}FadeOutCel.c.o" ¶
					"{ObjectDir}FadeToBlack.c.o" ¶
					"{ObjectDir}FrameBufferToCel.c.o" ¶
					"{ObjectDir}FreeBuffer.c.o" ¶
					"{ObjectDir}GetAnimCel.c.o" ¶
					"{ObjectDir}Leaks.c.o" ¶
					"{ObjectDir}LinkCel.c.o" ¶
					"{ObjectDir}LoadFile.c.o" ¶
					"{ObjectDir}MakeNewCel.c.o" ¶
					"{ObjectDir}MakeNewDupCCB.c.o" ¶
					"{ObjectDir}MapP2Cel.c.o" ¶
					"{ObjectDir}MoveCel.c.o" ¶
					"{ObjectDir}OffsetCel.c.o" ¶
					"{ObjectDir}ParseAnim.c.o" ¶
					"{ObjectDir}ParseCel.c.o" ¶
					"{ObjectDir}PreMoveCel.c.o" ¶
					"{ObjectDir}ReadControlPad.c.o" ¶
					"{ObjectDir}SetCelScale.c.o" ¶
					"{ObjectDir}SetChannel.c.o" ¶
					"{ObjectDir}SetFadeInCel.c.o" ¶
					"{ObjectDir}SetFadeOutCel.c.o" ¶
					"{ObjectDir}SetMixer.c.o" ¶
					"{ObjectDir}SetMixerChannel.c.o" ¶
					"{ObjectDir}SetQuad.c.o" ¶
					"{ObjectDir}SetRectf16.c.o" ¶
					"{ObjectDir}UMemory.c.o" ¶
					"{ObjectDir}UMemoryDebug.c.o" ¶
					"{ObjectDir}WriteMacFile.c.o" ¶


#####################################
#	Default build rules
#####################################
All				Ä	Lib3DO.lib

{ObjectDir}		Ä	:

.c.o			Ä	.c
	{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s

#####################################
#	Target build rules
#####################################
Lib3DO.lib		ÄÄ	Lib3DO.lib.make {OBJECTS}
	{LIBRARIAN}	{LOptions}		¶
				Lib3DO.lib		¶
				{OBJECTS}
#	Duplicate -y Lib3DO.lib {3DOLibs}Lib3DO.lib

#####################################
#	Include file dependencies
#####################################


Lib3DO.lib.c		Ä	{3DOIncludes}Portfolio.h ¶
					{3DOIncludes}Init3DO.h ¶
					{3DOIncludes}Parse3DO.h ¶
					{3DOIncludes}BlockFile.h ¶
					{3DOIncludes}Leaks.h ¶
					{3DOIncludes}UMemory.h ¶
					{3DOIncludes}Utils3DO.h 
					