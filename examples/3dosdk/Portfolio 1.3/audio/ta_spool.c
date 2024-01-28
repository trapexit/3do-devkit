/***************************************************************
**
** Demonstrate the sspl sound spooler.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "mem.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

/* Include this when using the Audio Folio */
#include "audio.h"
#include "soundspooler.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  /*	PRT(("Ran %s, got 0x%x\n", name, val)); */                               \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      ERR (("Failure in %s: 0x%x\n", name, val));                             \
      PrintfSysErr (Result);                                                  \
      goto cleanup;                                                           \
    }

#define NUM_BUFFERS (4)
#define NUMCHANNELS (2) /* Stereo */
#define NUMFRAMES (8 * 1024)
#define SAMPSIZE (sizeof (int16) * NUMFRAMES * NUMCHANNELS)

/************************************************************************
** Fill in a sample with sawtooth waves.
************************************************************************/
void
FillBufferWithSaw (int16 *Data, int32 NumSamples, int32 Octave)
{
  int32 i;

  /* Fill sample with a sawtooth  in different octaves. */
  for (i = 0; i < NumSamples; i++)
    {
      *Data++ = (int16)((i << (7 + Octave)) & 0xFFFF);
    }
}

/************************************************************************
** Callback function for buffer completion processor.
************************************************************************/
void
MyBufferProcessor (SoundSpooler *sspl, SoundBufferNode *sbn)
{
  PRT (("MyBufferProcessor: 0x%08x done, signal = 0x%08x, BufIndex = %d\n",
        sbn, sbn->sbn_Signal, ssplGetUserData (sspl, sbn)));
}

/************************************************************************
** Main test.
************************************************************************/
int
main (int argc, char *argv[])
{
  /* Declare local variables */
  Item OutputIns;
  Item SamplerIns;
  int32 Result;
  int32 SignalMask;
  SoundSpooler *sspl;
  char *Data[NUM_BUFFERS];
  int32 MySignal[NUM_BUFFERS], CurSignals;
  int32 i;
  int32 BufIndex, Joy;
  ControlPadEventData cped;

  PRT (("%s started.\n", argv[0]));

  /* Initalize local variables */
  BufIndex = 0;
  OutputIns = 0;
  SamplerIns = 0;
  Result = -1;
  sspl = NULL;

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (Result < 0)
    {
      ERR (("main: error in InitEventUtility\n"));
      PrintfSysErr (Result);
      goto cleanup;
    }

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Use directout instead of mixer. */
  OutputIns = LoadInstrument ("directout.dsp", 0, 0);
  CHECKRESULT (OutputIns, "LoadInstrument");
  Result = StartInstrument (OutputIns, NULL);
  CHECKRESULT (SamplerIns, "StartInstrument: OutputIns");

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

  /* Allocate and fill buffers with arbitrary test data. */
  for (i = 0; i < NUM_BUFFERS; i++)
    {
      Data[i] = (char *)AllocMem (SAMPSIZE, MEMTYPE_AUDIO);
      if (Data[i] == NULL)
        {
          ERR (("Not enough memory for sample .\n"));
          goto cleanup;
        }
      FillBufferWithSaw ((int16 *)Data[i], SAMPSIZE / sizeof (int16), i);
    }

  /* Create SoundSpooler data structure. */
  sspl = ssplCreateSoundSpooler (NUM_BUFFERS, SamplerIns);
  if (sspl == NULL)
    {
      ERR (("ssplCreateSoundSpooler failed!\n"));
      goto cleanup;
    }

  /* Fill the sound queue by queuing up all the buffers. "Preroll" */
  BufIndex = 0;
  SignalMask = 0;
  for (i = 0; i < NUM_BUFFERS; i++)
    {
      /* Dispatch buffers full of sound to spooler. Set User Data to BufIndex.
      ** ssplSpoolData returns a signal which can be checked to see when the
      *data
      ** has completed it playback.
      */
      MySignal[BufIndex]
          = ssplSpoolData (sspl, Data[BufIndex], SAMPSIZE, (void *)BufIndex);
      if (MySignal[BufIndex] <= 0)
        {
          ERR (("ssplSpoolBuffer failed 0x%x\n", MySignal[BufIndex]));
          goto cleanup;
        }
      SignalMask |= MySignal[BufIndex];
      BufIndex++;
      if (BufIndex >= NUM_BUFFERS)
        BufIndex = 0;
    }

  /* Start Spooler instrument. Will begin playing any queued buffers. */
  Result = ssplStartSpooler (sspl, 0x7FFF);
  CHECKRESULT (Result, "ssplStartSpooler");

  /* Play buffers loop. */
  do
    {
      /* Wait for some buffer(s) to complete. */
      DBUG (("WaitSignal(0x%x)\n", SignalMask));
      CurSignals = WaitSignal (SignalMask);
      DBUG (("WaitSignal() got 0x%x\n", CurSignals));

      /* Tell sound spooler that the buffer(s) have completed. */
      Result = ssplProcessSignals (sspl, CurSignals, MyBufferProcessor);
      CHECKRESULT (Result, "ssplProcessSignals");

      /* Read current state of Control Pad. */
      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          ERR (("Error in GetControlPad\n"));
          PrintfSysErr (Result);
        }
      Joy = cped.cped_ButtonBits;

      /* Simulate data starvation. Hang if we hit A button. */
      if (Joy & ControlA)
        {
          PRT (("Hang spooler.\n"));
          do
            {
              Result = GetControlPad (1, TRUE, &cped);
              Joy = cped.cped_ButtonBits;
            }
          while (Joy & ControlA);
          PRT (("Unhang spooler.\n"));
        }

      /* Pause if we hit button B. */
      if (Joy & ControlB)
        {
          PRT (("Pause.\n"));
          ssplPause (sspl);
          do
            {
              Result = GetControlPad (1, TRUE, &cped);
              Joy = cped.cped_ButtonBits;
            }
          while (Joy & ControlB);
          ssplResume (sspl);
          PRT (("Resume.\n"));
        }

      /* Single shot mode. Send single buffers by hiting C, until Start hit. */
      if (Joy & ControlC)
        {
          PRT (("Single shot mode.\n"));
          do
            {
              Result = GetControlPad (1, TRUE, &cped);
              Joy = cped.cped_ButtonBits;
              if (Joy & ControlC)
                {
                  if (ssplPlayData (sspl, Data[BufIndex], SAMPSIZE) > 0)
                    {
                      BufIndex++;
                      if (BufIndex >= NUM_BUFFERS)
                        BufIndex = 0;
                    }
                }
            }
          while ((Joy & ControlStart) == 0);
        }

      /*
      ** Spool as many buffers as are available.
      ** ssplSpoolData will return positive signals as long as it accepted the
      *data.
      */
      while (ssplSpoolData (sspl, Data[BufIndex], SAMPSIZE, (void *)BufIndex)
             > 0)
        {
          /* INSERT YOUR CODE HERE TO FILL UP THE NEXT BUFFER */
          BufIndex++;
          if (BufIndex >= NUM_BUFFERS)
            BufIndex = 0;
        }
    }
  while ((Joy & ControlX) == 0);

  /* Stop Spooler. */
  Result = ssplStopSpooler (sspl);
  CHECKRESULT (Result, "StopSoundSpooler");

cleanup:
  TraceAudio (0);
  ssplDeleteSoundSpooler (sspl);
  UnloadInstrument (SamplerIns);
  UnloadInstrument (OutputIns);
  CloseAudioFolio ();
  KillEventUtility ();
  PRT (("All done.\n"));
  return ((int)Result);
}
