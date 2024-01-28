
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: FadeFromBlack.c,v 1.9 1994/11/03 18:19:12 vertex Exp $
**
**  Lib3DO routine to fade the screen up from all-black.  Assumes the CLUTs are
**  are in an all-black state on entry.  (If they're not, you probably
**  won't get the visual effect you're after, but it won't crash.)
**
******************************************************************************/

#include "displayutils.h"

#define STD_FULL_INTENSITY                                                    \
  0x00F8 /* full intensity R,G,B in standard system CLUT entry */

void
FadeFromBlack (ScreenContext *sc, int32 frameCount)
{
  int32 i;
  int32 j;
  int32 k;
  ubyte color;
  int32 index;
  uint32 colorEntries[32];
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

  for (i = 1; i < frameCount; i++)
    {
      WaitVBL (VBLIOReq, 1);
      k = (i * STD_FULL_INTENSITY) / frameCount;
      for (index = 0; index <= 31; index++)
        {
          color = (ubyte)(k * index / 31);
          colorEntries[index]
              = MakeCLUTColorEntry (index, color, color, color);
        }
      for (j = 0; j < sc->sc_NumScreens; j++)
        {
          SetScreenColors (sc->sc_ScreenItems[j], colorEntries, 32);
        }
    }

  /* now use the ResetScreenColors() call to do the last fade-up step. */

  for (j = 0; j < sc->sc_NumScreens; j++)
    {
      ResetScreenColors (sc->sc_ScreenItems[j]);
    }

  DeleteVBLIOReq (VBLIOReq);
}
