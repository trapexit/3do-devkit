
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ta_spool.c,v 1.16 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ta_spool
|||	ta_spool - Demonstrates the music.lib sound spooler.
|||
|||	  Synopsis
|||
|||	    ta_spool
|||
|||	  Description
|||
|||	    Uses the music.lib sound spooler to fill buffers, parse information
in the
|||	    buffers, signal our task when a buffer has been exhausted, and
refill the
|||	    buffers. Use this sample code as a basis for developing your own
routines
|||	    for playing large sampled files, or handling other kinds of
buffered data.
|||
|||	    When you press one of the buttons, the following events take place:
|||
|||	    * Button Asimulates a hold on data delivery. The application
continues to
|||	      consume information in its buffers.
|||
|||	    * Button Bstops the playback function. Releasing it restarts
playback.
|||	      Holding down either shift button causes the spooler to be aborted
instead
|||	      of merely stopped.
|||
|||	    * Button Centers one-shot mode. Each C button press starts a
ssplPlayData()
|||	       sequence. Press the play button returns to normal mode.
|||
|||	    * Start Buttonpauses the playback function. Releasing it lets the
|||	      application continue.
|||
|||	    * Stop (X) Buttonquit.
|||
|||	  Associated Files
|||
|||	    ta_spool.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
|||	  See Also
|||
|||	    ssplCreateSpooler(), tsp_spoolsoundfile
|||
**/

#include <event.h>
#include <mem.h>
#include <operror.h>
#include <stdio.h>
#include <types.h>

#include <audio.h>
#include <soundspooler.h>

#define DEBUG_Status                                                          \
  0 /* turns on SoundSpooler status display for every cycle thru event loop   \
     */
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
      PrintError (0, name, NULL, Result);                                     \
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
int32
MySoundBufferFunc (SoundSpooler *sspl, SoundBufferNode *sbn, int32 msg)
{
  char *msgdesc;

  switch (msg)
    {
    case SSPL_SBMSG_INITIAL_START:
      msgdesc = "Initial Start";
      break;
    case SSPL_SBMSG_LINK_START:
      msgdesc = "Link Start";
      break;
    case SSPL_SBMSG_STARVATION_START:
      msgdesc = "Starvation Start";
      break;
    case SSPL_SBMSG_COMPLETE:
      msgdesc = "Complete";
      break;
    case SSPL_SBMSG_ABORT:
      msgdesc = "Abort";
      break;
    default:
      msgdesc = "<unknown>";
      break;
    }

  PRT (("spool: %s #%d\n", msgdesc, ssplGetSequenceNum (sspl, sbn)));

  return 0;
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
  memset (Data, 0, sizeof Data);

#ifdef MEMDEBUG
  Result = CreateMemDebug (MEMDEBUGF_ALLOC_PATTERNS | MEMDEBUGF_FREE_PATTERNS
                               | MEMDEBUGF_PAD_COOKIES
                               | MEMDEBUGF_CHECK_ALLOC_FAILURES
                               | MEMDEBUGF_KEEP_TASK_DATA,
                           NULL);
  CHECKRESULT (Result, "CreateMemDebug");
#endif

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_ISFOCUSED);
  CHECKRESULT (Result, "init EventUtility");

  /* Initialize audio, return if error. */
  Result = OpenAudioFolio ();
  CHECKRESULT (Result, "open audio folio");

  /* Use directout instead of mixer. */
  OutputIns = LoadInstrument ("directout.dsp", 0, 100);
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
  ssplSetSoundBufferFunc (sspl, MySoundBufferFunc);

  /* Fill the sound queue by queuing up all the buffers. "Preroll" */
  BufIndex = 0;
  SignalMask = 0;
  for (i = 0; i < NUM_BUFFERS; i++)
    {
      /* Dispatch buffers full of sound to spooler. Set User Data to BufIndex.
      ** ssplSpoolData returns a signal which can be checked to see when the
      *data
      ** has completed it playback. If it returns 0, there were no buffers
      *available.
      */
      Result = ssplSpoolData (sspl, Data[BufIndex], SAMPSIZE, NULL);
      CHECKRESULT (Result, "spool data");
      if (!Result)
        {
          ERR (("Out of buffers\n"));
          goto cleanup;
        }
      MySignal[BufIndex] = Result;
      SignalMask |= MySignal[BufIndex];
      BufIndex++;
      if (BufIndex >= NUM_BUFFERS)
        BufIndex = 0;
    }

  ssplDumpSoundSpooler (sspl);

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
      Result = ssplProcessSignals (sspl, CurSignals, NULL);
      CHECKRESULT (Result, "ssplProcessSignals");

      /* Read current state of Control Pad. */
      Result = GetControlPad (1, FALSE, &cped);
      CHECKRESULT (Result, "GetControlPad()");
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

      /* Stop on B button (abort on Shift+B), restart on release */
      if (Joy & ControlB)
        {
          if (Joy & (ControlLeftShift | ControlRightShift))
            {
              PRT (("Abort.\n"));
              Result = ssplAbort (sspl, NULL);
              CHECKRESULT (Result, "ssplAbort");
            }
          else
            {
              PRT (("Stop.\n"));
              Result = ssplStopSpooler (sspl);
              CHECKRESULT (Result, "ssplStopSpooler");
            }
          do
            {
              Result = GetControlPad (1, TRUE, &cped);
              Joy = cped.cped_ButtonBits;
            }
          while (Joy & ControlB);
          Result = ssplStartSpooler (sspl, 0x7FFF);
          CHECKRESULT (Result, "ssplStartSpooler");
          PRT (("Restart.\n"));
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
                  Result = ssplPlayData (sspl, Data[BufIndex], SAMPSIZE);
                  CHECKRESULT (Result, "play data");
                  BufIndex++;
                  if (BufIndex >= NUM_BUFFERS)
                    BufIndex = 0;
                }
            }
          while ((Joy & ControlStart) == 0);
        }

      /* Pause while holding play/pause button. */
      if (Joy & ControlStart)
        {
          PRT (("Pause.\n"));
          ssplPause (sspl);
          do
            {
              Result = GetControlPad (1, TRUE, &cped);
              Joy = cped.cped_ButtonBits;
            }
          while (Joy & ControlStart);
          ssplResume (sspl);
          PRT (("Resume.\n"));
        }

#if DEBUG_Status
      ssplDumpSoundSpooler (sspl);
#endif

      /*
      ** Spool as many buffers as are available.
      ** ssplSpoolData will return positive signals as long as it accepted the
      *data.
      */
      while ((Result = ssplSpoolData (sspl, Data[BufIndex], SAMPSIZE, NULL))
             > 0)
        {
          /* INSERT YOUR CODE HERE TO FILL UP THE NEXT BUFFER */
          BufIndex++;
          if (BufIndex >= NUM_BUFFERS)
            BufIndex = 0;
        }
      CHECKRESULT (Result, "spool data");
    }
  while ((Joy & ControlX) == 0);

  /* Stop Spooler. */
  Result = ssplAbort (sspl, NULL);
  CHECKRESULT (Result, "StopSoundSpooler");

cleanup:
  TraceAudio (0);
  ssplDeleteSoundSpooler (sspl);
  for (i = 0; i < NUM_BUFFERS; i++)
    {
      if (Data[i])
        FreeMem (Data[i], SAMPSIZE);
    }
  UnloadInstrument (SamplerIns);
  UnloadInstrument (OutputIns);
  CloseAudioFolio ();
  KillEventUtility ();
  PRT (("All done.\n"));

#ifdef MEMDEBUG
  DumpMemDebug (NULL);
  DeleteMemDebug ();
#endif

  return ((int)Result);
}
