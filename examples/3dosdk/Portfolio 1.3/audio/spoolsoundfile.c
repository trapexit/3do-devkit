/* $Id: spoolsoundfile.c,v 1.6 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************
**
** Spool a Sound File from Disk.
**
** This example creates a background thread that runs independantly
** from the foreground task.  You can communicate with it via signals.
** This could be built into a fancier soundfile server using Messages.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include "audio.h"
#include "debug.h"
#include "filefunctions.h"
#include "music.h"
#include "operror.h"
#include "types.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

#define MAXAMPLITUDE (0x7FFF)

/*
** Allocate enough space so that you don't get stack overflows.
** An overflow will be characterized by seemingly random crashes
** that defy all attempts at logical analysis.  You might want to
** start big then reduce the size till you crash, then double it.
*/
#define STACKSIZE (10000)

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, Result);                           \
      goto error;                                                             \
    }

#define CHECKPTR(val, name)                                                   \
  if (val == 0)                                                               \
    {                                                                         \
      Result = -1;                                                            \
      ERR (("Failure in %s\n", name));                                        \
      goto error;                                                             \
    }

#define NUMBLOCKS (64)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)
#define NUMBUFFS (2)

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
  int32 Priority;
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
  CHECKRESULT (gSignal1, "AllocSignal");

  Priority = 180;
  SpoolerThread = CreateThread ("SoundSpooler", Priority, SpoolSoundFileThread,
                                STACKSIZE);
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

int32
PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps)
{
  int32 Result = 0;
  SoundFilePlayer *sfp;
  int32 SignalIn, SignalsNeeded;
  int32 LoopCount;

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
