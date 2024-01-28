#ifndef __CELUTILS_H
#define __CELUTILS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: celutils.h,v 1.5 1994/11/08 06:23:14 ewhac Exp $
**
**  Lib3DO utilities for working with cels and related datatypes.
**
**  Handy mnemonic hints:  parms to all these functions are generally in the
**  order that they appear in the function name.  For example, SRectFromCrect()
**  takes an SRect as the 1st parm, a CRect as the 2nd; CenterCelOverIPoint()
**  takes a cel as the 1st parm, an IPoint as the 2nd, and so on.  Also, the
**  order of parms is generally (destination, source) (like the ANSI string
**  library) when the whole concept of src/dst has any meaning to the function.
**
**  For functions that convert between rectangle types, or calc the union or
**  intersection of two rectangles, the destination rect can be the same as
**  either source rect.      (Note that for intersections, the dest rect is
**  modified even when there's no common area.)
**
**  Only the intersection functions and the cel creation functions can return
**  a NULL pointer.  The other functions return the same destination pointer
**  as you passed in, to allow inline calling syntax such as:
**          CenterRectCelOverIPoint(cel, IPointFromIVal(&workpoint, myX, myY));
**  When the intersection functions return NULL, that indicates that there
**  was no common area between the two rectangles to be intersected.  When
**  the cel creation functions return NULL, it indicates insufficient
**  memory to create the cel.
**
**  The Render-In-Cel functions do NO CLIPPING of any sort; passing parms
**  that would cause rendering outside the cel's data buffer will result
**  in crashes and other buggy stuff.
**
**  Most of these functions expect non-NULL pointers, and have no internal
**  error checking for valid pointers and other parms.  Notable exceptions
**  are DeleteCel(), LastCelInList(), and the ChainCels() calls, all of
**  which are specifically designed to handle NULL pointers intelligently.
**
******************************************************************************/


#include "graphics.h"
#include "operamath.h"
#include "mem.h"
#include "macros3do.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef struct FPoint {	/* Frac16 point */
	frac16	x;
	frac16	y;
} FPoint;

typedef struct IPoint {	/* Integer point */
	int32	x;
	int32	y;
} IPoint;

typedef struct SRect {	/* Size Rectangle	- specifies topleft corner and size */
	IPoint	pos;		/*	x/y of TopLeft corner */
	IPoint	size;		/*	x/y sizes (ie, width and height) */
} SRect;

typedef struct CRect {	/* Corner Rectangle	- specifies diagonal corners */
	IPoint	tl;			/* 	TopLeft corner */
	IPoint	br;			/* 	BottomRight corner */
} CRect;

typedef struct CQuad {	/* Corner quad		- specifies all four corners */
	IPoint	tl;			/*	TopLeft corner */
	IPoint	tr;			/* 	TopRight corner */
	IPoint	br;			/* 	BottomRight corner */
	IPoint	bl;			/* 	BottomLeft corner */
} CQuad;

/*----------------------------------------------------------------------------
 * Options flags for various functions.
 *	Where flags are defined as 0x bitmapped values, they can be ORed together.
 *--------------------------------------------------------------------------*/

#define CLONECEL_CCB_ONLY		0x00000000
#define CLONECEL_COPY_PIXELS	0x00000001
#define CLONECEL_COPY_PLUT		0x00000002

#define INITCEL_UNCODED			0x00000000
#define INITCEL_CODED			0x00000001

#define CREATECEL_UNCODED		INITCEL_UNCODED
#define CREATECEL_CODED			INITCEL_CODED

/*----------------------------------------------------------------------------
 * CCB field access macros.
 *	There are some fields in CCBs that must be intrepreted one way or
 *	another based on the setting of some flag.  These macros help access
 *	the values in such fields by doing the interpretation for you.
 *--------------------------------------------------------------------------*/

#define IS_LASTCEL(ccb)		(((ccb)->ccb_Flags & CCB_LAST) || (ccb)->ccb_NextPtr == NULL)

#define CEL_NEXTPTR(ccb)	(CCB *)		(((ccb)->ccb_Flags & CCB_LAST)  ? NULL : ((ccb)->ccb_Flags & CCB_NPABS) ? (ccb)->ccb_NextPtr : AddToPtr(&((ccb)->ccb_NextPtr), (int32)((ccb)->ccb_NextPtr)+4))
#define CEL_PLUTPTR(ccb)	(uint16 *)	(((ccb)->ccb_Flags & CCB_PPABS) ? (ccb)->ccb_PLUTPtr   : AddToPtr(&((ccb)->ccb_PLUTPtr), (int32)((ccb)->ccb_PLUTPtr)+4))
#define CEL_DATAPTR(ccb)	(CelData *)	(((ccb)->ccb_Flags & CCB_SPABS) ? (ccb)->ccb_SourcePtr : AddToPtr(&((ccb)->ccb_SourcePtr), (int32)((ccb)->ccb_SourcePtr)+4))

#define CEL_PRE0WORD(ccb)	(((ccb)->ccb_Flags & CCB_CCBPRE) ? (ccb)->ccb_PRE0 : ((uint32 *)CEL_DATAPTR(ccb))[0])
#define CEL_PRE1WORD(ccb)	(((ccb)->ccb_Flags & CCB_CCBPRE) ? (ccb)->ccb_PRE1 : ((uint32 *)CEL_DATAPTR(ccb))[1])

/*----------------------------------------------------------------------------
 * Misc cel macros.
 *--------------------------------------------------------------------------*/

#define SKIP_CEL(ccb)		ccb->ccb_Flags |= CCB_SKIP
#define UNSKIP_CEL(ccb)		ccb->ccb_Flags &= ~CCB_SKIP

#define LAST_CEL(ccb)		ccb->ccb_Flags |= CCB_LAST
#define UNLAST_CEL(ccb)		ccb->ccb_Flags &= ~CCB_LAST

/*----------------------------------------------------------------------------
 * Widely-used PIXC values.
 *--------------------------------------------------------------------------*/

#define	PIXC_OPAQUE			0x1F001F00		/*  "Well-known" value  */

/*----------------------------------------------------------------------------
 * macros for Point and Rect conversions.
 *	These exist mostly to support library internals, but feel free to use
 *	them in application code if you want.
 *--------------------------------------------------------------------------*/

#define XSIZEFROMCRECT(r)	((r)->br.x - (r)->tl.x + 1)
#define YSIZEFROMCRECT(r)	((r)->br.y - (r)->tl.y + 1)
#define XCORNERFROMSRECT(r)	((r)->pos.x + (r)->size.x - 1)
#define YCORNERFROMSRECT(r)	((r)->pos.y + (r)->size.y - 1)

/*----------------------------------------------------------------------------
 * Prototypes...
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

/*----------------------------------------------------------------------------
 * functions for creating lists of cels.
 *	All of these quietly cope with NULL pointers in fairly intelligent ways.
 *--------------------------------------------------------------------------*/

CCB *		LastCelInList(CCB *list);							/* returns -> last cel in list */
CCB *		ChainCelsAtTail(CCB *existingCels, CCB *newCels);	/* returns -> new last cel in list (LastCelInList(newcels)) */
CCB *		ChainCelsAtHead(CCB *existingCels, CCB *newCels);	/* returns -> new first cel in list (newcels) */
void		LinkCel(CCB *ccb, CCB *nextCCB);

/*----------------------------------------------------------------------------
 * functions for mapping and moving and sizing cels.
 *--------------------------------------------------------------------------*/

void 		OffsetCel(CCB *ccb, int32 xOffset, int32 yOffset);

void		OffsetCelByFDelta(CCB *list, FPoint *deltaXY);
void		OffsetCelByIDelta(CCB *list, IPoint *deltaXY);

void		OffsetAACelByFDelta(CCB *list, FPoint *deltaXY);
void		OffsetAACelByIDelta(CCB *list, IPoint *deltaXY);

void		OffsetCelListByFDelta(CCB *list, FPoint *deltaXY, Boolean copyPerspective);
void		OffsetCelListByIDelta(CCB *list, IPoint *deltaXY, Boolean copyPerspective);

void		MapCelToFPoint(CCB *cel, FPoint *newPosition);
void		MapCelToIPoint(CCB *cel, IPoint *newPosition);

void		MapAACelToFPoint(CCB *aacel, FPoint *newPosition);
void		MapAACelToIPoint(CCB *aacel, IPoint *newPosition);

void		MapCelListToFPoint(CCB *list, FPoint *newPosition, Boolean copyPerspective);
void		MapCelListToIPoint(CCB *list, IPoint *newPosition, Boolean copyPerspective);

void		MapCelToCRect(CCB *cel, CRect *rect);
void		MapCelToSRect(CCB *cel, SRect *rect);

void		MapAACelToCRect(CCB *aacel, CRect *rect);
void		MapAACelToSRect(CCB *aacel, SRect *rect);

void		MapCelListToCRect(CCB *list, CRect *rect);
void		MapCelListToSRect(CCB *list, SRect *rect);

void		MapCelToCQuad(CCB *cel, CQuad *quad);
void		MapAACelToCQuad(CCB *aacel, CQuad *quad);
void		MapCelListToCQuad(CCB *list, CQuad *quad);

void		CenterRectCelOverFPoint(CCB *cel, FPoint *point);
void		CenterRectCelOverIPoint(CCB *cel, IPoint *point);

void		CenterRectAACelOverFPoint(CCB *cel, FPoint *point);
void		CenterRectAACelOverIPoint(CCB *cel, IPoint *point);

void		CenterRectCelListOverFPoint(CCB *cel, FPoint *point);
void		CenterRectCelListOverIPoint(CCB *cel, IPoint *point);

void		CenterRectCelInCRect(CCB *cel, CRect *rect);
void		CenterRectCelInSRect(CCB *cel, SRect *rect);

void		CenterRectAACelInCRect(CCB *cel, CRect *rect);
void		CenterRectAACelInSRect(CCB *cel, SRect *rect);

void		CenterRectCelListInCRect(CCB *cel, CRect *rect);
void		CenterRectCelListInSRect(CCB *cel, SRect *rect);

void		CenterRectCelInDisplay(CCB *cel);
void		CenterRectAACelInDisplay(CCB *cel);
void		CenterRectCelListInDisplay(CCB *cel);

/*----------------------------------------------------------------------------
 * functions for Point and Rect conversions.
 *--------------------------------------------------------------------------*/

FPoint *	FPointFromIVal(FPoint *dst, int32 x, int32 y);
FPoint *	FPointFromFVal(FPoint *dst, frac16 x, frac16 y);
FPoint *	FPointFromIPoint(FPoint *dst, IPoint *src);

IPoint *	IPointFromIVal(IPoint *dst, int32 x, int32 y);
IPoint *	IPointFromFVal(IPoint *dst, frac16 x, frac16 y);
IPoint *	IPointFromFPoint(IPoint *dst, FPoint *src);

CRect *		CRectFromIVal(CRect *dst, int32  tlx, int32  tly, int32  brx, int32  bry);
CRect *		CRectFromSRect(CRect *dst, SRect *src);
CRect *		CRectFromCel(CRect *dst, CCB *cel);

SRect *		SRectFromIVal(SRect *dst, int32 x, int32 y, int32 w, int32 h);
SRect *		SRectFromCRect(SRect *dst, CRect *src);
SRect *		SRectFromCel(SRect *dst, CCB *cel);

IPoint *	ISizeFromCRect(IPoint *dst, CRect *src);
IPoint *	ICornerFromSRect(IPoint *dst, SRect *src);

/*----------------------------------------------------------------------------
 * functions for manipulating Points and Rects.
 *--------------------------------------------------------------------------*/

FPoint *	CenterFPointInDisplay(void);
IPoint *	CenterIPointInDisplay(void);

IPoint *	CenterIPointInSRect(IPoint *dst, SRect *rect);
IPoint *	CenterIPointInCRect(IPoint *dst, CRect *rect);

SRect *		CenterSRectOverIPoint(SRect *dst, IPoint *point);
CRect *		CenterCRectOverIPoint(CRect *dst, IPoint *point);

SRect *		CenterSRectInSRect(SRect *dst, SRect *rect);
SRect *		CenterSRectInDisplay(SRect *dst);

CRect *		CenterCRectInCRect(CRect *dst, CRect *rect);
CRect *		CenterCRectInDisplay(CRect *dst);

SRect *		OffsetSRect(SRect *dst, IPoint *delta);
CRect *		OffsetCRect(CRect *dst, IPoint *delta);

SRect *		InsetSRect(SRect *dst, IPoint *delta);
CRect *		InsetCRect(CRect *dst, IPoint *delta);

SRect *		SRectBounds(SRect *dst, SRect *rect1, SRect *rect2);
SRect *		SRectIntersection(SRect *dst, SRect *rect1, SRect *rect2);

CRect *		CRectBounds(CRect *dst, CRect *rect1, CRect *rect2);
CRect *		CRectIntersection(CRect *dst, CRect *rect1, CRect *rect2);

Boolean		IPointIsInSRect(IPoint *point, SRect *rect);
Boolean		IPointIsInCRect(IPoint *point, CRect *rect);

/*----------------------------------------------------------------------------
 * functions for creating and deleting cels.
 *	DeleteCel() can delete any of the cel types created by these functions,
 *	including one returned by LoadCel().  UnloadCel() just calls DeleteCel().
 *	DeleteCel() can handle a NULL pointer, and always returns a NULL pointer.
 *	DeleteCelList() walks the ccb_NextPtr list, calling DeleteCel() on each.
 *--------------------------------------------------------------------------*/

CCB *		DeleteCel(CCB *cel);
CCB *		DeleteCelList(CCB *celList);

CCB *		CreateCel(int32 width, int32 height, int32 bitsPerPixel, int32 options, void *dataBuf);
CCB *		CreateBackdropCel(int32 width, int32 height, int32 color, int32 opacityPercent);
CCB *		CreateLRFormCel(CCB *dst, Item screenItem, SRect *subRect);
CCB *		CreateSubrectCel(CCB *dst, CCB *src, SRect *subRect);

CCB *		LoadCel (char *filename, uint32 memTypeBits);
void 		UnloadCel(CCB *cel);

CCB *		CloneCel(CCB *src, int32 options);

/*----------------------------------------------------------------------------
 * functions for rendering into a cel's data buffer.
 *--------------------------------------------------------------------------*/

void		RenderCelPixel(CCB *cel, int32 pixel, int32 x, int32 y);
void		RenderCelHLine(CCB *cel, int32 pixel, int32 x, int32 y, int32 w);
void		RenderCelVLine(CCB *cel, int32 pixel, int32 x, int32 y, int32 h);
void		RenderCelFillRect(CCB *cel, int32 pixel, int32 x, int32 y, int32 w, int32 h);
void		RenderCelOutlineRect(CCB *cel, int32 pixel, int32 x, int32 y, int32 w, int32 h);

int32		ReturnCelPixel(CCB *cel, int32 x, int32 y);

/*----------------------------------------------------------------------------
 * misc functions.
 *--------------------------------------------------------------------------*/

CCB *		CrossFadeCels8(Item screen, int32 step, CCB *oldCel, CCB *newCel);
CCB *		CrossFadeCels(Item screen, int32 step, CCB *oldCel, CCB *newCel);

CCB *		ParseCel(void *inBuf, int32 inBufSize);

int32		InitCel(CCB * cel, int32 width, int32 height, int32 bitsPerPixel, int32 options);

/*----------------------------------------------------------------------------
 * functions that support library internals, but might be generally useful.
 *--------------------------------------------------------------------------*/

int32		GetCelBitsPerPixel(CCB *cel);
int32		GetCelBytesPerRow(CCB *cel);
int32		GetCelDataBufferSize(CCB *cel);

#ifdef __cplusplus
  }
#endif

#endif /* __CELUTILS_H */
