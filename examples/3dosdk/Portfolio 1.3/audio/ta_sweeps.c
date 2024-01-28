/* $Id: ta_sweeps.c,v 1.19 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************
**
** Play Simple Sawtooth waveform.
** Sweep the frequency smoothly as fast as possible to
** test Knob speed.
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
** 930210 PLB Use DirectOut for simplicity.
** 930315 PLB Conforms to new API
*/

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
  Item SawIns, OutputIns;
  Item FreqKnob, LoudKnob;
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
