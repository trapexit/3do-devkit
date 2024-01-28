/*****************************************************************************
 *	File:			CenterCelOnScreen.c
 *
 *	Contains:		Routine to center a cel in the display.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This function will change all of the projection parameters for the cel.
 *	IE, if the cel is set to project at twice its normal width/height
 *before this call, it will back to x1 (and centered, of course) after the
 *call.
 *
 *	The CenterRectCel() family of CelUtils functions will not change the
 *	projection parms, only the XY.  They're faster too.
 ****************************************************************************/

#include "Utils3DO.h"

void
CenterCelOnScreen (CCB *ccb)
{
  Rectf16 celRect, frameRect;
  Point q[4];

  SetRectf16 (&frameRect, 0, 0, 320, 240);
  SetRectf16 (&celRect, 0, 0, ccb->ccb_Width, ccb->ccb_Height);
  CenterRectf16 (q, &celRect, &frameRect);
  MapCel (ccb, q);
}
