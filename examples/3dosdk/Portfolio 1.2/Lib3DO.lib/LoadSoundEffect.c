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

/* this points to the head of a list of all soundInfo's created by LoadSoundEffect */
/* It is used by FreeSoundEffects() to clean up nicely */
static SoundInfo *SoundInfoHead;

Item LoadSoundEffect(char* sFilename, int nNumVoices)
{
	SoundInfo *theSoundInfo;
	Item retval = -1;
	
	theSoundInfo = (SoundInfo *)ALLOCMEM(sizeof(SoundInfo),0);
	
	retval = LoadSoundFX(sFilename, nNumVoices, theSoundInfo);
	if(retval < 0) {
		DIAGNOSTIC("LoadSoundEffect failed");
		FREEMEM( theSoundInfo, sizeof(SoundInfo) );
	} else {
		theSoundInfo->next = SoundInfoHead;
		SoundInfoHead = theSoundInfo;
	}
	return retval;
}

void
FreeSoundEffects(void)
{
	SoundInfo * si = SoundInfoHead;
	
	while(si) {
#ifndef CardinalChange
		DetachSample(si->iSoundAttachment);
#else
		DetachSample(si->iSoundEffect,si->iSoundData,0);
#endif
		UnloadSample(si->iSoundData);
		UnloadInstrument(si->iSoundEffect);
		si = si->next;
		FREEMEM(SoundInfoHead, sizeof(SoundInfo));
		SoundInfoHead = si;
	}
}
