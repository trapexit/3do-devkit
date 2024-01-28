
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: CreateSubrectCel.c,v 1.2 1994/10/05 18:01:36 vertex Exp $
**
**  Lib3DO routine to create cel from subrect of another cel's data.
**
**  DeleteCel() compatible.
**
**  BUG: This won't properly subrect an LRFORM source cel.
**
******************************************************************************/

#include "celutils.h"
#include "debug3do.h"
#include "operror.h"

static int32 bppTable[] = { 0, 1, 2, 4, 6, 8, 16, 0 };
static int32 bppShifts[] = { 0, 0, 1, 2, 0, 3, 4, 0 };

/*----------------------------------------------------------------------------
 * CreateSubrectCel()
 *	Create a cel that projects a sub-rectangle of another cel's data
 *buffer. Useful for hither clipping.  This will create a new cel if the dst
 *pointer is NULL, or will use an existing cel if dst is provided.  The idea is
 *to call this once with a NULL dst parm to create a proper-type cel, then make
 *subsequent calls passing in cel created the first time if you want extract
 *new sub-rectangles from the same source cel.
 *--------------------------------------------------------------------------*/

CCB *
CreateSubrectCel (CCB *dst, CCB *src, SRect *subRect)
{
  int32 bpp;
  int32 bppIndex;
  int32 woffset;
  int32 skipX;
  uint32 pre0;
  uint32 pre1;
  char *buffer;
  CCB *cel = NULL;

  /*------------------------------------------------------------------------
   * Create a cel if the caller didn't supply one.
   * If the caller did supply one, it had better be correct in terms of
   * bits per pixel and whatnot, or everything dies horribly.  We don't
   * check the caller's ccb because if the caller is supplying the ccb
   * the supposition is that performance is the goal.
   *----------------------------------------------------------------------*/

  if (dst != NULL)
    {
      cel = dst;
    }
  else
    {
      if ((cel = CloneCel (src, CLONECEL_CCB_ONLY)) == NULL)
        {
          DIAGNOSE_SYSERR (NOMEM, ("Can't create Subrect Cel\n"));
          goto ERROR_EXIT;
        }
      cel->ccb_Flags |= CCB_CCBPRE;
    }

  /*------------------------------------------------------------------------
   * Now do the calcs needed for a subrect extract.
   *----------------------------------------------------------------------*/

  buffer = (char *)src->ccb_SourcePtr;

  if (src->ccb_Flags & CCB_CCBPRE)
    {
      pre0 = src->ccb_PRE0;
      pre1 = src->ccb_PRE1;
    }
  else
    {
      pre0 = ((uint32 *)buffer)[0];
      pre1 = ((uint32 *)buffer)[1];
    }

  bppIndex = pre0 & PRE0_BPP_MASK;
  bpp = bppTable[bppIndex];

  if (bpp < 8)
    {
      woffset = (pre1 & PRE1_WOFFSET8_MASK) >> PRE1_WOFFSET8_SHIFT;
    }
  else
    {
      woffset = (pre1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;
    }

  if (bpp == 6)
    {
      buffer += (subRect->pos.x >> 4) * 24;
      skipX = subRect->pos.x & 0x0F;
    }
  else
    {
      buffer += (subRect->pos.x * bpp) >> 5;
      skipX = (woffset & 0x1F) >> bppShifts[bppIndex];
    }

  buffer
      += subRect->pos.y * (woffset + PRE1_WOFFSET_PREFETCH) * sizeof (int32);

  cel->ccb_PRE0 = (pre0 & ~(PRE0_VCNT_MASK | PRE0_SKIPX_MASK))
                  | ((subRect->size.y - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT)
                  | (skipX << PRE0_SKIPX_SHIFT);

  cel->ccb_PRE1
      = (pre1 & ~(PRE1_TLHPCNT_MASK))
        | ((subRect->size.x - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT);

  cel->ccb_Width = subRect->size.x;
  cel->ccb_Height = subRect->size.y;

  return cel;

ERROR_EXIT:

  if (cel != dst)
    {
      DeleteCel (cel);
    }

  return NULL;
}
