/*****************************************************************************
 *	File:			SetMixer.c
 *
 *	Contains:		Routine to set volumes on the global mixer.
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

Boolean
SetMixer (int nChannel, int32 nVolume, int32 nBalance)
{
  int32 nLeftLevel, nRightLevel;
  int i;

  nVolume >>= 2; /* Split master volume over four channels */

  if (nBalance < 0x4000)
    {
      nLeftLevel = nVolume;
      nRightLevel = (nBalance * nVolume) >> 14;
    }
  else
    {
      nRightLevel = nVolume;
      nLeftLevel = ((0x8000 - nBalance) * nVolume) >> 14;
    }

  if (nChannel < 0)
    {
      i = 0;
      nChannel = 3;
    }
  else
    {
      i = nChannel;
    }

  for (; i <= nChannel; i++)
    {
      if (!SetMixerChannel (i, nLeftLevel, nRightLevel))
        return FALSE;
    }

  return TRUE;
}
