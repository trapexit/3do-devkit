/*******************************************************************************************
 *	File:		LoopMIDIFiles.c
 *
 *	Contains:	Code to demonstrate looping and sequencing of multiple
 *MIDI files
 *
 *	Usage:		LoopMIDIFiles [ <PIMap> <MIDIFile1> <MIDIFile2>
 *<MIDIFile3>]
 *
 *				NOTE: If you specify any command line arguments you
 *must include all 4!
 *
 * 				This program loads three MIDI files and plays them
 *using Juggler and Score toolbox.
 *
 * 				The three files can be played back in varying order
 *using the A, B, and C buttons on the Joypad.
 *
 *				A Button = play sequence1 when current sequence is
 *finished B Button = play sequence2 when current sequence is finished C Button
 *= play sequence3 when current sequence is finished
 *
 *				X Button = stop and exit
 *
 *				Sequence1 is always first, and will loop until the
 *stop button is pressed, or a button is used to change the order of playback.
 *
 *
 *	Written by:		Software Attic
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 * 		9/22/94 	rdg 	Added workaround compile switch for
 *music.lib bug. 9/20/94 	rdg 	Updated for attic release. 5/06/94
 *rdg 	Adapted from PlayMF.c by Phil Burk
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.1d2"
#define PROGRAM_NAME_STRING "LoopMIDIFiles"

/* This switch works around a bug in the Portfolio 1.3 and 1.4 versions of
 * the music.lib.  Essentially the bug is that the value passed to
 * custom Juggler stop funtions in "TimeStopped" is bogus.  If this switch
 * is on, we call GetAudioTime() insteadof using the time passed to us
 * by the Juggler.  A fixed version of the music.lib will be shipped before
 * Christmas.
 */
#define USE_MUSIC_LIB_BUG_WORKAROUND 1

#include "stdio.h"

#include "Init3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "JoyPad.h"

#include "audio.h"
#include "music.h"

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
      result = val;                                                           \
      ERR (("Failure in %s: $%x\n", name, val));                              \
      PrintfSysErr (result);                                                  \
      goto cleanup;                                                           \
    }

/* Save memory by lowering MAXPROGRAMNUM to highest program number. */
#define MAXPROGRAMNUM (10)
#define MAX_SCORE_VOICES (8)
#define MIXER_NAME ("mixer8x2.dsp")

/* Raise this value till the sound clips then drop it till you feel safe.
 * (MAXDSPAMPLITUDE/MAX_SCORE_VOICES) is guaranteed safe if all you are
 * doing is playing scores, but it is exceedingly conservative.
 */
#define MIXER_AMPLITUDE (0x6000)

/* These are used to alter the playback speed while the scores are playing.
 * The up scalar is frac16-speak for 1.25 and the down .85.  These values
 * are multiplied by the duration value for every note in a collection.
 */
#define NOTE_DUR_UP_SCALAR ((1 << 16) | 0x4000)
#define NOTE_DUR_DOWN_SCALAR (0xB000)

/*****************************************************************
        Function Prototypes.
 *****************************************************************/
static void usage (void);

static Err StartItUp (void);
static void ShutItDown (void);

static ScoreContext *SetupScoreContext (char *mapfile);
static Err CleanupScoreContext (ScoreContext *scon);

static Collection *LoadScore (ScoreContext *scon, char *MIDIFile);
static Err LoopScores (Collection *Coll1, Collection *Coll2,
                       Collection *Coll3);
static Err UnloadScore (ScoreContext *scon, Collection *CollectionPtr);

static Err ScaleSequenceTimes (Sequence *Self, frac16 Scalar);
static Err ScaleCollectionTimes (Collection *Coll, frac16 Scalar);

static void MyStopFunction (Collection *obj, uint32 TimeStopped);

/*****************************************************************
 * Static global which contains a pointer to the next collection
 * to play...  there ought to be a better way to do this, but I
 * can't think of one right now.
 *****************************************************************/
static Collection *NextCollPtr;

/*********************************************************************************
 * Routine to output command line help
 */
void
usage (void)
{
  printf ("%s, Version %s\n", PROGRAM_NAME_STRING, PROGRAM_VERSION_STRING);
  printf ("Summary: Loops and Sequences 3 MIDIFiles using Juggler.\n");
  printf ("Usage: %s [<PIMap> <MIDIFile1> <MIDIFile2> <MIDIFile3>]\n",
          PROGRAM_NAME_STRING);
  printf ("NOTE: If you specify any command line arguments you must include "
          "all 4!\n");
}

/*****************************************************************
        Open folios and initalize stuff.
 *****************************************************************/
static Err
StartItUp (void)
{
  int32 result;

  /* OpenMathFolio to get MathBase */
  result = OpenMathFolio ();
  if (result < 0)
    {
      ERR (("OpenMathFolio() failed! Did you run operamath?\n"));
      PrintfSysErr (result);
      return (-1);
    }

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Required before playing scores. */
  InitJuggler ();

  /* Everything was cool */
  return 0;
}

/*****************************************************************
        Close folios and cleanup stuff.
 *****************************************************************/
static void
ShutItDown (void)
{
  TermJuggler ();
  CloseAudioFolio ();
}

/******************************************************************
 * Create a ScoreContext, and load a PIMap from a text file.
 ******************************************************************/
static ScoreContext *
SetupScoreContext (char *mapfile)
{
  int32 result;
  ScoreContext *scon;

  /* Create a context for interpreting a MIDI score and tracking notes. */
  scon = CreateScoreContext (MAXPROGRAMNUM);
  if (scon == NULL)
    return NULL;

  /* Specify a mixer to use for the score voices. */
  result
      = InitScoreMixer (scon, MIXER_NAME, MAX_SCORE_VOICES, MIXER_AMPLITUDE);
  CHECKRESULT (result, "InitScoreMixer");

  /* Load Instrument Templates from disk and fill Program Instrument Map.
   * As an alternative, you could use SetPIMapEntry()
   */
  result = LoadPIMap (scon, mapfile);
  CHECKRESULT (result, "LoadPIMap");

  return scon;

cleanup:
  CleanupScoreContext (scon);
  return NULL;
}

/******************************************************************
 * Scale the timestamps in a Juggler Sequence.  This allows you
 * to play the score without having to muck with the audio timer.
 ******************************************************************/
static Err
ScaleSequenceTimes (Sequence *Self, frac16 Scalar)
{
  int32 indx, eventSize, many;
  Time *eventPtr;
  TagArg Tags[4];

  /* Get info from Sequence using Tags.  */
  Tags[0].ta_Tag = JGLR_TAG_MANY;
  Tags[1].ta_Tag = JGLR_TAG_EVENTS;
  Tags[2].ta_Tag = JGLR_TAG_EVENT_SIZE;
  Tags[3].ta_Tag = TAG_END;

  GetObjectInfo (Self, Tags);

  many = (int32)Tags[0].ta_Arg;
  eventPtr = (Time *)Tags[1].ta_Arg;
  eventSize = (int32)Tags[2].ta_Arg;

  DBUG (("ScaleSequenceTimes(): objStartTime = %ld, many = %ld, eventPtr = "
         "%ld, eventSize = %ld\n",
         collStartTime, many, eventPtr, eventSize));

  /* Scan through array and scale each timestamp. */
  indx = 0;
  while (indx < many)
    {
      /* Scale time stamp.  Since we're using absolute time stamps,
       * we have to first subtract the collection start time from the event
       * time, scale it, and then add the start time back.
       */
      *eventPtr = MulSF16 ((*eventPtr), Scalar);

      /* Index to next Event */
      eventPtr = (Time *)(((char *)eventPtr) + eventSize);

      indx++;
    }

  return 0;
}

/******************************************************************
 * Scale the timestamps in all the sequences in a Juggler Collection.
 ******************************************************************/
static Err
ScaleCollectionTimes (Collection *Coll, frac16 Scalar)
{
  int32 result;
  int32 i, many;
  TagArg Tags[2];
  Sequence *seq;

  /* Query how many sequences it contains. */
  Tags[0].ta_Tag = JGLR_TAG_MANY;
  Tags[1].ta_Tag = TAG_END;

  GetObjectInfo (Coll, Tags);

  many = (int32)Tags[0].ta_Arg;

  for (i = 0; i < many; i++)
    {
      result = GetNthFromObject (Coll, i, &seq);
      CHECKRESULT (result, "GetNthFromCollection");

      /* WARNING - assumes contents of Collection are sequences. */
      result = ScaleSequenceTimes (seq, Scalar);
      CHECKRESULT (result, "ScaleSequenceTimes");
    }

cleanup:
  return result;
}

/******************************************************************
 * Create a collection and load a score into it from a MIDI File.
 ******************************************************************/
static Collection *
LoadScore (ScoreContext *scon, char *MIDIFile)
{
  int32 result;
  int32 many;
  frac16 originalRate;
  TagArg Tags[2];
  frac16 scalar;

  Collection *collectionPtr;

  MIDIFileParser MFParser;

  /* Create a collection and load MIDI File into it. */
  collectionPtr = (Collection *)CreateObject (&CollectionClass);
  if (collectionPtr == NULL)
    {
      ERR (("LoadScore: Failure to create Collection\n"));
      goto cleanup;
    }

  /* Set context used to play collection. */
  Tags[0].ta_Tag = JGLR_TAG_CONTEXT;
  Tags[0].ta_Arg = (void *)scon;
  Tags[1].ta_Tag = TAG_END;
  SetObjectInfo (collectionPtr, Tags);

  /* Load it from a MIDIFile */
  result = MFLoadCollection (&MFParser, MIDIFile, collectionPtr);
  if (result)
    {
      ERR (("Error loading MIDI File = $%x\n", result));
      goto cleanup;
    }

  /* Query how many sequences it contains. */
  Tags[0].ta_Tag = JGLR_TAG_MANY;
  GetObjectInfo (collectionPtr, Tags);

  many = (int32)Tags[0].ta_Arg;
  PRT (("Collection contains %d sequences.\n", many));

  /* Don't change the audio clock because that might interfere with other
   * tasks. */
  originalRate = GetAudioRate ();
  scalar = DivSF16 (originalRate, MFParser.mfp_Rate);

  /* Traverse all of the times stamps in every sequence in the collection and
   * scale them so that we dont have to muck with the Audio clock.
   */
  ScaleCollectionTimes (collectionPtr, scalar);
  PRT (("Scale sequence times. mfp_Rate = 0x%x, scalar = 0x%x\n",
        MFParser.mfp_Rate, scalar));

  return collectionPtr;

cleanup:
  UnloadScore (scon, collectionPtr);
  return NULL;
}

/******************************************************************
 * Unload the collection which frees the sequences, then
 * destroy collection.
 ******************************************************************/
static Err
UnloadScore (ScoreContext *scon, Collection *collectionPtr)
{
  if (collectionPtr != NULL)
    {
      MFUnloadCollection (collectionPtr);
      DestroyObject ((COBObject *)collectionPtr);
    }

  return 0;
}

/******************************************************************
 * Unload the PIMap which frees the instruments and samples, then
 * delete the ScoreContext.
 ******************************************************************/
static Err
CleanupScoreContext (ScoreContext *scon)
{
  if (scon != NULL)
    {
      UnloadPIMap (scon);
      TermScoreMixer (scon);
      DeleteScoreContext (scon);
    }

  return 0;
}

/******************************************************************
 * Callback function to pass to the juggler.  This is used to
 * chain the collections.  When a collection stops, it's StopFunction
 * is called, which in this case starts the next collection in the
 * chain using NextCollPtr.  This value is set in LoopScores() based on
 * Joypad input.
 ******************************************************************/
static void
MyStopFunction (Collection *obj, uint32 TimeStopped)
{
#if USE_MUSIC_LIB_BUG_WORKAROUND
  TimeStopped = (uint32)GetAudioTime ();
#endif

  DBUG (("MyStopFunction(): GetAudioTime() returned %ld, Time Stopped = %ld\n",
         (int32)GetAudioTime (), (int32)TimeStopped));

  StartObject (NextCollPtr, TimeStopped, 1, NULL);
}

/*****************************************************************
 * Main playback loop to service juggler and deal with user input.
 *****************************************************************/
static Err
LoopScores (Collection *Coll1, Collection *Coll2, Collection *Coll3)
{
  int32 result;                             /* success or failure */
  int32 numReps;                            /* times to repeat collection */
  int32 nextSignals, cueSignal, signalsGot; /* various signals */
  int32 timerPending;                       /* are we waiting for cue signal */
  Item myCue;                               /* cue for Juggler timing */

  AudioTime curTime, nextTime; /* for use with BumpJuggler() */

  JoyPadState jpState; /* JoyPad context */

  /* Initalize things. */
  timerPending = FALSE;
  numReps = 1;
  nextSignals = 0;

  /* Make sure we can hold down the shift keys */
  SetJoyPadContinuous (PADSHIFT, 1);

  /* Create a Cue for timing the score. */
  myCue = CreateCue (NULL);
  CHECKRESULT (myCue, "CreateCue");

  cueSignal = GetCueSignal (myCue);

  /* Drive Juggler using Audio timer.
   * Delay start by adding ticks to avoid stutter on startup. Optional.
   */
  nextTime = GetAudioTime () + 40;
  curTime = nextTime;

  DBUG (("LoopScores(): Start at time %d\n", nextTime));

  /* This tells the Juggler to process collection 1 when BumpJuggler() is
   * later called.
   */
  StartObject (Coll1, nextTime, numReps, NULL);

  /* Set up Coll1 to repeat until something else happens */
  NextCollPtr = Coll1;

  do
    {
      /* Read current state of Control Pad. */
      GetJoyPad (&jpState, 1);

      /* Forced Quit by hitting X button*/
      if (jpState.xBtn)
        {
          /* Need to figure out how to ask Juggler which object(s) are
           * currently executing
           */
          StopObject (Coll1, curTime);
          StopObject (Coll2, curTime);
          StopObject (Coll3, curTime);
          break;
        }

      /* Point NextCollPtr to the proper next collection based on button
       * input. MyStopFunction() will start this sequence when the
       * currently executing sequence is done.
       */
      if (jpState.aBtn)
        NextCollPtr = Coll1;

      if (jpState.bBtn)
        NextCollPtr = Coll2;

      if (jpState.cBtn)
        NextCollPtr = Coll3;

      /* Request a timer wake up at the next event time. */
      if (!timerPending)
        {
          result = SignalAtTime (myCue, nextTime);
          if (result < 0)
            return result;
          timerPending = TRUE;
        }

      /* Wait for timer signal or signal(s) from BumpJuggler */
      signalsGot = WaitSignal (cueSignal | nextSignals);

      /* Did we wake up because of a timer signal? */
      if (signalsGot & cueSignal)
        {
          timerPending = FALSE;
          curTime = nextTime;
        }
      else
        {
          /* Get current time to inform Juggler. */
          curTime = GetAudioTime ();
        }

      /* Tell Juggler to process any pending events, eg. Notes. result > 0 if
       * done. */
      result = BumpJuggler (curTime, &nextTime, signalsGot, &nextSignals);

#if 0
	PRT(("LoopScores(): Juggler Bumped - curTime = %ld, nextTime = %ld.\n", curTime, nextTime));
#endif
    }
  while (result == 0);

  AbortTimerCue (myCue);
  DeleteCue (myCue);

cleanup:
  return result;
}

/*****************************************************************
        Load MIDI files into juggler objects and play them.
 *****************************************************************/
int
main (int32 argc, char *argv[])
{
  int32 result;
  TagArg Tags[2];

  Collection *Coll1 = NULL;
  Collection *Coll2 = NULL;
  Collection *Coll3 = NULL;

  ScoreContext *scon = NULL;

  char *mapfile;

  char *midifile1;
  char *midifile2;
  char *midifile3;

  /* Parse command line args. */
  if (argc == 1)
    {
      mapfile = "/remote/LoopMIDIFilesData/AGroove.pimap";

      midifile1 = "/remote/LoopMIDIFilesData/AGroove1.mf";
      midifile2 = "/remote/LoopMIDIFilesData/AGroove2.mf";
      midifile3 = "/remote/LoopMIDIFilesData/AGroove3.mf";
    }
  else if (argc == 5)
    {
      mapfile = argv[1];

      midifile1 = argv[2];
      midifile2 = argv[3];
      midifile3 = argv[4];
    }
  else
    {
      usage ();
      return 1;
    }

  /* Initalize folios and stuff */
  if (StartItUp ())
    goto cleanup;

  /* Find and load the PIMap, this will cause all the samples and instruments
   * we need to be set up in the score context
   */
  scon = SetupScoreContext (mapfile);
  if (scon == NULL)
    goto cleanup;

  /* Load and parse the MIDIfiles into juggler objects */
  Coll1 = LoadScore (scon, midifile1);
  if (Coll1 == NULL)
    goto cleanup;

  Coll2 = LoadScore (scon, midifile2);
  if (Coll2 == NULL)
    goto cleanup;

  Coll3 = LoadScore (scon, midifile3);
  if (Coll3 == NULL)
    goto cleanup;

  /* Setup the User Stop Function. It will determine which sequence plays next.
   */
  Tags[0].ta_Tag = JGLR_TAG_STOP_FUNCTION;
  Tags[0].ta_Arg = (void *)&MyStopFunction;
  Tags[1].ta_Tag = TAG_END;

  SetObjectInfo (Coll1, Tags);
  SetObjectInfo (Coll2, Tags);
  SetObjectInfo (Coll3, Tags);

  /* Play the score collection. */
  result = LoopScores (Coll1, Coll2, Coll3);

cleanup:
  UnloadScore (scon, Coll1);
  UnloadScore (scon, Coll2);
  UnloadScore (scon, Coll3);

  CleanupScoreContext (scon);

  ShutItDown ();

  PRT (("%s finished.\n", argv[0]));

  return (int)result;
}
