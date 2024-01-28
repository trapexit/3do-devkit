
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: CrossFadeCels.c,v 1.4 1994/11/08 06:24:13 ewhac Exp $
**
**  Lib3DO routine to cross-fade between two cels.
**
**  These functions accept either a screen or bitmap item number for drawing
**  the cels.  If zero is passed for the screen item, the calcs are done, but
**  no drawing; this lets the caller do the drawing using the returned CCB ptr.
**
**  The CrossFadeCels8() function fades from the old to the new cel in 8
**  equal steps.  The caller increments the step parameter from 0 thru 7 on
**  successive calls.  Step 0 is 7/8 old cel and 1/8 new cel, step 1 is
**  6/8 old and 2/8 new, and so on up to step 7 which is no old cel and
**  all new cel.
**
**  The CrossFadeCels() function fades from the old to the new cel in 16
**  equal steps.  The usage of the step parameter is the same as above, except
**  that the range is (of course) 0 thru 15.  The logic used gives 16
**  equal steps, with no artifacting caused by the blends adding up to
**  15/16 or 17/16 of the original intensity.
**
**  These functions make certains assumptions about the cels.  If the cels
**  don't match the assumptions your mileage may vary, visually.  Both cels
**  should be single cels, not lists of cels.  Both cels should project to the
**  same screen area.  If the cels contain transparency in differing locations
**  and there are non-black pixels under the transparency area in the bitmap,
**  things are gonna look weird in that area.  The PIXC words in both cels
**  are modified on each call; after the last call (step=7 or step=15) both
**  cels' PIXC words are reset to 0x1F001F00; if this doesn't work for you,
**  save and restore the PIXC words yourself, or clone the CCBs and pass the
**  clones to these functions.
**
**  Other than the limitations/assumptions mentioned above, any type of cels
**  should work just fine (EG, coded/uncoded, packed/not-packed, any flags
**  settings, any PLUT values, any size/perspective mapping, etc).  The two
**  cels can be different types (EG, oldCel is coded6/newCel is uncoded16,
**  oldCel is LRFORM/newCel isn't, etc).
**
**  Each cel is drawn just once; no intermediate pre-scale-the-bitmap drawing
**  is done.  These routines have been tested by crossfading between two
**  fullscreen LRFORM cels at 60hz; 16 fade steps ran in 16 VBLs.
**
**  Right now, both routines are coded so that you can start at any fade
**  level, and skip intermediate levels if you want.  (The last step should
**  NOT be skipped, because it's the one that restores the PIXC words.)
**  I'm not sure this start-anywhere/skip stuff should be supported though;
**  we could buy a little effeciency by requiring all steps to be called.
**  (The caller could pass a zero screen item to do the calcs but skip the
**  drawing if we were to require all steps be called.)
**
******************************************************************************/

#include "celutils.h"
#include "debug3do.h"

#define DUP16(word) ((word) | ((word) << 16))

/*****************************************************************************
 * CrossFadeCels8()
 *	Cross-fade from the old cel to the new cel in 8 equal steps.
 ****************************************************************************/

CCB *
CrossFadeCels8 (Item screen, int32 step, CCB *oldCel, CCB *newCel)
{
  static uint32 pixctable[] = {
    DUP16 (PPMPC_MF_1 | PPMPC_SF_8), /*  1/8 cel */
    DUP16 (PPMPC_MF_2 | PPMPC_SF_8), /*  2/8 cel */
    DUP16 (PPMPC_MF_3 | PPMPC_SF_8), /*  3/8 cel */
    DUP16 (PPMPC_MF_4 | PPMPC_SF_8), /*  4/8 cel */
    DUP16 (PPMPC_MF_5 | PPMPC_SF_8), /*  5/8 cel */
    DUP16 (PPMPC_MF_6 | PPMPC_SF_8), /*  6/8 cel */
    DUP16 (PPMPC_MF_7 | PPMPC_SF_8), /*  7/8 cel */
  };
  Item bitmap;
  Screen *scr;
  CCB *drawCel;

  if (step < 7)
    {
      oldCel->ccb_PIXC = pixctable[6 - step];
      newCel->ccb_PIXC
          = pixctable[step]
            | DUP16 (PPMPC_2S_CFBD); /* OR in PIXC bits to use CFDB as
                                        secondary source */
      oldCel->ccb_Flags &= ~CCB_LAST;
      oldCel->ccb_NextPtr = newCel;
      drawCel = oldCel;
    }
  else
    {
      oldCel->ccb_PIXC = PIXC_OPAQUE;
      newCel->ccb_PIXC = PIXC_OPAQUE;
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
      if (scr->scr.n_Type == SCREENNODE)
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
    DUP16 (PPMPC_MF_1 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  1/16 cel + CFBD */
    DUP16 (PPMPC_MF_2 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  2/16 cel + CFBD */
    DUP16 (PPMPC_MF_3 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  3/16 cel + CFBD */
    DUP16 (PPMPC_MF_4 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  4/16 cel + CFBD */
    DUP16 (PPMPC_MF_5 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  5/16 cel + CFBD */
    DUP16 (PPMPC_MF_6 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  6/16 cel + CFBD */
    DUP16 (PPMPC_MF_7 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  7/16 cel + CFBD */
    DUP16 (PPMPC_MF_8 | PPMPC_SF_16 | PPMPC_2S_CFBD), /*  8/16 cel + CFBD */
    DUP16 (PPMPC_MF_1 | PPMPC_SF_16 | PPMPC_2S_PDC
           | PPMPC_AV_SF2_2), /*  1/16 cel + 1/2 cel =  9/16 cel */
    DUP16 (PPMPC_MF_2 | PPMPC_SF_16 | PPMPC_2S_PDC
           | PPMPC_AV_SF2_2), /*  2/16 cel + 1/2 cel = 10/16 cel */
    DUP16 (PPMPC_MF_3 | PPMPC_SF_16 | PPMPC_2S_PDC
           | PPMPC_AV_SF2_2), /*  3/16 cel + 1/2 cel = 11/16 cel */
    DUP16 (PPMPC_MF_4 | PPMPC_SF_16 | PPMPC_2S_PDC
           | PPMPC_AV_SF2_2), /*  4/16 cel + 1/2 cel = 12/16 cel */
    DUP16 (PPMPC_MF_5 | PPMPC_SF_16 | PPMPC_2S_PDC
           | PPMPC_AV_SF2_2), /*  5/16 cel + 1/2 cel = 13/16 cel */
    DUP16 (PPMPC_MF_6 | PPMPC_SF_16 | PPMPC_2S_PDC
           | PPMPC_AV_SF2_2), /*  6/16 cel + 1/2 cel = 14/16 cel */
    DUP16 (PPMPC_MF_7 | PPMPC_SF_16 | PPMPC_2S_PDC
           | PPMPC_AV_SF2_2), /*  7/16 cel + 1/2 cel = 15/16 cel */
  };
  Item bitmap;
  Screen *scr;
  CCB *drawCel;

  if (step >= 15)
    {
      oldCel->ccb_PIXC = PIXC_OPAQUE; /* end case, restore the pixc  */
      newCel->ccb_PIXC = PIXC_OPAQUE; /* words in both cels to normal */
      newCel->ccb_Flags |= CCB_LAST;
      newCel->ccb_NextPtr = NULL;
      drawCel = newCel;
    }
  else if (step == 7)
    {
      oldCel->ccb_PIXC
          = DUP16 (PPMPC_MF_8 | PPMPC_SF_16); /* 8/16 cel + 0 (special case) */
      newCel->ccb_PIXC = pixctable[step];     /* 8/16 cel + CFDB */
      oldCel->ccb_Flags &= ~CCB_LAST;
      oldCel->ccb_NextPtr = newCel;
      drawCel = oldCel;
    }
  else if (step < 7)
    {
      oldCel->ccb_PIXC
          = pixctable[14 - step]; /* old cel gets high-intensity opaque pixc */
      newCel->ccb_PIXC
          = pixctable[step]; /* new cel gets low-intensity blend pixc */
      oldCel->ccb_Flags &= ~CCB_LAST;
      oldCel->ccb_NextPtr = newCel;
      drawCel = oldCel;
    }
  else
    {
      oldCel->ccb_PIXC
          = pixctable[14 - step]; /* old cel gets low-intensity blend pixc */
      newCel->ccb_PIXC
          = pixctable[step]; /* new cel gets high-intensity opaque pixc */
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
      if (scr->scr.n_Type == SCREENNODE)
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
