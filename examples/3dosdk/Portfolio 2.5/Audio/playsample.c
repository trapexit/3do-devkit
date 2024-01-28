
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: playsample.c,v 1.12 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/playsample
|||	playsample - Plays an AIFF sample in memory using the control pad.
|||
|||	  Synopsis
|||
|||	    playsample [\<sample file> [\<rate>]]
|||
|||	  Description
|||
|||	    This program shows how to load an AIFF sample file and play it
using the
|||	    control pad. Use the A button to start the sample, the B button to
release
|||	    the sample, and the C button to stop the sample. The X button quits
the
|||	    program.
|||
|||	  Arguments
|||
|||	    sample file                  Name of a compatible AIFF sample file.
|||	                                 Defaults to sinewave.aiff.
|||
|||	    rate                         Proportion of 44,100 Hz for the sample
rate
|||	                                 expressed as a 1.15 fraction. Unity is
|||	                                 32768, which is the default. Examples:
16384
|||	                                 for 22,050 Hz, 65535 for nearly 88,200
Hz.
|||
|||	  Associated Files
|||
|||	    playsample.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
**/

#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "operror.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"
/* Include this when using the music.lib */
#include "music.h"

/* Handy printing and debugging macros. */
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
      PrintError (0, "\\failure in", name, val);                              \
      goto cleanup;                                                           \
    }

/************************************************************/
int
main (int argc, char *argv[])
{
  Item OutputIns = 0;
  Item SamplerIns = 0;
  Item SampleItem = 0;
  Item Attachment = 0;
  char *SampleName = "sinewave.aiff";
  int32 Rate = 0x8000;
  int32 Result = -1;
  int32 DoIt = TRUE;
  int32 IfVariable = FALSE;
  char *InstrumentName;
  uint32 Buttons;
  ControlPadEventData cped;

  PRT (("Usage: %s <samplefile> <rate>\n", argv[0]));

  /* Get optional argumants from command line. */
  if (argc > 1)
    SampleName = argv[1];
  if (argc > 2)
    {
      Rate = atoi (argv[2]);
      IfVariable = TRUE;
    }

  /* Print menu of button commands. */
  PRT (("Button Menu:\n"));
  PRT (("   A = Start\n"));
  PRT (("   B = Release\n"));
  PRT (("   C = Stop\n"));
  PRT (("   X = Exit\n"));

  /* Initialize audio, return if error. */
  Result = OpenAudioFolio ();
  if (Result < 0)
    {
      PrintError (0, "Audio Folio could not be opened.", 0, Result);
      return (-1);
    }

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (Result < 0)
    {
      PrintError (0, "InitEventUtility", 0, Result);
      goto cleanup;
    }

  /* Load a directout instrument to send the sound to the DAC. */
  OutputIns = LoadInstrument ("directout.dsp", 0, 100);
  CHECKRESULT (OutputIns, "LoadInstrument");
  StartInstrument (OutputIns, NULL);

  /* Load digital audio AIFF Sample file from disk. */
  SampleItem = LoadSample (SampleName);
  CHECKRESULT (SampleItem, "LoadSample");

  /* Look at sample information for fun. */
  DebugSample (SampleItem);

  /* Select an apropriate sample playing instrument based on sample format. */
  InstrumentName = SelectSamplePlayer (SampleItem, IfVariable);
  if (InstrumentName == NULL)
    {
      ERR (("No instrument to play that sample.\n"));
      goto cleanup;
    }

  /* Load Sampler instrument */
  PRT (("Use instrument: %s\n", InstrumentName));
  SamplerIns = LoadInstrument (InstrumentName, 0, 100);
  CHECKRESULT (SamplerIns, "LoadInstrument");

  /* Connect Sampler Instrument to DirectOut. Works for mono or stereo. */
  Result = ConnectInstruments (SamplerIns, "Output", OutputIns, "InputLeft");
  if (Result >= 0)
    {
      Result
          = ConnectInstruments (SamplerIns, "Output", OutputIns, "InputRight");
      CHECKRESULT (Result, "ConnectInstruments");
    }
  else
    {
      Result = ConnectInstruments (SamplerIns, "LeftOutput", OutputIns,
                                   "InputLeft");
      CHECKRESULT (Result, "ConnectInstruments");
      Result = ConnectInstruments (SamplerIns, "RightOutput", OutputIns,
                                   "InputRight");
      CHECKRESULT (Result, "ConnectInstruments");
    }

  /* Attach the sample to the instrument for playback. */
  Attachment = AttachSample (SamplerIns, SampleItem, 0);
  CHECKRESULT (Attachment, "AttachSample");

  /*
  ** Start the instrument at the given "rate", which is the Proportion of
  ** 44,100 Hz for the sample rate expressed as a 1.15 fraction. Unity is
  *32768.
  ** Note that adjusting the rate of a fixed-rate sample player has no effect.
  ** You could also pass AF_TAG_PITCH and AF_TAG_AMPLITUDE to
  *StartInstrument().
  ** See StartInstrument() autodocs for more detail.
  */
  Result = StartInstrumentVA (SamplerIns, AF_TAG_RATE, Rate, TAG_END);

  /* Interactive event loop. */
  while (DoIt)
    {
      /* Get User input. */
      Result = GetControlPad (1, TRUE, &cped);
      if (Result < 0)
        {
          PrintError (0, "read control pad in", "PlaySoundFile", Result);
        }
      Buttons = cped.cped_ButtonBits;

      /* Process buttons pressed. */
      if (Buttons & ControlX) /* EXIT */
        {
          DoIt = FALSE;
        }
      if (Buttons & ControlA) /* START */
        {
          Result = StartInstrumentVA (SamplerIns, AF_TAG_RATE, Rate, TAG_END);
          CHECKRESULT (Result, "StartInstrument");
        }
      if (Buttons & ControlB) /* RELEASE */
        {
          Result = ReleaseInstrument (SamplerIns, NULL);
          CHECKRESULT (Result, "ReleaseInstrument");
        }
      if (Buttons & ControlC) /* STOP */
        {
          Result = StopInstrument (SamplerIns, NULL);
          CHECKRESULT (Result, "StopInstrument");
        }
    }

  Result = StopInstrument (SamplerIns, NULL);
  CHECKRESULT (Result, "StopInstrument");

cleanup:

  DetachSample (Attachment);
  UnloadSample (SampleItem);
  UnloadInstrument (SamplerIns);
  UnloadInstrument (OutputIns);

  /* Cleanup the EventBroker. */
  Result = KillEventUtility ();
  if (Result < 0)
    {
      PrintError (0, "KillEventUtility", 0, Result);
    }
  CloseAudioFolio ();
  PRT (("%s finished.\n", argv[0]));
  return ((int)Result);
}
