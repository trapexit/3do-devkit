#ifndef __UTILS3DO_H
#define __UTILS3DO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: utils3do.h,v 1.8 1994/10/05 17:34:41 vertex Exp $
**
**  Lib3DO graphic utility routines for 3DO
**
**  This header is all but obsolete.  It includes Parse3DO.h, which includes
**  most of the current headers (CelUtils, AnimUtils, etc).  This header
**  exists mainly for compatibility for old code.
**
******************************************************************************/


#include "parse3do.h"
#include "blockfile.h"

/*----------------------------------------------------------------------------
 * Marginal macros.
 *	Most of this stuff should probably go away.
 *--------------------------------------------------------------------------*/

#define	MAX_SCALE	25		/* Max value for SetCelScale routine */

#define	SET_TO_SHADOW(ccb)	ccb->ccb_PIXC = PPMPC_1S_CFBD | PPMPC_MF_8 | PPMPC_SF_8
#define	SET_TO_AVERAGE(ccb)	ccb->ccb_PIXC = 0x01F80L
#define	SET_TO_NORMAL(ccb)	ccb->ccb_PIXC = 0x01F00L

#define FADE_FRAMECOUNT 48
#define	SCALER_MASK		(PPMPC_MF_MASK + PPMPC_SF_MASK +(PPMPC_MF_MASK<<16) +(PPMPC_SF_MASK<<16))

#define	NUM_FADE_STEPS	20
#define	MAX_FADE_IN		22

/*----------------------------------------------------------------------------
 * Obsolete stuff.
 *	These functions are going away in future versions.
 *--------------------------------------------------------------------------*/

#ifndef DIAGNOSTIC
  #define DIAGNOSTIC(x)		printf("Error: %s\n", x)
#endif

typedef struct CelLink {		/* what is this and what's it for and who */
	struct CelLink	*next;		/* uses it?  It should probably go away.  */
	struct CelLink	*prev;
	CCB		*ccb;
} CelLink;

typedef struct Rectf16 {
	frac16 rectf16_XLeft;
	frac16 rectf16_YTop;
	frac16 rectf16_XRight;
	frac16 rectf16_YBottom;
} Rectf16;

typedef	struct MoveVect {
	frac16	xVector;
	frac16	yVector;
} MoveVect;

typedef struct MoveRec {
	MoveVect	curQuadf16[4];	/* the current Coord positions for the Cel */
	MoveVect	quadIncr[4];	/* X and Y increments for the Cel's corners */
} MoveRec;

#ifdef __cplusplus
  extern "C" {
#endif

void 	PreMoveCel(CCB *ccb, Point *beginQuad, Point *endQuad, int32 numberOfFrames, MoveRec *pMove);
void 	MoveCel(CCB *ccb, MoveRec *pMove);

void 	MapP2Cel(CCB* ccb, Point* quad);

void 	SetQuad(Point *r, Coord left, Coord top, Coord right, Coord bottom);
void 	SetRectf16(Rectf16 *r, Coord left, Coord top, Coord right, Coord bottom);
void	CenterRectf16(Point *q, Rectf16 *rect, Rectf16 *Frame);
void	CenterCelOnScreen(CCB *ccb);

void 	SetCelScale(CCB *ccb, CCB *maskccb, int32 step);
void	SetFadeInCel(CCB *ccb, CCB *maskccb, int32 *stepValue);
Boolean	FadeInCel(CCB *ccb, CCB *maskccb, int32 *stepValue);
void	SetFadeOutCel(CCB *ccb, CCB *maskccb, int32 *stepValue);
Boolean	FadeOutCel(CCB *ccb, CCB *maskccb, int32 *stepValue);

int32 	ReadControlPad(int32 lControlMask);

Boolean FrameBufferToCel(Item iScreen, CCB* cel);
CCB* 	MakeNewCel(Rectf16 *r);
CCB*	MakeNewDupCCB(CCB *ccb);
void 	FreeBuffer(char *filename, int32 *fileBuffer);

#ifdef __cplusplus
  }
#endif

#endif /* __UTILS3DO_H */
