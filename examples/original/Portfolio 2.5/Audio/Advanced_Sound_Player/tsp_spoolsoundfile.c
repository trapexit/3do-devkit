
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: tsp_spoolsoundfile.c,v 1.18 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/tsp_spoolsoundfile
|||	tsp_spoolsoundfile - Plays an AIFF sound file from a thread using the
|||	    advanced sound player.
|||
|||	  Synopsis
|||
|||	    tsp_spoolsoundfile \<sound file> [\<num repeats>]
|||
|||	  Description
|||
|||	    Plays an AIFF sound file using a thread to manage playback.
|||
|||	    This is a variation on the original SpoolSoundfile program that
used the
|||	    original sound file playersee CreateSoundFilePlayer(). This version
uses
|||	    the Advanced Sound Player (see spCreatePlayer()), which supports
looping
|||	    sound file playback.
|||
|||	  Arguments
|||
|||	    sound file                   Name of an AIFF file to play.
|||
|||	    num repeats                  Optional number of times to repeat the
sound
|||	                                 file. Defaults to 1.
|||
|||	  Caveats
|||
|||	    Calls ScanSample() to get information used to pick a sample player
which
|||	    prevents playing AIFF files with sustain or release loops (see
|||	    ScanSample()Caveats).
|||
|||	  Associated Files
|||
|||	    tsp_spoolsoundfile.c
|||
|||	  Location
|||
|||	    examples/Audio/Advanced_Sound_Player
|||
|||	  See Also
|||
|||	    spCreatePlayer(), spoolsoundfile, tsp_algorithmic, tsp_rooms
|||
**/

#include <audio.h>
#include <kernel.h>
#include <mem.h>
#include <musicerror.h>
#include <score.h> /* SelectSamplePlayer() */
#include <soundplayer.h>
#include <stdio.h>

/* -------------------- Parameters */

#define NUMBLOCKS (32)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS * BLOCKSIZE)
#define NUMBUFFS (4)
#define MAXAMPLITUDE (0x7FFF)

/*
** Allocate enough space so that you don't get stack overflows.
** An overflow will be characterized by seemingly random crashes
** that defy all attempts at logical analysis.  You might want to
** start big then reduce the size till you crash, then double it.
*/
#define STACKSIZE (10000)
#define PRIORITY (180)

/* -------------------- Macros */

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

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

/* -------------------- Globals for SpoolSoundFileThread */

char *gFileName;
int32 gSignal1;
Item gMainTaskItem;
int32 gNumReps;

/* -------------------- Functions */

void SpoolSoundFile (char *fileName, int32 numRepeats);
void SpoolSoundFileThread (void);

/* -------------------- main() */

int
main (int argc, char *argv[])
{
  int32 Result = 0;

#ifdef MEMDEBUG
  Result = CreateMemDebug (MEMDEBUGF_ALLOC_PATTERNS | MEMDEBUGF_FREE_PATTERNS
                               | MEMDEBUGF_PAD_COOKIES
                               | MEMDEBUGF_CHECK_ALLOC_FAILURES
                               | MEMDEBUGF_KEEP_TASK_DATA,
                           NULL);
  CHECKRESULT (Result, "CreateMemDebug");
#endif

  /* Initialize audio, return if error. */
  Result = OpenAudioFolio ();
  CHECKRESULT (Result, "open audio folio");

  /* Get sample name from command line. */
  if (argc < 2)
    {
      PRT (("Usage: %s <samplefile> [numreps]\n", argv[0]));
      goto error;
    }

  /* do the demo */
  SpoolSoundFile (argv[1], argc > 2 ? atoi (argv[2]) : 1);

error:
  CloseAudioFolio ();

#ifdef MEMDEBUG
  DumpMemDebug (NULL);
  DeleteMemDebug ();
#endif

  return 0;
}

/*
    Sets up globals for background thread, starts it,
    then waits for its completion. Instead of just waiting
    for it to complete, this function could do other stuff
    after starting the thread.
*/
void
SpoolSoundFile (char *fileName, int32 numRepeats)
{
  Item SpoolerThread = 0;
  int32 Result = 0;

  /* put parameters into globals for thread */
  gFileName = fileName;
  gNumReps = numRepeats;

  PRT (("tsp_spoolsoundfile: Play file %s %d times.\n", gFileName, gNumReps));

  /* Get parent task Item so that thread can signal back. */
  gMainTaskItem = CURRENTTASK->t.n_Item;

  /* Allocate a signal for each thread to notify parent task. */
  gSignal1 = AllocSignal (0);
  CHECKSIGNAL (gSignal1, "AllocSignal");

  /* create the thread. execution can begin immediately */
  SpoolerThread = CreateThread ("SpoolSoundFileThread", PRIORITY,
                                SpoolSoundFileThread, STACKSIZE);
  CHECKRESULT (SpoolerThread, "CreateThread");

  /*
      Do nothing for now but we could easily go off and do other stuff here!.
      OR together signals from other sources for a multi event top level.
      In this example, we just wait for the thread to finish.
  */

  PRT (("tsp_spoolsoundfile: Foreground waiting for signal from background "
        "spooler.\n"));
  WaitSignal (gSignal1);
  PRT (("tsp_spoolsoundfile: Background spooler finished.\n"));

error:
  DeleteThread (SpoolerThread);
  PRT (("tsp_spoolsoundfile: Playback complete.\n"));
}

/* -------------------- SpoolSoundFileThread() */

int32 PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps);
Err LoopDecisionFunction (SPAction *resultAction, int32 *remainingCountP,
                          SPSound *sound, const char *markerName);
Err SelectFixedSamplerForFile (char **resultInstrumentName, char *fileName);

void
SpoolSoundFileThread (void)
{
  int32 Result;

  /* open the audio folio for this this thread. */
  Result = OpenAudioFolio ();
  CHECKRESULT (Result, "open audio folio");

  /* play the sound file passed thru the globals */
  Result = PlaySoundFile (gFileName, BUFSIZE, gNumReps);

error:
  SendSignal (gMainTaskItem, gSignal1);
  CloseAudioFolio ();
  WaitSignal (0);
  /* Waits forever. Don't return! Thread gets deleted by parent. */
}

int32
PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps)
{
  int32 Result = 0;
  char *samplername;
  Item samplerins = 0;
  Item outputins = 0;
  SPPlayer *player = NULL;
  SPSound *sound = NULL;
  int32 RemainingCount = NumReps;

  PRT (("tsp_spoolsoundfile thread: PlaySoundFile() using Advanced Sound "
        "Player. NumReps=%ld\n",
        NumReps));

  /* pick appropriate sample player */
  Result = SelectFixedSamplerForFile (&samplername, FileName);
  CHECKRESULT (Result, "select sample player");

  /* load and connect instruments */
  PRT (("tsp_spoolsoundfile thread: using '%s'\n", samplername));
  samplerins = LoadInstrument (samplername, 0, 100);
  CHECKRESULT (samplerins, "load sampler instrument");

  outputins = LoadInstrument ("directout.dsp", 0, 100);
  CHECKRESULT (outputins, "load output instrument");

  /* try mono connection */
  ConnectInstruments (samplerins, "Output", outputins, "InputLeft");
  Result = ConnectInstruments (samplerins, "Output", outputins, "InputRight");

  /* if that failed, try stereo connection */
  if (Result < 0)
    {
      Result = ConnectInstruments (samplerins, "LeftOutput", outputins,
                                   "InputLeft");
      CHECKRESULT (Result, "connect left");
      Result = ConnectInstruments (samplerins, "RightOutput", outputins,
                                   "InputRight");
      CHECKRESULT (Result, "connect right");
    }

  /* start output instrument */
  Result = StartInstrument (outputins, NULL);
  CHECKRESULT (Result, "start output");

  /* create player */
  Result = spCreatePlayer (&player, samplerins, NUMBUFFS, BufSize, NULL);
  CHECKRESULT (Result, "create player");

  Result = spAddSoundFile (&sound, player, FileName);
  if (Result < 0)
    {
      PrintError (NULL, "add sound", FileName, Result);
      goto error;
    }

  /* set up default action to loop sound */
  Result = spLoopSound (sound);
  CHECKRESULT (Result, "loop sound");

  /* Install decision function. */
  Result = spSetMarkerDecisionFunction (
      sound, SP_MARKER_NAME_END, (SPDecisionFunction)LoopDecisionFunction,
      &RemainingCount);
  CHECKRESULT (Result, "set marker decision function");

  /* start playing */
  Result = spStartReading (sound, SP_MARKER_NAME_BEGIN);
  CHECKRESULT (Result, "start reading");
  Result = spStartPlayingVA (player, AF_TAG_AMPLITUDE, MAXAMPLITUDE, TAG_END);
  CHECKRESULT (Result, "start playing");

  {
    const int32 playersigs = spGetPlayerSignalMask (player);

    while (spGetPlayerStatus (player) & SP_STATUS_F_BUFFER_ACTIVE)
      {
        const int32 sigs = WaitSignal (playersigs);

        Result = spService (player, sigs);
        CHECKRESULT (Result, "Service");
      }
  }

  spStop (player);
  PRT (("tsp_spoolsoundfile thread: done.\n"));

error:
  spDeletePlayer (player);
  UnloadInstrument (samplerins);
  UnloadInstrument (outputins);
  return Result;
}

/*
    This decision function decrements the RemainingCount variable.
    Normally it returns default, but when the count is exhausted it
    returns stop.
*/
Err
LoopDecisionFunction (SPAction *resultAction, int32 *remainingCountP,
                      SPSound *sound, const char *markerName)
{
  /* decrement remaining repeat count */
  (*remainingCountP)--;

  PRT (("tsp_spoolsoundfile thread: %d more times to go\n", *remainingCountP));

  /* loop back to beginning (default action) or stop if no more repeats */
  if (*remainingCountP <= 0)
    return spSetStopAction (resultAction);
  else
    return 0;
}

Err
SelectFixedSamplerForFile (char **resultSamplerName, char *fileName)
{
  char *InstrumentName = NULL;
  Item TempSample = 0;
  int32 Result = 0;

  /* validate resultSamplerName */
  if (!resultSamplerName)
    return ML_ERR_BADPTR;

  /* scan AIFF file to get an item for SelectSamplePlayer() */
  TempSample = ScanSample (fileName, 0);
  CHECKRESULT (TempSample, "ScanSample");

  /* find a suitable instrument */
  InstrumentName = SelectSamplePlayer (TempSample, FALSE);
  if (InstrumentName == NULL)
    {
      ERR (("No instrument to play that sample.\n"));
      Result = ML_ERR_UNSUPPORTED_SAMPLE;
      goto error;
    }

error:
  UnloadSample (TempSample);
  *resultSamplerName = InstrumentName;
  return Result;
}
