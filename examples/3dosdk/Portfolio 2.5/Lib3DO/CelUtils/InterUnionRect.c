
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: InterUnionRect.c,v 1.3 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO routines to calc intersections and unions between rectangles.
**
**  The destination rectangle can be the same as either source rectangle
**  for these functions if you find that convenient.  Note, however, that
**  for the Intersection functions, the destination retangle gets modified
**  even if there is no common area between the two sources.
**
******************************************************************************/


#include "celutils.h"

/*----------------------------------------------------------------------------
 * SRectBounds()
 *	Calc and return the bounding box of two SRects.
 *--------------------------------------------------------------------------*/

SRect * SRectBounds(SRect *dst, SRect *rect1, SRect *rect2)
{
	CRect	cr1, cr2;

	CRectFromSRect(&cr1, rect1);
	CRectFromSRect(&cr2, rect2);

	return SRectFromCRect(dst, CRectBounds(&cr1, &cr1, &cr2));
}

/*----------------------------------------------------------------------------
 * SRectIntersection()
 *	Calc and return the intersection of two SRects.  Returns NULL if there
 *	is no common area between the two rectangles.
 *--------------------------------------------------------------------------*/

SRect * SRectIntersection(SRect *dst, SRect *rect1, SRect *rect2)
{
	CRect	cr1, cr2;

	CRectFromSRect(&cr1, rect1);
	CRectFromSRect(&cr2, rect2);

	return SRectFromCRect(dst, CRectIntersection(&cr1, &cr1, &cr2));
}

/*----------------------------------------------------------------------------
 * CRectBounds()
 *	Calc and return the bounding box of two CRects.
 *--------------------------------------------------------------------------*/

CRect * CRectBounds(CRect *dst, CRect *rect1, CRect *rect2)
{
	dst->tl.x = (rect1->tl.x < rect2->tl.x) ? rect1->tl.x : rect2->tl.x;
	dst->tl.y = (rect1->tl.y < rect2->tl.y) ? rect1->tl.y : rect2->tl.y;
	dst->br.x = (rect1->br.x > rect2->br.x) ? rect1->br.x : rect2->br.x;
	dst->br.y = (rect1->br.y > rect2->br.y) ? rect1->br.y : rect2->br.y;

	return dst;
}

/*----------------------------------------------------------------------------
 * SRectIntersection()
 *	Calc and return the intersection of two SRects.  Returns NULL if there
 *	is no common area between the two rectangles.
 *--------------------------------------------------------------------------*/

CRect * CRectIntersection(CRect *dst, CRect *rect1, CRect *rect2)
{
	dst->tl.x = (rect1->tl.x > rect2->tl.x) ? rect1->tl.x : rect2->tl.x;
	dst->tl.y = (rect1->tl.y > rect2->tl.y) ? rect1->tl.y : rect2->tl.y;
	dst->br.x = (rect1->br.x < rect2->br.x) ? rect1->br.x : rect2->br.x;
	dst->br.y = (rect1->br.y < rect2->br.y) ? rect1->br.y : rect2->br.y;

	if (dst->tl.x > dst->br.x || dst->tl.y > dst->br.y) {
		return NULL;
	}

	return dst;
}

/*----------------------------------------------------------------------------
 * IPointIsInSRect()
 *	Return TRUE/FALSE indicating that the point is/isn't in the rectangle.
 *--------------------------------------------------------------------------*/

Boolean IPointIsInSRect(IPoint *point, SRect *rect)
{
	return 	point->x >= rect->pos.x
		&&	point->y >= rect->pos.y
		&&	point->x <= XCORNERFROMSRECT(rect)
		&&	point->y <= YCORNERFROMSRECT(rect)
		;
}

/*----------------------------------------------------------------------------
 * IPointIsInCRect()
 *	Return TRUE/FALSE indicating that the point is/isn't in the rectangle.
 *--------------------------------------------------------------------------*/

Boolean IPointIsInCRect(IPoint *point, CRect *rect)
{
	return 	point->x >= rect->tl.x
		&&	point->y >= rect->tl.y
		&&	point->x <= rect->br.x
		&&	point->y <= rect->br.y
		;
}
