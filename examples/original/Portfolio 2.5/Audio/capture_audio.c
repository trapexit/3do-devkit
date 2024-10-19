
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: capture_audio.c,v 1.8 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/capture_audio
|||	capture_audio - Record the output from the DSPP to a host file.
|||
|||	  Synopsis
|||
|||	    capture_audio [\<num frames>] [\<dest file>]
|||
|||	  Description
|||
|||	    Captures the output from the DSP into a file on the development
station's
|||	    filesystem. It can be used to check the sound output of a program
at
|||	    important points.
|||
|||	    Once the output from the DSP has been captured, you can load it
into
|||	    SoundHack and edit it as follows:
|||
|||	    1. From the File menu in SoundHack, choose Open Any.
|||
|||	    2. Select the file that was captured with capture_audio.
|||
|||	    3. Use the Header Change command and set the captured file's header
to a
|||	    rate of 44100, with 2 channels, and a format of 16-bit Linear.
|||
|||	    4. Select Save As command and save the sound file as a 16-bit AIFF
format
|||	    file.
|||
|||	    You can then load the AIFF file you created and listen to the
captured
|||	    sounds.
|||
|||	  Arguments
|||
|||	    num frames                    Number of sample frames to capture.
|||	                                 Defaults to 10000.
|||
|||	    dest file                    Name of the file to save the captured
frames
|||	                                 to. Defaults to capture.raw.
|||
|||	  Caveats
|||
|||	    This program adds an instrument at priority 0. Because the
execution order
|||	    of equal priority instruments depends isn't as predictable as those
of
|||	    differing priority, this program may not be able to capture from
other
|||	    instruments at priority 0.
|||
|||	  Associated Files
|||
|||	    capture_audio.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
**/

#include "debug.h"
#include "mem.h"
#include "operror.h"
#include "stdarg.h"
#include "stdio.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define NUMCHANNELS (2) /* Stereo */

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
      PrintError (0, "\\failure in", name, Result);                           \
      goto cleanup;                                                           \
    }

/***********************************************************************/
Err
WriteDataToMac (char *DataPtr, int32 NumBytes, char *FileName)
{
  FILE *fid;
  int32 Result = 0;
  int32 NumWritten;

  fid = fopen (FileName, "w");
  if (fid == NULL)
    {
      printf ("Could not open %s\n", FileName);
      return -1;
    }

  NumWritten = fwrite (DataPtr, 1, NumBytes, fid);
  if (NumWritten != NumBytes)
    {
      ERR (("Error writing file.\n"));
      Result = -2;
      goto cleanup;
    }

cleanup:
  if (fid)
    fclose (fid);
  return Result;
}

/***********************************************************************/
int
main (int argc, char *argv[])
{
  /* Declare local variables */
  Item DelayLine = 0, DelayIns = 0;
  Item DelayAtt = 0;
  Item TapIns;
  Item SleepCue = 0;
  int32 NumBytes, NumFrames, NumTicks;
  int32 Result;
  char *FileName;
  uint16 *DelayData;

  PRT (("%s <#frames> <MacFileName>\n", argv[0]));

  /* Get input parameters. */
  NumFrames = (argc > 1) ? atoi (argv[1]) : 10000;
  PRT (("Capture %d frames.\n", NumFrames));
  NumBytes = NumFrames * NUMCHANNELS * sizeof (int16);

  FileName = (argc > 2) ? argv[2] : "capture.raw";

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Load instrument to tap output.  Zero priority to be at end. */
  TapIns = LoadInstrument ("tapoutput.dsp", 0, 0);
  CHECKRESULT (TapIns, "LoadInstrument");

  /* Create a Cue for signalback */
  SleepCue = CreateCue (NULL);
  CHECKRESULT (SleepCue, "CreateCue");

  DelayLine = CreateDelayLine (NumBytes, 2, FALSE);
  CHECKRESULT (DelayLine, "CreateDelayLine");

  /* Find out the delay line address so we can save the data. */
  {
    TagArg Tags[] = { { AF_TAG_ADDRESS }, TAG_END };

    Result = GetAudioItemInfo (DelayLine, Tags);
    CHECKRESULT (Result, "GetAudioItemInfo");
    DelayData = (uint16 *)Tags[0].ta_Arg;
  }
  /*
  ** Load the basic delay instrument which just writes data to
  ** an output DMA channel of the DSP.
  */
  DelayIns = LoadInstrument ("delaystereo.dsp", 0, 50);
  CHECKRESULT (DelayIns, "LoadInstrument");

  /* Attach the delay line to the delay instrument output. */
  DelayAtt = AttachSample (DelayIns, DelayLine, "OutFIFO");
  CHECKRESULT (DelayAtt, "AttachDelay");

  /* Connect tap Instrument to Delays */
  Result = ConnectInstruments (TapIns, "LeftOutput", DelayIns, "InputLeft");
  CHECKRESULT (Result, "ConnectInstruments");
  Result = ConnectInstruments (TapIns, "RightOutput", DelayIns, "InputRight");
  CHECKRESULT (Result, "ConnectInstruments");

  /* Start capturing sound. */
  Result = StartInstrument (TapIns, NULL);
  CHECKRESULT (Result, "StartInstrument");
  Result = StartInstrument (DelayIns, NULL);
  CHECKRESULT (Result, "StartInstrument");

  /* Wait for delay line to fill. */
  NumTicks = NumFrames / GetAudioDuration ();
  SleepUntilTime (SleepCue, GetAudioTime () + NumTicks + 1);

  /* Write data to disk using Mac file access routines. */
  Result = WriteDataToMac ((char *)DelayData, NumBytes, FileName);
  CHECKRESULT (Result, "WriteDataToMac");

  PRT (("Captured audio written to file named %s\n", FileName));

cleanup:
  DeleteDelayLine (DelayLine);
  UnloadInstrument (DelayIns);
  DeleteCue (SleepCue);
  UnloadInstrument (TapIns);

  CloseAudioFolio ();
  PRT (("All done.\n"));
  return ((int)Result);
}
