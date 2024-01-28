/*
        File:		eventJoy.c

        Contains:	demo program for control pad

        Written by:	Jay Moreland

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 6/28/93	JAY		first checked in

        To Do:
*/

#include "event.h"
#include "joypad.h"
#include "portfolio.h"

void
printJoyState (JoyPadState *joyStatePtr)
{
  char aLine[133];
  bool addPlus;

  addPlus = false;
  aLine[0] = 0;

  if (joyStatePtr->leftShift)
    {
      strcat (aLine, "LEFTSHIFT");
      addPlus = true;
    }
  if (joyStatePtr->rightShift)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "RIGHTSHIFT");
      addPlus = true;
    }
  if (joyStatePtr->xBtn)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "XBTN");
      addPlus = true;
    }
  if (joyStatePtr->startBtn)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "STARTBTN");
      addPlus = true;
    }
  if (joyStatePtr->aBtn)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "ABTN");
      addPlus = true;
    }
  if (joyStatePtr->bBtn)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "BBTN");
      addPlus = true;
    }
  if (joyStatePtr->cBtn)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "CBTN");
      addPlus = true;
    }
  if (joyStatePtr->upArrow)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "UPARROW");
      addPlus = true;
    }
  if (joyStatePtr->downArrow)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "DOWNARROW");
      addPlus = true;
    }
  if (joyStatePtr->leftArrow)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "LEFTARROW");
      addPlus = true;
    }
  if (joyStatePtr->rightArrow)
    {
      if (addPlus)
        strcat (aLine, "+");
      strcat (aLine, "RIGHTARROW");
    }
  printf ("%s\n", aLine);
}

Boolean
CompareJoyPadStates (JoyPadState *joyStatePtr, JoyPadState *joyStatePtr2)
{
  if (joyStatePtr->leftShift != joyStatePtr2->leftShift)
    return false;
  if (joyStatePtr->rightShift != joyStatePtr2->rightShift)
    return false;
  if (joyStatePtr->xBtn != joyStatePtr2->xBtn)
    return false;
  if (joyStatePtr->startBtn != joyStatePtr2->startBtn)
    return false;
  if (joyStatePtr->aBtn != joyStatePtr2->aBtn)
    return false;
  if (joyStatePtr->bBtn != joyStatePtr2->bBtn)
    return false;
  if (joyStatePtr->cBtn != joyStatePtr2->cBtn)
    return false;
  if (joyStatePtr->upArrow != joyStatePtr2->upArrow)
    return false;
  if (joyStatePtr->downArrow != joyStatePtr2->downArrow)
    return false;
  if (joyStatePtr->leftArrow != joyStatePtr2->leftArrow)
    return false;
  if (joyStatePtr->rightArrow != joyStatePtr2->rightArrow)
    return false;
  return true;
}

JoyPadState prevState;

int
main (int argc, char *argv[])
{
  Boolean startButtonSeen;
  JoyPadState jpState;
  bool quitbitsSeen;

  quitbitsSeen = false;
  SetJoyPadContinuous (PADALL, 1);

  for (;;)
    {
      startButtonSeen = GetJoyPad (&jpState, 1);
      if (CompareJoyPadStates (&prevState, &jpState))
        ;
      else
        {
          printf ("Previous State: ");
          printJoyState (&prevState);
          printf ("Current State: ");
          printJoyState (&jpState);
        }
      prevState = jpState;
      if (startButtonSeen)
        {
          KillJoypad ();
          if (quitbitsSeen)
            break;
          quitbitsSeen = true;
        }
    }
  KillJoypad ();
  kprintf ("Bye!\n");
}
