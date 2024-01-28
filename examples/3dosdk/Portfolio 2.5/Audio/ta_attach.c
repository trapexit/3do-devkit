
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ta_attach.c,v 1.21 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ta_attach
|||	ta_attach - Experiments with sample attachments.
|||
|||	  Synopsis
|||
|||	    ta_attach
|||
|||	  Description
|||
|||	    Creates a pair of software-generated samples and attaches them to a
sample
|||	    player instrument. The attachments are then linked to one another
using
|||	    LinkAttachment() such that they play in a loop.
|||
|||	    This demonstrates how multiple attachments can be linked together
to form
|||	    a queue of sample buffers to play and how a client can receive
|||	    notification of when each sample buffer has finished playing. This
is the
|||	    technique that the Sound Spooler uses to queue sound buffers for
playback.
|||
|||	  Associated Files
|||
|||	    ta_attach.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
|||	  See Also
|||
|||	    ta_spool, ssplCreateSoundSpooler(), LinkAttachments()
|||
**/

#include <audio.h>
#include <mem.h>
#include <operror.h>
#include <stdio.h>

/* sample parameters */
#define SAMPWIDTH sizeof (int16)
#define NUMCHANNELS (2) /* Stereo */
#define NUMFRAMES (64 * 1024)
#define SAMPSIZE (SAMPWIDTH * NUMFRAMES * NUMCHANNELS)

/* assorted macros */
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

/* Variables local to this file. */
static Item SamplerIns;

/* Local function prototypes. */
static int32 BuildSample (Item *SampPtr, Item *CuePtr, Item *AttPtr,
                          int32 Octave);
static void TeardownSample (Item sample);

int
main (int argc, char *argv[])
{
  /* Declare local variables */
  Item OutputIns;
  Item Samp1, Samp2;
  Item Att1, Att2;
  int32 Signal1, Signal2, SignalHit;
  Item Cue1, Cue2;
  int32 atnum, i;
  int32 Result;

  /* Initalize global variables */
  SamplerIns = 0;

  /* Initalize local variables */
  OutputIns = 0;
  Samp1 = 0;
  Samp2 = 0;
  Att1 = 0;
  Att2 = 0;
  Signal1 = 0;
  Signal2 = 0;
  Cue1 = 0;
  Cue2 = 0;
  Result = -1;

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

  /* Load fixed rate stereo sample player instrument */
  SamplerIns = LoadInstrument ("fixedstereosample.dsp", 0, 100);
  CHECKRESULT (SamplerIns, "LoadInstrument");

  /* Connect Sampler Instrument to DirectOut */
  Result
      = ConnectInstruments (SamplerIns, "LeftOutput", OutputIns, "InputLeft");
  CHECKRESULT (Result, "ConnectInstruments");
  Result = ConnectInstruments (SamplerIns, "RightOutput", OutputIns,
                               "InputRight");
  CHECKRESULT (Result, "ConnectInstruments");

  /* Build samples via software synthesis. */
  Result = BuildSample (&Samp1, &Cue1, &Att1, 2);
  CHECKRESULT (Result, "BuildSample");
  Result = BuildSample (&Samp2, &Cue2, &Att2, 0);
  CHECKRESULT (Result, "BuildSample");

  /* Get signals from Cues so we can call WaitSignal() */
  Signal1 = GetCueSignal (Cue1);
  Signal2 = GetCueSignal (Cue2);

  /* Link the Attachments in a circle to simulate double buffering. */
  LinkAttachments (Att1, Att2);
  LinkAttachments (Att2, Att1);

  /* Request a signal when the sample ends. */
  Result = MonitorAttachment (Att1, Cue1, CUE_AT_END);
  CHECKRESULT (Result, "MonitorAttachment");

  Result = MonitorAttachment (Att2, Cue2, CUE_AT_END);
  CHECKRESULT (Result, "MonitorAttachment");

  Result = StartInstrument (SamplerIns, NULL);
  CHECKRESULT (Result, "StartInstrument");

  /* Let it loop several times. */
  for (i = 0; i < 7; i++)
    {
      SignalHit = WaitSignal (Signal1 | Signal2);
      atnum = (SignalHit & Signal1) ? 1 : 2;
      PRT (("Received signal from attachment %d!\n", atnum));
    }

  /* Unlink so that current one just finishes. */
  LinkAttachments (Att1, 0);
  LinkAttachments (Att2, 0);

  /* Wait for current one to finish. */
  SignalHit = WaitSignal (Signal1 | Signal2);
  atnum = (SignalHit & Signal1) ? 1 : 2;
  PRT (("Received FINAL signal from attachment %d!\n", atnum));

  ReleaseInstrument (SamplerIns, NULL);

cleanup:
  UnloadInstrument (OutputIns);
  UnloadInstrument (SamplerIns);
  DeleteCue (Cue1);
  DeleteCue (Cue2);
  DetachSample (Att1);
  DetachSample (Att2);
  TeardownSample (Samp1);
  TeardownSample (Samp2);
  CloseAudioFolio ();
  PRT (("All done.\n"));
  return ((int)Result);
}

/************************************************************************/
/* Fill in a sample with sawtooth waves. */
static int32
BuildSample (Item *SampPtr, Item *CuePtr, Item *AttPtr, int32 Octave)
{
  int16 *Data = NULL;
  Item Samp = 0, Cue = 0, Att = 0;
  int32 Result;

  /* Allocate memory for sample */
  if ((Data = (int16 *)AllocMem (SAMPSIZE, MEMTYPE_AUDIO)) == NULL)
    {
      Result = MakeErr (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
      goto cleanup;
    }

  /* Create sample item from sample parameters */
  Samp = CreateSampleVA (AF_TAG_ADDRESS, Data, AF_TAG_WIDTH, SAMPWIDTH,
                         AF_TAG_CHANNELS, NUMCHANNELS, AF_TAG_FRAMES,
                         NUMFRAMES, TAG_END);
  CHECKRESULT (Samp, "CreateSample");

  /* Create a Cue for signalback */
  Cue = CreateCue (NULL);
  CHECKRESULT (Cue, "CreateItem Cue");

  /* Attach the sample to the instrument. */
  Att = AttachSample (SamplerIns, Samp, 0);
  CHECKRESULT (Att, "AttachSample");

  /* Fill sample with a sawtooth at a high and low frequency in different
   * channels. */
  {
    int32 i;
    int16 *tdata = Data;

    for (i = 0; i < NUMFRAMES; i++)
      {
        *tdata++ = (int16)((((i << (8 + Octave)) & 0xFFFF) * i) / NUMFRAMES);
        *tdata++ = (int16)((((i << (6 + Octave)) & 0xFFFF) * (NUMFRAMES - i))
                           / NUMFRAMES); /* Lower frequency. */
      }
  }

  /* Return values to caller. */
  *SampPtr = Samp;
  *CuePtr = Cue;
  *AttPtr = Att;
  return 0;

cleanup:
  DetachSample (Att);
  DeleteCue (Cue);
  UnloadSample (Samp);
  FreeMem (Data, SAMPSIZE);
  return Result;
}

static void
TeardownSample (Item sample)
{
  int16 *data = NULL;

  /* get address of sample data */
  {
    TagArg Tags[] = { { AF_TAG_ADDRESS }, TAG_END };

    if (GetAudioItemInfo (sample, Tags) >= 0)
      {
        data = (int16 *)Tags[0].ta_Arg;
      }
  }

  /* get rid of sample item */
  UnloadSample (sample); /* There is unfortunately no DeleteSample()	yet.
                            Until there is, this is the correct function to
                            balance CreateSample(). */

  /* free sample data */
  FreeMem (data, SAMPSIZE);
}
