/*
        File:			OpenGraphicsNTSCPAL.c

        Contains:	A new version of the Lib3DO call OpenGraphics()
                                        that returns a screen context that
   matches the current video display mode.

        NOTE:			REQUIRES Portfolio 1.2 or later.

        Written by:	Kevin S. MacDonell

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                        This material constitutes confidential
   and proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):


        To Do:
*/

#include "debug3DO.h"
#include "graphics.h"
#include "init3DO.h"
#include "mem.h"
#include "types.h"

#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define NTSC_FREQUENCY 60
#define NTSC_TYPE DI_TYPE_NTSC
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define PAL_FREQUENCY 50
#define PAL_TYPE DI_TYPE_PAL2

// MOVE THIS TO STANDARD INTERFACE FILE
bool OpenGraphicsNTSCPAL (ScreenContext *sc, uint32 nScreens);

/*
        Purpose:	Create a screen context record with the number of
                                associated screens specified that matches the
   current video mode of the target machine (i.e., NTSC or PAL). Opens the
   Graphics Folio before doing anything...
        NOTE:		A 320x240 by 16-bit deep screen is 153,600 bytes.
                                A 388x288 by 16-bit deep screen is 211,184
   bytes. Thus the standard 2-screen usage will EITHER allocate buffers of 300K
   (NTSC) or 432K (PAL) (the overhead for either is the same).  Be sure you
   have enough memory! Inputs:	sc - A pointer to a screen context record (must
   be allocated). nScreens - number of screens to allocate (1-6) Outputs:
   OpenGraphicsNTSCPAL - returns TRUE if the screen context record was
   allocated.  FALSE if nScreens is out-of-range or the screen group could not
   be added.
*/
bool
OpenGraphicsNTSCPAL (ScreenContext *sc, uint32 nScreens)
{

  int32 width, height, diType, i;
  Item item;
  uint32 bmw[MAXSCREENS];
  uint32 bmh[MAXSCREENS];
  bool rslt = FALSE;
  TagArg taScreenTags[] = {
    /* Don't change the order of these without changing assumtion below */
    CSG_TAG_DISPLAYHEIGHT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SCREENCOUNT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SCREENHEIGHT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_BITMAPCOUNT,
    (void *)1,
    CSG_TAG_BITMAPWIDTH_ARRAY,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_BITMAPHEIGHT_ARRAY,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SPORTBITS,
    (void *)(MEMTYPE_VRAM | MEMTYPE_STARTPAGE),
    CSG_TAG_DISPLAYTYPE,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_DONE,
    (void *)0,
  };

  // Check for bad arguments
  if ((nScreens > MAXSCREENS) || (nScreens < 1))
    goto EXIT_MyOpenGraphics;

  // Open the graphics folio
  i = OpenGraphicsFolio ();
  if (i < 0)
    {
      DIAGNOSE (("Cannot open graphics folio (Error = %ld)\n", i));
      goto EXIT_MyOpenGraphics;
    }

  // Determine NTSC/PAL operating environment
  width = NTSC_WIDTH; // Assume NTSC
  height = NTSC_HEIGHT;
  diType = NTSC_TYPE;
  if (GrafBase->gf_VBLFreq == PAL_FREQUENCY)
    {
      width = PAL_WIDTH;
      height = PAL_HEIGHT;
      diType = PAL_TYPE;
    }
  else if (GrafBase->gf_VBLFreq != NTSC_FREQUENCY)
    {
      DIAGNOSE (("Unknown screen refresh rate %ld\n", GrafBase->gf_VBLFreq));
      goto EXIT_MyOpenGraphics;
    }

  // Set up the width and height arrays now that we know NTSC/PAL
  for (i = 0; i < nScreens; ++i)
    {
      bmw[i] = width;
      bmh[i] = height;
    }

  // Now create the screen group, add it to the display mechanism
  sc->sc_nScreens = (int32)nScreens;
  sc->sc_curScreen = (int32)0;
  i = GrafBase->gf_VRAMPageSize;
  sc->sc_nFrameBufferPages = (width * height * 2 + (i - 1)) / i;
  sc->sc_nFrameByteCount = sc->sc_nFrameBufferPages * i;

  taScreenTags[0].ta_Arg = (void *)height;          // CSG_TAG_DISPLAYHEIGHT
  taScreenTags[1].ta_Arg = (void *)sc->sc_nScreens; // CSG_TAG_SCREENCOUNT
  taScreenTags[2].ta_Arg = (void *)height;          // CSG_TAG_SCREENHEIGHT
  taScreenTags[4].ta_Arg = (void *)bmw;    // CSG_TAG_BITMAPWIDTH_ARRAY
  taScreenTags[5].ta_Arg = (void *)bmh;    // CSG_TAG_BITMAPHEIGHT_ARRAY
  taScreenTags[7].ta_Arg = (void *)diType; // CSG_TAG_DISPLAYTYPE

  item = CreateScreenGroup (&(sc->sc_Screens[0]), taScreenTags);
  if (item < 0)
    {
#if DEBUG
      PrintfSysErr (item);
#endif
      DIAGNOSE (("Cannot create screen group\n"));
      return FALSE;
    }
  i = AddScreenGroup (item, NULL);
  if (item < 0)
    {
#if DEBUG
      PrintfSysErr ((Item)i);
#endif
      DIAGNOSE (("Cannot add screen group\n"));
      return FALSE;
    }

  // Get the information about the screen group and update the ScreenContext
  // structure
  for (i = 0; i < sc->sc_nScreens; i++)
    {
      Screen *screen;
      screen = (Screen *)LookupItem (sc->sc_Screens[i]);

      if (screen == NULL)
        {
          DIAGNOSE (("Cannot locate screens\n"));
          return FALSE;
        }
      sc->sc_BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
      sc->sc_Bitmaps[i] = screen->scr_TempBitmap;
      EnableHAVG (sc->sc_Screens[i]);
      EnableVAVG (sc->sc_Screens[i]);
    }

  rslt = TRUE;

EXIT_MyOpenGraphics:
  return (rslt);
}
