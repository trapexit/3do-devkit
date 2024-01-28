/*****************************************************************************
 *	File:			LoadSoundEffect.c
 *
 *	Contains:		Routines to load/free sound effects.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	These routines ARE NOT thread-safe, due to the use of a static
 *variable.
 ****************************************************************************/

#include "Debug3DO.h"
#include "Utils3DO.h"
#include "audio.h"
#include "mem.h"

/* this points to the head of a list of all soundInfo's created by
 * LoadSoundEffect */
/* It is used by FreeSoundEffects() to clean up nicely */

static SoundInfo *SoundInfoHead;

Item
LoadSoundEffect (char *sFilename, int nNumVoices)
{
  SoundInfo *theSoundInfo;
  Item retval = -1;

  theSoundInfo = (SoundInfo *)ALLOCMEM (sizeof (SoundInfo), 0);

  retval = LoadSoundFX (sFilename, nNumVoices, theSoundInfo);
  if (retval < 0)
    {
      DIAGNOSE (("LoadSoundEffect failed"));
      FREEMEM (theSoundInfo, sizeof (SoundInfo));
    }
  else
    {
      theSoundInfo->next = SoundInfoHead;
      SoundInfoHead = theSoundInfo;
    }
  return retval;
}

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
