/*
        File:		ListHandler.c

        Contains:	The list handler: selectable things.

        Written by:	Randy Carr, Edward Harp

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 1/24/94	EGH		Added
   justification of text in cells. Should there be separate formatting for text
   & CCBs? 11/12/93	EGH		Began implementation. 10/20/93	RLC
   File created.

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

#ifndef UMEMORY_H
#include "UMemory.h"
#endif

#include "ListHandler.h"

/* LHCreateListHandler
 *
 * Create a new list handler.
 */
Err
LHCreateListHandler (
    const Rect
        *listBounds, // (in) screen limits of the list (NULL == entire screen)
    int32 listFlags, // (in)	flags controlling
    int32 selectionFlags,   // (in) flags controlling selection
    int32 defaultCellFlags, // (in) flags used when new cells are created
    ListHandler **listP)    // (out) pointer to the new list handler (or NULL)
{
  Err err = 0;
  ListHandler *tempListP;

  *listP = NULL;
  tempListP
      = (ListHandlerPtr)Malloc (sizeof (ListHandler), kDRAMAndClearMemory);

  if (tempListP)
    {
      // initialize the list handler record
      // note that fields set to 0 are commented out because cleared memory is
      // allocated in the first place
      tempListP->lh_Version = kCurrentListHandlerVersion;
      // tempListP->lh_ListAnchor = NULL;
      //  lh_CellCount = 0;
      // tempListP->lh_CurrentCell = NULL;
      // tempListP->lh_CurrentIndex = 0;
      if (listBounds)
        tempListP->lh_Bounds = *listBounds;
      tempListP->lh_ListFlags = listFlags;
      tempListP->lh_SelectionFlags = selectionFlags;
      // tempListP->lh_NextCellProc = NULL;
      tempListP->lh_DefaultCellFlags = defaultCellFlags;
      // tempListP->lh_DrawProc = NULL;
      tempListP->lh_HiliteColor = MakeRGB15 (31, 0, 0); // red

      *listP = tempListP;
    }
  else
    err = kLHOutOfMemError;

  return err;
}

/* LHDisposeListHandler
 *
 * Dispose of a list handler.
 */
Err
LHDisposeListHandler (ListHandler *listP) // (in) list handler to dispose
{
  Err err = 0;

  DEBUG_CHECKLISTPARAM (listP, "LHDisposeListHandler");

  Free ((void *)listP);

  return err;
}

#if 1
/* LHPadEvent
 *
 * Process an event. Determine if any directional buttons are down and select
 * the next cell.
 */
bool
LHPadEvent (
    ListHandler *listP,
    const ControlPadEventData *event) // (in) control pad event to process
{
  bool handled = FALSE;
  int32 direction = 0;

  if (event->cped_ButtonBits & ControlDown)
    direction |= kDown;
  if (event->cped_ButtonBits & ControlUp)
    direction |= kUp;
  if (event->cped_ButtonBits & ControlLeft)
    direction |= kLeft;
  if (event->cped_ButtonBits & ControlRight)
    direction |= kRight;

  if (direction)
    {
      LHSelectNextCellByDirection (listP, direction);
      handled = TRUE;
    }

  return handled;
}
#endif

/* LHDraw
 *
 * Draw the list's cells into the passed bit map.
 * NOTE: this is the only place where drawing actually occurs.
 */
Err
LHDraw (ListHandler *listP,
        Item bitMapItem) // (in) bit map to draw in
{
  Err err = 0;
  ListCell *cellP;
  bool drawCell = TRUE;

  DEBUG_CHECKLISTPARAM (listP, "LHDraw");

  cellP = NULL;

  while (LHGetNextCell (listP, &cellP))
    {
      if (listP->lh_ListFlags & kMapCCBsAtDrawTime)
        { // map CCBs to the cells now
          if (cellP->lcn_CCB)
            LHMapCCBToCell (cellP->lcn_CCB, cellP, &cellP->lcn_CellFormat,
                            (cellP->lcn_CellFlags & kScaleCellCCBs) != 0);

          if (cellP->lcn_HiliteCCB)
            LHMapCCBToCell (cellP->lcn_HiliteCCB, cellP,
                            &cellP->lcn_CellFormat,
                            (cellP->lcn_CellFlags & kScaleCellCCBs) != 0);
        }

      if (listP->lh_ListFlags & kClipCellsToBounds)
        { // check to see if the cell falls within list bounds
          Rect listRect = listP->lh_Bounds, cellRect = cellP->lcn_Bounds;

          drawCell = LHSectRect (&listRect, &cellRect, &listRect);
        }

      if (drawCell)
        {
          if (listP->lh_DrawProc)
            err = listP->lh_DrawProc (listP, cellP, bitMapItem);
          else
            err = LHStandardDrawProc (listP, cellP, bitMapItem);
        }
    }

  return err;
}

/* LHSetDrawProc
 */
void
LHSetDrawProc (ListHandler *listP, CellDrawProc drawProc)
{
  listP->lh_DrawProc = drawProc;
}

/* LHStandardDrawProc
 *
 * Draw a list element.
 */
Err
LHStandardDrawProc (ListHandler *listP, ListCell *cellP, Item bitMapItem)
{
  bool hilited = (cellP->lcn_CellFlags & kCellHilite) != 0;

  DEBUG_CHECKLISTPARAM (listP, "LHStandardDrawProc");

  // draw the cell's cel(s) if they exist
  if (hilited && (cellP->lcn_CellFlags & kHiliteByCCB))
    if (cellP->lcn_HiliteCCB)
      DrawCels (bitMapItem, cellP->lcn_HiliteCCB);
  if (cellP->lcn_CCB)
    DrawCels (bitMapItem, cellP->lcn_CCB);

  // draw hilite color if necessary
  if (hilited && cellP->lcn_CellFlags & kHiliteByColor)
    {
      GrafCon fillgc;

      fillgc.gc_FGPen = listP->lh_HiliteColor;
      FillRect (bitMapItem, &fillgc, &cellP->lcn_Bounds);
    }

  // draw the cell's text if it exists
  if (cellP->lcn_Text)
    {
      bool isOldFont;

      isOldFont = (cellP->lcn_CellFlags & kFont3DO) == 0;

      if (isOldFont)
        {
          MoveTo (&cellP->lcn_GCon, cellP->lcn_Bounds.rect_XLeft,
                  cellP->lcn_Bounds.rect_YTop);
          DrawText8 (&cellP->lcn_GCon, bitMapItem, (uint8 *)cellP->lcn_Text);
        }
      else
        {
          int32 textX, textY, cellHCentre, cellVCentre, textWidth, textHeight;

          textWidth = LHStringWidth ((char *)cellP->lcn_Text,
                                     cellP->lcn_FontDesc.lcn_Font3DO);
          textHeight = cellP->lcn_FontDesc.lcn_Font3DO->fd_ascent;
          cellHCentre = cellP->lcn_Bounds.rect_XLeft
                        + ((cellP->lcn_Bounds.rect_XRight
                            - cellP->lcn_Bounds.rect_XLeft)
                           / 2);
          cellVCentre = cellP->lcn_Bounds.rect_YTop
                        + ((cellP->lcn_Bounds.rect_YBottom
                            - cellP->lcn_Bounds.rect_YTop)
                           / 2);

          switch (cellP->lcn_CellFormat.hJustify)
            {
            default:
            case kCellJustLeft:
              // textX = cellP->lcn_Bounds.rect_XLeft;

              textX = cellP->lcn_Bounds.rect_XLeft + 2;
              break;

            case kCellJustCenter:
              textX = cellHCentre - (textWidth / 2);
              break;

            case kCellJustRight:
              textX = cellP->lcn_Bounds.rect_XRight - textWidth;
              break;
            }

          switch (cellP->lcn_CellFormat.vJustify)
            {
            default:
            case kCellJustLeft:
              textY = cellP->lcn_Bounds.rect_YTop;
              break;

            case kCellJustCenter:
              textY = cellVCentre - (textHeight / 2);
              break;

            case kCellJustRight:
              textY = cellP->lcn_Bounds.rect_YBottom - textHeight;
              break;
            }

          MoveTo (&cellP->lcn_GCon, textX, textY);

          DrawTextString (cellP->lcn_FontDesc.lcn_Font3DO, &cellP->lcn_GCon,
                          bitMapItem, (char *)cellP->lcn_Text);
        }
    }

  // check for cell frame hilite, draw if necessary
  if (hilited && (cellP->lcn_CellFlags & kHiliteByFrame))
    {
      LHFrameRect (bitMapItem, &cellP->lcn_Bounds,
                   cellP->lcn_CellFlags & kHiliteFrameMask,
                   listP->lh_HiliteColor);
    }

  return 0;
}

/* LHOffsetList
 *
 * Offset the entire list contents by the specified amount. This does not
 * offset the list bounds, but rather the bounds of each individual cell.
 */
Err
LHOffsetList (ListHandler *listP, int32 hOffset, int32 vOffset, bool reMapCCBs)
{
  ListCell *cellP = NULL;

  DEBUG_CHECKLISTPARAM (listP, "LHOffsetList");

  while (LHGetNextCell (listP, &cellP))
    {
      LHOffsetRect (&cellP->lcn_Bounds, hOffset, vOffset);

      if (reMapCCBs)
        {
          if (cellP->lcn_CCB)
            LHMapCCBToCell (cellP->lcn_CCB, cellP, &cellP->lcn_CellFormat,
                            (cellP->lcn_CellFlags & kScaleCellCCBs) != 0);

          if (cellP->lcn_HiliteCCB)
            LHMapCCBToCell (cellP->lcn_HiliteCCB, cellP,
                            &cellP->lcn_CellFormat,
                            (cellP->lcn_CellFlags & kScaleCellCCBs) != 0);
        }
    }

  return 0; // no error possible at this time
}

/* LHDisposeAllCells
 *
 * Dispose of all cell structures. Also removes all cell links in the list.
 */
Err
LHDisposeAllCells (ListHandler *listP, Boolean disposeCCBs)
{
  ListCell *cellP = listP->lh_ListAnchor, *nextCellP;

  DEBUG_CHECKLISTPARAM (listP, "LHDisposeAllCells");

  while (cellP != NULL)
    {
      // get the next oneÉ
      nextCellP = cellP;
      LHGetNextCell (listP, &nextCellP);

      // Ébefore disposing of the current
      (void)LHDisposeCell (cellP, disposeCCBs);

      cellP = nextCellP;
    }

  // make sure the list knows of no cells
  listP->lh_ListAnchor = NULL;
  listP->lh_CellCount = 0;

  return 0; // no error possible at this time
}

int32
LHCountCells (ListHandler *listP)
{
  return listP->lh_CellCount;
}

/* LHSetListNextCellProc
 */
void
LHSetListNextCellProc (ListHandler *listP, NextCellProc proc)
{
  listP->lh_NextCellProc = proc;
}

/* LHCreateCell
 *
 */
Err
LHCreateCell (
    ListHandler *listP, // (in) optional list as source of default flags
    ListCell **cellP,   // (out) the created cell or NULL
    Rect *cellRect)     // (in) rectangle for the cell
{
  Err err = 0;
  ListCell *tempCellP;

  *cellP = NULL;
  tempCellP = (ListCellPtr)Malloc (sizeof (ListCell), kDRAMAndClearMemory);

  if (tempCellP)
    {
      // tempCellP->lcn_Next = NULL;
      // tempCellP->lcn_Prev = NULL;
      tempCellP->lcn_Version = kCurrentListHandlerVersion;
      if (listP)
        tempCellP->lcn_CellFlags = listP->lh_DefaultCellFlags;
      // else
      //	tempCellP->lcn_CellFlags = 0;
      // tempCellP->lcn_Index = 0;
      // tempCellP->lcn_Owner = NULL;
      tempCellP->lcn_Bounds = *cellRect;
      // tempCellP->lcn_Text = NULL;
      // tempCellP->lcn_CCB = NULL;
      tempCellP->lcn_CellFormat.hJustify = kCellJustLeft;
      tempCellP->lcn_CellFormat.vJustify = kCellJustLeft;
      // tempCellP->lcn_CellFormat.hOffset = 0;
      // tempCellP->lcn_CellFormat.vOffset = 0;
      // tempCellP->lcn_HiliteCCB = NULL;
      // tempCellP->lcn_ReservedPtr = NULL;
      // tempCellP->lcn_UserValue = 0;

      *cellP = tempCellP;
    }
  else
    err = kLHOutOfMemError;

  return err;
}

/* LHDisposeCell
 *
 */
Err
LHDisposeCell (ListCell *cellP, bool disposeCCBs)
{
  Err err = 0;

  // dispose of CCBs if necessary
  if (disposeCCBs)
    {
    }

  Free ((void *)cellP);

  return err;
}

/* LHAddCell
 *
 * Add a cell to a list. Maintain cell indicies.
 * NOTE: Assumes that cell is initialized per LHCreateCell().
 */
void
LHAddCell (ListHandler *listP, ListCell *insertBeforeP, ListCell *cellP)
{
  if (listP->lh_ListAnchor == NULL)
    { // first and only one

      listP->lh_ListAnchor = cellP;
      cellP->lcn_Prev = cellP;
      cellP->lcn_Next = NULL;
      cellP->lcn_CellFlags |= kIsLastCell;
      cellP->lcn_Index = 0;

      if (listP->lh_SelectionFlags & kAlwaysSelectOne)
        { // there must be one selected
          LHSelectCell (listP, cellP, TRUE);
        }
    }
  else if (insertBeforeP != NULL)
    { // inserted

      int32 index;
      ListCell *indCellP;

      // insert the cell
      cellP->lcn_Prev = insertBeforeP->lcn_Prev;
      insertBeforeP->lcn_Prev = cellP;
      if (insertBeforeP->lcn_Prev)
        insertBeforeP->lcn_Prev->lcn_Next = cellP;
      cellP->lcn_Next = insertBeforeP;
      if (insertBeforeP == listP->lh_ListAnchor) // becomes the first
        listP->lh_ListAnchor = cellP;

      // make sure that the end of list flag is not set
      cellP->lcn_CellFlags &= ~kIsLastCell;

      // fix the indiciesÉ
      indCellP = cellP; // Éstarting here
      cellP->lcn_Index = index = insertBeforeP->lcn_Index;
      while (LHGetNextCell (listP, &indCellP))
        indCellP->lcn_Index = ++index;
    }
  else
    { // added to the end

      ListCell *lastCellP;

      lastCellP = LHGetLastCell (listP);
      if (lastCellP != NULL) // <-- this shouldn't be necessary, butÉ
        {
          lastCellP->lcn_Next = cellP;
          lastCellP->lcn_CellFlags &= ~kIsLastCell;

          cellP->lcn_Prev = lastCellP;
          cellP->lcn_Next = NULL;
          cellP->lcn_Index = lastCellP->lcn_Index + 1;
          cellP->lcn_CellFlags |= kIsLastCell;
        }

      // make the anchor's previous the new end
      // this makes it easy to find the end of the list
      listP->lh_ListAnchor->lcn_Prev = cellP;
    }

  cellP->lcn_Owner = listP;
  listP->lh_CellCount++;
}

/* LHDeleteCell
 */
void
LHDeleteCell (ListHandler *listP, ListCell *cellP)
{
  if (cellP == listP->lh_ListAnchor)
    {
      // if it is the first cell
      // move the anchor
      // cleanup the new anchor's previous

      listP->lh_ListAnchor = cellP->lcn_Next;

      if (cellP->lcn_Next != NULL)
        cellP->lcn_Next->lcn_Prev = cellP->lcn_Prev;
    }
  else if (cellP->lcn_Next == NULL)
    {
      // it is the last item

      // set the previous cell's next pointer to nil
      // set the anchor's previous ptr to the cell's previous ptr
      // set the kIsLastCell bit on the new last cell

      if (cellP->lcn_Prev != NULL)
        cellP->lcn_Prev->lcn_Next = NULL;

      listP->lh_ListAnchor->lcn_Prev = cellP->lcn_Prev;

      cellP->lcn_Prev->lcn_CellFlags |= kIsLastCell;
    }
  else // this is just a cell
    {
      if (cellP->lcn_Prev != NULL)
        cellP->lcn_Prev->lcn_Next = cellP->lcn_Next;

      if (cellP->lcn_Next != NULL)
        cellP->lcn_Next->lcn_Prev = cellP->lcn_Prev;
    }

  // if they delete the selection, clear the selection

  if (cellP == listP->lh_CurrentCell)
    {
      listP->lh_CurrentCell = NULL;
      listP->lh_CurrentIndex = 0;
    }

  listP->lh_CellCount--;
}

/* LHSelectCell
 *
 * Set the selection of the cell.
 */
void
LHSelectCell (ListHandler *listP, ListCell *cellP, bool select)
{
  // unselect old current if necessary
  if ((listP->lh_SelectionFlags & kSelectOneOnly) && select
      && listP->lh_CurrentCell)
    {
      listP->lh_CurrentCell->lcn_CellFlags &= ~kCellHilite;
    }

  // set the cell's hilite bit
  if (select)
    cellP->lcn_CellFlags |= kCellHilite;
  else
    cellP->lcn_CellFlags &= ~kCellHilite;

  // set the list's current cell
  listP->lh_CurrentCell = cellP;
  listP->lh_CurrentIndex = cellP->lcn_Index;
}

/* LHSelectIndexCell
 *
 * Set the selection of the indexed cell.
 */
void
LHSelectIndexCell (ListHandler *listP, int32 cellIndex, bool select)
{
  LHSelectCell (listP, LHGetIndexCell (listP, cellIndex), select);
}

/* LHSelectNextCell
 *
 * Select the next cell in the list based on the CurrentCell field.
 */
void
LHSelectNextCell (ListHandler *listP)
{
  ListCell *nextCellP = NULL;

  // find the cell after the current one, wrapping if necessary
  if (listP->lh_CurrentCell)
    {
      nextCellP = listP->lh_CurrentCell;

      if (!LHGetNextCell (listP, &nextCellP)
          && !(listP->lh_SelectionFlags & kNoWrapCellSelection))
        nextCellP = listP->lh_ListAnchor;
    }
  else
    // no current cell, so try the first one
    nextCellP = listP->lh_ListAnchor;

  if (nextCellP)
    LHSelectCell (listP, nextCellP, TRUE);
}

/* LHSelectPreviousCell
 *
 * Select the previous cell in the list based on the CurrentCell field.
 */
void
LHSelectPreviousCell (ListHandler *listP)
{
  ListCell *prevCellP = NULL;

  // find the cell after the current one, wrapping if necessary
  if (listP->lh_CurrentCell)
    {
      prevCellP = listP->lh_CurrentCell;

      if (!LHGetPreviousCell (listP, &prevCellP)
          && !(listP->lh_SelectionFlags & kNoWrapCellSelection))
        prevCellP = LHGetLastCell (listP);
    }
  else
    // no current cell, so try the first one
    prevCellP = listP->lh_ListAnchor;

  if (prevCellP)
    LHSelectCell (listP, prevCellP, TRUE);
}

/* LHSelectNextCellByDirection
 *
 * Select the next cell in the list based on the direction. Use the
 * direction flags (kUp, etc.) |'d together.
 */
void
LHSelectNextCellByDirection (ListHandler *listP, int32 direction)
{
  if (listP->lh_NextCellProc)
    listP->lh_NextCellProc (listP, direction);
  else
    LHStandardNextCellProc (listP, direction);
}

/* LHStandardNextCellProc
 *
 * A NextCellProc called if an optional one is not specified.
 */
void
LHStandardNextCellProc (ListHandler *listP, int32 direction)
{
  if ((direction & kUp) || (direction & kLeft))
    LHSelectPreviousCell (listP);
  else if ((direction & kRight) || (direction & kDown))
    LHSelectNextCell (listP);

  if ((listP->lh_ListFlags & kAutoScrollV)
      || (listP->lh_ListFlags & kAutoScrollH))
    {
      int32 scrollH = 0, scrollV = 0;
      ListCell *cellP;

      cellP = LHGetSelectedCell (listP);

      if (listP->lh_ListFlags & kAutoScrollV)
        { // verticle scroll

          // see if the cell is visible
          if (cellP->lcn_Bounds.rect_YBottom > listP->lh_Bounds.rect_YBottom)
            {
              scrollV = listP->lh_Bounds.rect_YBottom
                        - cellP->lcn_Bounds.rect_YBottom;
            }
          else if (cellP->lcn_Bounds.rect_YTop < listP->lh_Bounds.rect_YTop)
            {
              scrollV
                  = listP->lh_Bounds.rect_YTop - cellP->lcn_Bounds.rect_YTop;
            }
        }
      else if (listP->lh_ListFlags & kAutoScrollH)
        { // horizontal scroll

          // see if the cell is visible
          if (cellP->lcn_Bounds.rect_XRight > listP->lh_Bounds.rect_XRight)
            {
              scrollH = listP->lh_Bounds.rect_XRight
                        - cellP->lcn_Bounds.rect_XRight;
            }
          else if (cellP->lcn_Bounds.rect_XLeft < listP->lh_Bounds.rect_XLeft)
            {
              scrollH
                  = listP->lh_Bounds.rect_XLeft - cellP->lcn_Bounds.rect_XLeft;
            }
        }

      if (scrollH || scrollV)
        LHOffsetList (listP, scrollH, scrollV, TRUE);
    }
}

/* LHGetSelectedCell
 *
 * Get the currently selected cell. Useful for lists that can only have one
 * selected cell.
 */
ListCell *
LHGetSelectedCell (ListHandler *listP)
{
  return listP->lh_CurrentCell;
}

/* LHGetNextCell
 *
 * Get the next cell. Return whether there is a next.
 * NOTE: if cellP == NULL return the first cell.
 */
bool
LHGetNextCell (ListHandler *listP, ListCell **cellP)
{
  if (*cellP == NULL)
    *cellP = listP->lh_ListAnchor;
  else
    *cellP = (*cellP)->lcn_Next;

  return *cellP != NULL;
}

/* LHGetLastCell
 *
 * Find the last cell in the list by looking at the anchor's prev field.
 */
ListCell *
LHGetLastCell (ListHandler *listP)
{
  ListCell *lastCell = listP->lh_ListAnchor;

  if (lastCell != NULL)
    lastCell = lastCell->lcn_Prev;

  return lastCell;
}

/* LHGetIndexCell
 *
 * Return a pointer to the indexed cell. Returns NULL if not found.
 */
ListCell *
LHGetIndexCell (ListHandler *listP, int32 index)
{
  ListCell *indCellP = NULL, *cellP = NULL;
  bool found = false;

  while (!found && LHGetNextCell (listP, &cellP))
    {
      if (cellP->lcn_Index == index)
        {
          indCellP = cellP;
          found = true;
        }
    }

  return indCellP;
}

/* LHGetPreviousCell
 *
 * Get the previous cell.
 * NOTE: if NULL is passed return the first cell (NULL if none).
 */
bool
LHGetPreviousCell (ListHandler *listP, ListCell **cellP)
{
  if (*cellP == NULL)
    *cellP = LHGetLastCell (listP);
  else
    *cellP = (*cellP)->lcn_Prev;

  if (((*cellP)->lcn_CellFlags & kIsLastCell)
      != 0) // check that it didn't wrap
    *cellP = NULL;

  return *cellP != NULL;
}

/* LHGetCellBounds
 */
void
LHGetCellBounds (ListHandler *listP, ListCell *cellP, Rect *rectP)
{
  *rectP = cellP->lcn_Bounds;
}

/* LHSetCellBounds
 */
void
LHSetCellBounds (ListHandler *listP, ListCell *cellP, const Rect *rectP)
{
  cellP->lcn_Bounds = *rectP;
}

/* LHGetCellUserValue
 */
int32
LHGetCellUserValue (ListHandler *listP, ListCell *cellP)
{
  return cellP->lcn_UserValue;
}

/* LHSetCellUserValue
 */
void
LHSetCellUserValue (ListHandler *listP, ListCell *cellP, int32 userValue)
{
  cellP->lcn_UserValue = userValue;
}

/* LHSetCellFormat
 */
void
LHSetCellFormat (ListHandler *listP, ListCell *cellP,
                 CellJustification hJustify, CellJustification vJustify,
                 int32 hOffset, int32 vOffset)
{
  cellP->lcn_CellFormat.hJustify = hJustify;
  cellP->lcn_CellFormat.vJustify = vJustify;
  cellP->lcn_CellFormat.hOffset = hOffset;
  cellP->lcn_CellFormat.vOffset = vOffset;
}

/* LHGetCellFormat
 */
void
LHGetCellFormat (ListHandler *listP, ListCell *cellP, CellFormat *cellFormatP)
{
  *cellFormatP = cellP->lcn_CellFormat;
}

/* LHGetCellCCB
 */
CCB *
LHGetCellCCB (ListHandler *listP, ListCell *cellP)
{
  return cellP->lcn_CCB;
}

/* LHSetCellCCB
 */
void
LHSetCellCCB (
    ListHandler *listP, ListCell *cellP,
    CCB *ccb,          // (in) the CCB to set it to (it is valid to pass NULL)
    bool mapCCBToCell) // (in) should the CCB be mapped to the cell's bounds
{
  cellP->lcn_CCB = ccb;

  if (mapCCBToCell && ccb != NULL)
    LHMapCCBToCell (ccb, cellP, &cellP->lcn_CellFormat,
                    (cellP->lcn_CellFlags & kScaleCellCCBs) != 0);
}

/* LHGetCellHiliteCCB
 */
CCB *
LHGetCellHiliteCCB (ListHandler *listP, ListCell *cellP)
{
  return cellP->lcn_HiliteCCB;
}

/* LHSetCellHiliteCCB
 */
void
LHSetCellHiliteCCB (ListHandler *listP, ListCell *cellP, CCB *hiliteCCB,
                    bool mapCCBToCell)
{
  cellP->lcn_HiliteCCB = hiliteCCB;

  if (mapCCBToCell && hiliteCCB != NULL)
    LHMapCCBToCell (hiliteCCB, cellP, &cellP->lcn_CellFormat,
                    (cellP->lcn_CellFlags & kScaleCellCCBs) != 0);
}

/*
        LHSetCellMarkCCB
 */
void
LHSetCellMarkCCB (ListCell *cellP, CCB *markCCB, int32 horiz, int32 vert)
{
  cellP->lcn_Mark = markCCB;

  // use horiz, vert soon
}

/* LHMapCCBToCell
 *
 * Map the CCB to the cell.
 */
void
LHMapCCBToCell (CCB *ccb, ListCell *cellP, const CellFormat *cellFormatP,
                bool scaleCCB)
{
  if (scaleCCB)
    { // simply scale the CCB to the cell's rectangle
      Point quad[4];

      SetQuad (quad, cellP->lcn_Bounds.rect_XLeft, cellP->lcn_Bounds.rect_YTop,
               cellP->lcn_Bounds.rect_XRight, cellP->lcn_Bounds.rect_YBottom);
      MapCel (ccb, quad);
    }
  else
    { // use cell formatting to determine position
      int32 ccbX, ccbY, ccbWidth, ccbHeight, cellHCentre, cellVCentre;

      ccbWidth = ccb->ccb_Width;
      ccbHeight = ccb->ccb_Height;
      cellHCentre
          = cellP->lcn_Bounds.rect_XLeft
            + ((cellP->lcn_Bounds.rect_XRight - cellP->lcn_Bounds.rect_XLeft)
               / 2);
      cellVCentre
          = cellP->lcn_Bounds.rect_YTop
            + ((cellP->lcn_Bounds.rect_YBottom - cellP->lcn_Bounds.rect_YTop)
               / 2);

      switch (cellP->lcn_CellFormat.hJustify)
        {
        default:
        case kCellJustLeft:
          ccbX = cellP->lcn_Bounds.rect_XLeft;
          break;

        case kCellJustCenter:
          ccbX = cellHCentre - (ccbWidth / 2);
          break;

        case kCellJustRight:
          ccbX = cellP->lcn_Bounds.rect_XRight - ccbWidth;
          break;
        }

      switch (cellP->lcn_CellFormat.vJustify)
        {
        default:
        case kCellJustLeft:
          ccbY = cellP->lcn_Bounds.rect_YTop;
          break;

        case kCellJustCenter:
          ccbY = cellVCentre - (ccbHeight / 2);
          break;

        case kCellJustRight:
          ccbY = cellP->lcn_Bounds.rect_YBottom - ccbHeight;
          break;
        }

      ccbX += cellP->lcn_CellFormat.hOffset;
      ccbY += cellP->lcn_CellFormat.vOffset;

      ccb->ccb_XPos = ccbX << 16;
      ccb->ccb_YPos = ccbY << 16;
    }
}

/* LHSetCellText
 *
 */
Err
LHSetCellText (ListHandler *listP, ListCell *cellP, GrafCon *gc,
               int32 textType, void *text)
{
  DEBUG_CHECKLISTPARAM (listP, "LHSetCellText");

  cellP->lcn_GCon = *gc;
  cellP->lcn_Text = text;

  return 0;
}

/* LHFrameRect
 *
 * Frame a rectangle.
 * Note that line thicknesses > 1 are drawn inside the rect.
 */
void
LHFrameRect (Item bitMapItem, const Rect *framedRect, int32 lineThickness,
             Color lineColor)
{
  GrafCon gc;
  int32 i;
  Rect r;

  r = *framedRect;
  SetFGPen (&gc, lineColor);
  for (i = 0; i < lineThickness; i++)
    {
      MoveTo (&gc, r.rect_XLeft, r.rect_YTop);
      DrawTo (bitMapItem, &gc, r.rect_XRight, r.rect_YTop);
      DrawTo (bitMapItem, &gc, r.rect_XRight, r.rect_YBottom);
      DrawTo (bitMapItem, &gc, r.rect_XLeft, r.rect_YBottom);
      DrawTo (bitMapItem, &gc, r.rect_XLeft, r.rect_YTop);

      r.rect_XLeft++;
      r.rect_YTop++;
      r.rect_XRight--;
      r.rect_YBottom--;
    }
}

/**
 ** Some rectangle utilities:
 **
 ** Candidate for a set of functions?
 **/

/* LHOffsetRect
 *
 */
void
LHOffsetRect (Rect *offsetRect, int32 hOffset, int32 vOffset)
{
  offsetRect->rect_XLeft += hOffset;
  offsetRect->rect_YTop += vOffset;
  offsetRect->rect_XRight += hOffset;
  offsetRect->rect_YBottom += vOffset;
}

/* LHSectRect
 *
 */
bool
LHSectRect (const Rect *srcRectP, const Rect *destRectP, Rect *resultRectP)
{
  bool sected; // was an intersection found

  // efficient piece of boolean logic from SpriteWorldª©¨
  sected = (!((srcRectP->rect_YTop >= destRectP->rect_YBottom)
              || (srcRectP->rect_YBottom <= destRectP->rect_YTop)
              || (srcRectP->rect_XLeft >= destRectP->rect_XRight)
              || (srcRectP->rect_XRight <= destRectP->rect_XLeft)));

  // add code to calc the intersection:
  if (sected && resultRectP != NULL)
    {
      Rect sectRect;

      sectRect.rect_XLeft = (srcRectP->rect_XLeft > destRectP->rect_XLeft)
                                ? srcRectP->rect_XLeft
                                : destRectP->rect_XLeft;
      sectRect.rect_YTop = (srcRectP->rect_YTop > destRectP->rect_YTop)
                               ? srcRectP->rect_YTop
                               : destRectP->rect_YTop;
      sectRect.rect_XRight = (srcRectP->rect_XRight < destRectP->rect_XRight)
                                 ? srcRectP->rect_XRight
                                 : destRectP->rect_XRight;
      sectRect.rect_YBottom
          = (srcRectP->rect_YBottom < destRectP->rect_YBottom)
                ? srcRectP->rect_YBottom
                : destRectP->rect_YBottom;

      *resultRectP = sectRect;
    }

  return sected;
}

/* LHStringWidth
 *
 * Return the width of the string in pixels given the passed font.
 */
int32
LHStringWidth (char *string, FontDescriptor *font)
{
  int32 width = 0, charCount = 0;

  while (*string)
    {
      width += GetFontCharInfo (font, *string, NULL);
      string++;
      charCount++;
    }
  width += font->fd_charExtra * (charCount - 1);

  return width;
}

#ifdef kLHDebug

/* LHCheckListParam
 *
 * Check the list parameter for validity for debugging purposes.
 * This function should not be called by released code! It is only
 * intended to aid in development.
 */
Err
LHCheckListParam (ListHandler *listP, char *functionName)
{
  Err err = 0;

  // check for NULL and the off chance that an odd random address is passed
  if (listP == NULL || ((int32)listP & 1))
    {
      err = kLHBadParamError;
      printf ("bad ListHandlerPtr in %s: %d, listP", functionName, listP);
    }

  return err;
}

#endif // kLHDebug
