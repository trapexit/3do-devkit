/*****************************************************************************
 * File:			PointConversions.c
 *
 * Contains:		Convert between various datatypes involving x/y
 *'points'.
 *
 * Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 * History:
 * 02/28/94  Ian 	New today.
 *
 * Implementation notes:
 *
 ****************************************************************************/

#include "CelUtils.h"

/*----------------------------------------------------------------------------
 * FPointFromIVal()
 *	Convert a pair of integers into an FPoint.
 *--------------------------------------------------------------------------*/

FPoint *
FPointFromIVal (FPoint *dst, int32 x, int32 y)
{
  dst->x = Convert32_F16 (x);
  dst->y = Convert32_F16 (y);
  return dst;
}

/*----------------------------------------------------------------------------
 * FPointFromFVal()
 *	Convert a pair of frac16s into an FPoint.
 *--------------------------------------------------------------------------*/

FPoint *
FPointFromFVal (FPoint *dst, frac16 x, frac16 y)
{
  dst->x = x;
  dst->y = y;
  return dst;
}

/*----------------------------------------------------------------------------
 * FPointFromFVal()
 *	Convert an integer point into an FPoint.
 *--------------------------------------------------------------------------*/

FPoint *
FPointFromIPoint (FPoint *dst, IPoint *src)
{
  dst->x = Convert32_F16 (src->x);
  dst->y = Convert32_F16 (src->y);
  return dst;
}

/*----------------------------------------------------------------------------
 * IPointFromIVal()
 *	Convert a pair of integers into an IPoint.
 *--------------------------------------------------------------------------*/

IPoint *
IPointFromIVal (IPoint *dst, int32 x, int32 y)
{
  dst->x = x;
  dst->y = y;
  return dst;
}

/*----------------------------------------------------------------------------
 * IPointFromFVal()
 *	Convert a pair of frac16s into an IPoint.
 *--------------------------------------------------------------------------*/

IPoint *
IPointFromFVal (IPoint *dst, frac16 x, frac16 y)
{
  dst->x = ConvertF16_32 (x);
  dst->y = ConvertF16_32 (y);
  return dst;
}

/*----------------------------------------------------------------------------
 * IPointFromFPoint()
 *	Convert a frac16 point into an IPoint.
 *--------------------------------------------------------------------------*/

IPoint *
IPointFromFPoint (IPoint *dst, FPoint *src)
{
  dst->x = ConvertF16_32 (src->x);
  dst->y = ConvertF16_32 (src->y);
  return dst;
}
