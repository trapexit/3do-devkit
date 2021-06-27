/*
	File:		Utils3DO.h

	Contains:	graphic utility routines for the 3DO Station.

	Written by:	Neil Cormia

	Copyright:	© 1992 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 6/15/93	JAY		change SET_TO_NORMAL macro to use new name (ccb_PIXC) for old
									name (ccb_PPMPC).
		 <5>	 1/28/93	dsm		fixed bug in UNLAST_CEL macro.
		 <4>	12/10/92	JML		GetCel and AnimCels moved back to Utils3DO.h
		 <3>	12/10/92	JML		GetCel and AnimCels moved to Parse3DO.h
		 <2>	12/10/92	JML		Integration with Neil's header
		 <1>	 12/9/92	JML		first checked in

	To Do:
*/

#pragma force_top_level
#pragma include_only_once

#ifndef __Utils3DO_h
#define __Utils3DO_h

#ifndef __Parse3DO_h
#include "parse3do.h"
#endif

#include "debug.h"

/* ================== DEFINES and MACROS ================== */

#define	MAX_SCALE	25		/* Max value for SetCelScale routine */



#define SKIP_CEL(ccb)		ccb->ccb_Flags |= CCB_SKIP
#define UNSKIP_CEL(ccb)		ccb->ccb_Flags &= ~CCB_SKIP

#define LAST_CEL(ccb)		ccb->ccb_Flags	|= CCB_LAST
#define UNLAST_CEL(ccb)		ccb->ccb_Flags	&= ~CCB_LAST

#define	SET_TO_SHADOW(ccb)	ccb->ccb_PIXC = PPMPC_1S_CFBD | PPMPC_MF_8 | PPMPC_SF_8
#define	SET_TO_AVERAGE(ccb)	ccb->ccb_PIXC = 0x01F80L
#define	SET_TO_NORMAL(ccb)	ccb->ccb_PIXC = 0x01F00L

#define FADE_FRAMECOUNT 48
#define	SCALER_MASK		(PPMPC_MF_MASK + PPMPC_SF_MASK + (PPMPC_MF_MASK<<16) + (PPMPC_SF_MASK<<16))

#define CenterHotSpot	1
#define UpperLeftHotSpot	2
#define LowerLeftHotSpot	3
#define UpperRightHotSpot	4
#define LowerRightHotSpot	5

#if DEBUG
#define DIAGNOSTIC(s)		kprintf("Error: %s\n", s)
#else
#define DIAGNOSTIC(s)
#endif

/* ================== STRUCTURES ================== */

typedef struct tag_Rectf16
	{
	frac16 rectf16_XLeft;
	frac16 rectf16_YTop;
	frac16 rectf16_XRight;
	frac16 rectf16_YBottom;
	} Rectf16;

typedef	struct tag_MoveVect
	{
	frac16	xVector;
	frac16	yVector;
	} MoveVect;
	
typedef struct tag_MoveRec
	{
	MoveVect	curQuadf16[4];	/* the current Coord positions for the Cel */
	MoveVect	quadIncr[4];	/* X and Y increments for the Cel's corners */
	} MoveRec;

typedef struct CelLink {
	struct CelLink	*next;
	struct CelLink	*prev;
	CCB		*ccb;
} CelLink;


/* ================== TABLES ================== */

#define	NUM_FADE_STEPS	20
#define	MAX_FADE_IN		22

/*	This array contains the Mult and Div values to OR into the PPMP
	control word after masking off the old values. These values are
	organized to provide the smoothest ramping possible
*/
extern ulong ScalarValues[NUM_FADE_STEPS];


/* ================== PROTOTYPES ================== */

#ifdef __cplusplus 		
extern "C" {
#endif
extern void MapP2Cel(CCB* ccb, Point* quad);
extern void SetCelScale (CCB *ccb, CCB *maskccb, int32 step);
extern void	SetFadeInCel (CCB *ccb, CCB *maskccb, int32 *stepValue);
extern bool	FadeInCel (CCB *ccb, CCB *maskccb, int32 *stepValue);
extern void	SetFadeOutCel (CCB *ccb, CCB *maskccb, int32 *stepValue);
extern bool	FadeOutCel (CCB *ccb, CCB *maskccb, int32 *stepValue);

extern void PreMoveCel (CCB *ccb, Point *beginQuad, Point *endQuad, int32 numberOfFrames, MoveRec *pMove);
extern void MoveCel (CCB *ccb, MoveRec *pMove);

extern void SetQuad (Point *r, Coord left, Coord top, Coord right, Coord bottom);
extern void SetRectf16 (Rectf16 *r, Coord left, Coord top, Coord right, Coord bottom);
extern void	CenterRectf16 (Point *q, Rectf16 *rect, Rectf16 *Frame);
extern void	CenterCelOnScreen (CCB *ccb);

extern bool FrameBufferToCel(Item iScreen, CCB* cel);
extern CCB* MakeNewCel(Rectf16 *r);

extern CCB*	MakeNewDupCCB( CCB *ccb );
extern void	LinkCel( CCB *ccb, CCB *nextCCB );

extern void OffsetCel (CCB *ccb, int32 xOffset, int32 yOffset);
extern void FreeBuffer (char *filename, long *fileBuffer);

extern void	DrawAnimCel(ANIM *pAnim, Item bitmapItem, long xPos, long yPos, frac16 frameIncrement, long hotSpot);
extern CCB*	GetAnimCel(ANIM *pAnim, frac16 frameIncrement);

extern bool DrawImage( Item iScreen, ubyte* pbImage, ScreenContext *sc);
extern void FadeToBlack(ScreenContext *sc, int32 nFrames);
extern void FadeFromBlack(ScreenContext *sc, int32 frameCount);
extern long ReadControlPad(long lControlMask);
extern bool SetChannel(Item iInstrument, int nChannel);
extern bool SetMixer(int nChannel, int32 nVolume, int32 nBalance);

extern long  WriteMacFile(char *filename, char *buf, long count );

#ifdef __cplusplus
}
#endif	


#define TEST_SCRIPT_COUNT 1000
#endif
