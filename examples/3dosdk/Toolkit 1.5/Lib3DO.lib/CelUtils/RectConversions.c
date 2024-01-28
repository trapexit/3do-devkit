/*****************************************************************************
 *	File:			RectConversions.c
 *
 *	Contains:		Convert between various datatypes involving
 *rectangles.
 *
 *	Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	02/28/94  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	The rect-to-rect conversions work even when src and dst are the same
 *rect.
 ****************************************************************************/

#include "CelUtils.h"

/*----------------------------------------------------------------------------
 * ISizeFromCRect()
 *	Return the XY sizes expressed by a CRect.
 *--------------------------------------------------------------------------*/

IPoint *
ISizeFromCRect (IPoint *dst, CRect *src)
{
  dst->x = XSIZEFROMCRECT (src);
  dst->y = YSIZEFROMCRECT (src);
  return dst;
}

/*----------------------------------------------------------------------------
 * ICornerFromSRect()
 *	Return the bottom right corner expressed by an SRect.
 *--------------------------------------------------------------------------*/

IPoint *
ICornerFromSRect (IPoint *dst, SRect *src)
{
  dst->x = XCORNERFROMSRECT (src);
  dst->y = YCORNERFROMSRECT (src);
  return dst;
}

/*----------------------------------------------------------------------------
 * CRectFromIVal()
 *	Convert integer values to a CRect.
 *--------------------------------------------------------------------------*/

CRect *
CRectFromIVal (CRect *dst, int32 tlx, int32 tly, int32 brx, int32 bry)
{
  dst->tl.x = tlx;
  dst->tl.y = tly;
  dst->br.x = brx;
  dst->br.y = bry;
  return dst;
}

/*----------------------------------------------------------------------------
 * CRectFromSRect()
 *	Convert an SRect to a CRect that describes the same area.
 *--------------------------------------------------------------------------*/

CRect *
CRectFromSRect (CRect *dst, SRect *src)
{
  dst->tl = src->pos;
  dst->br.x = XCORNERFROMSRECT (src);
  dst->br.y = YCORNERFROMSRECT (src);
  return dst;
}

/*----------------------------------------------------------------------------
 * SRectFromIVal()
 *	Convert integer values to an SRect.
 *--------------------------------------------------------------------------*/

SRect *
SRectFromIVal (SRect *dst, int32 x, int32 y, int32 w, int32 h)
{
  dst->pos.x = x;
  dst->pos.y = y;
  dst->size.x = w;
  dst->size.y = h;
  return dst;
}

/*----------------------------------------------------------------------------
 * CRectFromSRect()
 *	Convert a CRect to an SRect that describes the same area.
 *
 * Note: this has logic to cope with a NULL pointer to help support the
 *		 behavior of SRectIntersection().  Don't count on this always
 *being so; a change in SRectIntersection() could lead to removal of the NULL
 *pointer check here.
 *--------------------------------------------------------------------------*/

SRect *
SRectFromCRect (SRect *dst, CRect *src)
{

  if (src)
    {
      dst->pos = src->tl;
      dst->size.x = XSIZEFROMCRECT (src);
      dst->size.y = YSIZEFROMCRECT (src);
    }
  return dst;
}
