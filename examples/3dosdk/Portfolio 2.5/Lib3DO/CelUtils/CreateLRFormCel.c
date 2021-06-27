
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: CreateLRFormCel.c,v 1.2 1994/10/05 17:59:33 vertex Exp $
**
**  Lib3DO routine to create cel from a screen or portion of a screen.
**
**  DeleteCel() compatible.
**
******************************************************************************/


#include "celutils.h"
#include "debug3do.h"
#include "operror.h"

/*----------------------------------------------------------------------------
 * CreateLRFormCel()
 *	Create a cel that projects LRForm (IE, from a screen/bitmap) data.
 *	The cel can project a sub-rectangle of the source bitmap, or if the
 *	subRect pointer is NULL it projects the entire bitmap.
 *--------------------------------------------------------------------------*/

CCB * CreateLRFormCel(CCB *dst, Item screenItem, SRect *subRect)
{
	Screen *screen;
	Bitmap *bitmap;
	void *	buffer;
	SRect	screenRect;
	CCB *	cel = NULL;

	/*------------------------------------------------------------------------
	 * Use the screen item to locate the bitmap.
	 *----------------------------------------------------------------------*/

	if ((screen = (Screen*)LookupItem(screenItem)) == NULL) {
		DIAGNOSE_SYSERR(BADITEM, ("Can't find screen item %ld\n", screenItem));
		goto ERROR_EXIT;
	}
  	bitmap = screen->scr_TempBitmap;

	/*------------------------------------------------------------------------
	 * A NULL subRect pointer implies a full-screen cel.
	 *----------------------------------------------------------------------*/

	if (subRect == NULL) {
		subRect = SRectFromIVal(&screenRect, 0, 0, bitmap->bm_Width, bitmap->bm_Height);
	}

	/*------------------------------------------------------------------------
	 * Get address of the first pixel in the subrect within the bitmap buffer.
	 *----------------------------------------------------------------------*/

	buffer = GetPixelAddress(screenItem, subRect->pos.x, subRect->pos.y);

	/*------------------------------------------------------------------------
	 * Create a cel if the caller didn't give us one.
	 *----------------------------------------------------------------------*/

	if ((cel = dst) == NULL) {
		if ((cel = CreateCel(subRect->size.x, subRect->size.y, 16, CREATECEL_UNCODED, buffer)) == NULL) {
			DIAGNOSE_SYSERR(NOMEM, ("Can't create LRForm Cel\n"));
			goto ERROR_EXIT;
		}
	}

	/*------------------------------------------------------------------------
	 * Now override the flags and preamble words needed for an lrform subrect.
	 * (Note that there is a sort of implied mul-by-two in the WOFFSET calc
	 * here, in that we don't turn pixels-per-row into words-per-row (which
	 * would be a div-by-two).  Although the docs don't mention it as such, an
	 * LRFORM cel needs preamble words that reflect half the cel's height and
	 * twice its row width, which accounts for the interleaved LRFORM scheme.)
	 *----------------------------------------------------------------------*/

	cel->ccb_Flags |= CCB_BGND | PMODE_ONE;

	cel->ccb_PRE0	= ((((subRect->size.y+1) >> 1) - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT)
					| PRE0_LINEAR
					| ((subRect->pos.x & 1) << PRE0_SKIPX_SHIFT)
					| PRE0_BPP_16;

	cel->ccb_PRE1	= ((bitmap->bm_Width - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT)
					| PRE1_TLLSB_PDC0
					| PRE1_LRFORM
					| ((subRect->size.x - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT);

	return cel;

ERROR_EXIT:

	if (cel != dst) {
		DeleteCel(cel);
	}

	return NULL;
}
