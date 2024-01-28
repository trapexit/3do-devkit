
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: tj_canon.c,v 1.12 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/tj_canon
|||	tj_canon - Uses the juggler to create and play a semi-random canon.
|||
|||	  Synopsis
|||
|||	    tj_canon \<PIMap>
|||
|||	  Description
|||
|||	    This program shows how to create and play a simple canon using the
juggler
|||	    and score-playing routines.
|||
|||	    It creates a single melodic line, and stores it as a sequence to be
|||	    repeated from 1 to NUMLINES times.
|||
|||	  Arguments
|||
|||	    PIMap                        Name of a PIMap to use. This PIMap
should
|||	                                 have an instrument defined for program
|||	                                 number 1.
|||
|||	  Associated Files
|||
|||	    tj_canon.c
|||
|||	  Location
|||
|||	    examples/Audio/Juggler
|||
**/

#include "audio.h"
#include "event.h"
#include "music.h"
#include "operror.h"
#include "stdio.h"

#define VERSION "V24.0"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

#define DURATION (50)
#define NUMLINES (3)
#define REPEATCOL (16)
#define REPEATSEQ (2)

/*****************************************************************/

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      ERR (("Failure in %s: $%x\n", name, val));                              \
      PrintfSysErr (Result);                                                  \
      goto cleanup;                                                           \
    }

/* Macro to simplify error checking. */
#define CHECKPTR(ptr, name)                                                   \
  if (ptr == NULL)                                                            \
    {                                                                         \
      ERR (("Failure in %s\n", name));                                        \
      goto cleanup;                                                           \
    }

#define MAXPROGRAMNUM                                                         \
  (32) /* Save memory by lowering this to highest program. */
#define kMaxScoreVoices (8)

#define NUMNOTES (16)
#define NUMEVENTS (NUMNOTES * 2 + 1) /* On & Off + Final Off*/
static MIDIEvent CanonData[NUMEVENTS];

#define CANONMANY (sizeof (CanonData) / sizeof (MIDIEvent))

int32 PlayMyJugglee (Jugglee *JglPtr, char *mapfile);
int32 PlayJugglee (Jugglee *JglPtr, uint32 NumReps);
int32 PlayCanon (char *scorefile, int32 NumReps);

Err ComposeSequence (void);

/* This function is called when the Collection repeats. */
int32
UserRepeatFunc (Jugglee *Self, Time RepeatTime)
{
  ComposeSequence ();
  return 0;
}

/*****************************************************************/
int
main (int argc, char *argv[])
{
  char *pimapfile;
  int32 Result = 0;
  int32 NumReps;

  PRT (("Play a Canon, %s\n", VERSION));
  PRT (("Usage: %s <PIMap> [<num repeats>]\n", argv[0]));

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_FocusListener);
  if (Result < 0)
    {
      ERR (("main: error in InitEventUtility\n"));
      PrintfSysErr (Result);
      goto cleanup;
    }

  /* OpenMathFolio to get MathBase */
  Result = OpenMathFolio ();
  if (Result < 0)
    {
      ERR (("OpenMathFolio() failed! Did you run operamath?\n"));
      PrintfSysErr (Result);
      return (-1);
    }

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  InitJuggler ();

  /* Get arguments from command line. */
  if (argc < 2)
    goto cleanup;
  pimapfile = argv[1];
  NumReps = (argc > 2) ? atoi (argv[2]) : REPEATCOL;

  Result = PlayCanon (pimapfile, NumReps);
  CHECKRESULT (Result, "PlayCanon");

cleanup:
  TermJuggler ();
  CloseAudioFolio ();
  KillEventUtility ();
  return (int)Result;
}

/*****************************************************************/
/* Add Note to  */
Err
AddNote (MIDIEvent *MData, int32 Index, int32 Time, int32 Channel, int32 Note,
         int32 Vel)
{
  MIDIEvent *mev;

  mev = &MData[Index];
  mev->mev_Time = Time;
  mev->mev_Command = (unsigned char)(0x90 + Channel - 1);
  mev->mev_Data1 = (unsigned char)Note;
  mev->mev_Data2 = (unsigned char)Vel;
  mev->mev_Data3 = 0;

  return 0;
}

/*****************************************************************/
/* Generate Sequence of Notes for Canon */
Err
ComposeSequence (void)
{
  int32 i;
  int32 Channel = 1;
  int32 Vel = 64;
  int32 Note;
  int32 Scale[] = { 55, 60, 62, 65, 67, 69 };
  int32 Time = 0;
  int32 Duration;

  for (i = 0; i < NUMNOTES; i++)
    {
      Note = Scale[Choose (6)];
      Duration = (Choose (2) + 1) * DURATION;
      AddNote (CanonData, i * 2, Time, Channel, Note, Vel);
      AddNote (CanonData, (i * 2) + 1, Time + (Duration >> 1), Channel, Note,
               0);
      Time += Duration;
    }
  /* Pad sequence to end of time. */
  AddNote (CanonData, i * 2, Time, Channel, 0, 0);

  return 0;
}

/*****************************************************************/
Err
DumpSequence (MIDIEvent *MIDIData, int32 NumEvents)
{
  int32 i;
  uint32 Data, *DataPtr;

  for (i = 0; i < NumEvents; i++)
    {
      DataPtr = (uint32 *)&MIDIData[i].mev_Command;
      Data = *DataPtr;
      PRT (("%d: T=%d, M=0x%8X\n", i, MIDIData[i].mev_Time, Data));
    }

  return 0;
}

/*****************************************************************/
Sequence *
CreateCanonSequence (int32 Delay)
{
  Sequence *Seq;

  ComposeSequence ();

  Seq = (Sequence *)CreateObject (&SequenceClass);
  CHECKPTR (Seq, "CreateObject");

  {
    TagArg Tags[10];
    int32 i = 0;

    /* define TagList */
    Tags[i].ta_Tag = JGLR_TAG_INTERPRETER_FUNCTION;
    Tags[i++].ta_Arg = (TagData)InterpretMIDIEvent; /* From music.lib */
    Tags[i].ta_Tag = JGLR_TAG_MAX;
    Tags[i++].ta_Arg = (TagData)(NUMEVENTS);
    Tags[i].ta_Tag = JGLR_TAG_MANY;
    Tags[i++].ta_Arg = (TagData)(NUMEVENTS);
    Tags[i].ta_Tag = JGLR_TAG_EVENTS;
    Tags[i++].ta_Arg = (TagData)&CanonData[0];
    Tags[i].ta_Tag = JGLR_TAG_EVENT_SIZE;
    Tags[i++].ta_Arg = (TagData)sizeof (MIDIEvent);
    Tags[i].ta_Tag = JGLR_TAG_START_DELAY;
    Tags[i++].ta_Arg = (TagData)Delay;
    Tags[i].ta_Tag = JGLR_TAG_STOP_DELAY;
    Tags[i++].ta_Arg = (TagData)(DURATION * 4);
    Tags[i].ta_Tag = TAG_END;

    /* Set various parameters in object by using TagList */
    SetObjectInfo (Seq, Tags);
  }

cleanup:
  return Seq;
}

/*****************************************************************/
int32
PlayCanon (char *pimapfile, int32 NumReps)
{
  Jugglee *CollectionPtr;
  int32 Result;
  ScoreContext *scon;
  Sequence *Seq;
  int32 i;

  Result = -1;
  CollectionPtr = NULL;

  /* Create a context for interpreting a MIDI score and tracking notes. */
  scon = CreateScoreContext (MAXPROGRAMNUM);
  if (scon == NULL)
    {
      Result = AF_ERR_NOMEM;
      goto cleanup;
    }

  /* Specify a mixer to use for the score voices. */
  Result = InitScoreMixer (scon, "mixer8x2.dsp", kMaxScoreVoices,
                           (MAXDSPAMPLITUDE / kMaxScoreVoices));
  CHECKRESULT (Result, "InitScoreMixer");

  /* Create a collection and load MIDI File into it. */
  CollectionPtr = (Jugglee *)CreateObject (&CollectionClass);
  if (CollectionPtr == NULL)
    {
      ERR (("Failure to create Jugglee\n"));
      goto cleanup;
    }
  CollectionPtr->jglr_UserContext = scon;

  for (i = 0; i < NUMLINES; i++)
    {
      Seq = CreateCanonSequence ((i + 1) * DURATION * 2);
      CHECKPTR (Seq, "CreateCanonSequence");

      PrintObject (Seq);

      Seq->jglr_UserContext = scon;

      /* Add Sequence to Collection for parallel play */
      Result = CollectionPtr->Class->Add (CollectionPtr, Seq, REPEATSEQ);
      if (Result)
        {
          PRT (("Add returned 0x%x\n", Result));
        }
      PRT (("Collection contains %d sequences.\n", CollectionPtr->jglr_Many));
    }

  /* Set repeat function and repeat delay in Collection. */
  {
    TagArg Tags[10];
    int32 i = 0;

    Tags[i].ta_Tag = JGLR_TAG_REPEAT_FUNCTION;
    Tags[i++].ta_Arg = (TagData)UserRepeatFunc;
    Tags[i].ta_Tag = JGLR_TAG_REPEAT_DELAY;
    Tags[i++].ta_Arg = (TagData)(DURATION * 4);
    SetObjectInfo (CollectionPtr, Tags);
  }

  /* Load Instrument Templates from disk and fill Program Instrument Map. */
  Result = LoadPIMap (scon, pimapfile);
  CHECKRESULT (Result, "LoadPIMap");

  /* Play the collection. */
  Result = PlayJugglee (CollectionPtr, NumReps);
  CHECKRESULT (Result, "PlayJugglee");

  PRT (("Finished playing.\n"));

cleanup:
  UnloadPIMap (scon);
  while (GetNthFromObject (CollectionPtr, 0, &Seq) == 0)
    {
      RemoveNthFromObject (CollectionPtr, 0);
      DestroyObject ((COBObject *)Seq);
    }

  TermScoreMixer (scon);
  DeleteScoreContext (scon);
  return Result;
}

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
  Tags[0].ta_Arg = (TagData)IfMute;
  Tags[1].ta_Tag = TAG_END;

  return SetObjectInfo (Child, Tags);
}

/*****************************************************************
** Play a Collection, a Sequence or any other Juggler object.
*****************************************************************/
int32
PlayJugglee (Jugglee *JglPtr, uint32 NumReps)
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

  DBUG (("PlayJugglee: Start at time %d\n", NextTime));
  StartObject (JglPtr, NextTime, NumReps, NULL);

  do
    {
      /* Use Control Pad to experiment with Muting (optional). */
      /* Read current state of Control Pad. */
      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          ERR (("Error in GetControlPad\n"));
          PrintfSysErr (Result);
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
                  ERR (("Error in GetControlPad\n"));
                  PrintfSysErr (Result);
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

#if 0
	DumpTraceRecord();
	DumpSequence( CanonData, NUMEVENTS );
#endif

  DeleteItem (MyCue);

cleanup:
  return Result;
}
