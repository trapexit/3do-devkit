
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ta_pitchnotes.c,v 1.31 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ta_pitchnotes
|||	ta_pitchnotes - Plays a sample at different MIDI pitches.
|||
|||	  Synopsis
|||
|||	    ta_pitchnotes [\<sample file> [\<duration>]]
|||
|||	  Description
|||
|||	    This program loads and plays the AIFF sample file at several
different
|||	    pitches. It does this by selecting a MIDI note number, which the
audio
|||	    folio maps to a frequency.
|||
|||	  Arguments
|||
|||	    sample file                  Name of a sample to be played. The
sample
|||	                                 should be compatible with sampler.dsp
|||	                                 (16-bit monophonic).
|||
|||	    duration                     Duration of each note, in audio ticks.
|||	                                 Defaults to 240.
|||
|||	  Caveats
|||
|||	    Because this sample program uses SleepaudioTicks(), the notes are
not
|||	    necessarily rhythmic. Use the juggler to play rhythmic scores.
|||
|||	  Associated Files
|||
|||	    ta_pitchnotes.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
**/

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
      PrintError (0, "\\failure in", name, Result);                           \
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
      SampleItem = LoadSample (argv[1]);
    }
  else
    {
      SampleItem
          = LoadSample ("$samples/PitchedL/SynthBass/BassSynth.C3LM44k.aiff");
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

int32
PlayPitchNote (Item Instrument, int32 Note, int32 Velocity, int32 Duration)
{
  int32 Result;

  /*
  Notes:
      . Error trapping has been removed for brevity.
      . Use of SleepAudioTicks() is not a real good way to do
        this sort of delay in real code (see Caveats).
*/

  Result = StartInstrumentVA (Instrument, AF_TAG_VELOCITY, Velocity,
                              AF_TAG_PITCH, Note, TAG_END);
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

  MixerIns = LoadInstrument ("mixer4x2.dsp", 0, 100);
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
