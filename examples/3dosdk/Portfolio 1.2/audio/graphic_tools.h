#pragma include_only_once
/*
** graphic_tools Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1993, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "debug.h"
#include "graphics.h"
#include "mem.h"
#include "nodes.h"
#include "stdarg.h"
#include "string.h"
#include "types.h"

#define RED_MASK 0x7C00
#define GREEN_MASK 0x03E0
#define BLUE_MASK 0x001F
#define RED_SHIFT 10
#define GREEN_SHIFT 5
#define BLUE_SHIFT 0
#define ONE_RED (1 << REDSHIFT)
#define ONE_GREEN (1 << GREENSHIFT)
#define ONE_BLUE (1 << BLUESHIFT)
#define MAX_RED (RED_MASK >> RED_SHIFT)
#define MAX_GREEN (GREEN_MASK >> GREEN_SHIFT)
#define MAX_BLUE (BLUE_MASK >> BLUE_SHIFT)
#define SCREENWIDTH (320)
#define SCREENHEIGHT (240)
#define MAX_X_POS (SCREENWIDTH - 1)
#define MAX_Y_POS (SCREENHEIGHT - 1)
#define LEFT_VISIBLE_EDGE (20)
#define TOP_VISIBLE_EDGE (12)

#define CURBITMAPITEM BitmapItems[ScreenSelect]
#define DRAWTO(xp, yp)                                                        \
  {                                                                           \
    DrawTo (CURBITMAPITEM, &GCon[0], (xp), (yp));                             \
  }

/* Prototypes */
int32 ClearScreen (void);
int32 DrawNumber (int32 num);
int32 InitGraphics (int32 NumScr);
int32 ShowScreenLimits (int32 x1, int32 y1, int32 x2, int32 y2);
int32 SwitchScreens (void);
int32 TermGraphics (void);
void DrawRect (int32 XLeft, int32 YTop, int32 XRight, int32 YBottom);
void ToggleScreen (void);
