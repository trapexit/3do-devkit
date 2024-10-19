
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ta_timer.c,v 1.25 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ta_timer
|||	ta_timer - Demonstrates use of the audio timer.
|||
|||	  Synopsis
|||
|||	    ta_timer
|||
|||	  Description
|||
|||	    This program shows how to examine and change the rate of the audio
clock.
|||	    It demonstrates use of cues to signal your task at a specific time.
It
|||	    also demonstrates how the audio folio deals with bad audio rate
values.
|||
|||	    Pressing A from the main menu runs the audio clock at successively
faster
|||	    rates, using SleepAudioTime().
|||
|||	    Pressing B from the main menu uses SignalAtTime() in conjunction
with cues
|||	    to wait. While using this method, you can press A or B to abort one
of two
|||	    cues prematurely.
|||
|||	    Pressing C from the main menu feeds illegal values to
SetAudioRate().
|||
|||	  Associated Files
|||
|||	    ta_timer.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
**/

#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "kernel.h"
#include "operror.h"
#include "stdio.h"
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

/***************************************************************/
Err
TestBadRates (void)
{
  Item Owner;
  frac16 OriginalRate;
  uint32 Duration;
  int32 Result;

  Owner = OwnAudioClock ();
  CHECKRESULT (Owner, "OwnAudioClock");

  OriginalRate = GetAudioRate ();
  PRT (("Original Rate = %d/sec\n", ConvertF16_32 (OriginalRate)));

#define TESTRATE(Rate)                                                        \
  Result = SetAudioRate (Owner, Convert32_F16 (Rate));                        \
  Duration = GetAudioDuration ();                                             \
  PRT (("Rate = %d, Duration = %d, Result = 0x%x\n", Rate, Duration, Result));

  TESTRATE (1050);
  TESTRATE (1010);
  TESTRATE (990);
  TESTRATE (400);
  TESTRATE (10);
  TESTRATE (2);
  TESTRATE (1);

  SetAudioRate (Owner, OriginalRate);
  Result = DisownAudioClock (Owner);
  CHECKRESULT (Result, "DisownAudioClock");

cleanup:
  return Result;
}

/***************************************************************/
Err
TestRateChange (void)
{
  Item Owner;
  int32 Rate;
  frac16 OriginalRate;
  Item MyCue;
  int32 Result;
  int32 i;

  MyCue = CreateCue (NULL);
  CHECKRESULT (MyCue, "CreateCue");

  Owner = OwnAudioClock ();
  CHECKRESULT (Owner, "OwnAudioClock");

  OriginalRate = GetAudioRate ();
  PRT (("Original Rate = %d/sec , 0x%x\n", ConvertF16_32 (OriginalRate),
        OriginalRate));
  PRT (("Original Duration = %d\n", GetAudioDuration ()));
  Rate = 100;

  /* Wait for one second at increasingly fast clock rates. */
  for (i = 0; i < 4; i++)
    {
      //		SetAudioRate(Owner, Convert32_F16(Rate) );
      SetAudioRate (Owner, Convert32_F16 (Rate));
      PRT (("Sleep for %d ticks.\n", Rate));
      Result = SleepUntilTime (MyCue, Rate + GetAudioTime ());
      Rate = Rate * 2;
    }

  SetAudioRate (Owner, OriginalRate);
  Result = DisownAudioClock (Owner);
  CHECKRESULT (Result, "DisownAudioClock");

cleanup:
  DeleteCue (MyCue);
  return Result;
}

Err
WaitForButtonsUp (int32 Button)
{

  ControlPadEventData cped;
  int32 Result;

  Result = GetControlPad (1, FALSE, &cped);
  if (Result < 0)
    {
      PrintError (0, "get control pad data in", "WaitForButtonsUp", Result);
    }

  if (cped.cped_ButtonBits & Button)
    {
      do
        {
          Result = GetControlPad (1, TRUE, &cped);
          if (Result < 0)
            {
              PrintError (0, "get control pad data in", "WaitForButtonsUp",
                          Result);
            }
        }
      while (cped.cped_ButtonBits & Button);
    }
  return Result;
}

/****************************************************************
** Start two timers and watch for their signals to come back.
** Optionally abort them using A or B button.
****************************************************************/
Err
TestAbortCue (void)
{
  Item MyCue1, MyCue2 = 0, MyCue3 = 0;
  int32 Signal1, Signal2, Signals;
  ControlPadEventData cped;
  uint32 Joy;
  int32 Result;

  MyCue1 = CreateCue (NULL);
  CHECKRESULT (MyCue1, "CreateCue");
  MyCue2 = CreateCue (NULL);
  CHECKRESULT (MyCue2, "CreateCue");
  MyCue3 = CreateCue (NULL);
  CHECKRESULT (MyCue3, "CreateCue");

  Signal1 = GetCueSignal (MyCue1);
  PRT (("Signal1 = 0x%08x\n", Signal1));

  Signal2 = GetCueSignal (MyCue2);
  PRT (("Signal2 = 0x%08x\n", Signal2));

  PRT (("A to abort 1, B to abort 2, C to stop looping.\n"));

  /* Schedule signals at one and two seconds from now. */
  Result = SignalAtTime (MyCue1, GetAudioTime () + 240);
  CHECKRESULT (Result, "SignalAtTime");
  Result = SignalAtTime (MyCue2, GetAudioTime () + (2 * 240));
  CHECKRESULT (Result, "SignalAtTime");

  do
    {
      Signals = GetCurrentSignals ();
      PRT (("Signals = 0x%08x, Time = %d\n", Signals, GetAudioTime ()));

      /* Read Control Pad. */
      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          PrintError (0, "get control pad data in", "TestAbortCue", Result);
        }
      Joy = cped.cped_ButtonBits;

      if (Joy & ControlA)
        {
          PRT ((" Abort Cue 1\n"));
          Result = AbortTimerCue (MyCue1);
          CHECKRESULT (Result, "AbortTimerCue 1");
        }
      else if (Joy & ControlB)
        {
          PRT ((" Abort Cue 2\n"));
          Result = AbortTimerCue (MyCue2);
          CHECKRESULT (Result, "AbortTimerCue 2");
        }
      else
        {
          SleepUntilTime (MyCue3, GetAudioTime () + 24);
        }
    }
  while ((Joy & ControlC) == 0);

cleanup:
  DeleteCue (MyCue1);
  DeleteCue (MyCue2);
  return Result;
}

/****************************************************************
** Menu of timer tests.
****************************************************************/
int
main (int argc, char *argv[])
{
  int32 Result;
  ControlPadEventData cped;
  uint32 Joy;
  int32 doit;
  int32 ifhelp;

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (Result < 0)
    {
      ERR (("main: error in InitEventUtility\n"));
      PrintError (0, "init event utility", 0, Result);
      goto cleanup;
    }

  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  //	TraceAudio( TRACE_TOP | TRACE_TIMER | TRACE_INT );

  PRT (("\nta_timer: Start testing Audio timer.\n"));
  PRT (("Default duration = %d\n", GetAudioDuration ()));

  /* Allow user to select test. */
  doit = TRUE;
  ifhelp = TRUE;

  do
    {
      if (ifhelp)
        {
          PRT (("---------------------------------------------------\n"));
          PRT (("Use Joypad to select timer test...\n"));
          PRT (("   A = Timer rate changes.\n"));
          PRT (("   B = Abort Timer Cue.\n"));
          PRT (("   C = Bad Rate Values.\n"));
          PRT (("   STOP = quit.\n"));
          PRT (("---------------------------------------------------\n"));
          ifhelp = FALSE;
        }

      /* Read Control Pad. */
      Result = GetControlPad (1, TRUE, &cped);
      if (Result < 0)
        {
          PrintError (0, "get control pad data", 0, Result);
        }
      Joy = cped.cped_ButtonBits;

      switch (Joy)
        {
        case ControlA:
          WaitForButtonsUp (ControlA);
          Result = TestRateChange ();
          CHECKRESULT (Result, "TestRateChange");
          ifhelp = TRUE;
          break;

        case ControlB:
          WaitForButtonsUp (ControlB);
          Result = TestAbortCue ();
          CHECKRESULT (Result, "TestPoly");
          ifhelp = TRUE;
          break;

        case ControlC:
          Result = TestBadRates ();
          CHECKRESULT (Result, "TestBadRates");
          break;

        case ControlX:
          doit = FALSE;
          break;
        }
    }
  while (doit);

  PRT (("ta_timer: Finished!\n"));
cleanup:
  TraceAudio (0);
  KillEventUtility ();
  return ((int)Result);
}
