
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: beep.c,v 1.6 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/beep
|||	beep - Simple beep demonstration.
|||
|||	  Synopsis
|||
|||	    beep
|||
|||	  Description
|||
|||	    Plays synthetic waveform for 2 seconds. This demonstrates loading,
|||	    connecting and playing instruments. It also demonstrates use of the
audio
|||	    timer for time delays.
|||
|||	  Associated Files
|||
|||	    beep.c
|||
|||	  Location
|||
|||	    Examples/Audio
|||
**/

#include "operror.h"
#include "stdio.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }

#define INS_NAME "triangle.dsp"

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (NULL, name, NULL, Result);                                  \
      goto cleanup;                                                           \
    }

int
main (int32 argc, char *argv[])
{
  Item OscIns = 0;
  Item OutputIns = 0;
  Item SleepCue = 0;
  int32 TicksPerSecond;
  int32 Result = -1;

  PRT (("%s\n", argv[0]));

  /* Initialize audio, return if error. */
  if (OpenAudioFolio () < 0)
    {
      PRT (("Audio Folio could not be opened!\n"));
      return (-1);
    }
  /*
  ** The audio clock rate is usually around 240 ticks per second.
  ** It is possible to change the rate using SetAudioRate().
  ** We can query the audio rate by calling GetAudiorate().
  */
  TicksPerSecond = ConvertF16_32 (GetAudioRate ());

  /*
  ** Create a Cue item that we can use with the Audio Timer functions.
  ** It contains a Signal that is used to wake us up.
  */
  SleepCue = CreateCue (NULL);
  CHECKRESULT (SleepCue, "CreateCue");

  /*
  ** Load "directout" for connecting to DAC.
  ** You must connect to a "directout.dsp" or a mixer for
  ** the sound to be heard.
  */
  OutputIns = LoadInstrument ("directout.dsp", 0, 100);
  CHECKRESULT (OutputIns, "LoadInstrument");

  /* Load description of synthetic waveform instrument */
  OscIns = LoadInstrument (INS_NAME, 0, 100);
  CHECKRESULT (OscIns, "LoadInstrument");

  /* Connect output of sawtooth to left and right inputs. */
  Result = ConnectInstruments (OscIns, "Output", OutputIns, "InputLeft");
  CHECKRESULT (Result, "ConnectInstruments");
  Result = ConnectInstruments (OscIns, "Output", OutputIns, "InputRight");
  CHECKRESULT (Result, "ConnectInstruments");

  /*
  ** Start the directout instrument so that you hear all of the
  ** sawtooth output.
  */
  Result = StartInstrument (OutputIns, NULL);
  CHECKRESULT (Result, "StartInstrument OutputIns");

  /*
  ** Play a note using StartInstrument.
  ** You can pass optional TagArgs to control pitch or amplitude.
  */
  Result = StartInstrument (OscIns, NULL);
  CHECKRESULT (Result, "StartInstrument OscIns");

  /*
  ** Go to sleep for about 2 seconds.
  */
  SleepUntilTime (SleepCue, GetAudioTime () + (2 * TicksPerSecond));

  /* Now stop the sound. */
  StopInstrument (OscIns, NULL);
  StopInstrument (OutputIns, NULL);

  PRT (("%s all done.\n", argv[0]));

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */
  UnloadInstrument (OscIns);
  UnloadInstrument (OutputIns);
  DeleteCue (SleepCue);
  CloseAudioFolio ();
  return ((int)Result);
}
