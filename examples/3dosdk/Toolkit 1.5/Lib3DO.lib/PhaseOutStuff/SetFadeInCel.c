/*****************************************************************************
 *	File:			SetFadeInCel.c
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
#include "string.h"

void
SetFadeInCel (CCB *ccb, CCB *maskccb, int32 *stepValue)
{

  /* Copy the CCB data to the maskCCB and set the maskCCB
   * to use the data to generate a shadow of the asteroid.
   * Set the asteroid CCB to blend with the shadow and set
   * the starting scale for both CCB's.
   */
  memcpy (maskccb, ccb, sizeof (CCB));
  SET_TO_SHADOW (maskccb);
  SET_TO_AVERAGE (ccb);
  *stepValue = 0;

  /* Link the cel in front of its shadow mask. The intensity
   * of the shadow will be the inverse of the intensity of the
   * cel. Since the cel will blend with the shadow, the
   * effect will be to fade the cel in from the background.
   */
  LinkCel (maskccb, ccb);
}
