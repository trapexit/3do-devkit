
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: loopstereosoundfile.c,v 1.3 1995/01/06 01:04:15 ceckhaus Exp $
**
******************************************************************************/

#include "loopstereosoundfile.h"
#include "audio.h"
#include "debug3do.h"
#include "soundplayer.h"
#include "types.h"

#define NUMBUFFERS 4
#define NUMBLOCKS (32)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)

Err
LoopStereoSoundFile (const char *filename)
/*
        Play a stereo sound file in a dedicated endless loop.

        Loads and connects sample player instrument to an output instrument,
   sets up a sound player to play the sound file in a continuous loop, and then
        enters an infinite loop to play the sound file.

        Returns an error code (a negative number) if any operation fails.
*/
{

  int32 Result = -1;
  SPPlayer *player = NULL;
  SPSound *sound = NULL;
  Item samplerins = 0;
  Item outputins = 0;

  /* set up instruments */
  if ((samplerins = LoadInstrument ("fixedstereosample.dsp", 0, 100)) < 0)
    {
      DIAGNOSE_SYSERR (samplerins, ("Can't load fixedstereosample.dsp\n"));
      Result = samplerins;
      goto CLEANUP;
    }
  if ((outputins = LoadInstrument ("directout.dsp", 0, 100)) < 0)
    {
      DIAGNOSE_SYSERR (outputins, ("Can't load directout.dsp\n"));
      Result = outputins;
      goto CLEANUP;
    }
  if ((Result
       = ConnectInstruments (samplerins, "LeftOutput", outputins, "InputLeft"))
      < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't connect instruments\n"));
      goto CLEANUP;
    }
  if ((Result = ConnectInstruments (samplerins, "RightOutput", outputins,
                                    "InputRight"))
      < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't connect instruments\n"));
      goto CLEANUP;
    }
  if ((Result = StartInstrument (outputins, NULL)) < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't start output instrument\n"));
      goto CLEANUP;
    }

  /* set up sound player */
  if ((Result
       = spCreatePlayer (&player, samplerins, NUMBUFFERS, BUFSIZE, NULL))
      < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't create player\n"));
      goto CLEANUP;
    }
  if ((Result = spAddSoundFile (&sound, player, filename)) < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't add sound file\n"));
      goto CLEANUP;
    }
  if ((Result = spLoopSound (sound)) < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't loop sound file\n"));
      goto CLEANUP;
    }

  /* start reading and playing sound */
  if ((Result = spStartReading (sound, SP_MARKER_NAME_BEGIN)) < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't start reading\n"));
      goto CLEANUP;
    }
  if ((Result = spStartPlaying (player, NULL)) < 0)
    {
      DIAGNOSE_SYSERR (Result, ("Can't start playing\n"));
      goto CLEANUP;
    }

  /* service sound file and keep playing forever */
  {
    const int32 playersigs = spGetPlayerSignalMask (player);

    while (spGetPlayerStatus (player) & SP_STATUS_F_BUFFER_ACTIVE)
      {
        const int32 sigs = WaitSignal (playersigs);

        if ((Result = spService (player, sigs)) < 0)
          {
            DIAGNOSE_SYSERR (Result, ("Can't service the sound file\n"));
            goto CLEANUP;
          }
      }
  }

CLEANUP:
  spDeletePlayer (player);
  UnloadInstrument (outputins);
  UnloadInstrument (samplerins);

  return Result;
}
