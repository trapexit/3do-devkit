/*****************************************************************************
 *	File:			SetRectf16.c 
 *
 *	Contains:		Routine to set a frac16 rect from integer coords.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 ****************************************************************************/
 
#include "Utils3DO.h"

 
void SetRectf16( Rectf16 *r, Coord left, Coord top, Coord right, Coord bottom )
{
	r->rectf16_XLeft	= Convert32_F16 (left);
	r->rectf16_YTop		= Convert32_F16 (top);
	r->rectf16_XRight	= Convert32_F16 (right);
	r->rectf16_YBottom	= Convert32_F16 (bottom);
}
