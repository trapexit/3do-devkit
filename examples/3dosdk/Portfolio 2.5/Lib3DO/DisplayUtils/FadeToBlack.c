
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: FadeToBlack.c,v 1.9 1994/11/03 18:19:12 vertex Exp $
**
**  Lib3DO routine to fade the screen down to all-black.  Assumes the CLUTs are
**  are in standard system full-intensity state to start with.  (If they're
**  not, you probably won't get exactly the visual effect you're after, but
**  it won't crash.  If you had the CLUT entries at greater than standard full
**  intensity (> 0xF8) everything will be fine.  If the CLUTs were at less
**  than full intensity, the screen will just get brighter for a vbl tick
**  or two at the start of the fadeout.  We could fix this to fade from the
**  current values to black if only the ReadScreenColor() (aka ReadCLUTColor())
**  OS routine worked correctly.)
**
******************************************************************************/

#include "displayutils.h"

#define STD_FULL_INTENSITY                                                    \
  0x00F8 /* full intensity R,G,B in standard system CLUT entry */

static void
do_fade_step (ScreenContext *sc, int32 k)
{
  int32 j;
  ubyte color;
  int32 index;
  uint32 colorEntries[32];

  for (index = 0; index <= 31; index++)
    {
      color = (ubyte)((k * index) / 31);
      colorEntries[index] = MakeCLUTColorEntry (index, color, color, color);
    }

  for (j = 0; j < sc->sc_NumScreens; j++)
    {
      SetScreenColors (sc->sc_ScreenItems[j], colorEntries, 32);
    }
}

void
FadeToBlack (ScreenContext *sc, int32 frameCount)
{
  int32 i;
  int32 j;
  int32 k;
  Item VBLIOReq;

  VBLIOReq = CreateVBLIOReq ();

  /* first blast the special background register to zeroes.  this only */
  /* needs to be done once (assuming it needs to be done at all, something */
  /* I'm not really sure about, but the old code did it.) */

  for (j = 0; j < sc->sc_NumScreens; j++)
    {
      SetScreenColor (sc->sc_ScreenItems[j], MakeCLUTColorEntry (32, 0, 0, 0));
    }

  /* now run the fade from the first step thru the last fade step */
  /* before full color. */

  for (i = frameCount - 1; i > 0; i--)
    {
      WaitVBL (VBLIOReq, 1);
      k = (i * STD_FULL_INTENSITY) / frameCount;
      do_fade_step (sc, k);
    }

  /* now do the last step which zeroes all the CLUTs.  this is done outside */
  /* the loop in case the frame count is less than 2 (causing us to skip the */
  /* loop altogether) */

  do_fade_step (sc, 0);

  DeleteVBLIOReq (VBLIOReq);
}
