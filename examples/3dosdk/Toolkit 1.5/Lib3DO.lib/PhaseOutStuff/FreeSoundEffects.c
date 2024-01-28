/*****************************************************************************
 *	File:			FreeSoundEffects.c
 *
 *	Contains:		Routine to free sound effects loaded via
 *LoadSoundEffect().
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This routine is not thread-safe.
 ****************************************************************************/

#include "Utils3DO.h"
#include "audio.h"
#include "mem.h"

void
FreeSoundEffects (void)
{
  SoundInfo *si = SoundInfoHead;

  while (si)
    {
#ifndef CardinalChange
      DetachSample (si->iSoundAttachment);
#else
      DetachSample (si->iSoundEffect, si->iSoundData, 0);
#endif
      UnloadSample (si->iSoundData);
      UnloadInstrument (si->iSoundEffect);
      si = si->next;
      FREEMEM (SoundInfoHead, sizeof (SoundInfo));
      SoundInfoHead = si;
    }
}
