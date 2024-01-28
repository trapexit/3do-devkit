
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: CreateBasicDisplay.c,v 1.8 1994/12/08 03:58:20 vertex Exp $
**
**  Lib3DO routines to open and close basic screen displays.
**
******************************************************************************/

#include "displayutils.h"
#include "graphics.h"
#include "mem.h"
#include "string.h"
#include "tags.h"
#include "types.h"

/*****************************************************************************/

Item
CreateBasicDisplay (ScreenContext *sc, uint32 displayType, uint32 numScreens)
{
  Item result;
  Item group;
  Bitmap *bm;
  Screen *scr;
  uint32 pageSize;
  uint8 i;
  DisplayInfo *di;
  int32 height;
  TagArg *tag;

#ifdef DEBUG
  if (!IsMemWritable (sc, sizeof (ScreenContext)))
    return BADPTR;

  if ((numScreens == 0) || (numScreens > MAXSCREENS))
    return -1;
#else
  if (!sc)
    return BADPTR;
#endif

  memset (sc, 0, sizeof (ScreenContext));

  result = OpenGraphicsFolio ();
  if (result >= 0)
    {
      if (displayType == DI_TYPE_DEFAULT)
        {
          group
              = CreateScreenGroupVA (sc->sc_ScreenItems, CSG_TAG_SPORTBITS,
                                     (MEMTYPE_BANKSELECT | MEMTYPE_BANK1),
                                     CSG_TAG_SCREENCOUNT, numScreens, TAG_END);
          if (group < 0)
            {
              /* if it failed in the first bank, try the second bank of VRAM */
              group = CreateScreenGroupVA (
                  sc->sc_ScreenItems, CSG_TAG_SPORTBITS,
                  (MEMTYPE_BANKSELECT | MEMTYPE_BANK2), CSG_TAG_SCREENCOUNT,
                  numScreens, TAG_END);
            }
        }
      else
        {
          height = 200;

          di = GetFirstDisplayInfo ();
          while (NextNode ((Node *)di))
            {
              tag = FindTagArg (di->di_Tags, DI_TYPE);
              if (tag)
                {
                  if (tag->ta_Arg == (void *)displayType)
                    {
                      tag = FindTagArg (di->di_Tags, DI_HEIGHT);
                      if (tag)
                        height = (int32)tag->ta_Arg;
                      break;
                    }
                }
              di = (DisplayInfo *)NextNode ((Node *)di);
            }

          group = CreateScreenGroupVA (
              sc->sc_ScreenItems, CSG_TAG_DISPLAYTYPE, displayType,
              CSG_TAG_SPORTBITS, (MEMTYPE_BANKSELECT | MEMTYPE_BANK1),
              CSG_TAG_SCREENCOUNT, numScreens, CSG_TAG_SCREENHEIGHT, height,
              CSG_TAG_DISPLAYHEIGHT, height, TAG_END);
          if (group < 0)
            {
              /* if it failed in the first bank, try the second bank of VRAM */
              group = CreateScreenGroupVA (
                  sc->sc_ScreenItems, CSG_TAG_DISPLAYTYPE, displayType,
                  CSG_TAG_SPORTBITS, (MEMTYPE_BANKSELECT | MEMTYPE_BANK2),
                  CSG_TAG_SCREENCOUNT, numScreens, CSG_TAG_SCREENHEIGHT,
                  height, CSG_TAG_DISPLAYHEIGHT, height, TAG_END);
            }
        }

      result = group;
      if (result >= 0)
        {
          result = AddScreenGroup (group, NULL);
          if (result >= 0)
            {
              for (i = 0; i < numScreens; i++)
                {
                  scr = (Screen *)LookupItem (sc->sc_ScreenItems[i]);
                  sc->sc_BitmapItems[i] = scr->scr_TempBitmap->bm.n_Item;
                  sc->sc_Bitmaps[i] = scr->scr_TempBitmap;
                  EnableHAVG (sc->sc_ScreenItems[i]);
                  EnableVAVG (sc->sc_ScreenItems[i]);
                }

              scr = (Screen *)LookupItem (sc->sc_ScreenItems[0]);
              bm = sc->sc_Bitmaps[0];
              pageSize = GetPageSize (GetMemType (bm->bm_Buffer));

              sc->sc_ScreenGroup = group;
              sc->sc_DisplayType = scr->scr_VDLPtr->vdl_DisplayType;
              sc->sc_NumScreens = numScreens;
              sc->sc_CurrentScreen = 0;
              sc->sc_NumBitmapPages
                  = (bm->bm_Width * 2 * bm->bm_Height + pageSize - 1)
                    / pageSize;
              sc->sc_NumBitmapBytes = sc->sc_NumBitmapPages * pageSize;
              sc->sc_BitmapBank = GetBankBits (bm->bm_Buffer);
              sc->sc_BitmapWidth = bm->bm_Width;
              sc->sc_BitmapHeight = bm->bm_Height;

              return group;
            }
          DeleteScreenGroup (group);
        }
      CloseGraphicsFolio ();
    }

  return result;
}

/*****************************************************************************/

Err
DeleteBasicDisplay (ScreenContext *sc)
{
  Err result;

#ifdef DEBUG
  if (!IsMemReadable (sc, sizeof (ScreenContext)))
    return BADPTR;
#else
  if (!sc)
    return BADPTR;
#endif

  RemoveScreenGroup (sc->sc_ScreenGroup);
  result = DeleteScreenGroup (sc->sc_ScreenGroup);
  CloseGraphicsFolio ();

  return result;
}
