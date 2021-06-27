/*****************************************************************************
 *	File:			FadeOutCel.c 
 *
 *	Contains:		Routine to calc a cel fade-out.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
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

Boolean FadeOutCel(CCB *ccb, CCB *maskccb, int32 *stepValue)
{
	if (*stepValue == (MAX_SCALE-1))
		SET_TO_AVERAGE (ccb);

	/* Fade the cel out one step at a time */
	SetCelScale (ccb, maskccb, --(*stepValue));
	
	if ( *stepValue > 0)
		return true;

	return false;
}
