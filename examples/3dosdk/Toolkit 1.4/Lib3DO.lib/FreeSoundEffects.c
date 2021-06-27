#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

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
