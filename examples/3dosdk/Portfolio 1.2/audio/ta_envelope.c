/***************************************************************
**
** Use an envelope to modulate amplitude of sawtooth.
** By:  Phil Burk
**
** Copyright (c) 1993, 3DO Company.
** This program is proprietary and confidential.
**
**	History:
**	6/14/93		rdg		overhauled to conform to dragon
**						 references to changedir, /remote,
* /dsp, and /aiff removed
***************************************************************/

#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "kernel.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define ENVINSNAME "envelope.dsp"
#define SAWINSNAME "sawtooth.dsp"
#define MIXERINSNAME "mixer2x2.dsp"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  DBUG (("%s, val = 0x%x\n", name, val));                                     \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      ERR (("Failure in %s: $%x\n", name, val));                              \
      PrintfSysErr (val);                                                     \
      goto cleanup;                                                           \
    }

/* Define Tags for StartInstrument */
TagArg SawTags[] = { { AF_TAG_AMPLITUDE, 0 }, { AF_TAG_RATE, 0 }, { 0, 0 } };

typedef struct EnvVoice
{
  /* Declare local variables */
  Item envv_SawTmp;
  Item envv_SawIns;
  Item envv_EnvIns;
  Item envv_EnvCue;
  Item envv_Envelope;
  Item envv_EnvAttachment;
  Item envv_FreqKnob;
  Item envv_AmplitudeKnob;
  int32 envv_EnvCueSignal;
} EnvVoice;

#define NUM_ENV_VOICES (2)

EnvVoice EnvVoices[NUM_ENV_VOICES];

/* Variables local to this file. */
static Item MixerTmp = -1;
static Item MixerIns = -1;
static Item LeftGainKnob = -1;
static Item RightGainKnob = -1;
static ufrac16 TimeScale = Convert32_F16 (1);

/* Local function prototypes. */
static int32 SetupMixer (int32 NumChannels, int32 Amplitude);
static Item CreateAttachment (Item Instrument, Item Envelope, char *FIFOName,
                              uint32 Flags);

/* Times are relative to the START of the envelope. */
static DataTimePair EnvPoints1[] = {
  /* Time,  Data */
  { 0, 0 },        /* 0 */
  { 1000, 30000 }, /* 1 */
  { 1300, 20000 }, /* 2 */
  { 1600, 1000 },  /* 3 */
  { 2000, 32000 }, /* 4 */
  { 2500, 4000 },  /* 5 */
  { 3000, 8000 },  /* 6 */
  { 4000, 20000 }, /* 7 */
  { 10000, 0 }     /* 8 */
};
#define kNumEnvPoints1 (sizeof (EnvPoints1) / sizeof (DataTimePair))

/* Piano style envelope with release jump at 2. */
static DataTimePair EnvPoints2[] = {
  /* Time,  Data */
  { 0, 0 },        /* 0 */
  { 1000, 30000 }, /* 1 */
  { 10000, 0 },    /* 2 */
  { 11000, 0 }     /* 3 */
};
#define kNumEnvPoints2 (sizeof (EnvPoints2) / sizeof (DataTimePair))

/* Piano style envelope with release jump at 2 followed by wiggle. */
static DataTimePair EnvPoints3[] = {
  /* Time,  Data */
  { 0, 0 },        /* 0 */
  { 1000, 30000 }, /* 1 */
  { 10000, 0 },    /* 2 */
  { 11000, 8000 }, /* 3 */
  { 12000, 0000 }  /* 4 */
};
#define kNumEnvPoints3 (sizeof (EnvPoints3) / sizeof (DataTimePair))

/* Envelope suggested by Don Veca. */
static DataTimePair EnvPoints4[] = {
  /* Time,  Data */
  { 0, 0 },        /* 0 */
  { 1000, 30000 }, /* 1 */
  { 3000, 10000 }, /* 2 SUSTAIN POINT and RELEASE JUMP */
  { 3500, 20000 }, /* 3 */
  { 4000, 0 }      /* 4 */
};
#define kNumEnvPoints4 (sizeof (EnvPoints4) / sizeof (DataTimePair))

/* Piano style envelope with VERY SHORT ATTACK release jump at 2. */
static DataTimePair EnvPoints5[] = {
  /* Time,  Data */
  { 0, 0 },     /* 0 */
  { 1, 30000 }, /* 1 */
  { 600, 0 },   /* 2 */
  { 1000, 0 }   /* 3 */
};
#define kNumEnvPoints5 (sizeof (EnvPoints5) / sizeof (DataTimePair))

/* Simple envelope that starts != 0. */
static DataTimePair EnvPoints6[] = {
  /* Time,  Data */
  { 0, 30000 }, /* 0 */
  { 600, 0 },   /* 1 */
};
#define kNumEnvPoints6 (sizeof (EnvPoints6) / sizeof (DataTimePair))

/* Long envelope that starts != 0. */
static DataTimePair EnvPoints7[] = {
  /* Time,  Data */
  { 0, 30000 },   /* 0 */
  { 5000, 5000 }, /* 1 */
  { 12000, 0 },   /* 2 */
};
#define kNumEnvPoints7 (sizeof (EnvPoints7) / sizeof (DataTimePair))
Item MakeEnvelopes (int32 TestIndex);

/******************************************************************/
static Item
CreateAttachment (Item Instrument, Item Envelope, char *FIFOName, uint32 Flags)
{
  /* Declare local variables */
  Item Result;
  TagArg Tags[5];

  /* Initalize local variables */
  Result = -1;

  Tags[0].ta_Tag = AF_TAG_HOOKNAME;
  Tags[0].ta_Arg = (void *)FIFOName;
  Tags[1].ta_Tag = AF_TAG_ENVELOPE;
  Tags[1].ta_Arg = (void *)Envelope;
  Tags[2].ta_Tag = AF_TAG_INSTRUMENT;
  Tags[2].ta_Arg = (void *)Instrument;
  Tags[3].ta_Tag = AF_TAG_SET_FLAGS;
  Tags[3].ta_Arg = (void *)Flags;
  Tags[4].ta_Tag = TAG_END;

  Result = CreateItem (MKNODEID (AUDIONODE, AUDIO_ATTACHMENT_NODE), Tags);

  return Result;
}

/***************************************************************************/
Err
CleanupEnvVoice (EnvVoice *envv)
{
  int32 Result;
  Result = StopInstrument (envv->envv_SawIns, NULL);
  CHECKRESULT (Result, "StopInstrument Saw");
  Result = StopInstrument (envv->envv_EnvIns, NULL);
  CHECKRESULT (Result, "StopInstrument Env");

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */
  FreeInstrument (envv->envv_SawIns);
  UnloadInsTemplate (envv->envv_SawTmp);
  UnloadInstrument (envv->envv_EnvIns);
  return 0;
}

/***************************************************************************/
Err
SetupEnvVoice (EnvVoice *envv, int32 Amplitude, int32 Frequency, int32 Channel,
               int32 TestIndex)
{
  char ChannelName[] = "Input?";
  int32 Result;

  /* Load description of Sawtooth instrument */
  envv->envv_SawTmp = LoadInsTemplate (SAWINSNAME, 0);
  CHECKRESULT (envv->envv_SawTmp, "LoadInsTemplate");

  /* Make an instrument based on template. */
  envv->envv_SawIns = AllocInstrument (envv->envv_SawTmp, 0);
  CHECKRESULT (envv->envv_SawIns, "AllocInstrument");

  /* Load Envelope dsp instrument. */
  envv->envv_EnvIns = LoadInstrument (ENVINSNAME, 0, 100);
  CHECKRESULT (envv->envv_EnvIns, "LoadInstrument");

  /* Connect Envelope to amplitude of sawtooth. */
  Result = ConnectInstruments (envv->envv_EnvIns, "Output", envv->envv_SawIns,
                               "Amplitude");
  CHECKRESULT (Result, "ConnectInstruments");

  envv->envv_Envelope = MakeEnvelopes (TestIndex);

  /* Attach envelope to envelope player. */
  envv->envv_EnvAttachment = CreateAttachment (
      envv->envv_EnvIns, envv->envv_Envelope, "Env", AF_ATTF_FATLADYSINGS);
  CHECKRESULT (envv->envv_EnvAttachment, "CreateAttachment");

  /* Create a Cue to monitor Envelope Attachment */
  envv->envv_EnvCue = CreateCue (NULL);
  CHECKRESULT (envv->envv_EnvCue, "CreateCue");
  envv->envv_EnvCueSignal = GetCueSignal (envv->envv_EnvCue);
  Result = MonitorAttachment (envv->envv_EnvAttachment, envv->envv_EnvCue,
                              CUE_AT_END);
  CHECKRESULT (Result, "MonitorAttachment");

  /* Connect Sawtooth to Mixer */
  PRT (("ta_sawtooth: Connect Instruments, Saw -> Mixer\n"));
  ChannelName[5] = '0' + Channel;
  Result = ConnectInstruments (envv->envv_SawIns, "Output", MixerIns,
                               ChannelName);
  CHECKRESULT (Result, "ConnectInstruments");

  /* Play a note using StartInstrument */
  SawTags[0].ta_Arg = (void *)Amplitude;
  SawTags[1].ta_Arg = (void *)Frequency;
  Result = StartInstrument (envv->envv_SawIns, &SawTags[0]);
  CHECKRESULT (Result, "StartInstrument Saw");
  return Result;
cleanup:

  CleanupEnvVoice (envv);

  return Result;
}
/******************************************************************/
int
main (int argc, char *argv[])
{

  Item TimerCue;
  int32 Result;
  int32 i;
  int32 TestIndex;
  int32 IQuit, NoteOnA, NoteOnB;
  uint32 Buttons;
  ControlPadEventData cped;
  TagArg Tags[2];

  Result = 0;

  PRT (("\nta_envelope\n"));

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_FocusListener);
  if (Result < 0)
    {
      ERR (("main: error in InitEventUtility\n"));
      PrintfSysErr (Result);
      goto cleanup;
    }

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  //	TraceAudio(TRACE_ENVELOPE);

  TestIndex = (argc > 1) ? atoi (argv[1]) : 1;

  if (SetupMixer (2, 0x4000))
    return -1;

  for (i = 0; i < NUM_ENV_VOICES; i++)
    {
      SetupEnvVoice (&EnvVoices[i], 0x6000, 100 * i + 273, i, TestIndex);
    }

  TimerCue = CreateCue (NULL);
  CHECKRESULT (TimerCue, "CreateCue");

  IQuit = FALSE;
  NoteOnA = FALSE;
  NoteOnB = FALSE;

  Tags[0].ta_Tag = AF_TAG_TIME_SCALE;
  Tags[0].ta_Arg = (void *)TimeScale;
  Tags[1].ta_Tag = TAG_END;

  do
    {
// #define MONITOR_ENV
#ifdef MONITOR_ENV
      /* Read current state of Control Pad. */
      do
        {
          /* Sleep to give other tasks time. */
          SleepUntilTime (TimerCue, GetAudioTime () + 10);

          Result = GetControlPad (1, FALSE, &cped);
          if (Result < 0)
            {
              ERR (("Error in GetControlPad\n"));
              PrintfSysErr (Result);
            }
          Buttons = cped.cped_ButtonBits;

          if (GetCurrentSignals () & EnvCueSignal)
            {
              PRT (("Received signal from MonitorAttachment!\n"));
              WaitSignal (EnvCueSignal); /* Clear the signal */
            }

          /* Print progress of envelope. */
          PRT (("Envelope at %d\n", WhereAttachment (EnvAttachment)));
        }
      while (Buttons == 0);
#else
      Result = GetControlPad (1, TRUE, &cped);
      if (Result < 0)
        {
          ERR (("Error in GetControlPad\n"));
          PrintfSysErr (Result);
        }
      Buttons = cped.cped_ButtonBits;
#endif

      if ((Buttons & ControlA) || (Buttons & ControlC))
        {
          if (NoteOnA)
            {
              PRT (("Release A.\n"));
              Result = ReleaseInstrument (EnvVoices[0].envv_EnvIns, Tags);
              CHECKRESULT (Result, "ReleaseInstrument Env");
              NoteOnA = FALSE;
            }
          else
            {
              PRT (("Start A.\n"));
              Result = StartInstrument (EnvVoices[0].envv_EnvIns, Tags);
              CHECKRESULT (Result, "StartInstrument Env");
              NoteOnA = TRUE;
            }
        }
      if ((Buttons & ControlB) || (Buttons & ControlC))
        {
          if (NoteOnB)
            {
              PRT (("Release B.\n"));
              Result = ReleaseInstrument (EnvVoices[1].envv_EnvIns, Tags);
              CHECKRESULT (Result, "ReleaseInstrument Env");
              NoteOnB = FALSE;
            }
          else
            {
              PRT (("Start B.\n"));
              Result = StartInstrument (EnvVoices[1].envv_EnvIns, Tags);
              CHECKRESULT (Result, "StartInstrument Env");
              NoteOnB = TRUE;
            }
        }

      if (Buttons & ControlUp)
        {
          TimeScale = (TimeScale * 5) / 4;
          Tags[0].ta_Arg = (void *)TimeScale;
          PRT (("TimeScale = 0x%x\n", TimeScale));
        }
      if (Buttons & ControlDown)
        {
          TimeScale = (TimeScale * 4) / 5;
          Tags[0].ta_Arg = (void *)TimeScale;
          PRT (("TimeScale = 0x%x\n", TimeScale));
        }

      if (Buttons & ControlStart)
        {
          IQuit = TRUE;
        }
    }
  while (!IQuit);

cleanup:
  for (i = 0; i < NUM_ENV_VOICES; i++)
    {
      CleanupEnvVoice (&EnvVoices[i]);
    }

  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  FreeInstrument (MixerIns);
  UnloadInsTemplate (MixerTmp);
  CloseAudioFolio ();
  KillEventUtility ();
  PRT (("%s all done.\n", argv[0]));
  return ((int)Result);
}

/******************************************************************/
Item
MakeEnvelopes (int32 TestIndex)
{
  Item Result;
  TagArg Tags[20];

  Result = 0;
  Tags[0].ta_Tag = TAG_END;

  /* Create Envelope Item. */
  switch (TestIndex)
    {
    case 1:
      PRT (("Sustain loop, no release loop.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints1;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints1;
      Tags[2].ta_Tag = AF_TAG_SUSTAINBEGIN;
      Tags[2].ta_Arg = (void *)1;
      Tags[3].ta_Tag = AF_TAG_SUSTAINEND;
      Tags[3].ta_Arg = (void *)3;
      Tags[4].ta_Tag = AF_TAG_RELEASEBEGIN;
      Tags[4].ta_Arg = (void *)-1;
      Tags[5].ta_Tag = AF_TAG_RELEASEEND;
      Tags[5].ta_Arg = (void *)-1;
      Tags[6].ta_Tag = TAG_END;
      break;
    case 2:
      PRT (("Sustain loop, Release loop.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints1;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints1;
      Tags[2].ta_Tag = AF_TAG_SUSTAINBEGIN;
      Tags[2].ta_Arg = (void *)1;
      Tags[3].ta_Tag = AF_TAG_SUSTAINEND;
      Tags[3].ta_Arg = (void *)3;
      Tags[4].ta_Tag = AF_TAG_RELEASEBEGIN;
      Tags[4].ta_Arg = (void *)4;
      Tags[5].ta_Tag = AF_TAG_RELEASEEND;
      Tags[5].ta_Arg = (void *)6;
      Tags[6].ta_Tag = TAG_END;
      break;
    case 3:
      PRT (("Release jump.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints2;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints2;
      Tags[2].ta_Tag = AF_TAG_RELEASEJUMP;
      Tags[2].ta_Arg = (void *)2;
      Tags[3].ta_Tag = TAG_END;
      break;

    case 4:
      PRT (("Release jump with wiggle and release time.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints3;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints3;
      Tags[2].ta_Tag = AF_TAG_RELEASEJUMP;
      Tags[2].ta_Arg = (void *)2;
      Tags[3].ta_Tag = AF_TAG_RELEASEBEGIN;
      Tags[3].ta_Arg = (void *)3;
      Tags[4].ta_Tag = AF_TAG_RELEASEEND;
      Tags[4].ta_Arg = (void *)4;
      Tags[5].ta_Tag = AF_TAG_RELEASETIME;
      Tags[5].ta_Arg = (void *)2000;
      Tags[6].ta_Tag = TAG_END;
      break;

    case 5:
      PRT (("Release jump with sustain at 1.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints2;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints2;
      Tags[2].ta_Tag = AF_TAG_RELEASEJUMP;
      Tags[2].ta_Arg = (void *)2;
      Tags[3].ta_Tag = AF_TAG_SUSTAINBEGIN;
      Tags[3].ta_Arg = (void *)1;
      Tags[4].ta_Tag = AF_TAG_SUSTAINEND;
      Tags[4].ta_Arg = (void *)1;
      Tags[5].ta_Tag = TAG_END;
      break;

    case 6:
      PRT (("Just sustain at 1, no jump.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints2;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints2;
      Tags[2].ta_Tag = AF_TAG_SUSTAINBEGIN;
      Tags[2].ta_Arg = (void *)1;
      Tags[3].ta_Tag = AF_TAG_SUSTAINEND;
      Tags[3].ta_Arg = (void *)1;
      Tags[4].ta_Tag = TAG_END;
      break;

    case 7:
      PRT (("Release jump = sustain at 2.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints4;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints4;
      Tags[2].ta_Tag = AF_TAG_RELEASEJUMP;
      Tags[2].ta_Arg = (void *)2;
      Tags[3].ta_Tag = AF_TAG_SUSTAINBEGIN;
      Tags[3].ta_Arg = (void *)2;
      Tags[4].ta_Tag = AF_TAG_SUSTAINEND;
      Tags[4].ta_Arg = (void *)2;
      Tags[5].ta_Tag = TAG_END;
      break;

    case 8:
      PRT (("Test FLS bit.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints4;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints4;
      Tags[2].ta_Tag = AF_TAG_SET_FLAGS;
      Tags[2].ta_Arg = (void *)AF_ENVF_FATLADYSINGS;
      Tags[3].ta_Tag = AF_TAG_SUSTAINBEGIN;
      Tags[3].ta_Arg = (void *)2;
      Tags[4].ta_Tag = AF_TAG_SUSTAINEND;
      Tags[4].ta_Arg = (void *)2;
      Tags[5].ta_Tag = TAG_END;
      break;

    case 9:
      PRT (("Release jump to last point.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints2;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints2;
      Tags[2].ta_Tag = AF_TAG_RELEASEJUMP;
      Tags[2].ta_Arg = (void *)(kNumEnvPoints2 - 1);
      Tags[3].ta_Tag = TAG_END;
      break;

    case 10:
      PRT (("Release jump to last point with VERY SHORT ATTACK.\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints5;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints5;
      Tags[2].ta_Tag = AF_TAG_RELEASEJUMP;
      Tags[2].ta_Arg = (void *)(kNumEnvPoints5 - 2);
      Tags[3].ta_Tag = TAG_END;
      break;

    case 11:
      PRT (("Envelope that starts at 30000\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints6;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints6;
      Tags[2].ta_Tag = TAG_END;
      break;

    case 12:
      PRT (("Long Envelope that starts at 30000\n"));
      Tags[0].ta_Tag = AF_TAG_ADDRESS;
      Tags[0].ta_Arg = (void *)EnvPoints7;
      Tags[1].ta_Tag = AF_TAG_FRAMES;
      Tags[1].ta_Arg = (void *)kNumEnvPoints7;
      Tags[2].ta_Tag = TAG_END;
      break;

    default:
      ERR (("Invalid envelope test index = %d\n", TestIndex));
      Result = -1;
    }

  Result = CreateItem (MKNODEID (AUDIONODE, AUDIO_ENVELOPE_NODE), Tags);
  CHECKRESULT (Result, "MakeEnvelopes");

cleanup:
  return Result;
}

/******************************************************************/
static int32
SetupMixer (int32 NumChannels, int32 Amplitude)
{
  /* Declare local variables */
  int32 Result, i;
  char LeftName[] = "LeftGain?";
  char RightName[] = "RightGain?";

  /* Initalize local variables */
  Result = 0;

  /* Initalize global variables */
  MixerTmp = -1;
  MixerIns = -1;
  LeftGainKnob = -1;
  RightGainKnob = -1;

  /* Load the instrument template for the mixer */
  MixerTmp = LoadInsTemplate (MIXERINSNAME, 0);
  CHECKRESULT (MixerTmp, "LoadInsTemplate");

  /* Make an instrument based on template. */
  MixerIns = AllocInstrument (MixerTmp, 0);
  CHECKRESULT (MixerIns, "AllocInstrument");

  for (i = 0; i < NumChannels; i++)
    {

      /* Attach the Left and Right gain knobs. */
      LeftName[8] = '0' + i;
      LeftGainKnob = GrabKnob (MixerIns, LeftName);
      CHECKRESULT (LeftGainKnob, "GrabKnob");
      RightName[9] = '0' + i;
      RightGainKnob = GrabKnob (MixerIns, RightName);
      CHECKRESULT (RightGainKnob, "GrabKnob");

      /* Set Mixer Levels */
      TweakKnob (LeftGainKnob, Amplitude);
      TweakKnob (RightGainKnob, Amplitude);

      ReleaseKnob (LeftGainKnob);
      ReleaseKnob (RightGainKnob);
    }

  /* Mixer must be started */
  Result = StartInstrument (MixerIns, NULL);

  return Result;

cleanup:
  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  FreeInstrument (MixerIns);
  UnloadInsTemplate (MixerTmp);
  return Result;
}
