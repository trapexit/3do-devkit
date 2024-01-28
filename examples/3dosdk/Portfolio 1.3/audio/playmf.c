/* $Id: playmf.c,v 1.35 1994/03/01 01:36:27 shawn Exp $ */
/*
** Play a MIDI File using Juggler and Score toolbox from the music.lib
** and Audio Folio.
**
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/
/*
** 930315 PLB Conforms to new API
** 930506 PLB Updated for 1.1.17 BBS release.
** 930702 PLB Updated for Dragon release.
** 931221 PLB Major overhaul. Reorganised into pmf routines.
**            Use timestamp scaling for tempo instead of SetAudioRate().
*/

#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

#include "audio.h"
#include "music.h"

#define VERSION "V1.0"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

/*****************************************************************/

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, val);                              \
      goto cleanup;                                                           \
    }

#define MAXPROGRAMNUM                                                         \
  (128) /* Save memory by lowering this to highest program number. */
#define MAX_SCORE_VOICES (8)
#define MIXER_NAME ("mixer8x2.dsp")

// Raise this value till the sound clips then drop it till you feel safe.
// (MAXDSPAMPLITUDE/MAX_SCORE_VOICES) is guaranteed safe if all you are
// doing is playing scores
#define MIXER_AMPLITUDE (MAXDSPAMPLITUDE / MAX_SCORE_VOICES) * 2

// Prototypes
ScoreContext *pmfSetupScoreContext (char *mapfile);
Jugglee *pmfLoadScore (ScoreContext *scon, char *scorefile);
Err pmfPlayScore (Jugglee *JglPtr, uint32 NumReps);
Err pmfCleanupScoreContext (ScoreContext *scon);
Err pmfUnloadScore (ScoreContext *scon, Jugglee *CollectionPtr);
Err pmfPlayScoreMute (Jugglee *JglPtr, uint32 NumReps);
Err PlayMIDIFile (char *scorefile, char *mapfile, int32 NumReps);

/*****************************************************************/
int
main (int argc, char *argv[])
{
  char *scorefile = "$phil/zap/zap.mf";
  char *mapfile = "$phil/zap/zap.pimap";
  int32 Result = 0;
  int32 NumReps;

  PRT (("Play MIDI File, %s\n", VERSION));
  PRT (("Usage: %s <MIDIFilename> <PIMapFilename> <NumReps>\n", argv[0]));
  PRT (("WARNING: Stack must be set to 6000 using modbin.\n"));

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (Result < 0)
    {
      PrintError (0, "init event utility", 0, Result);
      goto cleanup;
    }

  /* OpenMathFolio to get MathBase */
  Result = OpenMathFolio ();
  if (Result < 0)
    {
      PrintError (0, "open math folio", 0, Result);
      ERR (("Did you run operamath?\n"));
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

  /* Get files from command line. */
  if (argc > 1)
    scorefile = argv[1];
  if (argc > 2)
    mapfile = argv[2];
  NumReps = (argc > 3) ? atoi (argv[3]) : 1;

  Result = PlayMIDIFile (scorefile, mapfile, NumReps);
  CHECKRESULT (Result, "PlayMIDIFile");

cleanup:
  TermJuggler ();
  CloseAudioFolio ();
  KillEventUtility ();
  PRT (("%s finished.\n", argv[0]));
  return (int)Result;
}

/******************************************************************
** Create a ScoreContext, load a MIDIFile and play it.
******************************************************************/
Err
PlayMIDIFile (char *scorefile, char *mapfile, int32 NumReps)
{
  Jugglee *CollectionPtr;
  int32 Result;
  ScoreContext *scon;

  Result = -1;
  CollectionPtr = NULL;

  scon = pmfSetupScoreContext (mapfile);
  if (scon == NULL)
    goto cleanup;

  CollectionPtr = pmfLoadScore (scon, scorefile);
  if (CollectionPtr == NULL)
    goto cleanup;

  /* Play the score collection. */
  Result = pmfPlayScore (CollectionPtr, NumReps);

cleanup:
  pmfUnloadScore (scon, CollectionPtr);
  pmfCleanupScoreContext (scon);
  return Result;
}

/******************************************************************
** Create a ScoreContext, and load a PIMap from a text file.
******************************************************************/
ScoreContext *
pmfSetupScoreContext (char *mapfile)
{
  ScoreContext *scon;
  int32 Result;

  /* Create a context for interpreting a MIDI score and tracking notes. */
  scon = CreateScoreContext (MAXPROGRAMNUM);
  if (scon == NULL)
    {
      return NULL;
    }

  /* Specify a mixer to use for the score voices. */
  Result
      = InitScoreMixer (scon, MIXER_NAME, MAX_SCORE_VOICES, MIXER_AMPLITUDE);
  CHECKRESULT (Result, "InitScoreMixer");

  /* Load Instrument Templates from disk and fill Program Instrument Map. */
  /* As an alternative, you could use SetPIMapEntry() */
  Result = LoadPIMap (scon, mapfile);
  CHECKRESULT (Result, "LoadPIMap");

  return scon;

cleanup:
  pmfCleanupScoreContext (scon);
  return NULL;
}

/******************************************************************
** Scale the timestamps in a Juggler Sequence.
******************************************************************/
static int32
pmfScaleSequenceTimes (Sequence *Self, frac16 Scalar)
{
  Time *EventPtr;
  int32 Indx;
  TagArg Tags[4];
  int32 EventSize, Many;

  /* Get info from Sequence using Tags.  */
  Tags[0].ta_Tag = JGLR_TAG_MANY;
  Tags[1].ta_Tag = JGLR_TAG_EVENTS;
  Tags[2].ta_Tag = JGLR_TAG_EVENT_SIZE;
  Tags[3].ta_Tag = TAG_END;
  GetObjectInfo (Self, Tags);
  Many = (int32)Tags[0].ta_Arg;
  EventPtr = (Time *)Tags[1].ta_Arg;
  EventSize = (int32)Tags[2].ta_Arg;
  DBUG (("pmfScaleSequenceTimes: Many = %d, EventPtr = %d, EventSize = %d\n",
         Many, EventPtr, EventSize));

  /* Scan through array and scale each timestamp. */
  Indx = 0;
  while (Indx < Many)
    {
      /* Scale time stamp. */
      *EventPtr = MulSF16 ((*EventPtr), Scalar);

      /* Index to next Event */
      EventPtr = (Time *)(((char *)EventPtr) + EventSize);
      Indx++;
    }

  return 0;
}

/******************************************************************
** Create a collection and load a score into it from a MIDI File.
******************************************************************/
Jugglee *
pmfLoadScore (ScoreContext *scon, char *scorefile)
{
  MIDIFileParser MFParser;
  Jugglee *CollectionPtr;
  int32 Result;
  frac16 OriginalRate;
  TagArg Tags[2];
  frac16 Scalar;
  int32 i, Many;
  Jugglee *Seq;

  /* Create a collection and load MIDI File into it. */
  CollectionPtr = (Jugglee *)CreateObject (&CollectionClass);
  if (CollectionPtr == NULL)
    {
      ERR (("pmfLoadScore: Failure to create Collection\n"));
      goto cleanup;
    }

  /* Set context used to play collection. */
  Tags[0].ta_Tag = JGLR_TAG_CONTEXT;
  Tags[0].ta_Arg = (void *)scon;
  Tags[1].ta_Tag = TAG_END;
  SetObjectInfo (CollectionPtr, Tags);

  /* Load it from a MIDIFile */
  Result
      = MFLoadCollection (&MFParser, scorefile, (Collection *)CollectionPtr);
  if (Result)
    {
      ERR (("Error loading MIDI File = $%x\n", Result));
      goto cleanup;
    }

  /* Query how many sequences it contains. */
  Tags[0].ta_Tag = JGLR_TAG_MANY;
  GetObjectInfo (CollectionPtr, Tags);
  Many = (int32)Tags[0].ta_Arg;
  PRT (("Collection contains %d sequences.\n", Many));

  /* Don't change the audio clock because that might interfere with other
   * tasks. */
  OriginalRate = GetAudioRate ();
  Scalar = DivSF16 (OriginalRate, MFParser.mfp_Rate);
  PRT (("Scale sequence times. mfp_Rate = 0x%x, Scalar = 0x%x\n",
        MFParser.mfp_Rate, Scalar));

  for (i = 0; i < Many; i++)
    {
      Result = GetNthFromObject (CollectionPtr, i, &Seq);
      CHECKRESULT (Result, "GetNthFromCollection");
      /* WARNING - assumes contents of Collection are sequences. */
      Result = pmfScaleSequenceTimes ((Sequence *)Seq, Scalar);
      CHECKRESULT (Result, "ScaleSequenceTimes");
    }

  return CollectionPtr;

cleanup:
  pmfUnloadScore (scon, CollectionPtr);
  return NULL;
}

/******************************************************************
** Unload the collection which frees the sequences, then
** destroy collection.
******************************************************************/
Err
pmfUnloadScore (ScoreContext *scon, Jugglee *CollectionPtr)
{
  if (CollectionPtr != NULL)
    {
      MFUnloadCollection ((Collection *)CollectionPtr);
      DestroyObject ((COBObject *)CollectionPtr);
    }

  return 0;
}

/******************************************************************
** Unload the PIMap which frees the instruments and samples, then
** delete the ScoreContext.
******************************************************************/
Err
pmfCleanupScoreContext (ScoreContext *scon)
{
  if (scon != NULL)
    {
      UnloadPIMap (scon);
      TermScoreMixer (scon);
      DeleteScoreContext (scon);
    }

  return 0;
}

/*****************************************************************
** Play a Collection, a Sequence or any other Juggler object.
** This routine is likely to be carved up for use in your application.
*****************************************************************/
int32
pmfPlayScore (Jugglee *JglPtr, uint32 NumReps)
{
  AudioTime CurTime, NextTime;
  int32 Result;
  int32 NextSignals, CueSignal, SignalsGot;
  Item MyCue;
  uint32 Joy;
  int32 QuitNow;
  int32 IfTimerPending;
  ControlPadEventData cped;

  QuitNow = 0;
  NextSignals = 0;
  IfTimerPending = FALSE;

  /* Create a Cue for timing the score. */
  MyCue = CreateCue (NULL);
  CHECKRESULT (MyCue, "CreateCue");
  CueSignal = GetCueSignal (MyCue);

  /* Drive Juggler using Audio timer. */
  /* Delay start by adding ticks to avoid stutter on startup. Optional. */
  NextTime = GetAudioTime () + 40;
  CurTime = NextTime;

  DBUG (("pmfPlayScore: Start at time %d\n", NextTime));
  /* This tells the Juggler to process this object when BumpJuggler() is
  ** later called.  Multiple objects could be started and will be
  ** juggled by Juggler.
  */
  StartObject (JglPtr, NextTime, NumReps, NULL);

  do
    {
      /* Read current state of Control Pad. */
      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          PrintError (0, "read control pad in", "pmfPlayScore", Result);
        }
      Joy = cped.cped_ButtonBits;

      /* Forced Quit by hitting ControlX */
      if (Joy & ControlX)
        {
          StopObject (JglPtr, CurTime);
          QuitNow = TRUE;
        }
      else
        {
          /* Request a timer wake up at the next event time. */
          if (!IfTimerPending)
            {
              Result = SignalAtTime (MyCue, NextTime);
              if (Result < 0)
                return Result;
              IfTimerPending = TRUE;
            }

          /* Wait for timer signal or signal(s) from BumpJuggler */
          SignalsGot = WaitSignal (CueSignal | NextSignals);

          /* Did we wake up because of a timer signal? */
          if (SignalsGot & CueSignal)
            {
              IfTimerPending = FALSE;
              CurTime = NextTime;
            }
          else
            {
              /* Get current time to inform Juggler. */
              CurTime = GetAudioTime ();
            }

          /* Tell Juggler to process any pending events, eg. Notes. Result > 0
           * if done. */
          Result = BumpJuggler (CurTime, &NextTime, SignalsGot, &NextSignals);
        }
    }
  while ((Result == 0) && (QuitNow == 0));

  DeleteCue (MyCue);

cleanup:
  return Result;
}

/************** YOU MAY IGNORE THE FOLLOWING CODE ****************
** It was written to illustrate muting and pause functions
** and is probably more complicated than you need.
*****************************************************************/

/*****************************************************************
** Set the Mute flag for the Nth child of a collection.
*****************************************************************/
int32
MuteNthChild (Jugglee *JglPtr, int32 SeqNum, int32 IfMute)
{
  TagArg Tags[2];
  Jugglee *Child;
  int32 Result;

  DBUG (("Set mute of #%d to %d\n", SeqNum, IfMute));

  Result = GetNthFromObject (JglPtr, SeqNum, &Child);
  if (Result < 0)
    {
      ERR (("Jugglee has no %dth child.\n", SeqNum));
      return 0;
    }

  Tags[0].ta_Tag = JGLR_TAG_MUTE;
  Tags[0].ta_Arg = (void *)IfMute;
  Tags[1].ta_Tag = TAG_END;

  return SetObjectInfo (Child, Tags);
}

/*****************************************************************
** Play a Collection, a Sequence or any other Juggler object.
** Fancy version incorporates Muting and Pause tests.
*****************************************************************/
Err
pmfPlayScoreMute (Jugglee *JglPtr, uint32 NumReps)
{
  AudioTime CurTime, NextTime, PauseTime, ResumeTime, DeltaTime;
  int32 Result;
  int32 NextSignals, CueSignal, SignalsGot;
  Item MyCue;
  uint32 Joy;
  int32 QuitNow, i;
  int32 Muted[8]; /* Keep track of which ones are muted. */
  ControlPadEventData cped;

  for (i = 0; i < 8; i++)
    Muted[i] = 0;
  QuitNow = 0;
  NextSignals = 0;

  MyCue = CreateItem (MKNODEID (AUDIONODE, AUDIO_CUE_NODE), NULL);
  CHECKRESULT (MyCue, "CreateItem Cue");
  CueSignal = GetCueSignal (MyCue);

  /* Drive Juggler using Audio timer. */
  /* Delay start by 1/2 second by adding to avoid stutter on startup. */
  NextTime = GetAudioTime () + 120;
  CurTime = NextTime;
  DeltaTime = 0;

  DBUG (("pmfPlayScoreFancy: Start at time %d\n", NextTime));
  StartObject (JglPtr, NextTime, NumReps, NULL);

  do
    {
      /* Use Control Pad to experiment with Muting (optional). */
      /* Read current state of Control Pad. */
      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          ERR (("Error in GetControlPad\n"));
          PrintError (0, "read control pad in", "pmfPlayScoreMute", Result);
        }
      Joy = cped.cped_ButtonBits;

/* Define a macro to simplify code. */
#define TestMute(Mask, SeqNum)                                                \
  if ((Joy & Mask) && !Muted[SeqNum])                                         \
    {                                                                         \
      MuteNthChild (JglPtr, SeqNum, TRUE);                                    \
      Muted[SeqNum] = TRUE;                                                   \
    }                                                                         \
  else if (!(Joy & Mask) && Muted[SeqNum])                                    \
    {                                                                         \
      MuteNthChild (JglPtr, SeqNum, FALSE);                                   \
      Muted[SeqNum] = FALSE;                                                  \
    }
      TestMute (ControlA, 0);
      TestMute (ControlB, 1);
      TestMute (ControlC, 2);
      TestMute (ControlUp, 3);
      TestMute (ControlRight, 4);
      TestMute (ControlDown, 5);
      TestMute (ControlLeft, 6);

      if (Joy & ControlLeftShift)
        {
          PRT (("Time = 0x%x\n", GetAudioTime ()));
        }

      /* Pause by slipping Juggler time against clock time. */
      if (Joy & ControlRightShift)
        {
          PauseTime = GetAudioTime ();
          PRT (("Pause at %d\n", PauseTime));
          do
            {
              Result = GetControlPad (1, TRUE, &cped);
              if (Result < 0)
                {
                  PrintError (0, "read control pad in", "pmfPlayScoreMute",
                              Result);
                }
            }
          while ((cped.cped_ButtonBits & ControlRightShift) == 0);
          ResumeTime = GetAudioTime ();
          PRT (("Resume at %d\n", ResumeTime));
          DeltaTime += (ResumeTime - PauseTime);
          GetControlPad (1, TRUE, &cped);
        }

      if (Joy & ControlX) /* Forced Quit */
        {
          StopObject (JglPtr, CurTime);
          QuitNow = TRUE;
        }
      else
        {
          /* Request a timer wake up at the next event time. */
          Result = SignalAtTime (MyCue, NextTime + DeltaTime);
          if (Result < 0)
            return Result;

          /* Wait for timer signal or signal(s) from BumpJuggler */
          SignalsGot = WaitSignal (CueSignal | NextSignals);
          /* Sleeping now until we get signalled. */
          if (SignalsGot & CueSignal)
            {
              CurTime = NextTime;
            }
          else
            {
              CurTime = GetAudioTime () - DeltaTime;
            }

          /* Tell Juggler to do its thing. Result > 0 if done. */
          Result = BumpJuggler (CurTime, &NextTime, SignalsGot, &NextSignals);
        }
    }
  while ((Result == 0) && (QuitNow == 0));

  DeleteItem (MyCue);

cleanup:
  return Result;
}
