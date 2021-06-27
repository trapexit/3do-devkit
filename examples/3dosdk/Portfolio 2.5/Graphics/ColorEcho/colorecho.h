#ifndef __COLORECHO_H
#define __COLORECHO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: colorecho.h,v 1.5 1994/12/05 20:23:29 vertex Exp $
**
******************************************************************************/

#include "types.h"
#include "kernel.h"
#include "nodes.h"
#include "mem.h"
#include "string.h"
#include "stdlib.h"
#include "debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "event.h"
#include "graphics.h"
#include "operamath.h"
#include "init3do.h"


#define DISPLAY_WIDTH        (320)
#define DISPLAY_HEIGHT       (240)

#define CURBITMAPITEM sc->sc_BitmapItems[ScreenSelect]

#define MIDDLEX (DISPLAY_WIDTH >> 1)
#define MIDDLEY (DISPLAY_HEIGHT >> 1)
#define ANGLEDELTA ((HALFCIRCLE>>9))    /* Amount to rotate per frame. */
#define BYTESPERPIXEL (2)
#define MINOFFSET            (-20)
#define MAXOFFSET            (20)
#define ZOOMSHIFT            (9)
#define ZOOMONE              (1<<ZOOMSHIFT)
#define MAXZOOM              (ZOOMONE<<1)
#define MINZOOM              (ZOOMONE>>2)
#define MAXHANDSOFF          (5*60)       /* VBLs until Auto takes over. */

/* These represent the value one (1) in various number formats.
 * For example, ONE_12_20 is the value of 1 in fixed decimal format
 * of 12 bits integer, 20 bits fraction
 */
#define ONE_12_20  (1<<20)
#define ONE_16_16  (1<<16)

#define CEL_WIDTH  (DISPLAY_WIDTH)
#define CEL_HEIGHT (DISPLAY_HEIGHT)

#define PPMP_BOTH_NORMAL ((PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_NORMAL << PPMP_1_SHIFT))
#define PPMP_BOTH_AVERAGE ((PPMP_MODE_AVERAGE << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT))
#define PPMP_BOTH_MIXED ((PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT))

/* Flags */
#define CE_ENABLE_AUTO         (0x0001)
#define CE_PATTERN_ON          (0x0002)

typedef struct ColorEcho
{
	uint32 ce_Flags;
	int32  ce_Zoom;
	int32  ce_Radius;
	int32  ce_HalfDiagonal;
	int32  ce_MiddleX;
	int32  ce_MiddleY;
	int32  ce_XOffset;
	int32  ce_YOffset;
	int32  ce_IfSport;
	frac16 ce_Angle;
	frac16 ce_Theta;
	uint32 ce_PIXC;
	int32  ce_ZoomVelocity;
	int32  ce_AngleVelocity;
	int32  ce_XVelocity;
	int32  ce_YVelocity;
	struct CCB *ce_CCB;
	int32  ce_PatternSeed;
} ColorEcho;

int32  RandomBoxes ( Item BitMap, int32 NumBoxes );
int32  RandomPixels ( Item BitMap, int32 XCenter, int32 YCenter, int32 NumPixels );
uint32 Random (uint32);
int32  ce_Init( ColorEcho *ce );
int32  ce_DrawNextScreen( ScreenContext *sc, ColorEcho *ce );
void   ce_Freeze( ColorEcho *ce );
void   ce_Center( ColorEcho *ce );
int32  ce_Seed( ScreenContext *sc, ColorEcho *ce );
int32  ce_SeedPattern( ScreenContext *sc, ColorEcho *ce );

/* Sound routines. */
Err ceInitSound( void );
Err cePictureToSound( ScreenContext *sc, ColorEcho *ce );
Err ceStartVoices( void );

#endif /* __COLORECHO_H */
