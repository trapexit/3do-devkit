/***************************************************************
**
** Play Notes using Sampled Waveform  - ta_pitchnotes.c
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/*
** 921116 PLB Modified for explicit mixer connect.
** 921118 PLB Added ChangeDirectory("/remote") for filesystem.
** 921202 PLB Converted to LoadInstrument and GrabKnob.
** 921203 PLB Use AUDIODATADIR instead of /remote.
** 930315 PLB Conforms to new API
*/

#include "debug.h"
#include "filefunctions.h"
#include "operror.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) PRT (x)

#define INSNAME "sampler.dsp"
/* #define INSNAME "varmono8.dsp" */
#define STARTNOTE (30)
#define ENDNOTE (90)

int32 PlayFreqNote (Item Instrument, int32 Freq, int32 Duration);

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      ERR (("Failure in %s: $%x\n", name, val));                              \
      /*		PrintfSysErr(Result); */                                            \
      goto cleanup;                                                           \
    }

int32 PlayPitchNote (Item Instrument, int32 Note, int32 Velocity,
                     int32 Duration);

Item MixerIns = -1;
Item LeftGainKnob = -1;
Item RightGainKnob = -1;
int32 SetupMixer (void);

int
main (int argc, char *argv[])
{
  Item SamplerIns = 0;
  Item SampleItem = 0;
  Item Attachment = 0;
  int32 Duration;
  int32 Result = -1;
  int32 i;

  PRT (("ta_pitchnotes <samplefile>\n"));

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  TraceAudio (0);
  if (SetupMixer ())
    return -1;

  /* Load Sampler instrument */
  SamplerIns = LoadInstrument (INSNAME, 0, 100);
  CHECKRESULT (SamplerIns, "LoadInstrument");

  /* Load digital audio Sample from disk. */
  if (argc > 1)
    {
      SampleItem = LoadSample (argv[1]); /* from command line? */
    }
  else
    {
      SampleItem = LoadSample ("bass.aiff");
    }
  CHECKRESULT (SampleItem, "LoadSample");

  Duration = (argc > 2) ? atoi (argv[2]) : 240;
  PRT (("Duration = %d\n", Duration));

  /* Look at sample information. */
  DebugSample (SampleItem);

  /* Connect Sampler to Mixer */
  DBUG (("Connect Instruments, Sampler -> Mixer\n"));
  Result = ConnectInstruments (SamplerIns, "Output", MixerIns, "Input0");
  CHECKRESULT (Result, "ConnectInstruments");

  /* Attach the sample to the instrument. */
  Attachment = AttachSample (SamplerIns, SampleItem, 0);
  CHECKRESULT (Attachment, "AttachSample");

  /* Play several notes using a conveniance routine. */
  for (i = STARTNOTE; i < ENDNOTE; i++)
    {
      PRT (("Pitch = %d\n", i));
      PlayPitchNote (SamplerIns, i, 100, Duration);
    }

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */

  DetachSample (Attachment);
  UnloadInstrument (SamplerIns);
  UnloadSample (SampleItem);

  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  UnloadInstrument (MixerIns);
  PRT (("All Done---------------------------------\n"));
  CloseAudioFolio ();
  return ((int)Result);
}

/********************************************************************/
/***** Play a note based on MIDI pitch. *****************************/
/********************************************************************/

/* Define Tags for StartInstrument */
TagArg SamplerTags[]
    = { { AF_TAG_VELOCITY, 0 }, { AF_TAG_PITCH, 0 }, { TAG_END, 0 } };

int32
PlayPitchNote (Item Instrument, int32 Note, int32 Velocity, int32 Duration)
{
  int32 Result;

  SamplerTags[0].ta_Arg = (void *)Velocity;
  SamplerTags[1].ta_Arg = (void *)Note;
  Result = StartInstrument (Instrument, &SamplerTags[0]);
  Result = SleepAudioTicks (Duration >> 1);

  ReleaseInstrument (Instrument, NULL);
  Result = SleepAudioTicks (Duration >> 1);
  return 0;
}

/*********************************************************************/
int32
SetupMixer ()
{
  int32 Result = 0;

  MixerIns = LoadInstrument ("mixer4x2.dsp", 0, 0);
  CHECKRESULT (MixerIns, "LoadInstrument");

  /* Attach the Left and Right gain knobs. */
  LeftGainKnob = GrabKnob (MixerIns, "LeftGain0");
  CHECKRESULT (LeftGainKnob, "GrabKnob");
  RightGainKnob = GrabKnob (MixerIns, "RightGain0");
  CHECKRESULT (RightGainKnob, "GrabKnob");

  /* Set Mixer Levels */
  TweakKnob (LeftGainKnob, 0x6000);
  TweakKnob (RightGainKnob, 0x6000);
  /* Mixer must be started */
  Result = StartInstrument (MixerIns, NULL);
  return Result;

cleanup:
  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  UnloadInstrument (MixerIns);
  return Result;
}
