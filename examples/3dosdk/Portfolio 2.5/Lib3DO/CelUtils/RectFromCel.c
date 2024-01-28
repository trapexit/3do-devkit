
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: RectFromCel.c,v 1.2 1994/10/05 19:08:12 vertex Exp $
**
**  Lib3DO routines to return a rectangle describing how a cel is currently
**  mapped.
**
**  These work only if the cel is mapped to a rectangle.  If the cel is
**  mapped to an arbitrary non-rectangular quad, the return values will
**  be bogus, because these calcs don't factor in the perspective fields
**  that lead to non-rectangular mappings (HDY, VDX, HDDX, HDDY).
**
******************************************************************************/

#include "celutils.h"

/*----------------------------------------------------------------------------
 * SRectFromCel()
 *	Calc an SRect that describes a cel as currently projected.
 *--------------------------------------------------------------------------*/

SRect *
SRectFromCel (SRect *dst, CCB *cel)
{
  dst->pos.x = ConvertF16_32 (cel->ccb_XPos);
  dst->pos.y = ConvertF16_32 (cel->ccb_YPos);
  dst->size.x = ConvertF16_32 (
      MulSF16 ((cel->ccb_HDX >> 4), Convert32_F16 (cel->ccb_Width)));
  dst->size.y = ConvertF16_32 (
      MulSF16 ((cel->ccb_VDY), Convert32_F16 (cel->ccb_Height)));
  return dst;
}

/*----------------------------------------------------------------------------
 * CRectFromCel()
 *	Calc a CRect that describes a cel as currently projected.
 *--------------------------------------------------------------------------*/

CRect *
CRectFromCel (CRect *dst, CCB *cel)
{
  SRect srect;

  return CRectFromSRect (dst, SRectFromCel (&srect, cel));
}
