/* $Id: $ */

#ifndef __STLISTS_H
#define __STLISTS_H


/****************************************************************************/


/* Copyright (C) 1993-1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef FONTLIB_H
#include "FontLib.h"
#endif

#ifndef TEXTLIB_H
#include "TextLib.h"
#endif


/*****************************************************************************/


/* ListHandler list (lh_ListFlags) flags  */
#define kAutoScrollV            0x00000001 // automatically scroll selected cells into view
#define kClipCellsToBounds      0x00000004 // only draw cells within the list's bounds
#define kMapCCBsAtDrawTime      0x00000200 // map cell CCBs as cells are drawn
                                                                                        // this allows one CCB to be used in multiple cells

/* Selection (lh_SelectionFlags) flags
 * Control the way cells are selected by the list handler.
 */
#define kSelectOneOnly          0x00000001 // only select one cell at a time
#define kAlwaysSelectOne        0x00000002 // always select a cell
#define kNoWrapCellSelection    0x00000004 // do not wrap next cell selection @ ends of list

/* Cell flags (lcn_CellFlags) flags */
#define kCellHilite             0x00000100 // is the cell selected
#define kHiliteByCCB            0x00000800 // hilite by drawing hilite CCB
#define kIsLastCell             0x00002000 // is this cell the last one in the list
#define kFont3DO                0x00008000 // new font type text
#define kScaleCellCCBs          0x00040000 // scale the cell's CCBs to the cell's bounds

/* List handler errors */
#define kLHOutOfMemError        -1

/* justification (horizontal & vertical) for things drawn in cells */
typedef enum
{
    kCellJustLeft,
    kCellJustCenter,
    kCellJustRight
} CellJustification;

/* direction flags for selection of next cell or movement of the cursor */
#define kUp     0x00000001
#define kDown   0x00000002

/* Structure defining how a CCB is formatted in its cell */
typedef struct CellFormat
{
    CellJustification hJustify;
    CellJustification vJustify;
    int32             hOffset;
    int32             vOffset;
} CellFormat, *CellFormatPtr;

/* list cell structure */
typedef struct ListCell
{
    struct ListCell         *lcn_Next;             // pointer to next ListCell in list
    struct ListCell         *lcn_Prev;             // pointer to previous ListCell in list
    uint32                   lcn_CellFlags;        // list cell flags
    int32                    lcn_Index;            // cell index for this data structure
    struct ListHandler      *lcn_Owner;            // cell owner
    Rect                     lcn_Bounds;           // bounds of cell to be displayed
    CellFormat               lcn_CellFormat;       // format for the cell's contents
    CCB                     *lcn_CCB;              // a cell's ccb (list) for drawing (or NULL if none)
    CCB                     *lcn_HiliteCCB;        // a ccb for use when cell is hilited (or NULL if none)
    void                    *lcn_Text;             // original cell text (also can be used for searching)
    union {
            FontDescriptor  *lcn_Font3DO;          // 3DO font descriptor
            Font            *lcn_Font;             // old style font
    } lcn_FontDesc;
    GrafCon                  lcn_GCon;             // cell grafix info
    int32                    lcn_UserValue;        // some user controlled value
    CCB                     *lcn_Mark;             // optional marker cel (or NULL if none)
} ListCell, *ListCellPtr;


// proc that renders cells
typedef Err (*CellDrawProc)(struct ListHandler *listP, ListCell *listCell, Item bitMapItem);

// proc that selects the next cell
typedef void (*NextCellProc)(struct ListHandler *listP, int32 direction);

// proc that moves the cursor from cell to cell
typedef NextCellProc MoveCursorProc;

typedef struct ListHandler
{
    struct ListHandler *lh_NextList;           // pointer to next list (if any)
    struct ListHandler *lh_PrevList;           // pointer to previous list (if any)
    ListCell           *lh_ListAnchor;         // list of list cells
    int32               lh_CellCount;          // number of cells
    ListCell           *lh_CurrentCell;        // currently selected (target cell)
    ListCell           *lh_CursorCell;         // cell with cursor over it
    Color               lh_CursorColor;        // color to draw cursor with
    CCB                *lh_CursorCCB;          // CCB that acts as cursor
    MoveCursorProc      lh_MoveCursorProc;     // optional proc to move the cursor to the next cell
    int32               lh_CurrentIndex;       // currently selected index (target index)
    Rect                lh_Bounds;             // list bounds (Union of all visible cells)
    int32               lh_ListFlags;          // list flag
    int32               lh_SelectionFlags;     // selection flags
    NextCellProc        lh_NextCellProc;       // optional proc that selects the next cell
    int32               lh_DefaultCellFlags;   // default list cell flags
    CellDrawProc        lh_DrawProc;           // optional drawing proc
    Color               lh_HiliteColor;        // color to use for drawing hilite outlines
    int32               lh_UserValue;          // some user controlled value
} ListHandler, *ListHandlerPtr;


/*****************************************************************************/


Err LHCreateListHandler(const Rect   *listBounds,
                        int32         listFlags,
                        int32         selectionFlags,
                        int32         defaultCellFlags,
                        ListHandler **listP);

Err LHDeleteListHandler(ListHandler *listP);

Err LHDraw(ListHandler *listP, Item bitMapItem);

void LHSetDrawProc(ListHandler *listP, CellDrawProc drawProc);

Err LHOffsetList(ListHandler *listP,
                 int32        hOffset,
                 int32        vOffset,
                 bool         reMapCCBs);

Err LHCreateTiledCell(ListHandler  *listP,
                      ListCell    **cellP,
                      int32         width,
                      int32         height);

Err LHDisposeCell(ListCell *cellP);

void LHAddCell(ListHandler *listP,
               ListCell    *insertBeforeP,              // NULL == append to end
               ListCell    *cellP);

void LHDeleteCell(ListHandler *listP, ListCell *cellP);
void LHSelectCell(ListHandler *listP, ListCell *cellP, bool select);
void LHSelectNextCellByDirection(ListHandler *listP, int32 direction);
ListCell *LHGetSelectedCell(ListHandler *listP);
bool LHGetNextCell(ListHandler *listP, ListCell **cellP);
ListCell *LHGetLastCell(ListHandler *listP);
void LHGetCellBounds(ListHandler *listP, ListCell *cellP, Rect *rectP);
int32 LHGetCellUserValue(ListHandler *listP, ListCell *cellP);
void LHSetCellUserValue(ListHandler *listP, ListCell *cellP, int32 userValue);

void LHSetCellFormat(ListHandler       *listP,
                     ListCell          *cellP,
                     CellJustification  hJustify,
                     CellJustification  vJustify,
                     int32              hOffset,
                     int32              vOffset);

void LHSetCellHiliteCCB(ListHandler *listP,
                        ListCell    *cellP,
                        CCB         *hiliteCCB,
                        bool         mapCCBToCell);

Err LHSetCellText(ListHandler *listP,
                  ListCell    *cellP,
                  GrafCon     *gc,
                  int32        textType,
                  void        *text);

void LHOffsetRect(Rect  *offsetRect,
                  int32  hOffset,
                  int32  vOffset);


/*****************************************************************************/


#endif /* __STLISTS_H */
