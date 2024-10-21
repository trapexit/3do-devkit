/*****************************************************************************
 *	File:			LoadSoundFX.c
 *
 *	Contains:
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *	09/06/93  JAY	change nNumVoices parameter in LoadInstrument to 0 for
 *					DragonTail6 and removed search directories for
 *dsp files and conditional compiles for old systems
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "Debug3DO.h"
#include "Utils3DO.h"
#include "audio.h"
#include "string.h"

Item
LoadSoundFX (char *sFilename, int nNumVoices, SoundInfo *theSoundInfo)
{
  (void)nNumVoices; // apparently not used

  memset (theSoundInfo, 0, sizeof (theSoundInfo));

  theSoundInfo->iSoundEffect = LoadInstrument ("$audio/dsp/sampler.dsp", 0, 0);
  if (theSoundInfo->iSoundEffect < 0)
    {
      DIAGNOSE_SYSERR (theSoundInfo->iSoundEffect,
                       ("Cannot load sampler instrument"));
      return theSoundInfo->iSoundTemplate;
    }

  theSoundInfo->iSoundData = LoadSample (sFilename);
  if (theSoundInfo->iSoundData < 0)
    {
      DIAGNOSE_SYSERR (theSoundInfo->iSoundData,
                       ("Cannot load sample %s", sFilename));
      UnloadInstrument (theSoundInfo->iSoundEffect);
      return theSoundInfo->iSoundData;
    }

  theSoundInfo->iSoundAttachment
      = AttachSample (theSoundInfo->iSoundEffect, theSoundInfo->iSoundData, 0);
  if (theSoundInfo->iSoundAttachment < 0)
    {
      DIAGNOSE_SYSERR (theSoundInfo->iSoundAttachment,
                       ("Cannot attach sample %s", sFilename));
      UnloadSample (theSoundInfo->iSoundData);
      UnloadInstrument (theSoundInfo->iSoundEffect);
      return theSoundInfo->iSoundAttachment;
    }

  return theSoundInfo->iSoundEffect;
}
