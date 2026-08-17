#pragma once

#pragma force_top_level
#pragma once
/******************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
 *reserved.
 **  This material contains confidential information that is the property of The
 *3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: colorecho.h,v 1.5 1994/12/05 20:23:29 vertex Exp $
 **
 ******************************************************************************/

#include "debug.h"
#include "event.h"
#include "graphics.h"
#include "init3do.h"
#include "kernel.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "types.h"

#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (240)

#define CURBITMAPITEM sc->sc_BitmapItems[ScreenSelect]

#define MIDDLEX (DISPLAY_WIDTH >> 1)
#define MIDDLEY (DISPLAY_HEIGHT >> 1)
#define ANGLEDELTA ((HALFCIRCLE >> 9)) /* Amount to rotate per frame. */
#define BYTESPERPIXEL (2)
#define MINOFFSET (-20)
#define MAXOFFSET (20)
#define ZOOMSHIFT (9)
#define ZOOMONE (1 << ZOOMSHIFT)
#define MAXZOOM (ZOOMONE << 1)
#define MINZOOM (ZOOMONE >> 2)
#define MAXHANDSOFF (5 * 60) /* VBLs until Auto takes over. */

#define CEL_WIDTH (DISPLAY_WIDTH)
#define CEL_HEIGHT (DISPLAY_HEIGHT)

#define PPMP_BOTH_NORMAL ((PPMP_MODE_NORMAL << PPMP_0_SHIFT) | (PPMP_MODE_NORMAL << PPMP_1_SHIFT))
#define PPMP_BOTH_AVERAGE                                                                          \
  ((PPMP_MODE_AVERAGE << PPMP_0_SHIFT) | (PPMP_MODE_AVERAGE << PPMP_1_SHIFT))
#define PPMP_BOTH_MIXED ((PPMP_MODE_NORMAL << PPMP_0_SHIFT) | (PPMP_MODE_AVERAGE << PPMP_1_SHIFT))

/* Flags */
#define CE_ENABLE_AUTO (0x0001)
#define CE_PATTERN_ON (0x0002)

typedef struct ColorEcho
{
  u32 ce_Flags;
  s32 ce_Zoom;
  s32 ce_Radius;
  s32 ce_HalfDiagonal;
  s32 ce_MiddleX;
  s32 ce_MiddleY;
  s32 ce_XOffset;
  s32 ce_YOffset;
  s32 ce_IfSport;
  frac16 ce_Angle;
  frac16 ce_Theta;
  u32 ce_PIXC;
  s32 ce_ZoomVelocity;
  s32 ce_AngleVelocity;
  s32 ce_XVelocity;
  s32 ce_YVelocity;
  struct CCB *ce_CCB;
  s32 ce_PatternSeed;
} ColorEcho;

s32
RandomBoxes(Item BitMap, s32 NumBoxes);
s32
RandomPixels(Item BitMap, s32 XCenter, s32 YCenter, s32 NumPixels);
u32 Random(u32);
s32
ce_Init(ColorEcho *ce);
s32
ce_DrawNextScreen(ScreenContext *sc, ColorEcho *ce);
void
ce_Freeze(ColorEcho *ce);
void
ce_Center(ColorEcho *ce);
s32
ce_Seed(ScreenContext *sc, ColorEcho *ce);
s32
ce_SeedPattern(ScreenContext *sc, ColorEcho *ce);
