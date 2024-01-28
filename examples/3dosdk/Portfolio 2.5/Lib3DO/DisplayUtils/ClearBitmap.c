
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ClearBitmap.c,v 1.5 1994/12/12 18:38:35 vertex Exp $
**
**  Lib3DO routine to clear a screen or bitmap.
**
**  The ioreq parm is optional; if 0 is passed, an IOReq is allocated/deleted
**  internally.  Either a Bitmap* parm or a Bitmap or Screen item can be
**  passed.  For best performance, pass an ioreq (obtained via
**  CreateVRAMIOReq()) and a non-NULL Bitmap* parm.
**
******************************************************************************/

#include "debug3do.h"
#include "displayutils.h"
#include "mem.h"

static int32 cachedPageSize;

Err
ClearBitmap (Item ioreq, Item screen, Bitmap *bm, int32 value)
{
  Err err;
  int32 pageSize;
  int32 numPages;
  Screen *scr;
  bool deleteIO;

  if (cachedPageSize == 0)
    cachedPageSize = GetPageSize (MEMTYPE_VRAM);

  pageSize = cachedPageSize;
  deleteIO = FALSE;
  if (ioreq < 0)
    {
      ioreq = CreateVRAMIOReq ();
      if (ioreq < 0)
        {
          err = ioreq;
          DIAGNOSE_SYSERR (err, ("CreateVRAMIOReq() failed\n"));
          goto ERROR_EXIT;
        }
      deleteIO = TRUE;
    }

  if (bm == NULL)
    {
      if ((scr = (Screen *)LookupItem (screen)) == NULL)
        {
          err = BADITEM;
          DIAGNOSE_SYSERR (err, ("LookupItem(%ld) failed\n", screen));
          goto ERROR_EXIT;
        }

      switch (scr->scr.n_Type)
        {
        case SCREENNODE:
          bm = scr->scr_TempBitmap;
          break;
        case BITMAPNODE:
          bm = (Bitmap *)scr; /* ooops, LookupItem really gave us a Bitmap*,
                                 not a Screen* */
          break;
        default:
          err = BADSUBTYPE;
          DIAGNOSE_SYSERR (
              err, ("Item %08lx is neither screen nor bitmap\n", screen));
          goto ERROR_EXIT;
        }
    }

  numPages = (bm->bm_Width * bm->bm_Height * 2 + pageSize - 1) / pageSize;

  if ((err = SetVRAMPages (ioreq, bm->bm_Buffer, value, numPages, ~0)) < 0)
    {
      DIAGNOSE_SYSERR (err, ("SetVRAMPages() failed\n"));
      goto ERROR_EXIT;
    }

  err = 0;

ERROR_EXIT:

  if (deleteIO)
    DeleteVRAMIOReq (ioreq);

  return err;
}
