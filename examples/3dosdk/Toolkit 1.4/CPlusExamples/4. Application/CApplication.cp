/*
        File:		CApplication.cp

        Contains:	Simple application class

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CApplication.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"
#include "stdio.h"

#include "CPlusSwiHack.h"

CApplication::CApplication (void)
{
  fContinue = 0;

  if (InitEventUtility (1, 0, LC_Observer) < 0)
    {
      DIAGNOSTIC ("Can't initialize the event broker");
    }
}

CApplication::~CApplication (void) { KillEventUtility (); }

void
CApplication::Stop (void)
{
  fContinue = 0;
}

uint32
CApplication::DoControlPad (uint32 continuousMask)
/*
        Get the current control pad bits, masking out undesirable continuous
        button presses.
*/
{
  static uint32 gOldBits = 0;

  int32 aResult;
  ControlPadEventData aControlPadEvent;
  uint32 newBits;
  uint32 maskedBits;

  aResult = GetControlPad (1, FALSE, &aControlPadEvent);
  if (aResult < 0)
    {
      printf ("Error: GetControlPad() failed with error code %d\n", aResult);
      PrintfSysErr (aResult);
      return 0;
    }
  newBits = aControlPadEvent.cped_ButtonBits;

  maskedBits = newBits ^ gOldBits;

  maskedBits &= newBits;
  maskedBits |= (newBits & continuousMask);

  gOldBits = newBits;
  return maskedBits;
}

void
CApplication::Run (void)
{
  Item aVBLIOReq;
  uint32 aControlBits;

  fContinue = 1;

  aVBLIOReq = GetVBLIOReq ();

  while (fContinue)
    {
      aControlBits = DoControlPad (kControlAll);

      if (aControlBits == 0L)
        continue;

      if (aControlBits & ControlStart)
        OnStart ();

      else if (aControlBits & ControlX)
        OnX ();

      else if (aControlBits & ControlRight)
        OnRight ();

      else if (aControlBits & ControlLeft)
        OnLeft ();

      else if (aControlBits & ControlUp)
        OnUp ();

      else if (aControlBits & ControlDown)
        OnDown ();

      else if (aControlBits & ControlA)
        OnA ();

      else if (aControlBits & ControlB)
        OnB ();

      else if (aControlBits & ControlC)
        OnC ();

      /*
                      else
                              printf("unknown value received from ControlPad:
         %ld\n",aControlBits);
      */

      WaitVBL (aVBLIOReq, 1);
    }

  DeleteIOReq (aVBLIOReq);
}
