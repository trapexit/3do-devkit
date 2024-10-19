
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: spoolsoundfile.c,v 1.17 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/spoolsoundfile
|||	spoolsoundfile - Plays an AIFF sound file from a thread using the
original
|||	    sound file player.
|||
|||	  Synopsis
|||
|||	     spoolsoundfile \<sound file> [\<num repeats>]
|||
|||	  Description
|||
|||	    Plays an AIFF sound file using a thread to manage playback.
|||
|||	    This example creates a background thread that runs independently
from the
|||	    foreground task. You can communicate with it via signals. This
could be
|||	    built into a fancier soundfile server using messages.
|||
|||	    This example uses the original Sound File Player in music.lib. See
|||	    tsp_spoolsoundfile for an example using the Advanced Sound Player
to do
|||	    the same thing.
|||
|||	  Arguments
|||
|||	    sound file                   Name of an AIFF file to play.
|||
|||	    num repeats                  Number of times to repeat the sound
file.
|||	                                 Defaults to 1.
|||
|||	  Associated Files
|||
|||	    spoolsoundfile.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
|||	  See Also
|||
|||	    playsoundfile, tsp_spoolsoundfile
|||
**/

#include "audio.h"
#include "debug.h"
#include "filefunctions.h"
#include "music.h"
#include "operror.h"
#include "types.h"
#include <stdio.h>

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

#define MAXAMPLITUDE (0x7FFF)

/*
** Control which version of PlaySoundFile() is demonstrated:
**
**  0: use non-rewinding version (uses multiple calls Open/CloseSoundFile())
**  1: use rewinding version (uses RewindSoundFile())
*/
#define USE_REWIND 1

/*
** Allocate enough space so that you don't get stack overflows.
** An overflow will be characterized by seemingly random crashes
** that defy all attempts at logical analysis.  You might want to
** start big then reduce the size till you crash, then double it.
*/
#define STACKSIZE (10000)
#define PRIORITY (180)

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, Result);                           \
      goto error;                                                             \
    }

#define CHECKSIGNAL(val, name)                                                \
  if (val <= 0)                                                               \
    {                                                                         \
      Result = val ? val : AF_ERR_NOSIGNAL;                                   \
      PrintError (NULL, name, NULL, Result);                                  \
      goto error;                                                             \
    }

#define CHECKPTR(val, name)                                                   \
  if (val == 0)                                                               \
    {                                                                         \
      Result = -1;                                                            \
      ERR (("Failure in %s\n", name));                                        \
      goto error;                                                             \
    }

#define NUMBLOCKS (16)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)
#define NUMBUFFS (4)

int32 PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps);
void SpoolSoundFileThread (void);

/********* Globals for Thread **********/
char *gFileName;
int32 gSignal1;
Item gMainTaskItem;
int32 gNumReps;

int
main (int argc, char *argv[])
{
  int32 Result = 0;
  Item SpoolerThread;

  PRT (("Usage: %s <samplefile>\n", argv[0]));

  /* Get sample name from command line. */
  if (argc > 1)
    {
      gFileName = (char *)argv[1];
    }
  else
    {
      gFileName = "/remote/aiff/Yosemite.aiff";
    }

  gNumReps = 1;
  if (argc > 2)
    gNumReps = atoi (argv[2]);
  PRT (("Play file %s %d times.\n", gFileName, gNumReps));

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Get parent task Item so that thread can signal back. */
  gMainTaskItem = KernelBase->kb_CurrentTask->t.n_Item;

  /* Allocate a signal for each thread to notify parent task. */
  gSignal1 = AllocSignal (0);
  CHECKSIGNAL (gSignal1, "AllocSignal");

  SpoolerThread = CreateThread ("SpoolSoundFileThread", PRIORITY,
                                SpoolSoundFileThread, STACKSIZE);
  CHECKRESULT (SpoolerThread, "CreateThread");

  /* Do nothing for now but we could easily go off and do other stuff here!. */
  /* OR together signals from other sources for a multi event top level */
  PRT (("Foreground waiting for signal from background spooler.\n"));
  WaitSignal (gSignal1);
  PRT (("Background spooler finished.\n"));

  CloseAudioFolio ();
  DeleteThread (SpoolerThread);
  PRT (("Playback complete.\n"));
error:
  return ((int)Result);
}

/**************************************************************************
** Entry point for background thread.
**************************************************************************/
void
SpoolSoundFileThread (void)
{
  int32 Result;

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
    }

  Result = PlaySoundFile (gFileName, BUFSIZE, gNumReps);
  SendSignal (gMainTaskItem, gSignal1);

  CloseAudioFolio ();
  WaitSignal (0); /* Waits forever. Don't return! */
}

#if !USE_REWIND

/* demonstrates playing a file multiple times by reopening it each time */

int32
PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps)
{
  int32 Result = 0;
  SoundFilePlayer *sfp;
  int32 SignalIn, SignalsNeeded;
  int32 LoopCount;

  PRT (("PlaySoundFile() non-rewinding version. NumReps=%ld\n", NumReps));

  for (LoopCount = 0; LoopCount < NumReps; LoopCount++)
    {
      PRT (("Loop #%d\n", LoopCount));

      sfp = OpenSoundFile (FileName, NUMBUFFS, BufSize);
      CHECKPTR (sfp, "OpenSoundFile");

      Result = StartSoundFile (sfp, MAXAMPLITUDE);
      CHECKRESULT (Result, "StartSoundFile");

      /* Keep playing until no more samples. */
      SignalIn = 0;
      SignalsNeeded = 0;
      do
        {
          if (SignalsNeeded)
            SignalIn = WaitSignal (SignalsNeeded);
          Result = ServiceSoundFile (sfp, SignalIn, &SignalsNeeded);
          CHECKRESULT (Result, "ServiceSoundFile");
        }
      while (SignalsNeeded);

      Result = StopSoundFile (sfp);
      CHECKRESULT (Result, "StopSoundFile");

      Result = CloseSoundFile (sfp);
      CHECKRESULT (Result, "CloseSoundFile");
    }

  return 0;

error:
  return (Result);
}

#else

/* demonstrates playing a file multiple times using RewindSoundFile() */

int32
PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps)
{
  int32 Result = 0;
  SoundFilePlayer *sfp;
  int32 SignalIn, SignalsNeeded;
  int32 LoopCount;

  PRT (("PlaySoundFile() rewinding version. NumReps=%ld\n", NumReps));

  sfp = OpenSoundFile (FileName, NUMBUFFS, BufSize);
  CHECKPTR (sfp, "OpenSoundFile");

  for (LoopCount = 0; LoopCount < NumReps; LoopCount++)
    {
      PRT (("Loop #%d\n", LoopCount));

      if (LoopCount)
        {
          PRT (("rewind.."));
          Result = RewindSoundFile (sfp);
          CHECKRESULT (Result, "RewindSoundFile");
        }

      PRT (("start.."));
      Result = StartSoundFile (sfp, MAXAMPLITUDE);
      CHECKRESULT (Result, "StartSoundFile");

      /* Keep playing until no more samples. */
      SignalIn = 0;
      SignalsNeeded = 0;
      do
        {
          if (SignalsNeeded)
            SignalIn = WaitSignal (SignalsNeeded);
          Result = ServiceSoundFile (sfp, SignalIn, &SignalsNeeded);
          CHECKRESULT (Result, "ServiceSoundFile");
        }
      while (SignalsNeeded);

      PRT (("stop.."));
      Result = StopSoundFile (sfp);
      CHECKRESULT (Result, "StopSoundFile");

      PRT (("loop complete.\n"));
    }

  Result = CloseSoundFile (sfp);
  CHECKRESULT (Result, "CloseSoundFile");

  return 0;

error:
  return (Result);
}

#endif
