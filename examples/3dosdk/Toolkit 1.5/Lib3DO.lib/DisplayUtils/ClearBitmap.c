/*****************************************************************************
 * File:			ClearBitmap.c
 *
 * Contains:		Clears a screen or bitmap.
 *
 * Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 * History:
 *	07/25/94  Ian	Renamed from ClearScreen() to ClearBitmap().
 *					Modified to add an optional Bitmap* parm.  If
 *performance is an issue when using this function, passing a bitmap ptr
 *					instead of an item number will save a bunch of
 *cycles. Removed GrafBase references. 04/06/94  Ian 	New today.
 *
 * Implementation notes:
 *
 *	The ioreq parm is optional; if 0 is passed, an IOReq is
 *allocated/deleted internally.  Either a Bitmap* parm or a Bitmap or Screen
 *item can be passed.  For best performance, pass an ioreq (obtained via
 *GetVRAMIOReq()) and a non-NULL Bitmap* parm.
 ****************************************************************************/

#include "Debug3DO.h"
#include "DisplayUtils.h"
#include "mem.h"

static int32 page_size;

Err
ClearBitmap (Item ioreq, Item screen, Bitmap *bm, int32 value)
{
  Err err;
  int32 pageSize;
  int32 numPages;
  Screen *scr;
  Item theIOReq;

  if ((pageSize = page_size) == 0)
    {
      pageSize = page_size = GetPageSize (MEMTYPE_VRAM);
    }

  if ((theIOReq = ioreq) <= 0)
    {
      if ((theIOReq = GetVRAMIOReq ()) < 0)
        {
          err = theIOReq;
          DIAGNOSE_SYSERR (err, ("GetVRAMIOReq() failed\n"));
          goto ERROR_EXIT;
        }
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
        case TYPE_SCREEN:
          bm = scr->scr_TempBitmap;
          break;
        case TYPE_BITMAP:
          bm = (Bitmap *)
              scr; // ooops, LookupItem really gave us a Bitmap*, not a Screen*
          break;
        default:
          err = BADSUBTYPE;
          DIAGNOSE_SYSERR (
              err, ("Item %08lx is neither screen nor bitmap\n", screen));
          goto ERROR_EXIT;
        }
    }

  numPages = (bm->bm_Width * bm->bm_Height * 2 + pageSize - 1) / pageSize;

  if ((err
       = SetVRAMPages (theIOReq, bm->bm_Buffer, value, numPages, 0xFFFFFFFF))
      < 0)
    {
      DIAGNOSE_SYSERR (err, ("SetVRAMPages() failed\n"));
    }

  err = 0;

ERROR_EXIT:

  if (ioreq > 0 && ioreq != theIOReq)
    {
      DeleteItem (theIOReq);
    }

  return err;
}
