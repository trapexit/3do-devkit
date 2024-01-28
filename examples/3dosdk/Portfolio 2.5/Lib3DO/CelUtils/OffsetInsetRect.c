
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: OffsetInsetRect.c,v 1.2 1994/10/05 18:49:08 vertex Exp $
**
**  Lib3DO routines to offset or inset a rectangle.
**
**  The Inset function will also 'outset' a rectangle (IE, enlarge it) if the
**  delta parameter(s) are negative.
**
******************************************************************************/

#include "celutils.h"

/*----------------------------------------------------------------------------
 * OffsetSRect()
 *	Offset the position of an SRect by the specified XY deltas.
 *--------------------------------------------------------------------------*/

SRect *
OffsetSRect (SRect *dst, IPoint *delta)
{
  dst->pos.x += delta->x;
  dst->pos.y += delta->y;
  return dst;
}

/*----------------------------------------------------------------------------
 * OffsetCRect()
 *	Offset the position of a CRect by the specified XY deltas.
 *--------------------------------------------------------------------------*/

CRect *
OffsetCRect (CRect *dst, IPoint *delta)
{
  dst->tl.x += delta->x;
  dst->tl.y += delta->y;
  return dst;
}

/*----------------------------------------------------------------------------
 * InsetSRect()
 *	Change the size of an SRect by the XY deltas specified.  If the deltas
 *	are positive the rect insets (shrinks); if negative the rect grows.
 *--------------------------------------------------------------------------*/

SRect *
InsetSRect (SRect *dst, IPoint *delta)
{
  dst->pos.x += delta->x;
  dst->pos.y += delta->y;
  dst->size.x -= delta->x << 1;
  dst->size.y -= delta->y << 1;
  return dst;
}

/*----------------------------------------------------------------------------
 * InsetCRect()
 *	Change the size of a CRect by the XY deltas specified.  If the deltas
 *	are positive the rect insets (shrinks); if negative the rect grows.
 *--------------------------------------------------------------------------*/

CRect *
InsetCRect (CRect *dst, IPoint *delta)
{
  dst->tl.x += delta->x;
  dst->tl.y += delta->y;
  dst->br.x -= delta->x;
  dst->br.y -= delta->y;
  return dst;
}
