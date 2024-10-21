/*****************************************************************************
 *	File:			OpenAudio.c
 *
 *	Contains:		Open audio folio and load/start a mixer.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *	09/06/93  JAY	change 2nd argument in the LoadInstrument to 0 for
 *					DragonTail6.  Removed conditional compiles and
 *search directories for finding dsp files.
 *	08/05/93  JAY	putting in place the final system directory scheme for
 *the Dragon Tail release. Affects how dsp, aiff files are referenced. Getting
 *rid of the multiple trials of pathnames (almost). Replacing with alias $audio
 *which should work forever in any environment
 *
 *	Implementation notes:
 *
 *	This function is not thread-safe.
 ****************************************************************************/

#include "Debug3DO.h"
#include "Init3DO.h"
#include "audio.h"
#include "string.h"

#define DEFAULT_MIXER_SETTING 0x2000

Item TheMixer = -1;

Boolean
OpenAudio (void)
{
  char LeftName[32];
  char RightName[32];
  Item left, right;
  char cChannel;

  strcpy (LeftName, "LeftGain0");
  strcpy (RightName, "RightGain0");

  if (OpenAudioFolio () < 0)
    {
      DIAGNOSE (("Cannot open audio folio"));
      return FALSE;
    }

  TheMixer = LoadInstrument ("$audio/dsp/mixer4x2.dsp", 0, 0);

  if (TheMixer < 0)
    {
      DIAGNOSE_SYSERR (TheMixer, ("Cannot load mixer instrument"));
      return FALSE;
    }

  for (cChannel = '0'; cChannel < '4'; cChannel++)
    {
      LeftName[8] = cChannel;
      RightName[9] = cChannel;
      left = GrabKnob (TheMixer, LeftName);
      right = GrabKnob (TheMixer, RightName);
      if (right < 0 || left < 0)
        {
          DIAGNOSE (("Cannot set mixer channel"));
          return FALSE;
        }
      TweakKnob (right, DEFAULT_MIXER_SETTING);
      TweakKnob (left, DEFAULT_MIXER_SETTING);
      ReleaseKnob (right);
      ReleaseKnob (left);
    }

  StartInstrument (TheMixer, NULL);

  return TRUE;
}
