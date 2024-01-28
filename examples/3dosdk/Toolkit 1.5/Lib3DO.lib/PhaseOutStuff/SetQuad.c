/*****************************************************************************
 *	File:			SetQuad.c
 *
 *	Contains:		Routine to set a quad from a rect.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "Utils3DO.h"

void
SetQuad (Point *r, Coord left, Coord top, Coord right, Coord bottom)
{
  r[0].pt_X = left;
  r[0].pt_Y = top;
  r[1].pt_X = right;
  r[1].pt_Y = top;
  r[2].pt_X = right;
  r[2].pt_Y = bottom;
  r[3].pt_X = left;
  r[3].pt_Y = bottom;
}
