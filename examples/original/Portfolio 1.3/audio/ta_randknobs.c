/* $Id: ta_randknobs.c,v 1.11 1994/04/04 23:49:54 phil Exp $ */
/***************************************************************
**
** Play Knobs Randomly
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/*
** 921116 PLB Modified for explicit mixer connect.
** 921118 PLB Added ChangeDirectory("/remote") for filesystem.
** 921202 PLB Converted to LoadInstrument and GrabKnob, improved output.
** 921203 PLB Use /remote/dsp
** 930315 PLB Conforms to new API
** 940403 PLB Removed definitions of rand() and srand().
*/

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

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\failure in", name, Result);                           \
      goto cleanup;                                                           \
    }

#define MAX_KNOBS 32

int32 choose (int32 range);

Item MixerIns = -1;
Item LeftGainKnob = -1;
Item RightGainKnob = -1;
int32 SetupMixer (void);

int
main (int argc, char *argv[])
{
  Item TestIns = 0;
  Item NoiseIns = 0;
  Item NewDir;
  int32 i, j, Val, kn;
  int32 NumKnobs = 0, Result = -1;
  char *Name[MAX_KNOBS];
  Item TestKnob[MAX_KNOBS];
  int32 Min[MAX_KNOBS], Max[MAX_KNOBS], Default[MAX_KNOBS], err;
  TagArg Tags[] = {
    { AF_TAG_MIN, 0 },
    { AF_TAG_DEFAULT, 0 },
    { AF_TAG_MAX, 0 },
    { TAG_END, 0 },
  };

  /* Initialize audio, return if error. */
  NewDir = ChangeDirectory ("/remote/dsp");
  if (NewDir > 0)
    {
      PRT (("Directory changed to /remote/dsp for Mac based file-system.\n"));
    }
  else
    {
      ERR (("Directory /remote/dsp not found! Err = $%x\n", NewDir));
    }

  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  if (SetupMixer ())
    return -1;

  /* Load instrument definition/template */
  if (argc < 2)
    {
      PRT (("Usage:   ta_tweaknobs <filename.dsp>\n"));
      goto cleanup;
    }
  TestIns = LoadInstrument (argv[1], 0, 100);
  CHECKRESULT (TestIns, "LoadInstrument");
  NoiseIns = LoadInstrument ("noise.dsp", 0, 100);
  CHECKRESULT (TestIns, "LoadInstrument noise");

  PRT (("Connect Instruments, Noise -> Input\n"));
  Result = ConnectInstruments (NoiseIns, "Output", TestIns, "Input");
  if (Result < 0)
    PRT (("%s has no Input\n", argv[1]));

  /* Connect to Mixer */
  PRT (("Connect Instruments, Saw -> Mixer\n"));
  Result = ConnectInstruments (TestIns, "Output", MixerIns, "Input0");
  CHECKRESULT (Result, "ConnectInstruments");

  /* Play a note using StartInstrument and default Freq and Amplitude */
  Result = StartInstrument (TestIns, NULL);

  NumKnobs = GetNumKnobs (TestIns);
  PRT (("%d knobs.\n", NumKnobs));

  /* Attach all available knobs */
  for (i = 0; i < NumKnobs; i++)
    {
      Name[i] = GetKnobName (TestIns, i);
      if (Name[i] != NULL)
        {
          /* Attach knob so we can what's there. */
          TestKnob[i] = GrabKnob (TestIns, Name[i]);
          CHECKRESULT (TestKnob[i], "GrabKnob");

          /* Get attributes of knob. */
          err = GetAudioItemInfo (TestKnob[i], Tags);
          CHECKRESULT (err, "GetKnobInfo");
          /* Now Pull Values from TagList */
          Min[i] = (int32)Tags[0].ta_Arg;
          Default[i] = (int32)Tags[1].ta_Arg;
          Max[i] = (int32)Tags[2].ta_Arg;
        }
    }

  srand (121);

  /* Set defaults. */
  for (j = 0; j < NumKnobs; j++)
    {
      PRT (("Knob = %s\n", Name[j]));
      PRT (("   $%x, $%x, $%x\n", Min[j], Default[j], Max[j]));
      TweakKnob (TestKnob[j], Default[j]);
    }

#define NUMSTEPS 240
  /* Randomize the value of each knob. */
  for (i = 0; i < 100; i++)
    {
      /* Select a knob to tweak */
      kn = choose (NumKnobs);

      /* Randomly tweak selected knobs values between Min and Max */
      Val = Min[kn] + choose (Max[kn] - Min[kn]);
      PRT (("%8d => %s\n", Val, Name[kn]));
      TweakKnob (TestKnob[kn], Val);
      SleepAudioTicks (10);
    }

  StopInstrument (TestIns, NULL);

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */
  for (i = 0; i < NumKnobs; i++)
    {
      ReleaseKnob (TestKnob[i]);
    }
  UnloadInstrument (TestIns);
  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  UnloadInstrument (MixerIns);
  UnloadInstrument (NoiseIns);
  CloseAudioFolio ();
  return ((int)Result);
}

int32
choose (int32 range)
{
  int32 val, r16;

  r16 = rand () & 0xFFFF;
  val = (r16 * range) >> 16;
  return val;
}

/*********************************************************************/
int32
SetupMixer ()
{
  int32 Result = 0, VoiceID;

  MixerIns = LoadInstrument ("mixer4x2.dsp", 0, 0);
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
  VoiceID = StartInstrument (MixerIns, NULL);
  return Result;

cleanup:
  ReleaseKnob (LeftGainKnob);
  ReleaseKnob (RightGainKnob);
  UnloadInstrument (MixerIns);
  return Result;
}
