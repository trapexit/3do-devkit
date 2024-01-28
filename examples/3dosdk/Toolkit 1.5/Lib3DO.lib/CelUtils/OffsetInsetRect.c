/*****************************************************************************
 * File:			OffsetInsetRect.c
 *
 * Contains:		Offset or Inset a rectangle.
 *
 * Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 * History:
 * 02/27/94  Ian 	New today.
 *
 * Implementation notes:
 *
 *	The Inset function will also 'outset' a rectangle (IE, enlarge it) if
 *the delta parameter(s) are negative.
 ****************************************************************************/

#include "CelUtils.h"

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
