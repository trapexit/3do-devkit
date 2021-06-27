/*
	File:		LoadSoundFX.c

	Contains:	xxx put contents here xxx

	Written by:	Jay

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	  9/6/93	JAY		change nNumVoices parameter in LoadInstrument to 0 for
									DragonTail6 and removed search directories for dsp files and
									conditional compiles for old systems

	To Do:
*/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

#if DEBUG
#define DIAGNOSTIC(s)		kprintf("Error: %s\n", s)
#define DIAGSTRING(s,t)		kprintf("Fyi: %s %s\n", s ,t )
#else
#define DIAGNOSTIC(s)
#define DIAGSTRING(s,t)
#endif

Item LoadSoundFX(char* sFilename, int nNumVoices, SoundInfo *theSoundInfo)
	{
	char name[32];
	int i;
	
	memset(theSoundInfo,0,sizeof(theSoundInfo));

	strcpy(name,"$audio/dsp/sampler.dsp");
	theSoundInfo->iSoundEffect = LoadInstrument(name, 0, 0);
	if (theSoundInfo->iSoundEffect < 0)
		{
		DIAGNOSTIC("Cannot load sampler instrument");
		return theSoundInfo->iSoundTemplate;
		}
		
	theSoundInfo->iSoundData = LoadSample(sFilename);
	if (theSoundInfo->iSoundData < 0)
		{
		DIAGSTRING("Cannot load sample", sFilename);
		UnloadInstrument(theSoundInfo->iSoundEffect);
		return theSoundInfo->iSoundData;
		}

	theSoundInfo->iSoundAttachment = AttachSample(theSoundInfo->iSoundEffect, theSoundInfo->iSoundData, 0);
	if (theSoundInfo->iSoundAttachment < 0)
		{
		DIAGSTRING("Cannot attach sample", sFilename);
		UnloadSample(theSoundInfo->iSoundData);
		UnloadInstrument(theSoundInfo->iSoundEffect);
		return theSoundInfo->iSoundAttachment;
		}

	return theSoundInfo->iSoundEffect;
	}
