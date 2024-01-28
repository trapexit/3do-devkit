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
#		 <6>	 7/12/94	JAY		general library cleanup.  Enabled most compiler warning options.
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
Version			=	1.5b1

#####################################
#	Default compiler options
#####################################

CDebugOptions	= -d DEBUG={DebugFlag} # -g

COptions		= {CDebugOptions} -zps0 -zpv1 -za1 -fa -fh -fk -i ":Includes:" -i "{3DOIncludes}" 

SOptions		= -bi -g -i "{3DOIncludes}"

LOptions		= -c -o

#####################################
#		Object files
#####################################

ObjectDir		=	:Objects:

"{ObjectDir}"	Ä	: 										¶
					:AnimUtils:								¶
					:CelUtils:								¶
					:DisplayUtils:							¶
					:IOUtils:								¶
					:MiscUtils:								¶
					:Objects:								¶
					:PhaseOutStuff:							¶
					:TextLib:								¶

AnimUtils =			"{ObjectDir}DrawAnimCel.c.o"			¶
					"{ObjectDir}GetAnimCel.c.o"				¶
					"{ObjectDir}LoadAnim.c.o"				¶
					"{ObjectDir}ParseAnim.c.o"				¶


CelUtils =			"{ObjectDir}CenterPoint.c.o"			¶
					"{ObjectDir}CenterRect.c.o"				¶
					"{ObjectDir}CenterRectCelIPoint.c.o"	¶
					"{ObjectDir}CenterRectCelFPoint.c.o"	¶
					"{ObjectDir}CenterRectCelRect.c.o"		¶
					"{ObjectDir}ChainCels.c.o" 				¶
					"{ObjectDir}CloneCel.c.o" 				¶
					"{ObjectDir}CreateBackdropCel.c.o"	 	¶
					"{ObjectDir}CreateCel.c.o" 				¶
					"{ObjectDir}CreateLRFormCel.c.o" 		¶
					"{ObjectDir}CreateSubrectCel.c.o" 		¶
					"{ObjectDir}CrossFadeCels.c.o"			¶
					"{ObjectDir}DeleteCelMagic.c.o" 		¶
					"{ObjectDir}GetCelBufferSize.c.o"		¶
					"{ObjectDir}InitCel.c.o"				¶
					"{ObjectDir}InterUnionRect.c.o"			¶
					"{ObjectDir}LinkCel.c.o"				¶
					"{ObjectDir}LoadCel.c.o" 				¶
					"{ObjectDir}MapCelToPoint.c.o"			¶
					"{ObjectDir}MapCelToRect.c.o" 			¶
					"{ObjectDir}MapCelToQuad.c.o" 			¶
					"{ObjectDir}OffsetInsetRect.c.o"		¶
					"{ObjectDir}OffsetCel.c.o"	 			¶
					"{ObjectDir}ParseCel.c.o" 				¶
					"{ObjectDir}PointConversions.c.o"		¶
					"{ObjectDir}RectConversions.c.o" 		¶
					"{ObjectDir}RectFromCel.c.o"			¶
					"{ObjectDir}RenderCelPixel.c.o"			¶
					
					
DisplayUtils =		"{ObjectDir}ClearBitmap.c.o"			¶
					"{ObjectDir}DrawImage.c.o"				¶
					"{ObjectDir}LoadImage.c.o"				¶
					"{ObjectDir}FadeFromBlack.c.o"			¶
					"{ObjectDir}FadeToBlack.c.o"			¶
					"{ObjectDir}OpenGraphics.c.o"			¶


IOUtils =			"{ObjectDir}AsyncLoadFile.c.o"			¶
					"{ObjectDir}BlockFile.c.o"				¶
					"{ObjectDir}ChangeInitialDir.c.o"		¶
					"{ObjectDir}GetFileSize.c.o"			¶
					"{ObjectDir}LoadFile.c.o"				¶
					"{ObjectDir}ReadFile.c.o"				¶
					"{ObjectDir}SaveFile.c.o"				¶
					"{ObjectDir}TimerServicesAPI.c.o"		¶
					"{ObjectDir}TimerServicesAPIU.c.o"		¶
					"{ObjectDir}TimerServicesAPIV.c.o"		¶
					"{ObjectDir}TimerServicesThread.c.o"	¶
					"{ObjectDir}TimerUtilsGetIOReq.c.o"		¶
					"{ObjectDir}TimerUtilsGetTime.c.o"		¶
					"{ObjectDir}TimerUtilsSleep.c.o"		¶
					"{ObjectDir}WriteMacFile.c.o"			¶


MiscUtils =			"{ObjectDir}Debug3DO.c.o"				¶
					"{ObjectDir}GetChunk.c.o"				¶
					"{ObjectDir}MSEvents.c.o"				¶
					"{ObjectDir}GetChunk.c.o"				¶
					"{ObjectDir}UMemory.c.o"				¶
					"{ObjectDir}UMemoryDebug.c.o"			¶


PhaseOutStuff =		"{ObjectDir}CenterCelOnScreen.c.o"		¶
					"{ObjectDir}CenterRectf16.c.o"			¶
					"{ObjectDir}FadeInCel.c.o"				¶
					"{ObjectDir}FadeOutCel.c.o"				¶
					"{ObjectDir}FrameBufferToCel.c.o"		¶
					"{ObjectDir}FreeBuffer.c.o"				¶
					"{ObjectDir}LoadSoundFX.c.o"			¶
					"{ObjectDir}LoadSoundEffect.c.o"		¶
					"{ObjectDir}MakeNewCel.c.o"				¶
					"{ObjectDir}MakeNewDupCCB.c.o"			¶
					"{ObjectDir}MapP2Cel.c.o"				¶
					"{ObjectDir}MoveCel.c.o"				¶
					"{ObjectDir}OldFreeAnimFrames.c.o"		¶
					"{ObjectDir}OldLoadAnim.c.o"			¶
					"{ObjectDir}OldLoadCel.c.o"				¶
					"{ObjectDir}OpenAudio.c.o"				¶
					"{ObjectDir}OpenMacLink.c.o"			¶
					"{ObjectDir}OpenSPORT.c.o"				¶
					"{ObjectDir}PreMoveCel.c.o"				¶
					"{ObjectDir}ReadControlPad.c.o"			¶
					"{ObjectDir}SetCelScale.c.o"			¶
					"{ObjectDir}SetChannel.c.o"				¶
					"{ObjectDir}SetFadeInCel.c.o"			¶
					"{ObjectDir}SetFadeOutCel.c.o"			¶
					"{ObjectDir}SetMixer.c.o"				¶
					"{ObjectDir}SetMixerChannel.c.o"		¶
					"{ObjectDir}SetQuad.c.o"				¶
					"{ObjectDir}SetRectf16.c.o"				¶
					"{ObjectDir}ShutDown.c.o"				¶

					
TextLib =			"{ObjectDir}taTextLib.c.o"				¶
					"{ObjectDir}FontBlit3To8_.s.o"			¶
					"{ObjectDir}FontBlit5To8_.s.o"			¶
					"{ObjectDir}FontLib.c.o"				¶
					"{ObjectDir}TextLib.c.o"				¶


OBJECTS			=	{AnimUtils}								¶
					{CelUtils}								¶
					{DisplayUtils}							¶
					{IOUtils}								¶
					{MiscUtils}								¶
					{PhaseOutStuff}							¶
					{TextLib}								¶


#####################################
#	Default build rules
#####################################

CC				=	armcc
ASM				=	armasm
LIBRARIAN		=	armlib

.c.o			Ä	.c
	{CC}  {DepDir}{Default}.c {COptions} -o {TargDir}{Default}.c.o 

.s.o			Ä	.s
	{ASM} {DepDir}{Default}.s {SOptions} -o {TargDir}{Default}.s.o 

#####################################
#	Target build rules
#####################################

Lib3DO.lib		Ä	{OBJECTS}
	{LIBRARIAN}	{LOptions} Lib3DO.lib {OBJECTS}
	SetVersion  Lib3DO.lib -t 'vers' -version {Version}
	Duplicate   Lib3DO.lib "{3DOLibs}Lib3DO.lib"

