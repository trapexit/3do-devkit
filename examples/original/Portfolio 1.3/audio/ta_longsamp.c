/* $Id: ta_longsamp.c,v 1.17 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************
**
** Play Sampled Waveform for a long time as a loop.  - ta_longsamp.c
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
**	History:
**	6/14/93		rdg		overhauled to conform to dragon
**						 references to changedir, /remote,
* /dsp, and /aiff removed
** 921116 PLB Modified for explicit mixer connect.
** 921118 PLB Added ChangeDirectory("/remote") for filesystem.
** 921202 PLB Converted to LoadInstrument and GrabKnob.
** 921203 PLB Use AUDIODATADIR instead of /remote.
** 930315 PLB Conforms to new API
***************************************************************/

#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "operror.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define DEMOMODE
#ifdef DEMOMODE
#include "audiodemo.h"
#endif
#define DURATION 40

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

/* Define Tags for StartInstrument */
static TagArg SamplerTags[]
    = { { AF_TAG_AMPLITUDE, 0 }, { AF_TAG_RATE, 0 }, { 0, 0 } };

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, val);                              \
      goto cleanup;                                                           \
    }

Item MixerIns = -1;
Item LeftGainKnob = -1;
Item RightGainKnob = -1;
int32 SetupMixer (void);

extern char *SelectSamplePlayer (Item Sample, int32 IfVariable);

int
main (int argc, char *argv[])
{
  Item SamplerIns = 0;
  Item SampleItem = 0;
  Item Attachment = 0;
  char *SampleName = "bass.aiff";
  int32 Rate = 0x8000;
  int32 Result = -1;
  int32 IfVariable = FALSE;
  char *InstrumentName;
  uint32 Joy;
  int32 Where;

  PRT (("Usage: %s <samplefile> <rate>\n", argv[0]));

  if (argc > 1)
    SampleName = argv[1]; /* from command line? */
  if (argc > 2)
    {
      Rate = atoi (argv[2]);
      IfVariable = TRUE;
    }

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  if (SetupMixer ())
    return -1;

  /* Load digital audio Sample from disk. */
  SampleItem = LoadSample (SampleName);
  CHECKRESULT (SampleItem, "LoadSample");

  DBUG (("Rate = 0x%x\n", Rate));

  /* Look at sample information. */
  DebugSample (SampleItem);

  /* Load Sampler instrument */
  InstrumentName = SelectSamplePlayer (SampleItem, IfVariable);
  if (InstrumentName == NULL)
    {
      ERR (("No instrument to play that sample.\n"));
      goto cleanup;
    }
  PRT (("Use instrument: %s\n", InstrumentName));
  SamplerIns = LoadInstrument (InstrumentName, 0, 100);
  CHECKRESULT (SamplerIns, "LoadInstrument");

  /* Connect Sampler to Mixer */
  DBUG (("Connect Instruments, Sampler -> Mixer\n"));
  Result = ConnectInstruments (SamplerIns, "Output", MixerIns, "Input0");
  CHECKRESULT (Result, "ConnectInstruments");

  /* Attach the sample to the instrument. */
  Attachment = AttachSample (SamplerIns, SampleItem, 0);
  CHECKRESULT (Attachment, "AttachSample");

  /* Play one note a long time. */
  SamplerTags[0].ta_Arg = (int32 *)(MAXDSPAMPLITUDE / 2);
  SamplerTags[1].ta_Arg = (int32 *)Rate;
  Result = StartInstrument (SamplerIns, &SamplerTags[0]);
#ifdef DEMOMODE
  PRT (("Press joypad to stop sound.\n"));
  InitJoypad ();
  Joy = 0;
  while ((Joy & ControlStart) == 0)
    {
      Where = WhereAttachment (Attachment);
      PRT (("Where = %d\n", Where));
      ReadJoypad (&Joy);
    }

  TermJoypad ();
#else
  DBUG (("Play sample for %d seconds.\n", DURATION));
  SleepAudioTicks (DURATION * 240);
#endif

  ReleaseInstrument (SamplerIns, NULL);
  Result = SleepAudioTicks (60);

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */
  //	DetachSample( Attachment );
  UnloadSample (SampleItem);
  UnloadInstrument (SamplerIns);

  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  UnloadInstrument (MixerIns);

  CloseAudioFolio ();
  PRT (("%s finished.\n", argv[0]));
  return ((int)Result);
}

/*********************************************************************/
int32
SetupMixer ()
{
  int32 Result = 0;

  MixerIns = LoadInstrument ("mixer4x2.dsp", 0, 50);
  CHECKRESULT (MixerIns, "LoadInstrument");

  /* Attach the Left and Right gain knobs. */
  LeftGainKnob = GrabKnob (MixerIns, "LeftGain0");
  CHECKRESULT (LeftGainKnob, "GrabKnob");
  RightGainKnob = GrabKnob (MixerIns, "RightGain0");
  CHECKRESULT (RightGainKnob, "GrabKnob");

  /* Set Mixer Levels */
  TweakKnob (LeftGainKnob, 0x2000);
  TweakKnob (RightGainKnob, 0x2000);
  /* Mixer must be started */
  Result = StartInstrument (MixerIns, NULL);
  return Result;

cleanup:
  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  UnloadInstrument (MixerIns);
  return Result;
}
