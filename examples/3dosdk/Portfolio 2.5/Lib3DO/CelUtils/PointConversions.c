
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: PointConversions.c,v 1.2 1994/10/05 19:02:02 vertex Exp $
**
**  Lib3DO routines to convert between various datatypes involving x/y
**  'points'.
**
******************************************************************************/


#include "celutils.h"

/*----------------------------------------------------------------------------
 * FPointFromIVal()
 *	Convert a pair of integers into an FPoint.
 *--------------------------------------------------------------------------*/

FPoint * FPointFromIVal(FPoint *dst, int32 x, int32 y)
{
	dst->x = Convert32_F16(x);
	dst->y = Convert32_F16(y);
	return dst;
}

/*----------------------------------------------------------------------------
 * FPointFromFVal()
 *	Convert a pair of frac16s into an FPoint.
 *--------------------------------------------------------------------------*/

FPoint * FPointFromFVal(FPoint *dst, frac16 x, frac16 y)
{
	dst->x = x;
	dst->y = y;
	return dst;
}

/*----------------------------------------------------------------------------
 * FPointFromFVal()
 *	Convert an integer point into an FPoint.
 *--------------------------------------------------------------------------*/

FPoint * FPointFromIPoint(FPoint *dst, IPoint *src)
{
	dst->x = Convert32_F16(src->x);
	dst->y = Convert32_F16(src->y);
	return dst;
}

/*----------------------------------------------------------------------------
 * IPointFromIVal()
 *	Convert a pair of integers into an IPoint.
 *--------------------------------------------------------------------------*/

IPoint * IPointFromIVal(IPoint *dst, int32 x, int32 y)
{
	dst->x = x;
	dst->y = y;
	return dst;
}

/*----------------------------------------------------------------------------
 * IPointFromFVal()
 *	Convert a pair of frac16s into an IPoint.
 *--------------------------------------------------------------------------*/

IPoint * IPointFromFVal(IPoint *dst, frac16 x, frac16 y)
{
	dst->x = ConvertF16_32(x);
	dst->y = ConvertF16_32(y);
	return dst;
}

/*----------------------------------------------------------------------------
 * IPointFromFPoint()
 *	Convert a frac16 point into an IPoint.
 *--------------------------------------------------------------------------*/

IPoint * IPointFromFPoint(IPoint *dst, FPoint *src)
{
	dst->x = ConvertF16_32(src->x);
	dst->y = ConvertF16_32(src->y);
	return dst;
}
