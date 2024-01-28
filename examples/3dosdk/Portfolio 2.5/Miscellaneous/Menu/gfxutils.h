#ifndef __GFXUTILS_H
#define __GFXUTILS_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: gfxutils.h,v 1.1 1994/10/18 20:04:37 vertex Exp $
**
**  Simple general-purpose graphics utility routines for use in the menu
**  example program.
**
******************************************************************************/

#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __GRAPHICS_H
#include "graphics.h"
#endif

/*****************************************************************************/

void SetGfxUtilsItems (Item bitmap, Item vramIOReq);

void SetBitmap (uint32 color);
void SetFgColor (uint32 color);
void SetBgColor (uint32 color);

uint32 TextWidth (char *text);
uint32 CharWidth (char ch);
uint32 TextHeight (char *text);
uint32 CharHeight (char ch);

void DrawLine (uint32 x0, uint32 y0, uint32 x1, uint32 y1);
void DrawText (char *text, uint32 x, uint32 y);
void DrawBox (int32 x0, int32 y0, int32 x1, int32 y1);

/*****************************************************************************/

#endif /* __GFXUTILS_H */
