/*
        File:		ListHandler.h

        Contains:	Headers for ListHandler

        Written by:	Randy Carr, Edward Harp

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 12/7/93	EGH		Added kLHDebug
   switch debugging code. 11/12/93	EGH		Began implementation.
                                10/20/93	RLC		New file.

        To Do:

                ¥ Decide if we need so many separate calls to do both
   convenience things and Get/Set functions
*/

#ifndef _ListHandler_h
#define _ListHandler_h

#include "FontLib.h"
#include "TextLib.h"

#define kCurrentListHandlerVersion 0x0001

#define kLHDebug // define this to turn parameter checking etc. on

#define kFirstCell 1
#define kLastCell -1
#define kFromCurrentCell 0

#define kNoCell 0

/* ListHandler list (lh_ListFlags) flags
 */
#define kAutoScrollV                                                          \
  0x00000001 // automatically scroll selected cells into view
#define kAutoScrollH                                                          \
  0x00000002 // automatically scroll selected cells into view
#define kClipCellsToBounds                                                    \
  0x00000004 // only draw cells within the list's bounds
             // NOTE: partial cell clipping not supported!
#define kHiliteOnlyDrawProc                                                   \
  0x00000008 // call optional draw proc for selected cells only
             // after drawing the cell with standard proc
#define kAllCCBsAreLinked                                                     \
  0x00000010 // only call DrawCels for the first CCB encountered
#define kAutoLinkCCBs 0x00000020 // automatically link CCBs as they are set
#define kSetCCBsSkipFlag                                                      \
  0x00000040 // be nice & set all CCB's skip flags when necessary
#define kCursorByFrame 0x00000080  // draw cursor as a frame
#define kScaleCursorCCB 0x00000100 // scale the cursor CCB(s) to the cell
#define kMapCCBsAtDrawTime                                                    \
  0x00000200 // map cell CCBs as cells are drawn
             // this allows one CCB to be used in multiple cells

/* Selection (lh_SelectionFlags) flags
 * Control the way cells are selected by the list handler.
 */
#define kSelectOneOnly 0x00000001   // only select one cell at a time
#define kAlwaysSelectOne 0x00000002 // always select a cell
#define kNoWrapCellSelection                                                  \
  0x00000004 // do not wrap next cell selection @ ends of list
#define kFlipSelection 0x00000008

/* Cell flags (lcn_CellFlags) flags
 *
 */
#define kCellHilite 0x00000100 // is the cell selected
#define kHiliteByFrame                                                        \
  0x00000200 // draw hilite by drawing a frame of the list's hilite color
#define kHiliteFrameMask 0x000000FF // mask for finding frame thickness
#define kHiliteByColor 0x00000400   // hilite using list's hilite color
#define kHiliteByCCB 0x00000800     // hilite by drawing hilite CCB
#define kHiliteBothCCBs 0x00001000  // draw both CCBs when selected
#define kIsLastCell 0x00002000      // is this cell the last one in the list
#define kOldFont 0x00004000         // old font type text
#define kFont3DO 0x00008000         // new font type text
#define k8BitText 0x00010000        // text is eight bit
#define k16BitText 0x00020000       // text is 16 bit
#define kScaleCellCCBs 0x00040000 // scale the cell's CCBs to the cell's bounds

/* List handler errors
 */
#define kLHOutOfMemError -1
#define kLHBadParamError -2
#define kLHIndexOutOfRangeError -3

#define kDRAMAndClearMemory MEMTYPE_DRAM + MEMTYPE_FILL | 0x00

enum
{
  kUse8BitText = 0,
  kUse16BitText,
  kUse32BitText
};

/* justification (horizontal & vertical) for things drawn in cells
 */
typedef enum
{
  kCellJustLeft = 0,
  kCellJustCenter,
  kCellJustRight
} CellJustification;

/* direction flags for selection of next cell or movement of the cursor
 */
#define kUp 0x00000001
#define kDown 0x00000002
#define kLeft 0x00000004
#define kRight 0x00000008

/* Structure defining how a CCB is formatted in its cell
 */
typedef struct CellFormat
{
  CellJustification hJustify;
  CellJustification vJustify;
  int32 hOffset;
  int32 vOffset;
} CellFormat, *CellFormatPtr;

/* list cell structure
 */
typedef struct ListCell
{
  struct ListCell *lcn_Next;     // pointer to next ListCell in list
  struct ListCell *lcn_Prev;     // pointer to previous ListCell in list
  uint16 lcn_Version;            // version of of this ListCell
  uint32 lcn_CellFlags;          // list cell flags
  int32 lcn_Index;               // cell index for this data structure
  struct ListHandler *lcn_Owner; // cell owner
  Rect lcn_Bounds;               // bounds of cell to be displayed
  CellFormat lcn_CellFormat;     // format for the cell's contents
  CCB *lcn_CCB;       // a cell's ccb (list) for drawing (or NULL if none)
  CCB *lcn_HiliteCCB; // a ccb for use when cell is hilited (or NULL if none)
  void *lcn_Text;     // original cell text (also can be used for searching)
  union
  {
    FontDescriptor *lcn_Font3DO; // 3DO font descriptor
    Font *lcn_Font;              // old style font
  } lcn_FontDesc;
  GrafCon lcn_GCon;    // cell grafix info
  int32 lcn_UserValue; // some user controlled value

  /* add more things here */
  CCB *lcn_Mark; // optional marker cel (or NULL if none)
} ListCell, *ListCellPtr;

/* proc types
 */

// proc that renders cells
typedef Err (*CellDrawProc) (struct ListHandler *listP, ListCell *listCell,
                             Item bitMapItem);

// proc that selects the next cell
typedef void (*NextCellProc) (struct ListHandler *listP, int32 direction);

// proc that moves the cursor from cell to cell
typedef NextCellProc MoveCursorProc;

typedef struct ListHandler
{
  struct ListHandler *lh_NextList; // pointer to next list (if any)
  struct ListHandler *lh_PrevList; // pointer to previous list (if any)
  uint16 lh_Version;               // version of List Handler
  ListCell *lh_ListAnchor;         // list of list cells
  int32 lh_CellCount;              // number of cells
  ListCell *lh_CurrentCell;        // currently selected (target cell)
  ListCell *lh_CursorCell;         // cell with cursor over it
  Color lh_CursorColor;            // color to draw cursor with
  CCB *lh_CursorCCB;               // CCB that acts as cursor
  MoveCursorProc
      lh_MoveCursorProc;   // optional proc to move the cursor to the next cell
  int32 lh_CurrentIndex;   // currently selected index (target index)
  Rect lh_Bounds;          // list bounds (Union of all visible cells)
  int32 lh_ListFlags;      // list flag
  int32 lh_SelectionFlags; // selection flags
  NextCellProc lh_NextCellProc; // optional proc that selects the next cell
  int32 lh_DefaultCellFlags;    // default list cell flags
  CellDrawProc lh_DrawProc;     // optional drawing proc
  Color lh_HiliteColor;         // color to use for drawing hilite outlines
  int32 lh_UserValue;           // some user controlled value

  /* add more things here */
} ListHandler, *ListHandlerPtr;

Err LHCreateListHandler (const Rect *listBounds, int32 listFlags,
                         int32 selectionFlags, int32 defaultCellFlags,
                         ListHandler **listP);
Err LHDisposeListHandler (ListHandler *listP);

/* passing user events to a list
 */
// bool	LHPadEvent(ListHandler *listP, const ControlPadEventData *event);
// bool	LHCursorEvent(ListHandler *listP, ControlPadEventData *event);

/* drawing a list
 */
Err LHDraw (ListHandler *listP, Item bitMapItem);
void LHSetDrawProc (ListHandler *listP, CellDrawProc drawProc);
Err LHStandardDrawProc (ListHandler *listP, ListCell *cellP, Item bitMapItem);
Err LHOffsetList (ListHandler *listP, int32 hOffset, int32 vOffset,
                  bool reMapCCBs);
Err LHLinkListHandler (ListHandler *thisList, ListHandler *toThatList);
Err LHDisposeAllCells (ListHandler *list, Boolean disposeCCBs);
int32 LHCountCells (ListHandler *listP);
Err LHSearchCells (ListHandler *list, const void *dataPtr, int32 dataLen,
                   int32 textType, int32 startIndex, void *searchFunc,
                   int32 userData, int32 *result);

/*	Operations on cells
 */
Err LHCreateCell (ListHandler *listP, ListCell **cellP, Rect *cellRect);
Err LHDisposeCell (ListCell *cellP, bool disposeCCBs);

void LHAddCell (ListHandler *listP,
                ListCell *insertBeforeP, // NULL == append to end
                ListCell *cellP);

void LHDeleteCell (ListHandler *listP, ListCell *cellP);

Err LHGetIndexFrom (ListHandler *listP, ListCell *cell, int32 *cellIndex);
Err LHGetFromIndex (ListHandler *listP, int32 *cellIndex, ListCell *cellP);
Err LHGetListHandlerFrom (const ListCell *cell, ListHandler *list);

/* calls to select cells in the list
 */
void LHSelectCell (ListHandler *listP, ListCell *cellP, bool select);
void LHSelectIndexCell (ListHandler *listP, int32 cellIndex, bool select);
void LHSelectNextCell (ListHandler *listP);
void LHSelectPreviousCell (ListHandler *listP);
void LHSelectNextCellByDirection (ListHandler *listP, int32 direction);
void LHStandardNextCellProc (ListHandler *listP, int32 direction);

void LHGetIndSelectedCell (ListHandler *listP, int32 *cellIndex);
ListCell *LHGetSelectedCell (ListHandler *listP);
void LHGetNextSelectedCell (ListHandler *listP, ListCell **cellP);

/* calls to set and move the cursor
 */
void LHSetCursorCell (ListHandler *listP, ListCell *cellP);
ListCell *LHGetCursorCell (ListHandler *listP);
void LHMoveCursor (ListHandler *listP, int32 direction);

/* 	calls to walk cell list
 */
bool LHGetNextCell (ListHandler *listP, ListCell **cellP);
bool LHGetPreviousCell (ListHandler *listP, ListCell **cellP);
ListCell *LHGetLastCell (ListHandler *listP);
ListCell *LHGetIndexCell (ListHandler *listP, int32 index);

/*	calls to access list data
 */
void SetDefaultCellFlags (ListHandler *listP, int32 cellFlags);
int32 GetDefaultCellFlags (ListHandler *listP);
void GetListBounds (ListHandler *listP, Rect *r);
void LHSetListNextCellProc (ListHandler *listP, NextCellProc proc);

/* 	calls to access cell data
 */
void LHGetCellBounds (ListHandler *listP, ListCell *cellP, Rect *rectP);
void LHSetCellBounds (ListHandler *listP, ListCell *cellP, const Rect *rectP);
int32 LHGetCellUserValue (ListHandler *listP, ListCell *cellP);
void LHSetCellUserValue (ListHandler *listP, ListCell *cellP, int32 userValue);
uint32 LHGetCellFlags (ListHandler *listP, ListCell *cellP);
void LHSetCellFlags (ListHandler *listP, ListCell *cellP, uint32 cellFlags);
void LHSetCellFormat (ListHandler *listP, ListCell *cellP,
                      CellJustification hJustify, CellJustification vJustify,
                      int32 hOffset, int32 vOffset);
void LHGetCellFormat (ListHandler *listP, ListCell *cellP,
                      CellFormat *cellFormatP);
CCB *LHGetCellCCB (ListHandler *listP, ListCell *cellP);
void LHSetCellCCB (ListHandler *listP, ListCell *cellP, CCB *ccb,
                   bool mapCCBToCell);
CCB *LHGetCellHiliteCCB (ListHandler *listP, ListCell *cellP);
void LHSetCellHiliteCCB (ListHandler *listP, ListCell *cellP, CCB *hiliteCCB,
                         bool mapCCBToCell);
void LHGetCellGrafCon (ListHandler *listP, int32 cellIndex, GrafCon *gc);
void LHSetCellGrafCon (ListHandler *listP, int32 cellIndex, GrafCon *gc);
Err LHSetCellText (ListHandler *listP, ListCell *cellP, GrafCon *gc,
                   int32 textType, void *text);

/* utilitarian routines
 */
void LHFrameRect (Item bitMapItem, const Rect *framedRect, int32 lineThickness,
                  Color lineColor);

void LHOffsetRect (Rect *offsetRect, int32 hOffset, int32 vOffset);

bool LHSectRect (const Rect *srcRectP, const Rect *destRectP,
                 Rect *resultRectP);

void LHMapCCBToCell (CCB *ccb, ListCell *cellP, const CellFormat *cellFormatP,
                     bool scaleCCB);

int32 LHStringWidth (char *string, FontDescriptor *font);

void LHSetCellMarkCCB (ListCell *cellP, CCB *markCCB, int32 horiz, int32 vert);

/* debugging routines
 */
#ifdef kLHDebug

Err LHCheckListParam (ListHandler *listP, char *functionName);

#define DEBUG_CHECKLISTPARAM(p, s)                                            \
  {                                                                           \
    Err debugErr;                                                             \
                                                                              \
    debugErr = LHCheckListParam (p, s);                                       \
    if (debugErr != 0)                                                        \
      return debugErr;                                                        \
  }

#else

#define DEBUGCHECKLISTPARAM(p)

#endif // kLHDebug

#endif
