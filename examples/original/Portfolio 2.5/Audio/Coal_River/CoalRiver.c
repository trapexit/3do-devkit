
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: CoalRiver.c,v 1.12 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/coalriver
|||	CoalRiver - Plays a MIDI file using Juggler and Score toolbox.
|||
|||	  Synopsis
|||
|||	    CoalRiver
|||
|||	  Description
|||
|||	    This program plays a MIDI file using Juggler and Score toolbox from
the
|||	    music.lib and audio folio, and demonstrates special MIDI
interpreter
|||	    functions.
|||
|||	    The CoalRiver.pimap file references some AIFF samples which were
included
|||	    in the AIFF Samples folder. It is necessary to copy the AIFF
Samples
|||	    folder into the remote folder to use them in this program. To let
this
|||	    program know about the locations of the samples, use this command
in the
|||	    Debugger Terminal window:alias samples \<sample path>
|||
|||	    Then, the CoalRiver.pimap file will reference the appropriate
samples.
|||
|||	  Caveats
|||
|||	    No attempt was made to reduce the size of the samples. Most
real-world
|||	    titles use fewer or smaller samples.
|||
|||	  Associated Files
|||
|||	    CoalRiver.c                  Source code.
|||
|||	    CoalRiver.imag               Background image which contains
program
|||	                                 instructions.
|||
|||	    CoalRiver.mf                 MIDI format song. Music by John Byrd.
|||
|||	    CoalRiver.pimap              PIMap file to load.
|||
|||	  Location
|||
|||	    examples/Audio/CoalRiver
|||
**/

#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

#include "audio.h"
#include "displayutils.h"
#include "graphics.h"
#include "midifile.h"
#include "music.h"
#include "utils3do.h"

/*****************************************************************

Defines

These defines are used throughout the code to make it more legible.

******************************************************************/

#define VERSION "v1.0"

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
      ERR (("Failure in %s: $%x\n", name, val));                              \
      PrintfSysErr (Result);                                                  \
      goto cleanup;                                                           \
    }

#define MAXPROGRAMNUM                                                         \
  (15) /* Save memory by lowering this to highest program number. */
#define MAX_SCORE_VOICES (8)

#define BACKGROUND_IMAGE_FN "CoalRiver.IMAG"

// This mixer is where the output of the score player is mixed.
#define MIXER_NAME ("submixer8x2.dsp")

// This mixer is used for feedback and other effects; the previous
// mixer often plays through this mixer.
#define USER_MIXER_NAME ("submixer2x2.dsp")
#define MIXER_PRIORITY (120)

#define FILTER_NAME ("svfilter.dsp")
#define FILTER_PRIORITY (110)

// A standard instrument priority.
#define INS_PRIORITY (100)

// This describes the audio clock and how much the rates can be modified.
#define STANDARD_AUDIO_RATE (240)
#define LOW_AUDIO_RATE (75)
#define HIGH_AUDIO_RATE (800)
#define AUDIO_RATE_DELTA (6 << 16)

// Raise this value till the sound clips then drop it till you feel safe.
// (MAXDSPAMPLITUDE/MAX_SCORE_VOICES) is guaranteed safe if all you are
// doing is playing scores
#define MIXER_AMPLITUDE (MAXDSPAMPLITUDE / MAX_SCORE_VOICES) * 2

// This value describes the width of the delay line, or the length
// of the echo effect.
#define DELAY_FRAMES (5000)

// Special control messages that we may have to deal with from MIDI.  They
// may be embedded into a MIDI stream, or they might be generated manually
// as a result of user interaction.
typedef enum
{
  MCM_GRAMOPHONE_MODE = 11,
  MCM_TEMPO_CHANGE,
  MCM_NORMAL_MODE,
  MCM_ECHO_MODE
} MIDIControlMsgType;

/*****************************************************************

Contexts

Rather than create and use a slew of global variables in a program,
we allocate and use a context, which is nothing more than a
collection of variables that a chunk of code requires to be
allocated for it.  This is a convenient way to produce semi-
reusable and semi-reentrant code.

******************************************************************/

// A delay line requires the following variables.
typedef struct DelayContext
{
  Item dlc_MixerIns;
  Item dlc_OutputIns;
  Item dlc_DelayIns;
  Item dlc_DelayLine;
  Item dlc_TapIns;
} DelayContext;

typedef struct
{
  Item audioClock; /* we acquire the audio clock to change the playback rate */
  Item scon_MixerIns;       /* the mixer instrument that the score player
                                                    has allocated */
  Item directOutIns;        /* a direct output device; the score mixer outputs
                                                    to this most of the time */
  Item mixerIns;            /* a user mixer instrument; sounds that are not
                                                    made by the score player can use it */
  Item filterIns;           /* a filter instrument; used to get an effect */
  Item redNoiseInsTemplate; /* template for rednoise.dsp */
  Item noiseIns;            /* a noise instrument; patched into the user
                                                    mixer */
  Item redNoiseIns;         /* a red noise instrument */
  Item squareIns;           /* a square instrument; used to make pops */
  DelayContext dlc;         /* for managing the echo effect */
} UserMIDIPlayCtx, *UserMIDIPlayCtxPtr;

/*****************************************************************

Prototypes

Here we prototype each function that is externally callable.
We could have done this in a header file as well.

******************************************************************/

ScoreContext *pmfSetupScoreContext (char *mapfile);
Jugglee *pmfLoadScore (ScoreContext *scon, char *scorefile);
Err pmfPlayScore (Jugglee *JglPtr, uint32 NumReps, ScoreContext *scon);
Err pmfCleanupScoreContext (ScoreContext *scon);
Err pmfUnloadScore (ScoreContext *scon, Jugglee *CollectionPtr);
Err pmfPlayScoreMute (Jugglee *JglPtr, uint32 NumReps);
Err PlayMIDIFile (char *scorefile, char *mapfile, int32 NumReps);
Err HandleMIDIControlMsg (ScoreContext *scon, int32 Channel, int32 Index,
                          int32 Value);
Err UserInterpretMIDIEvent (Sequence *SeqPtr, MIDIEvent *MEvCur,
                            ScoreContext *scon);
int32 InitCustomDelay (UserMIDIPlayCtx *uctx, DelayContext *dlc,
                       int32 DelaySize, int32 DelayFrames);
int32 TermCustomDelay (UserMIDIPlayCtx *uctx, DelayContext *dlc);
Err HandleControlPad (ScoreContext *scon, ControlPadEventData *cped,
                      ControlPadEventData *cpedLast);

// Define the context globally here.  If this program were to be invoked
// multiple times, we could allocate the memory for ctx by using AllocMem
// instead of getting it this way.

UserMIDIPlayCtx ctx;

/*****************************************************************

The main program.

******************************************************************/
int
main (int argc, char *argv[])
{
  char *scorefile = "CoalRiver.mf";
  char *mapfile = "CoalRiver.pimap";
  int32 Result = 0;
  int32 NumReps;
  ScreenContext sc;
  ubyte *image = NULL;

  PRT (("CoalRiver, %s\n", VERSION));
  PRT (("WARNING: Stack must be set to at least 6000 using modbin.\n"));

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

  Result = CreateBasicDisplay (&sc, DI_TYPE_NTSC, 1);
  CHECKRESULT (Result, "CreateBasicDisplay");

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Build a screen group with a default number of screens (1) */

  image = (ubyte *)LoadImage (BACKGROUND_IMAGE_FN, (ubyte *)NULL,
                              (VdlChunk **)NULL, &sc);
  if (image == NULL)
    {
      ERR (("Cannot load " BACKGROUND_IMAGE_FN " from current directory \n"));
      PrintfSysErr (Result);
      return (-1);
    }

  sc.sc_curScreen = 0;

  DrawImage (sc.sc_Screens[0], image, &sc);

  DisplayScreen (sc.sc_Screens[0], 0);
  FadeFromBlack (&sc, 15);

  /* Required before playing scores. */
  InitJuggler ();

  /* Get files from command line. */
  if (argc > 1)
    scorefile = argv[1];
  if (argc > 2)
    mapfile = argv[2];
  NumReps = (argc > 3) ? atoi (argv[3]) : 100;

  Result = PlayMIDIFile (scorefile, mapfile, NumReps);
  CHECKRESULT (Result, "PlayMIDIFile");

cleanup:
  FadeToBlack (&sc, 15);
  UnloadImage ((void *)image);
  TermJuggler ();
  CloseAudioFolio ();
  KillEventUtility ();
  PRT (("%s finished.\n", argv[0]));
  return (int)Result;
}

void
pmfPrintInstructions ()
{
  printf ("\nCoalRiver version %s\n", VERSION);
  printf ("Control Left\t\tSlow down music\n");
  printf ("Control Right\t\tSpeed up music\n");
  printf ("A button\t\tEnter gramophone mode\n");
  printf ("B button\t\tEnter echo mode\n");
  printf ("C button\t\tEnter normal mode\n");
  printf ("Stop button\t\tQuit music\n");
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

  pmfPrintInstructions ();

  /* Play the score collection. */
  Result = pmfPlayScore (CollectionPtr, NumReps, scon);

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

  /* Now that the score mixer's been initialized, we can process the
     instrument's output.

     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

     CAUTION!  Because this function accesses the ScoreContext's mixer
     directly, it's conceivable that this function may become unusable in
     future releases of the software.  It will probably break if we were
     working with an external MIDI device, for example.
  */

  ctx.scon_MixerIns = scon->scon_MixerIns;
  CHECKRESULT (ctx.scon_MixerIns, "InitScoreMixer");

  /* Create a filter instrument. */

  ctx.filterIns = LoadInstrument (FILTER_NAME, 0, FILTER_PRIORITY);
  CHECKRESULT (ctx.filterIns, "filterIns");

  /* Create an output mixer. */

  ctx.mixerIns = LoadInstrument (USER_MIXER_NAME, 0, MIXER_PRIORITY);
  CHECKRESULT (ctx.mixerIns, "mixerIns");

  /* Create another output mixer. */

  ctx.directOutIns = LoadInstrument ("directout.dsp", 0, MIXER_PRIORITY);
  CHECKRESULT (ctx.directOutIns, "directOutIns");

  /* Create a noise instrument for popping! */

  ctx.redNoiseInsTemplate = LoadInsTemplate ("rednoise.dsp", 0);
  CHECKRESULT (ctx.redNoiseInsTemplate, "LoadInsTemplate");

  ctx.noiseIns = AllocInstrument (ctx.redNoiseInsTemplate, INS_PRIORITY);
  CHECKRESULT (ctx.noiseIns, "noiseIns");

  ctx.redNoiseIns = AllocInstrument (ctx.redNoiseInsTemplate, INS_PRIORITY);
  CHECKRESULT (ctx.noiseIns, "redNoiseIns");

  ctx.squareIns = LoadInstrument ("impulse.dsp", 0, INS_PRIORITY);
  CHECKRESULT (ctx.squareIns, "squareIns");

  /* Set up the delay line. */
  Result = InitCustomDelay (&ctx, &(ctx.dlc), DELAY_FRAMES * sizeof (int16),
                            DELAY_FRAMES - 100);
  CHECKRESULT (Result, "InitCustomDelay");

  /* Load Instrument Templates from disk and fill Program Instrument Map. */
  /* As an alternative, you could use SetPIMapEntry() */

  Result = LoadPIMap (scon, mapfile);
  CHECKRESULT (Result, "LoadPIMap");

  /* Let this task own the audio clock.*/

  ctx.audioClock = OwnAudioClock ();
  CHECKRESULT (Result, "audioClock");

  Result = SetAudioRate (ctx.audioClock, Convert32_F16 (STANDARD_AUDIO_RATE));
  CHECKRESULT (Result, "SetAudioRate");

  /* Set up the mixer for gramophone output. */

  HandleMIDIControlMsg (scon, 0, MCM_NORMAL_MODE, 0);
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
  int32 EventSize, Many;

  /* Get info from Sequence using Tags.  */
  {
    TagArg Tags[4];

    Tags[0].ta_Tag = JGLR_TAG_MANY;
    Tags[1].ta_Tag = JGLR_TAG_EVENTS;
    Tags[2].ta_Tag = JGLR_TAG_EVENT_SIZE;
    Tags[3].ta_Tag = TAG_END;

    GetObjectInfo (Self, Tags);

    Many = (int32)Tags[0].ta_Arg;
    EventPtr = (Time *)Tags[1].ta_Arg;
    EventSize = (int32)Tags[2].ta_Arg;
  }
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
  int32 Many;
  Jugglee *Seq;

  /* Create a collection and load MIDI File into it. */
  CollectionPtr = (Jugglee *)CreateObject (&CollectionClass);
  if (CollectionPtr == NULL)
    {
      ERR (("pmfLoadScore: Failure to create Collection\n"));
      goto cleanup;
    }

  /* Set context used to play collection. */
  {
    TagArg Tags[2];

    Tags[0].ta_Tag = JGLR_TAG_CONTEXT;
    Tags[0].ta_Arg = (TagData)scon;
    Tags[1].ta_Tag = TAG_END;

    SetObjectInfo (CollectionPtr, Tags);
  }

  /* Load it from a MIDIFile */
  Result
      = MFLoadCollection (&MFParser, scorefile, (Collection *)CollectionPtr);
  if (Result)
    {
      ERR (("Error loading MIDI File = $%x\n", Result));
      goto cleanup;
    }

  /* Query how many sequences it contains. */
  {
    TagArg Tags[2];

    Tags[0].ta_Tag = JGLR_TAG_MANY;
    Tags[1].ta_Tag = TAG_END;

    GetObjectInfo (CollectionPtr, Tags);

    Many = (int32)Tags[0].ta_Arg;
  }
  PRT (("Collection contains %d sequences.\n", Many));

  /*  Insert our own MIDI event interpreter! */
  {
    TagArg Tags[2];
    frac16 OriginalRate;
    frac16 Scalar;
    int32 i;

    Tags[0].ta_Tag = JGLR_TAG_INTERPRETER_FUNCTION;

    /*  Note: this line generates a warning in the current version of the
        compiler, and I can't seem to figure out how to tell the compiler
        that casting a function to an arbitrary pointer is OK. */

    Tags[0].ta_Arg = (TagData)UserInterpretMIDIEvent;
    Tags[1].ta_Tag = TAG_END;

    /* Don't change the audio clock here; we change it during playback. */
    OriginalRate = GetAudioRate ();
    Scalar = DivSF16 (OriginalRate, MFParser.mfp_Rate);
    PRT (("Scale sequence times. mfp_Rate = 0x%x, Scalar = 0x%x\n",
          MFParser.mfp_Rate, Scalar));

    for (i = 0; i < Many; i++)
      {
        Result = GetNthFromObject (CollectionPtr, i, &Seq);
        CHECKRESULT (Result, "GetNthFromCollection");

        /* WARNING - assumes contents of Collection are sequences. */

        /* Assign the new interpreter function to each sequence. */
        SetObjectInfo (Seq, Tags);

        Result = pmfScaleSequenceTimes ((Sequence *)Seq, Scalar);
        CHECKRESULT (Result, "ScaleSequenceTimes");
      }
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

      UnloadInstrument (ctx.filterIns);
      UnloadInstrument (ctx.mixerIns);
      UnloadInstrument (ctx.directOutIns);
      UnloadInstrument (ctx.squareIns);

      FreeInstrument (ctx.noiseIns);
      FreeInstrument (ctx.redNoiseIns);

      UnloadInsTemplate (ctx.redNoiseInsTemplate);

      DisownAudioClock (ctx.audioClock);
    }

  return 0;
}

/*****************************************************************
** Play a Collection, a Sequence or any other Juggler object.
** This routine is likely to be carved up for use in your application.
*****************************************************************/
int32
pmfPlayScore (Jugglee *JglPtr, uint32 NumReps, ScoreContext *scon)
{
  AudioTime CurTime, NextTime;
  int32 Result;
  int32 NextSignals, CueSignal, SignalsGot;
  Item MyCue;
  uint32 Joy;
  int32 QuitNow;
  int32 IfTimerPending;
  ControlPadEventData cped, cpedLast;

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

      cpedLast = cped;

      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          ERR (("Error in GetControlPad\n"));
          PrintfSysErr (Result);
        }
      Joy = cped.cped_ButtonBits;

      /* If the user pressed a control pad button, handle it */

      if ((cped.cped_ButtonBits != cpedLast.cped_ButtonBits)
          || (cped.cped_ButtonBits & (ControlLeft | ControlRight)))
        HandleControlPad (scon, &cped, &cpedLast);

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

/************************************************************************************

        This function handles an incoming MIDI control message.  This may
include an internal patch change, some sort of graphics display, or any timed
event.

*************************************************************************************/

Err
HandleMIDIControlMsg (ScoreContext *scon, int32 Channel, int32 Index,
                      int32 Value)
{
  int32 Result;
  Item Knob;
  frac16 newRate;
  DelayContext *dlc;
  UserMIDIPlayCtx *uctx;

  /* Screw around with some pointers here.  Your contexts will hopefully
     be given a little more forethought than mine.  jwb */

  dlc = &(ctx.dlc);
  uctx = &ctx;

  /* If we should enter gramophone mode ... */
  if (Index == MCM_GRAMOPHONE_MODE)
    {

      /* Connect the submixer to the mixer. */
      Result = ConnectInstruments (ctx.scon_MixerIns, "LeftOutput",
                                   ctx.mixerIns, "Input0");
      CHECKRESULT (Result, "ConnectInstruments");

      Result = DisconnectInstruments (ctx.scon_MixerIns, "RightOutput",
                                      ctx.directOutIns, "InputRight");
      CHECKRESULT (Result, "DisconnectInstruments");

      /* Connect the mixer to the filter. */
      Result = ConnectInstruments (ctx.mixerIns, "LeftOutput", ctx.filterIns,
                                   "Input");
      CHECKRESULT (Result, "ConnectInstruments");

      /* Connect the filter to the output. */
      Result = ConnectInstruments (ctx.filterIns, "Output", ctx.directOutIns,
                                   "InputLeft");
      CHECKRESULT (Result, "ConnectInstruments");

      Result = ConnectInstruments (ctx.filterIns, "Output", ctx.directOutIns,
                                   "InputRight");
      CHECKRESULT (Result, "ConnectInstruments");

      /* Connect the noise generator to the output mixer. */
      Result = ConnectInstruments (ctx.noiseIns, "Output", ctx.squareIns,
                                   "Amplitude");
      CHECKRESULT (Result, "ConnectInstruments");

      Result = ConnectInstruments (ctx.redNoiseIns, "Output", ctx.squareIns,
                                   "Frequency");
      CHECKRESULT (Result, "ConnectInstruments");

      Result = ConnectInstruments (ctx.squareIns, "Output", ctx.mixerIns,
                                   "Input1");
      CHECKRESULT (Result, "ConnectInstruments");

      /* Tweak the knobs on the filter. */
      Knob = GrabKnob (ctx.filterIns, "Amplitude");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 32767);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.filterIns, "Resonance");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 1120);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.filterIns, "Frequency");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 12464);
      ReleaseKnob (Knob);

      /* Tweak the knobs on the noise generators. */

      Knob = GrabKnob (ctx.noiseIns, "Amplitude");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0xFFFF);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.noiseIns, "Frequency");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 87);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.redNoiseIns, "Amplitude");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0xFFFF);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.redNoiseIns, "Frequency");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 300);
      ReleaseKnob (Knob);

      /* Tweak the knobs on the output mixer. */

      Knob = GrabKnob (ctx.mixerIns, "LeftGain0");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x6000);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.mixerIns, "RightGain0");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x6000);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.mixerIns, "LeftGain1");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x0800);
      ReleaseKnob (Knob);

      Knob = GrabKnob (ctx.mixerIns, "RightGain1");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x0800);
      ReleaseKnob (Knob);

      /* Start the direct out. */
      Result = StartInstrument (ctx.directOutIns, NULL);
      CHECKRESULT (Result, "StartInstrument");

      /* Start the filter and output mixer. */

      StartInstrument (ctx.filterIns, NULL);
      StartInstrument (ctx.mixerIns, NULL);
      StartInstrument (ctx.noiseIns, NULL);
      StartInstrument (ctx.redNoiseIns, NULL);
      StartInstrument (ctx.squareIns, NULL);

      return 0;
    }
  if (Index == MCM_NORMAL_MODE)
    {

      /* Stop the filter, the noise, the square. */
      Result = StopInstrument (ctx.filterIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      Result = StopInstrument (ctx.noiseIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      Result = StopInstrument (ctx.squareIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      Result = StopInstrument (ctx.redNoiseIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      Result = StopInstrument (dlc->dlc_DelayIns, NULL);
      CHECKRESULT (Result, "StartInstrument");

      Result = StopInstrument (dlc->dlc_TapIns, NULL);
      CHECKRESULT (Result, "StartInstrument");

      /* Connect the submixer to directout. */

      Result = ConnectInstruments (ctx.scon_MixerIns, "LeftOutput",
                                   ctx.directOutIns, "InputLeft");
      CHECKRESULT (Result, "ConnectInstruments");

      Result = ConnectInstruments (ctx.scon_MixerIns, "RightOutput",
                                   ctx.directOutIns, "InputRight");
      CHECKRESULT (Result, "ConnectInstruments");

      /* Start the direct out. */
      Result = StartInstrument (ctx.directOutIns, NULL);
      CHECKRESULT (Result, "StartInstrument");

      return 0;
    }
  if (Index == MCM_TEMPO_CHANGE)
    {
      /* figure out what the new rate should be based on the controller value
       */

      newRate = MulUF16 (((Convert32_F16 (Value) >> 7) + (1 << 15)),
                         Convert32_F16 (STANDARD_AUDIO_RATE));

      /* apply the new rate */

      Result = SetAudioRate (ctx.audioClock, newRate);
      CHECKRESULT (Result, "SetAudioRate");

      return 0;
    }

  if (Index == MCM_ECHO_MODE)
    {

      /* Stop the filter, the noise, the square. */

      Result = StopInstrument (ctx.filterIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      Result = StopInstrument (ctx.noiseIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      Result = StopInstrument (ctx.squareIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      Result = StopInstrument (ctx.redNoiseIns, NULL);
      CHECKRESULT (Result, "StopInstrument");

      /* Connect an output of the submixer into the left channel of the mixer.
       */

      Result = ConnectInstruments (uctx->scon_MixerIns, "LeftOutput",
                                   uctx->mixerIns, "Input0");

      /* Connect the output of the tap to the channel 1 of the mixer. */
      Result = ConnectInstruments (dlc->dlc_TapIns, "Output", uctx->mixerIns,
                                   "Input1");
      CHECKRESULT (Result, "ConnectInstruments");

      /* Connect the right output of the mixer back into the delay. */
      Result = ConnectInstruments (uctx->mixerIns, "RightOutput",
                                   dlc->dlc_DelayIns, "Input");
      CHECKRESULT (Result, "ConnectInstruments");

      /* Connect the left output of the mixer to both directouts. */
      Result = ConnectInstruments (uctx->mixerIns, "LeftOutput",
                                   uctx->directOutIns, "InputLeft");
      CHECKRESULT (Result, "ConnectInstruments");

      Result = ConnectInstruments (uctx->mixerIns, "LeftOutput",
                                   uctx->directOutIns, "InputRight");
      CHECKRESULT (Result, "ConnectInstruments");

      /* Start the direct out. */
      Result = StartInstrument (ctx.directOutIns, NULL);
      CHECKRESULT (Result, "StartInstrument");

      /* Mix for the Delay Line connected to channel 1 of the mixer. */
      Knob = GrabKnob (ctx.mixerIns, "RightGain1");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x5000);
      ReleaseKnob (Knob);

      /* Output for the Delay Line connected to channel 1 of the mixer. */
      Knob = GrabKnob (ctx.mixerIns, "LeftGain1");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x7FFF);
      ReleaseKnob (Knob);

      /* Delay mix from the Input, connected to channel 0 of the mixer. */
      Knob = GrabKnob (ctx.mixerIns, "RightGain0");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x40FF);
      ReleaseKnob (Knob);

      /* Passthrough connected to channel 0 of the mixer. */
      Knob = GrabKnob (ctx.mixerIns, "LeftGain0");
      CHECKRESULT (Knob, "GrabKnob");
      TweakRawKnob (Knob, 0x7FFF);
      ReleaseKnob (Knob);

      /* Start Delay first to test START_AT.  It is safer to start
      ** the tap after the delay.
      */

      Result = StartInstrument (dlc->dlc_DelayIns, NULL);
      CHECKRESULT (Result, "StartInstrument");

      Result = StartInstrument (dlc->dlc_TapIns, NULL);
      CHECKRESULT (Result, "StartInstrument");

      return 0;
    }
/*
If we've gotten this far, it's something we don't care about,
so pass it to the standard function for such things.
*/
cleanup:
  ChangeScoreControl (scon, Channel, Index, Value);
  return 0;
}

/**********************************************************************************
        This is our custom MIDI interpreter routine.  It replaces
InterpretMIDIEvent() for parsing Juggler messages.

        The format of the incoming MIDI message is as follows:

        1011nnnn   0ccccccc 0vvvvvvv  Control Change, (nnnn = channel,)
               ccccccc: control # (0-121),
               vvvvvvv: control value,
***********************************************************************************/
Err
UserInterpretMIDIEvent (Sequence *SeqPtr, MIDIEvent *MEvCur,
                        ScoreContext *scon)
{
  Err Result;

  /* Find out if the message is a control message. */

  if ((MEvCur->mev_Command & 0xF0) == 0xB0)
    {
      Result = HandleMIDIControlMsg (scon, (int32)MEvCur->mev_Command & 0x0F,
                                     (int32)MEvCur->mev_Data1,
                                     (int32)MEvCur->mev_Data2);
      return Result;
    }
  else
    return InterpretMIDIEvent (SeqPtr, MEvCur, scon);
}

/***************************************************************************************

Delay line code, cut and pasted with a few modifications from ta_customdelay.c.

****************************************************************************************/

/*********************************************************************

This function fills in a context for a custom delay.  It makes
a delay instrument, a delay line, and a delay tap, and connects
those elements together.

**********************************************************************/
int32
InitCustomDelay (UserMIDIPlayCtx *uctx, DelayContext *dlc, int32 DelaySize,
                 int32 DelayFrames)
{
  int32 Result = 0;
  Item Att;

/*
** Create a delay line.  This must be allocated by the AudioFolio
** because the memory is written to by hardware.  A delay line
** is just a sample with a special write permission.
** 1=channel, TR loop
*/
#define NUM_CHANNELS (1)
#define IF_LOOP (TRUE)
  dlc->dlc_DelayLine = CreateDelayLine (DelaySize, NUM_CHANNELS, IF_LOOP);
  CHECKRESULT (dlc->dlc_DelayLine, "CreateDelayLine");

  /*
  ** Load the basic delay instrument which just writes data to
  ** an output DMA channel of the DSP.
  */
  dlc->dlc_DelayIns = LoadInstrument ("delaymono.dsp", 0, INS_PRIORITY);
  CHECKRESULT (dlc->dlc_DelayIns, "LoadInstrument");

  /* Attach the delay line to the delay instrument output. */
  Att = AttachSample (dlc->dlc_DelayIns, dlc->dlc_DelayLine, "OutFIFO");
  CHECKRESULT (Att, "AttachDelay");
  Result = SetAudioItemInfoVA (Att, AF_TAG_START_AT, DelayFrames, TAG_END);
  CHECKRESULT (Result, "SetAudioItemInfo: START_AT");

  /*
  ** Load an instrument to read the output of the delay.
  */
  dlc->dlc_TapIns = LoadInstrument ("fixedmonosample.dsp", 0, INS_PRIORITY);
  CHECKRESULT (dlc->dlc_TapIns, "LoadInstrument");

  /* Attach the delay line to the delay tap. */
  Att = AttachSample (dlc->dlc_TapIns, dlc->dlc_DelayLine, "InFIFO");
  CHECKRESULT (Att, "AttachSample");

  return Result;

cleanup:
  TermCustomDelay (uctx, dlc);
  return Result;
}

int32
TermCustomDelay (UserMIDIPlayCtx *uctx, DelayContext *dlc)
{

  UnloadInstrument (dlc->dlc_DelayIns);
  UnloadInstrument (dlc->dlc_TapIns);
  UnloadSample (dlc->dlc_DelayLine);

  return 0;
}

/*******************************************************************************
Control pad handling routine.
********************************************************************************/

Err
HandleControlPad (ScoreContext *scon, ControlPadEventData *cped,
                  ControlPadEventData *cpedLast)
{
  frac16 newRate;
  int32 Result;

  /* On left direction, slow down the tempo */

  if (cped->cped_ButtonBits & ControlLeft)
    {

      newRate = GetAudioRate ();
      newRate = (newRate <= Convert32_F16 (LOW_AUDIO_RATE))
                    ? newRate
                    : (newRate - AUDIO_RATE_DELTA);

      /* apply the new rate */

      Result = SetAudioRate (ctx.audioClock, newRate);
      CHECKRESULT (Result, "SetAudioRate");
    }

  /* On right direction, speed up the tempo */
  if (cped->cped_ButtonBits & ControlRight)
    {
      newRate = GetAudioRate ();
      newRate = (newRate >= Convert32_F16 (HIGH_AUDIO_RATE))
                    ? newRate
                    : (newRate + AUDIO_RATE_DELTA);

      /* apply the new rate */

      Result = SetAudioRate (ctx.audioClock, newRate);
      CHECKRESULT (Result, "SetAudioRate");
    }

  /* On A button, enter gramophone mode */
  if ((cped->cped_ButtonBits & ControlA)
      && (!(cpedLast->cped_ButtonBits & ControlA)))
    {
      HandleMIDIControlMsg (scon, 0, MCM_GRAMOPHONE_MODE, 0);
    }

  /* On B button, enter echo mode */
  if ((cped->cped_ButtonBits & ControlB)
      && (!(cpedLast->cped_ButtonBits & ControlB)))
    {
      HandleMIDIControlMsg (scon, 0, MCM_ECHO_MODE, 0);
    }

  /* On C button, enter normal mode */
  if ((cped->cped_ButtonBits & ControlC)
      && (!(cpedLast->cped_ButtonBits & ControlC)))
    {
      HandleMIDIControlMsg (scon, 0, MCM_NORMAL_MODE, 0);
    }

cleanup:
  return 0;
}
