/*****************************************************************************
 * File:			CenterPoint.c
 *
 * Contains:		Center points within larger entities.
 *
 * Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 * History:
 *	07/23/94  Ian	Removed references to GrafBase fields.
 * 	02/27/94  Ian 	New today.
 *
 * Implementation notes:
 * 
 *	Right now, the display is assumed to be 320x240.  When the new OpenDisplay()
 *	library routine comes online it will provide proper display sizes we 
 *	can refer to.
 *
 *	Even though two static variables show up in this module (to get a slight
 *	performance boost when obtaining the screen center point), these functions
 *	are still multithreaded-safe.  Even if it should happen that two threads
 *	are executing the init code simultaneously, both would be trying to set the
 *	static variables to the same values anyway, so there's no problem with
 *	them stepping on each others' updates.  
 *
 *	Divide-by-two expressions herein are coded as ">> 1" expressions.  This
 *	is because the compiler generates painfully-correct code for signed 
 *	divide-by-two via shifting, and we know that the rounding difference
 *	between signed divide and signed shift makes no difference in our context.
 ****************************************************************************/

#include "CelUtils.h"

#define DISPLAY_WIDTH	320
#define DISPLAY_HEIGHT	240

static IPoint	iCenter;
static FPoint	fCenter;

/*----------------------------------------------------------------------------
 * CenterFPointInDisplay()
 *	Return a pointer to an FPoint that marks the center of the display.
 *--------------------------------------------------------------------------*/

FPoint * CenterFPointInDisplay(void)
{
	if (fCenter.x == 0) {
		fCenter.x = Convert32_F16(DISPLAY_WIDTH  >> 1);
		fCenter.y = Convert32_F16(DISPLAY_HEIGHT >> 1);
	}
	
	return &fCenter;
}

/*----------------------------------------------------------------------------
 * CenterIPointInDisplay()
 *	Return a pointer to an IPoint that marks the center of the display.
 *--------------------------------------------------------------------------*/

IPoint * CenterIPointInDisplay(void)
{
	if (iCenter.x == 0) {
		iCenter.x = DISPLAY_WIDTH  >> 1;
		iCenter.y = DISPLAY_HEIGHT >> 1;
	}
	
	return &iCenter;
}

/*----------------------------------------------------------------------------
 * CenterIPointInSRect()
 *	Calc the center point of an SRect.
 *--------------------------------------------------------------------------*/

IPoint * CenterIPointInSRect(IPoint *dst, SRect *rect)
{
	dst->x = rect->pos.x + (rect->size.x >> 1);
	dst->y = rect->pos.y + (rect->size.y >> 1);
	
	return dst;
}

/*----------------------------------------------------------------------------
 * CenterIPointInCRect()
 *	Calc the center point of a CRect.
 *--------------------------------------------------------------------------*/

IPoint * CenterIPointInCRect(IPoint *dst, CRect *rect)
{
	dst->x = rect->tl.x + (XSIZEFROMCRECT(rect) >> 1);
	dst->y = rect->tl.y + (YSIZEFROMCRECT(rect) >> 1);
	
	return dst;

}
