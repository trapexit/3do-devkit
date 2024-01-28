/* $Id: ta_makesamp.c,v 1.15 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************
**
** Make a stereo sample using software synthesis in 'C' then play it.
** ta_makesamp.c
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
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define NUMCHANNELS (2)
#define NUMFRAMES (32 * 1024)
#define SAMPSIZE (sizeof (int16) * NUMFRAMES * NUMCHANNELS)

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) PRT (x)

/* Define Tags for StartInstrument */

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, Result);                           \
      goto cleanup;                                                           \
    }

int
main (int argc, char *argv[])
{
  Item SamplerIns = 0;
  Item SampleItem = 0;
  Item OutputIns = 0;
  Item Attachment = 0;
  int32 freq;
  int32 i, Result = -1;
  int16 *Data;
  TagArg Tags[4];

  PRT (("Usage: %s <samplefile> <speed>\n", argv[0]));

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Use directout instead of mixer. */
  OutputIns = LoadInstrument ("directout.dsp", 0, 0);
  CHECKRESULT (OutputIns, "LoadInstrument");
  StartInstrument (OutputIns, NULL);

  /* Load Sampler instrument */
  SamplerIns = LoadInstrument ("fixedstereosample.dsp", 0, 100);
  CHECKRESULT (SamplerIns, "LoadInstrument");
  TraceAudio (0);

  /* Make it stereo */
  Tags[0].ta_Tag = AF_TAG_CHANNELS;
  Tags[0].ta_Arg = (int32 *)2;
  Tags[1].ta_Tag = AF_TAG_SUSTAINBEGIN;
  Tags[1].ta_Arg = (int32 *)0;
  Tags[2].ta_Tag = AF_TAG_SUSTAINEND;
  Tags[2].ta_Arg = (int32 *)(NUMFRAMES - 1);
  Tags[3].ta_Tag = TAG_END;

  SampleItem = MakeSample (SAMPSIZE, Tags);
  CHECKRESULT (SampleItem, "MakeSample");
  DBUG (("SampleItem = %x\n", SampleItem));

  if (argc > 2)
    {
      freq = atoi (argv[2]);
    }
  else
    {
      freq = 0x8000;
    }
  DBUG (("Frequency = 0x%x\n", freq));

  /* Look at sample information. */
  DebugSample (SampleItem);

  /* Connect Sampler to Mixer */
  Result
      = ConnectInstruments (SamplerIns, "LeftOutput", OutputIns, "InputLeft");
  CHECKRESULT (Result, "ConnectInstruments");
  Result = ConnectInstruments (SamplerIns, "RightOutput", OutputIns,
                               "InputRight");
  CHECKRESULT (Result, "ConnectInstruments");

  /* Attach the sample to the instrument. */
  Attachment = AttachSample (SamplerIns, SampleItem, 0);
  CHECKRESULT (Attachment, "AttachSample");

  /*
  ** Get data from sample using by sending a Taglist that gets filled in with
  *the
  ** current values.
  */
  Tags[0].ta_Tag = AF_TAG_ADDRESS;
  Tags[0].ta_Arg = NULL;
  Tags[1].ta_Tag = TAG_END;
  Result = GetAudioItemInfo (SampleItem, Tags);
  CHECKRESULT (Result, "GetAudioItemInfo");
  Data = Tags[0].ta_Arg;

  /* Fill sample with a sawtooth at a high and low frequency in different
   * channels. */
  for (i = 0; i < NUMFRAMES; i++)
    {
      *Data++ = (int16)((((i << 8) & 0xFFFF) * i) / NUMFRAMES);
      *Data++ = (int16)((((i << 6) & 0xFFFF) * (NUMFRAMES - i))
                        / NUMFRAMES); /* Lower frequency. */
    }

/* Play one note a long time. */
#define DURATION 30
  DBUG (("Play sample for %d seconds.\n", DURATION));
  Tags[0].ta_Tag = AF_TAG_AMPLITUDE;
  Tags[0].ta_Arg = (int32 *)0x7FFF;
  Tags[1].ta_Tag = AF_TAG_RATE;
  Tags[1].ta_Arg = (int32 *)freq;
  Tags[2].ta_Tag = TAG_END;
  Result = StartInstrument (SamplerIns, &Tags[0]);
  Result = SleepAudioTicks (60 * DURATION);

  ReleaseInstrument (SamplerIns, NULL);
  Result = SleepAudioTicks (60);

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */
  DetachSample (Attachment);
  UnloadSample (SampleItem);
  UnloadInstrument (SamplerIns);
  UnloadInstrument (OutputIns);

  CloseAudioFolio ();
  return ((int)Result);
}
