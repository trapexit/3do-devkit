
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ta_sweeps.c,v 1.25 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ta_sweeps
|||	ta_sweeps - Demonstrates tweaking knobs.
|||
|||	  Synopsis
|||
|||	    ta_sweeps
|||
|||	  Description
|||
|||	    This program quickly modulates the amplitude and frequency of a
sawtooth
|||	    instrument via tweaking the control knobs repeatedly.
|||
|||	    Sweep the frequency smoothly as fast as possible to test Knob
speed.
|||
|||	  Caveats
|||
|||	    This specific sound effect could have been performed entirely
within the
|||	    DSP, freeing up the main processor for other purposes. See the
|||	    documentation for sawenvsvfenv.dsp.
|||
|||	  Associated Files
|||
|||	    ta_sweeps.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
|||	  See Also
|||
|||	    sawenvsvfenv.dsp
|||
**/

#include "debug.h"
#include "filefunctions.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) PRT (x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, val);                              \
      goto cleanup;                                                           \
    }

int
main (int argc, char *argv[])
{
  Item SawIns = 0, OutputIns = 0;
  Item FreqKnob = 0, LoudKnob = 0;
  int32 Result = 0;
  uint32 StartTime, EndTime;
  int32 i, j;

  PRT (("%s\n", argv[0]));

  /* Initialize audio, return if error. */
  Result = OpenAudioFolio ();
  CHECKRESULT (Result, "OpenAudioFolio");

  /* Trace top level calls of Audio Folio */
  TraceAudio (TRACE_TOP);

  /* Load "directout" for connecting to DAC. */
  OutputIns = LoadInstrument ("directout.dsp", 0, 100);
  CHECKRESULT (OutputIns, "LoadInstrument");

  /* Load Sawtooth instrument */
  SawIns = LoadInstrument ("sawtooth.dsp", 0, 100);
  CHECKRESULT (SawIns, "LoadInstrument");

  /* Connect output of sawtooth to left and right inputs. */
  Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
  CHECKRESULT (Result, "ConnectInstruments");
  Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputRight");
  CHECKRESULT (Result, "ConnectInstruments");

  /* Attach the Frequency knob. */
  FreqKnob = GrabKnob (SawIns, "Frequency");
  CHECKRESULT (FreqKnob, "GrabKnob");
  /* Attach the Amplitude knob. */
  LoudKnob = GrabKnob (SawIns, "Amplitude");
  CHECKRESULT (LoudKnob, "GrabKnob");

  /* Start playing without tags. */
  TweakKnob (LoudKnob, 0);
  StartInstrument (SawIns, NULL);
  StartInstrument (OutputIns, NULL);

  /* Trace slows down execution. */
  TraceAudio (0);

  /* Sweep the frequency while increasing the loudness. */
  StartTime = GetAudioTime ();
#define NUM_SWEEPS (50)
  for (i = 0; i < NUM_SWEEPS; i++)
    {
      /* Lower volume to avoid blowing eardrums. */
      TweakKnob (LoudKnob, (i * MAXDSPAMPLITUDE) / (NUM_SWEEPS * 4));
      for (j = 0; j < 2000; j++)
        {
          TweakRawKnob (FreqKnob, j + 20);
        }
    }
  EndTime = GetAudioTime ();

  /* Stop all voices of that instrument. */
  StopInstrument (SawIns, NULL);
  StopInstrument (OutputIns, NULL);

  PRT (("%d knob updates completed in %d ticks.\n", NUM_SWEEPS * 2000,
        EndTime - StartTime));

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */
  ReleaseKnob (FreqKnob);
  ReleaseKnob (LoudKnob);
  UnloadInstrument (SawIns);
  UnloadInstrument (OutputIns);
  CloseAudioFolio ();
  return ((int)Result);
}
