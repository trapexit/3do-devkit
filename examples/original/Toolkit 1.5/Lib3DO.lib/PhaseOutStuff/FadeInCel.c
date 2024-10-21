/*****************************************************************************
 *	File:			FadeInCel.c
 *
 *	Contains:		Routine to calc a cel fade-in.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This is pretty ugly stuff.  Check out the CrossFadeCels() routine for
 *	some code that may serve your needs better.
 ****************************************************************************/

#include "Utils3DO.h"

Boolean
FadeInCel (CCB *ccb, CCB *maskccb, int32 *stepValue)
{

  /* Fade the cel in one step at a time */
  if (*stepValue < MAX_SCALE)
    {
      SetCelScale (ccb, maskccb, (*stepValue)++);
      if (*stepValue == MAX_SCALE)
        {
          SET_TO_NORMAL (ccb);
          return false;
        }
      else
        return true;
    }

  return false;
}
