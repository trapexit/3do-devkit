
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: RenderCelPixel.c,v 1.3 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO routines to render pixels into a cel's data buffer.  Functions are
**  available to render individual pixels, and several basic
**  shapes including horizontal and vertical lines, and
**  filled and outlined rectangles.
**
**  Use CreateCel() to create empty cels suitable for use with these functions.
**  TextCel cels should work fine too, but you'll have to be aware of and work
**  with the anti-aliasing color pallete attached to a TextCel.
**
**  These are definitely NOT high-performance rendering routines.  They should
**  be adequate for creating simple dialog boxes, buttons, highlighting cels,
**  and other create-once-use-a-lot sort of items.
**
**  These routines currently support all cel types EXCEPT 6-bpp and LRForm.
**
******************************************************************************/


#include "celutils.h"
#include "debug3do.h"

/*----------------------------------------------------------------------------
 * private datatype, holds pre-calc'd values (or will someday anyway).
 *--------------------------------------------------------------------------*/

typedef struct RenderCelInfo {
	void *	pixels;
	int32	bytesPerRow;
	int32	bitsPerPixel;
	int32	pixelMask;
} RenderCelInfo;

/*****************************************************************************
 *
 ****************************************************************************/

static Boolean GetRenderCelInfo_(RenderCelInfo *pInfo, CCB *pCel)
{
	pInfo->pixels = CEL_DATAPTR(pCel);
	if (!(pCel->ccb_Flags & CCB_CCBPRE)) {
		pInfo->pixels = AddToPtr(pInfo->pixels, 2*sizeof(int32));
	}

	pInfo->bytesPerRow 	= GetCelBytesPerRow(pCel);
	pInfo->bitsPerPixel	= GetCelBitsPerPixel(pCel);
	pInfo->pixelMask	= (1L << pInfo->bitsPerPixel) - 1L;

#ifdef DEBUG
	if (CEL_PRE0WORD(pCel) & PRE0_LITERAL) {
		DIAGNOSE(("Can't render into packed cels\n"));
		return FALSE;
	}
	if (CEL_PRE1WORD(pCel) & PRE1_LRFORM) {
		DIAGNOSE(("Can't render into LRFORM cels\n"));
		return FALSE;
	}
	if (pInfo->bitsPerPixel == 6) {
		DIAGNOSE(("Can't render into 6 bit per pixel cels\n"));
		return FALSE;
	}
#endif

	return TRUE;
}

/*****************************************************************************
 *
 ****************************************************************************/

static int32 ReturnCelPixel_(RenderCelInfo *celInfo, int32 x, int32 y)
{
	uchar *			pPixel;
	int32			bitOffset;
	int32			bitShift;
	int32			temp;

	bitOffset = x * celInfo->bitsPerPixel;
	pPixel    = (uchar *)AddToPtr(celInfo->pixels, y * celInfo->bytesPerRow + bitOffset / 8);

	switch (celInfo->bitsPerPixel) {
	  case 16:
	  	temp = *(ushort *)pPixel;
		break;
	  case 8:
	  	temp = *pPixel;
		break;
	  default:
		bitShift  = (8 - celInfo->bitsPerPixel) - (bitOffset & 0x0007);
		temp = ((uint32)*pPixel >> bitShift);
		break;
	}

	return (temp & celInfo->pixelMask);
}

/*****************************************************************************
 *
 ****************************************************************************/

int32 ReturnCelPixel(CCB *pCel, int32 x, int32 y)
{
	RenderCelInfo	celInfo;

	if (!GetRenderCelInfo_(&celInfo, pCel)) {
		return 0;
	}

	return ReturnCelPixel_(&celInfo, x, y);
}

/*****************************************************************************
 *
 ****************************************************************************/

static void RenderCelPixel_(RenderCelInfo *celInfo, int32 pixel, int32 x, int32 y)
{
	uchar *			pPixel;
	int32			bitOffset;
	int32			bitShift;
	uint32			temp;

	bitOffset = x * celInfo->bitsPerPixel;
	pPixel    = (uchar *)AddToPtr(celInfo->pixels, y * celInfo->bytesPerRow + bitOffset / 8);

	switch (celInfo->bitsPerPixel) {
	  case 16:
	  	*(ushort *)pPixel = (ushort)pixel;
		break;
	  case 8:
	  	*pPixel = (uchar)pixel;
		break;
	  default:
		bitShift  = (8 - celInfo->bitsPerPixel) - (bitOffset & 0x0007);
		temp = *pPixel & ~(celInfo->pixelMask << bitShift);
		*pPixel = (uchar)(temp | ((pixel & celInfo->pixelMask) << bitShift));
		break;
	}
}

/*****************************************************************************
 *
 ****************************************************************************/

void RenderCelPixel(CCB *pCel, int32 pixel, int32 x, int32 y)
{
	RenderCelInfo	celInfo;

	if (!GetRenderCelInfo_(&celInfo, pCel)) {
		return;
	}

	RenderCelPixel_(&celInfo, pixel, x, y);
}

/*****************************************************************************
 *
 ****************************************************************************/

void RenderCelHLine(CCB *pCel, int32 pixel, int32 x, int32 y, int32 w)
{
	RenderCelInfo	celInfo;

	if (!GetRenderCelInfo_(&celInfo, pCel)) {
		return;
	}

	while (w--) {
		RenderCelPixel_(&celInfo, pixel, x++, y);
	}
}

/*****************************************************************************
 *
 ****************************************************************************/

void RenderCelVLine(CCB *pCel, int32 pixel, int32 x, int32 y, int32 h)
{
	RenderCelInfo	celInfo;

	if (!GetRenderCelInfo_(&celInfo, pCel)) {
		return;
	}

	while (h--) {
		RenderCelPixel_(&celInfo, pixel, x, y++);
	}
}

/*****************************************************************************
 *
 ****************************************************************************/

void RenderCelFillRect(CCB *pCel, int32 pixel, int32 x, int32 y, int32 w, int32 h)
{
	while (h--) {
		RenderCelHLine(pCel, pixel, x, y++, w);
	}
}

/*****************************************************************************
 *
 ****************************************************************************/

void RenderCelOutlineRect(CCB *pCel, int32 pixel, int32 x, int32 y, int32 w, int32 h)
{
	RenderCelHLine(pCel, pixel, x, 	   y,	  w);
	RenderCelHLine(pCel, pixel, x, 	   y+h-1, w);
	RenderCelVLine(pCel, pixel, x, 	   y,	  h);
	RenderCelVLine(pCel, pixel, x+w-1, y,	  h);
}
