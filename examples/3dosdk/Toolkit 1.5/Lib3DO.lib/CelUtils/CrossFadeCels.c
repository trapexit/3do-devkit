/*****************************************************************************
 * File:			CrossFadeCels.c
 *
 * Contains:		Cross-fade between two cels.
 *
 * Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 * History:
 * 04/07/94  Ian 	New today.  (Based on SMCrossFadeTransition() from the
 *					IVDS project.)
 *
 * Implementation notes:
 *
 *	These functions accept either a screen or bitmap item number for
 *drawing the cels.  If zero is passed for the screen item, the calcs are done,
 *but no drawing; this lets the caller do the drawing using the returned CCB
 *ptr.
 *
 *	The CrossFadeCels8() function fades from the old to the new cel in 8
 *	equal steps.  The caller increments the step parameter from 0 thru 7 on
 *	successive calls.  Step 0 is 7/8 old cel and 1/8 new cel, step 1 is
 *	6/8 old and 2/8 new, and so on up to step 7 which is no old cel and
 *	all new cel.
 *
 *	The CrossFadeCels() function fades from the old to the new cel in 16
 *	equal steps.  The usage of the step parameter is the same as above,
 *except that the range is (of course) 0 thru 15.  The logic used gives 16
 *	equal steps, with no artifacting caused by the blends adding up to
 *	15/16 or 17/16 of the original intensity.
 *
 *	These functions make certains assumptions about the cels.  If the cels
 *	don't match the assumptions your mileage may vary, visually.  Both cels
 *	should be single cels, not lists of cels.  Both cels should project to
 *the same screen area.  If the cels contain transparency in differing
 *locations and there are non-black pixels under the transparency area in the
 *bitmap, things are gonna look weird in that area.  The PIXC words in both
 *cels are modified on each call; after the last call (step=7 or step=15) both
 *	cels' PIXC words are reset to 0x1F001F00; if this doesn't work for you,
 *	save and restore the PIXC words yourself, or clone the CCBs and pass
 *the clones to these functions.
 *
 *	Other than the limitations/assumptions mentioned above, any type of
 *cels should work just fine (EG, coded/uncoded, packed/not-packed, any flags
 *	settings, any PLUT values, any size/perspective mapping, etc).  The two
 *	cels can be different types (EG, oldCel is coded6/newCel is uncoded16,
 *	oldCel is LRFORM/newCel isn't, etc).
 *
 *	Each cel is drawn just once; no intermediate pre-scale-the-bitmap
 *drawing is done.  These routines have been tested by crossfading between two
 *	fullscreen LRFORM cels at 60hz; 16 fade steps ran in 16 VBLs.
 *
 *	Right now, both routines are coded so that you can start at any fade
 *	level, and skip intermediate levels if you want.  (The last step should
 *	NOT be skipped, because it's the one that restores the PIXC words.)
 *	I'm not sure this start-anywhere/skip stuff should be supported though;
 *	we could buy a little effeciency by requiring all steps to be called.
 *	(The caller could pass a zero screen item to do the calcs but skip the
 *	drawing if we were to require all steps be called.)
 ****************************************************************************/

#include "CelUtils.h"
#include "Debug3DO.h"

/*****************************************************************************
 * CrossFadeCels8()
 *	Cross-fade from the old cel to the new cel in 8 equal steps.
 ****************************************************************************/

CCB *
CrossFadeCels8 (Item screen, int32 step, CCB *oldCel, CCB *newCel)
{
  static uint32 pixctable[] = {
    0x03000300, //  1/8 cel
    0x07000700, //  2/8 cel
    0x0B000B00, //  3/8 cel
    0x0F000F00, //  4/8 cel
    0x13001300, //  5/8 cel
    0x17001700, //  6/8 cel
    0x1B001B00, //  7/8 cel
  };
  Item bitmap;
  Screen *scr;
  CCB *drawCel;

  if (step < 7)
    {
      oldCel->ccb_PIXC = pixctable[6 - step];
      newCel->ccb_PIXC
          = pixctable[step]
            | 0x00800080; // OR in PIXC bits to use CFDB as secondary source
      oldCel->ccb_Flags &= ~CCB_LAST;
      oldCel->ccb_NextPtr = newCel;
      drawCel = oldCel;
    }
  else
    {
      oldCel->ccb_PIXC = 0x1F001F00;
      newCel->ccb_PIXC = 0x1F001F00;
      oldCel->ccb_Flags |= CCB_LAST;
      oldCel->ccb_NextPtr = NULL;
      drawCel = newCel;
    }

  if (screen > 0)
    {
      if ((scr = (Screen *)LookupItem (screen)) == NULL)
        {
          DIAGNOSE_SYSERR (BADITEM, ("LookupItem(%08lx) failed\n", screen));
          return drawCel;
        }
      if (scr->scr.n_Type == TYPE_SCREEN)
        {
          bitmap = scr->scr_TempBitmap->bm.n_Item;
        }
      else
        {
          bitmap = screen;
        }
      DrawCels (bitmap, drawCel);
    }

  return drawCel;
}

/*****************************************************************************
 * CrossFadeCels()
 *	Cross-fade from the old cel to the new cel in 16 equal steps.
 ****************************************************************************/

CCB *
CrossFadeCels (Item screen, int32 step, CCB *oldCel, CCB *newCel)
{
  static uint32 pixctable[] = {
    0x00800080, //  1/16 cel + CFDB
    0x04800480, //  2/16 cel + CFDB
    0x08800880, //  3/16 cel + CFDB
    0x0C800C80, //  4/16 cel + CFDB
    0x10801080, //  5/16 cel + CFDB
    0x14801480, //  6/16 cel + CFDB
    0x18801880, //  7/16 cel + CFDB
    0x1C801C80, //  8/16 cel + CFDB
    0x00D000D0, //  1/16 cel + 1/2 cel =  9/16 cel
    0x04D004D0, //  2/16 cel + 1/2 cel = 10/16 cel
    0x08D008D0, //  3/16 cel + 1/2 cel = 11/16 cel
    0x0CD00CD0, //  4/16 cel + 1/2 cel = 12/16 cel
    0x10D010D0, //  5/16 cel + 1/2 cel = 13/16 cel
    0x14D014D0, //  6/16 cel + 1/2 cel = 14/16 cel
    0x18D018D0  //  7/16 cel + 1/2 cel = 15/16 cel
  };
  Item bitmap;
  Screen *scr;
  CCB *drawCel;

  if (step >= 15)
    {
      oldCel->ccb_PIXC = 0x1F001F00; // end case, restore the pixc
      newCel->ccb_PIXC = 0x1F001F00; // words in both cels to normal
      newCel->ccb_Flags |= CCB_LAST;
      newCel->ccb_NextPtr = NULL;
      drawCel = newCel;
    }
  else if (step == 7)
    {
      oldCel->ccb_PIXC = 0x1C001C00;      // 8/16 cel + 0 (special case)
      newCel->ccb_PIXC = pixctable[step]; // 8/16 cel + CFDB
      oldCel->ccb_Flags &= ~CCB_LAST;
      oldCel->ccb_NextPtr = newCel;
      drawCel = oldCel;
    }
  else if (step < 7)
    {
      oldCel->ccb_PIXC
          = pixctable[14 - step]; // old cel gets high-intensity opaque pixc
      newCel->ccb_PIXC
          = pixctable[step]; // new cel gets low-intensity blend pixc
      oldCel->ccb_Flags &= ~CCB_LAST;
      oldCel->ccb_NextPtr = newCel;
      drawCel = oldCel;
    }
  else
    {
      oldCel->ccb_PIXC
          = pixctable[14 - step]; // old cel gets low-intensity blend pixc
      newCel->ccb_PIXC
          = pixctable[step]; // new cel gets high-intensity opaque pixc
      newCel->ccb_Flags &= ~CCB_LAST;
      newCel->ccb_NextPtr = oldCel;
      oldCel->ccb_Flags |= CCB_LAST;
      oldCel->ccb_NextPtr = NULL;
      drawCel = newCel;
    }

  if (screen > 0)
    {
      if ((scr = (Screen *)LookupItem (screen)) == NULL)
        {
          DIAGNOSE_SYSERR (BADITEM, ("LookupItem(%08lx) failed\n", screen));
          return drawCel;
        }
      if (scr->scr.n_Type == TYPE_SCREEN)
        {
          bitmap = scr->scr_TempBitmap->bm.n_Item;
        }
      else
        {
          bitmap = screen;
        }
      DrawCels (bitmap, drawCel);
    }

  return drawCel;
}
