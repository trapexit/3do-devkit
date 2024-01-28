/***************************************************************
**
** Play Simple Sawtooth waveform
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/*
** 11/16/92 PLB Modified for explicit mixer connect.
** 11/18/92 PLB Added ChangeDirectory("/remote") for filesystem.
** 930315 PLB Conforms to new API
*/

#include "debug.h"
#include "filefunctions.h"
#include "operamath.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define SAWINSNAME "sawenv.dsp"
#define OUTPUTNAME "directout.dsp"

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
      ERR (("Failure in %s: $%x\n", name, val));                              \
      goto cleanup;                                                           \
    }

/* Define Tags for StartInstrument */
TagArg SawTags[] = { { AF_TAG_AMPLITUDE, 0 }, { AF_TAG_RATE, 0 }, { 0, 0 } };

#define NOTESPEROCTAVE (5)
#define BASENOTE (AF_A440_PITCH)
#define BASEFREQ (440) /* A440 */
ufrac16 TuningTable[NOTESPEROCTAVE];

/* Define Tags for StartInstrument */
TagArg StartTags[]
    = { { AF_TAG_PITCH, 0 }, { AF_TAG_VELOCITY, 0 }, { TAG_END, 0 } };

/***************************************************************/
int32
PlayPitchNote (Item Instrument, int32 Note, int32 Velocity)
{
  int32 Result;

  StartTags[0].ta_Arg = (void *)Note;
  StartTags[1].ta_Arg = (void *)Velocity;
  Result = StartInstrument (Instrument, &StartTags[0]);
  Result = SleepAudioTicks (20);

  ReleaseInstrument (Instrument, NULL);
  Result = SleepAudioTicks (20);
  return 0;
}

/***************************************************************/
int
main (int argc, char *argv[])
{
  Item SawTmp;
  Item SawIns;
  Item OutputIns;
  Item Slendro;
  int32 Result;
  int32 i;
  frac16 BaseFreq;

  PRT (("\n%s\n", argv[0]));

  /* FindMathFolio to get MathBase */
  Result = OpenMathFolio ();
  if (Result < 0)
    {
      ERR (("FindMathFolio() failed! Did you run operamath?\n"));
      PrintfSysErr (Result);
      return Result;
    }

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Use directout instead of mixer. */
  OutputIns = LoadInstrument (OUTPUTNAME, 0, 0);
  CHECKRESULT (OutputIns, "LoadInstrument");
  StartInstrument (OutputIns, NULL);

  /* Load template of Sawtooth instrument */
  SawTmp = LoadInsTemplate (SAWINSNAME, 0);
  CHECKRESULT (SawTmp, "LoadInsTemplate");

  /* Make an instrument based on template. */
  SawIns = AllocInstrument (SawTmp, 0);
  CHECKRESULT (SawIns, "AllocInstrument");

  /* Connect Sawtooth to both sides of Mixer */
  Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
  CHECKRESULT (Result, "ConnectInstruments");
  Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputRight");
  CHECKRESULT (Result, "ConnectInstruments");

  /*	TraceAudio(TRACE_ENVELOPE); */

  /* Play an ascending scale using the default 12 toned equal tempered tuning.
   */
  PRT (("12 tone equal tempered scale.\n"));
  for (i = 50; i < (BASENOTE + (2 * NOTESPEROCTAVE)); i++)
    {
      PlayPitchNote (SawIns, i, 80);
    }

  /* Create a custom just intoned pentatonic tuning. */
  /* Calculate frequencies as ratios from the base frequency. */
  BaseFreq = Convert32_F16 (BASEFREQ);
  TuningTable[0] = BaseFreq; /* 1:1 */
  TuningTable[1] = MulUF16 (BaseFreq, DivUF16 (8, 7));
  TuningTable[2] = MulUF16 (BaseFreq, DivUF16 (5, 4));
  TuningTable[3] = MulUF16 (BaseFreq, DivUF16 (3, 2));
  TuningTable[4] = MulUF16 (BaseFreq, DivUF16 (7, 4));

  /* Create a tuning item that can be used with many instruments. */
  Slendro
      = CreateTuning (TuningTable, NOTESPEROCTAVE, NOTESPEROCTAVE, BASENOTE);
  CHECKRESULT (Slendro, "CreateTuning");

  /* Tell an instrument to use this tuning. */
  Result = TuneInstrument (SawIns, Slendro);
  CHECKRESULT (Result, "TuneInstrument");

  /* Play the same ascending scale using the custom tuning. */
  PRT (("Custom pentatonic scale.\n"));
  for (i = 50; i < (BASENOTE + (2 * NOTESPEROCTAVE)); i++)
    {
      PlayPitchNote (SawIns, i, 80);
    }

cleanup:
  /* The Audio Folio is immune to passing NULL values as Items. */
  FreeInstrument (SawIns);
  UnloadInsTemplate (SawTmp);

  UnloadInstrument (OutputIns);
  PRT (("%s all done.\n", argv[0]));
  TraceAudio (0);
  CloseAudioFolio ();
  return ((int)Result);
}
