
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: gfxutils.c,v 1.1 1994/10/18 20:03:14 vertex Exp $
**
**  Simple general-purpose graphics utility routines for use in the menu
**  example program.
**
******************************************************************************/

#include "gfxutils.h"
#include "graphics.h"
#include "mem.h"
#include "string.h"
#include "types.h"

/*****************************************************************************/

/* globals to hold the state of this module */
static GrafCon gc;
static Item bm;
static Item vr;

/*****************************************************************************/

/* Initialize this module. This determines which items we're working with */
void
SetGfxUtilsItems (Item bitmap, Item vramIOReq)
{
  memset (&gc, 0, sizeof (gc));
  bm = bitmap;
  vr = vramIOReq;
  ResetCurrentFont ();
}

/*****************************************************************************/

void
SetFgColor (uint32 color)
{
  SetFGPen (&gc, color);
}

/*****************************************************************************/

void
SetBgColor (uint32 color)
{
  SetBGPen (&gc, color);
}

/*****************************************************************************/

void
DrawLine (uint32 x0, uint32 y0, uint32 x1, uint32 y1)
{
  MoveTo (&gc, x0, y0);
  DrawTo (bm, &gc, x1, y1);
}

/*****************************************************************************/

/* draw some text using the current colors */
void
DrawText (char *text, uint32 x, uint32 y)
{
  Font *font;
  int32 plut[] = { 0, 0, 0, 0 };
  CCB ccb;

  MoveTo (&gc, x, y);

  plut[0] = (((int32)gc.gc_FGPen) & 0xffff)
            | (((int32)(gc.gc_BGPen & 0xffff)) << 16);

  font = GetCurrentFont ();
  ccb = *font->font_CCB;
  ccb.ccb_PLUTPtr = plut;
  SetCurrentFontCCB (&ccb);

  DrawText8 (&gc, bm, text);

  ResetCurrentFont ();
}

/*****************************************************************************/

/* Get the pixel width of a string. We currently assume an 8x8 font */
uint32
TextWidth (char *text)
{
  return (8 * strlen (text));
}

/*****************************************************************************/

/* Get the pixel width of a character. We currently assume an 8x8 font */
uint32
CharWidth (char ch)
{
  return (8);
}

/*****************************************************************************/

/* Get the pixel height of a string. We currently assume an 8x8 font */
uint32
TextHeight (char *text)
{
  return (8);
}

/*****************************************************************************/

/* Get the pixel height of a character. We currently assume an 8x8 font */
uint32
CharHeight (char ch)
{
  return (8);
}

/*****************************************************************************/

/* draw a filled box in the current bitmap */
void
DrawBox (int32 x0, int32 y0, int32 x1, int32 y1)
{
  Rect rect;

  rect.rect_XLeft = x0;
  rect.rect_XRight = x1;
  rect.rect_YTop = y0;
  rect.rect_YBottom = y1;

  FillRect (bm, &gc, &rect);
}

/*****************************************************************************/

/* Set a whole bitmap to the specified color */
void
SetBitmap (uint32 color)
{
  uint32 vramPageSize;
  Bitmap *bitmap;

  vramPageSize = GetPageSize (MEMTYPE_VRAM);
  bitmap = (Bitmap *)LookupItem (bm);

  SetVRAMPages (vr, bitmap->bm_Buffer, (color << 16) | color,
                (bitmap->bm_Width * bitmap->bm_Height * 2 + vramPageSize - 1)
                    / vramPageSize,
                0xffffffff);
}
