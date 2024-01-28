/*
        File:		ListUtilities.c

        Contains:	The list handler: utilitarian functions. The list
   handler code is NOT dependent on anything in this file. It is intended as
   programming aids only.

        Written by:	Edward Harp

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <1+>	  2/8/94	RLC		Add
   LHCreateCellFromCel() routine to build list cells from a cel file's cel.
                                11/19/93	EGH		Began
   implementation.

        To Do:
*/

#include "Portfolio.h"

#ifndef EVENT_H
#include "event.h"
#endif

#ifndef __DEBUG3DO_H__
#include "Debug3DO.h"
#endif

#ifndef __Utils3DO_h
#include "Utils3DO.h"
#endif

#ifndef __Parse3DO_h
#include "Parse3DO.h"
#endif

#include "ListUtilities.h"

/* LHCreateTiledCell
 *
 * Create a cell of the passed width & height positioned at the end of the
 * list. If it is the first cell, use the list bounds to define the cell
 * rectangle's position.
 */
Err
LHCreateTiledCell (ListHandler *listP, ListCell **cellP, int32 width,
                   int32 height)
{
  ListCell *lastCell;
  Rect cellRect;

  // calculate a rectangle for the cell
  lastCell = LHGetLastCell (listP);
  if (lastCell != NULL)
    {
      cellRect = lastCell->lcn_Bounds;
      cellRect.rect_YTop += height;
      cellRect.rect_YBottom += height;
    }
  else
    {
      cellRect = listP->lh_Bounds;
      cellRect.rect_YBottom = cellRect.rect_YTop + height;
    }

  return LHCreateCell (listP, cellP, &cellRect);
}

/* LHCreateCellFromCel
 *
 * Create a list cell from the passed cel (using its coords) & positioned at
 * the end of the list.
 *
 */
Err
LHCreateCellFromCel (ListHandler *listP, ListCell **cellP, CCB *ccb)
{
  if (ccb)
    {
      Rect cellRect;

      cellRect.rect_XLeft = ConvertF16_32 (ccb->ccb_XPos);
      cellRect.rect_YTop = ConvertF16_32 (ccb->ccb_YPos);

      cellRect.rect_XRight = cellRect.rect_XLeft + ccb->ccb_Width;
      cellRect.rect_YBottom = cellRect.rect_YTop + ccb->ccb_Height;

      // printf("cellRect is %ld, %ld, %ld, %ld", cellRect.rect_XLeft,
      // cellRect.rect_YTop,cellRect.rect_XRight,cellRect.rect_YBottom);

      return LHCreateCell (listP, cellP, &cellRect);
    }

  return -1;
}

Err
LHSetCellCCBs (ListCell *cellP, CCB *ccb, CCB *hiliteccb)
{
  if (cellP)
    {
      cellP->lcn_CCB = ccb;
      cellP->lcn_HiliteCCB = hiliteccb;

      if (hiliteccb)
        cellP->lcn_CellFlags |= kHiliteByCCB;
    }

  return 0;
}
