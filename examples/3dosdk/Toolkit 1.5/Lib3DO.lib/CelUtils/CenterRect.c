/*****************************************************************************
 *	File:			CenterRect.c
 *
 *	Contains:		Center rectangles within larger entities.
 *
 *	Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	07/23/94  Ian	Performance tweaks: more inline calcs instead of
 *calling other library routines to convert-and-calc. 02/27/94  Ian 	New
 *today.
 *
 *	Implementation notes:
 *
 *	The centering functions work regardless of which rectangle is larger.
 *	If the destination rect is larger, it will be centered over (rather
 *than within) the other rectangle, despite the "In" in the function names.
 *
 *	Divide-by-two expressions herein are coded as ">> 1" expressions.  This
 *	is because the compiler generates painfully-correct code for signed
 *	divide-by-two via shifting, and we know that the rounding difference
 *	between signed divide and signed shift makes no difference in our
 *context.
 ****************************************************************************/

#include "CelUtils.h"

/*----------------------------------------------------------------------------
 * CenterSRectOverIPoint()
 *	Align an SRect so that its center is at the specified point.
 *--------------------------------------------------------------------------*/

SRect *
CenterSRectOverIPoint (SRect *dst, IPoint *point)
{
  dst->pos.x = point->x - (dst->size.x >> 1);
  dst->pos.y = point->y - (dst->size.y >> 1);

  return dst;
}

/*----------------------------------------------------------------------------
 * CenterCRectOverIPoint()
 *	Align a CRect so that its center is at the specified point.
 *--------------------------------------------------------------------------*/

CRect *
CenterCRectOverIPoint (CRect *dst, IPoint *point)
{
  dst->tl.x = point->x - (XSIZEFROMCRECT (dst) >> 1);
  dst->tl.y = point->y - (YSIZEFROMCRECT (dst) >> 1);

  return dst;
}

/*----------------------------------------------------------------------------
 * CenterSRectInSRect()
 *	Align an SRect so that it is centered over or within the specified
 *rect.
 *--------------------------------------------------------------------------*/

SRect *
CenterSRectInSRect (SRect *dst, SRect *rect)
{
  dst->pos.x = rect->pos.x + ((rect->size.x - dst->size.x) >> 1);
  dst->pos.y = rect->pos.y + ((rect->size.y - dst->size.y) >> 1);

  return dst;
}

/*----------------------------------------------------------------------------
 * CenterSRectInDisplay()
 *	Align an SRect so that its center is at the display center point.
 *--------------------------------------------------------------------------*/

SRect *
CenterSRectInDisplay (SRect *dst)
{
  IPoint *point;

  point = CenterIPointInDisplay ();

  dst->pos.x = point->x - (dst->size.x >> 1);
  dst->pos.y = point->y - (dst->size.y >> 1);

  return dst;
}

/*----------------------------------------------------------------------------
 * CenterCRectInCRect()
 *	Align a CRect so that it is centered over or within the specified rect.
 *--------------------------------------------------------------------------*/

CRect *
CenterCRectInCRect (CRect *dst, CRect *rect)
{
  dst->tl.x
      = rect->tl.x + ((XSIZEFROMCRECT (rect) - XSIZEFROMCRECT (dst)) >> 1);
  dst->tl.y
      = rect->tl.y + ((YSIZEFROMCRECT (rect) - YSIZEFROMCRECT (dst)) >> 1);

  return dst;
}

/*----------------------------------------------------------------------------
 * CenterCRectInDisplay()
 *	Align a CRect so that its center is at the display center point.
 *--------------------------------------------------------------------------*/

CRect *
CenterCRectInDisplay (CRect *dst)
{
  IPoint *point;

  point = CenterIPointInDisplay ();

  dst->tl.x = point->x - (XSIZEFROMCRECT (dst) >> 1);
  dst->tl.y = point->y - (YSIZEFROMCRECT (dst) >> 1);

  return dst;
}
