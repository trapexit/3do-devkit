
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: simple_envelope.c,v 1.6 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/simple_envelope
|||	simple_envelope - Simple audio envelope example.
|||
|||	  Synopsis
|||
|||	    simple_envelope
|||
|||	  Description
|||
|||	    Simple demonstration of an envelope used to ramp amplitude of a
sawtooth
|||	    waveform. A 3-point envelope is used to provide a ramp up to a
specified
|||	    amplitude at the start of the sound and a ramp down when the sound
is to
|||	    be stopped. Using envelopes is one technique to avoid audio pops at
the
|||	    start and end of sounds.
|||
|||	  Associated Files
|||
|||	    simple_envelope.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
**/

#include <audio.h>
#include <mem.h>
#include <operamath.h> /* ConvertF16_32() */
#include <stdio.h>
#include <stdlib.h>
#include <task.h> /* WaitSignal() */

/* -------------------- Parameters */

#define DEFAULT_AMPLITUDE 0x7fff

/* -------------------- Macros */

/*
    Macro to simplify error checking.
    expression is only evaluated once and should return an int32
    (or something that can be cast to it sensibly)
*/
#define TRAP_FAILURE(_exp, _name)                                             \
  do                                                                          \
    {                                                                         \
      const int32 _result = (_exp);                                           \
      if (_result < 0)                                                        \
        {                                                                     \
          PrintError (NULL, _name, NULL, _result);                            \
          goto clean;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)

/* -------------------- Code */

void TestEnvelope (int32 amplitude);

int
main (int argc, char *argv[])
{
  printf ("%s: begin\n", argv[0]);

#ifdef MEMDEBUG
  TRAP_FAILURE (CreateMemDebug (MEMDEBUGF_ALLOC_PATTERNS
                                    | MEMDEBUGF_FREE_PATTERNS
                                    | MEMDEBUGF_PAD_COOKIES
                                    | MEMDEBUGF_CHECK_ALLOC_FAILURES
                                    | MEMDEBUGF_KEEP_TASK_DATA,
                                NULL),
                "CreateMemDebug");
#endif

  TRAP_FAILURE (OpenAudioFolio (), "OpenAudioFolio()");

  TestEnvelope (argc > 1 ? atoi (argv[1]) : DEFAULT_AMPLITUDE);

clean:
  CloseAudioFolio ();
  printf ("%s: end\n", argv[0]);

#ifdef MEMDEBUG
  DumpMemDebug (NULL);
  DeleteMemDebug ();
#endif

  return 0;
}

void
TestEnvelope (int32 amplitude)
{
  /* simple ramped gate envelope */
  DataTimePair envpoints[] = {
    { 0, 0 },
    { 1000, 0 }, /* fill in this data value from the amplitude argument */
    { 2000, 0 },
  };

  /* misc items and such */
  Item osc_ins = 0;
  Item env_ins = 0;
  Item env_data = 0;
  Item env_att = 0;
  Item env_cue = 0;
  Item out_ins = 0;
  Item sleep_cue = 0;
  int32 env_cuesignal;
  const uint32 TicksPerSecond = ConvertF16_32 (GetAudioRate ());

  /* set amplitude */
  envpoints[1].dtpr_Data = amplitude;

  /* Get instruments */
  TRAP_FAILURE (out_ins = LoadInstrument ("directout.dsp", 0, 100),
                "LoadInstrument() 'directout.dsp'");
  TRAP_FAILURE (osc_ins = LoadInstrument ("sawtooth.dsp", 0, 100),
                "LoadInstrument() 'sawtooth.dsp'");
  TRAP_FAILURE (env_ins = LoadInstrument ("envelope.dsp", 0, 100),
                "LoadInstrument() 'envelope.dsp'");

  /* Connect instruments */
  TRAP_FAILURE (ConnectInstruments (env_ins, "Output", osc_ins, "Amplitude"),
                "ConnectInstruments()");
  TRAP_FAILURE (ConnectInstruments (osc_ins, "Output", out_ins, "InputLeft"),
                "ConnectInstruments()");
  TRAP_FAILURE (ConnectInstruments (osc_ins, "Output", out_ins, "InputRight"),
                "ConnectInstruments()");

  /* create and attach envelope */
  TRAP_FAILURE (env_data = CreateItemVA (
                    MKNODEID (AUDIONODE, AUDIO_ENVELOPE_NODE), AF_TAG_ADDRESS,
                    envpoints, AF_TAG_FRAMES,
                    sizeof envpoints / sizeof envpoints[0],
                    AF_TAG_SUSTAINBEGIN, 1, AF_TAG_SUSTAINEND, 1,
                    AF_TAG_SET_FLAGS, AF_ENVF_FATLADYSINGS, TAG_END),
                "create envelope");
  TRAP_FAILURE (env_att = AttachEnvelope (env_ins, env_data, NULL),
                "AttachEnvelope()");

  /* get cue for envelope */
  TRAP_FAILURE (env_cue = CreateCue (NULL), "CreateCue()");
  env_cuesignal = GetCueSignal (env_cue);
  TRAP_FAILURE (MonitorAttachment (env_att, env_cue, CUE_AT_END),
                "MonitorAttachment()");

  /* create sleep cue */
  TRAP_FAILURE (sleep_cue = CreateCue (NULL), "CreateCue()");

  /* Start playing */
  printf ("starting...");
  TRAP_FAILURE (StartInstrument (out_ins, NULL), "start output");
  TRAP_FAILURE (StartInstrument (osc_ins, NULL), "start oscillator");
  TRAP_FAILURE (StartInstrument (env_ins, NULL), "start envelope");

  /* wait a couple of seconds */
  TRAP_FAILURE (
      SleepUntilTime (sleep_cue, GetAudioTime () + (2 * TicksPerSecond)),
      "sleep");

  /* release envelope */
  printf ("releasing...");
  TRAP_FAILURE (ReleaseInstrument (env_ins, NULL), "release envelope");

  /* wait until cued */
  WaitSignal (env_cuesignal);
  printf ("done.\n");

clean:
  DeleteCue (sleep_cue);
  DeleteCue (env_cue);
  DetachEnvelope (env_att);
  DeleteItem (env_data);
  UnloadInstrument (env_ins);
  UnloadInstrument (osc_ins);
  UnloadInstrument (out_ins);
}
