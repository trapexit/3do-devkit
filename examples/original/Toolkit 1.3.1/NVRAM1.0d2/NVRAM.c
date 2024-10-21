/*
        File:		NVRAM.c

        Contains:	NVRAM Utility Ñ an application for managing NVRAM
   filesystems

        Written by:	Francis Stanbach

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 1/26/93	fjs		Start Working
   on NVRAM utility 3/10/94	fjs		change the K figures to % tone
   down all of the cels

        To Do:
*/

#include "Debug3DO.h"
#include "FontLib.h"
#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "TextLib.h"
#include "Utils3DO.h"
#include "debug.h"
#include "directory.h"
#include "directoryfunctions.h"
#include "event.h"
#include "folio.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#ifndef _Failure_h
#include "Failure.h"
#endif

#ifndef _ListHandler_h
#include "ListHandler.h"
#endif

#ifndef _ListUtilities_h
#include "ListUtilities.h"
#endif

#define kTileBackGroundCel "Tile Background"
#define kBackGroundImage "Background Image"
#define kTitleCel "Title Cel"
#define kDollarCel "Dollar Cel"
#define kListCel "List Cel"
#define kHighlightCel "Highlight Cel"
#define kOptionsCel "Options Cel"
#define kSpaceUsedCel "Space Used Cel"
#define kSpaceFreeCel "Space Free Cel"
#define kAlertCel "Alert Cel"
#define kHelpCel "Help Cel"

#define kNextSaverCel "Next Saver Cel"

#define kListCelX 130
#define kListCelY 30

#define kTitleCelX 30
#define kTitleCelY 30

#define kOptionsCelX 30
#define kOptionsCelY 30

#define kNotSoBright 27

#if 1
#define ALERT(s) kprintf ("Alert: %s\n", s)
int nCrashFrameCount = 0;
#else
#define ALERT(s)
#endif

// definitions

typedef struct FileRecord
{
  char name[FILESYSTEM_MAX_NAME_LEN];
  uint32 size;
} FileRecord;

// globals

ScreenContext gScreen;

CCB *gTileBackground = NULL;
CCB *gListCel = NULL;
CCB *gHighlightCel = NULL;
CCB *gOptionsCel = NULL;
CCB *gSpaceUsedCel = NULL;
CCB *gSpaceFreeCel = NULL;
CCB *gNextSaverCel = NULL;
CCB *gAlertCel = NULL;
CCB *gHelpCel = NULL;

TextCel *gSaverNameCel = NULL;
TextCel *gSaverSpaceCel = NULL;

ListHandler *gList = NULL; // the main list
FontDescriptor *gFileFont; // the global font descriptor
int32 gFontHeight;         // height of the font + extra space

Item gVBLIOReq;

// prototypes

void SetupBackground (void);
bool Initialize (void);
void CreateFileList (void);
int32 GetFreeSize (void);
void CreateSpaceCels (void);
void UpdateSpaceCels (void);
void LHDeleteTiledCell (ListHandler *list, ListCell *deleteCell);
void DrawEverything (void);
void DeleteSelectedFile (void);
bool CleanupNVRAM (void);
void PrintListCells (ListHandler *list);

// This is where it all begins

int
main ()
{
  bool finished = false;
  ControlPadEventData controlPadEvent;
  int32 result;

  CleanupNVRAM (); // repair & defragment

  TRY FailFalse (Initialize ());

  while (!finished)
    {
      result = GetControlPad (1, FALSE, &controlPadEvent);

      if ((controlPadEvent.cped_ButtonBits & ControlStart)
          || (controlPadEvent.cped_ButtonBits & ControlX))
        finished = true;

      if ((controlPadEvent.cped_ButtonBits & ControlDown) && (result == 1))
        LHSelectNextCellByDirection (gList, kDown);

      if ((controlPadEvent.cped_ButtonBits & ControlUp) && (result == 1))
        LHSelectNextCellByDirection (gList, kUp);

      if ((controlPadEvent.cped_ButtonBits & ControlA) && (result == 1))
        DeleteSelectedFile ();

      if ((controlPadEvent.cped_ButtonBits & ControlC) && (result == 1))
        {
          ControlPadEventData cpe;

          // show the help screen
          gScreen.sc_curScreen = 1 - gScreen.sc_curScreen;

          DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen], gHelpCel);
          DisplayScreen (gScreen.sc_Screens[gScreen.sc_curScreen], 0);

          // wait while c is down
          while (!GetControlPad (1, false, &cpe)
                 && (cpe.cped_ButtonBits & ControlC))
            ;
        }

      gScreen.sc_curScreen = 1 - gScreen.sc_curScreen;

      DrawEverything ();
    }
  CATCH
  ALERT ("Failure somewhere");
  ENDTRY

  CleanupNVRAM (); // repair & defragment

  FadeToBlack (&gScreen, 1 * 30);
  KillEventUtility ();
  ShutDown ();
  return (0);
}

void
DeleteSelectedFile ()
{
  ListCell *lc;
  ControlPadEventData cpe;

  lc = LHGetSelectedCell (gList);

  if (!lc) // return if the list is empty
    return;

  // put up a confirmation

  gScreen.sc_curScreen = 1 - gScreen.sc_curScreen;

  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen], gAlertCel);
  DisplayScreen (gScreen.sc_Screens[gScreen.sc_curScreen], 0);

  // wait until a or b is hit
  while (GetControlPad (1, true, &cpe) && !(cpe.cped_ButtonBits & ControlA)
         && !(cpe.cped_ButtonBits & ControlC))
    ;

  // if a is hit, delete the file
  if (cpe.cped_ButtonBits & ControlA)
    {
      char fileName[256];
      FileRecord *fr;

      LHDeleteTiledCell (gList, lc);

      fr = (FileRecord *)LHGetCellUserValue (gList, lc);

      sprintf (fileName, "/NVRAM/%s", (*fr).name);

      DeleteFile (fileName);

      UpdateSpaceCels ();
    }
}

void
PrintListCells (ListHandler *list)
{
  ListCell *cellP = NULL;

  kprintf ("\n");

  while (LHGetNextCell (list, &cellP))
    kprintf ("cellp: %x, next: %x, prev: %x \n", cellP, cellP->lcn_Next,
             cellP->lcn_Prev);
}

void
LHDeleteTiledCell (ListHandler *list, ListCell *deleteCell)
{
  ListCell *previousCell;
  ListCell *nextCell;

  if (deleteCell == NULL)
    {
      ALERT ("passing nil to delete cell");
      return;
    }

  previousCell = deleteCell->lcn_Prev; // get the previous Cell
  nextCell = deleteCell->lcn_Next;     // get the next Cell

  // PrintListCells(list);

  LHDeleteCell (list, deleteCell);

  // PrintListCells(list);

  // retile cells below the deleted one
  if (nextCell != NULL)
    {
      ListCell *cellP = NULL;
      Rect rectp;
      int32 height;

      cellP = nextCell;

      LHGetCellBounds (list, cellP, &rectp);

      height = rectp.rect_YBottom - rectp.rect_YTop;

      LHOffsetRect (&cellP->lcn_Bounds, 0, -height);

      while (LHGetNextCell (list, &cellP))
        LHOffsetRect (&cellP->lcn_Bounds, 0, -height);

      // select the next cell in the list
      LHSelectCell (list, nextCell, true);
    }
  else
    {
      // select the last cell in the list
      ListCell *lc;

      lc = LHGetLastCell (list);
      if (lc != NULL)
        LHSelectCell (gList, lc, true);
    }

  // else nothing is left to select
}

// This routine bring everything up to speed

bool
Initialize (void)
{
  // long i;

  kprintf ("\n"); // move the debuggerÕs cursor past the 'Mac-->' prompt

  gVBLIOReq = GetVBLIOReq ();

  gScreen.sc_nScreens = 2;

  if (!OpenGraphics (&gScreen, 2) || !OpenSPORT () || !OpenAudio ())
    {
      ALERT ("Can't open a folio!");
      return FALSE;
    }

  if (InitEventUtility (1, 0, LC_Observer) < 0)
    {
      ALERT ("unable to open the event broker\n");
      return FALSE;
    }

  SetupBackground ();

  if ((gListCel = LoadCel (kListCel, MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the List Cel");
      return FALSE;
    }
  else
    {
      gListCel->ccb_XPos = (kListCelX << 16);
      gListCel->ccb_YPos = (kListCelY << 16);
    }

  if ((gOptionsCel = LoadCel (kOptionsCel, MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the Options Cel");
      return FALSE;
    }
  else
    {
      gOptionsCel->ccb_XPos = (kOptionsCelX << 16);
      gOptionsCel->ccb_YPos = (kOptionsCelY << 16);
    }

  if ((gHighlightCel = LoadCel (kHighlightCel, MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the Highlight Cel");
      return FALSE;
    }

  if ((gSpaceFreeCel = LoadCel (kSpaceFreeCel, MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the gSpaceFreeCel Cel");
      return FALSE;
    }

  if ((gSpaceUsedCel = LoadCel (kSpaceUsedCel, MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the gSpaceUsedCel Cel");
      return FALSE;
    }

  if ((gAlertCel = LoadCel (kAlertCel, MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the gAlertCel Cel");
      return FALSE;
    }
  else
    {
      gAlertCel->ccb_XPos = (0 << 16);
      gAlertCel->ccb_YPos = (0 << 16);

      gAlertCel->ccb_HDX = (1 << 20); // full size
      gAlertCel->ccb_VDY = (1 << 16); // full size
    }

  if ((gHelpCel = LoadCel (kHelpCel, MEMTYPE_CEL)) == NULL)
    {
      ALERT ("Cannot load the Help Cel");
      return FALSE;
    }
  else
    {
      gHelpCel->ccb_XPos = (25 << 16);
      gHelpCel->ccb_YPos = (25 << 16);
    }

  // do some actual work

  gFileFont = LoadFont ((char *)"Sans Serif 14", MEMTYPE_ANY);
  if (gFileFont == NULL)
    {
      ALERT ("Cannot load the font");
      return false;
    }

  gFontHeight = gFileFont->fd_charHeight + 2;

  CreateFileList ();
  CreateSpaceCels ();
  UpdateSpaceCels ();

  gScreen.sc_curScreen = 0; // initialize current screen

  DrawEverything ();

  return true;
}

#define kScreenWidth 320
#define kScreenHeight 240

// this routine tiles a cel across the screen

void
SetupBackground ()
{
  int32 numHCopies, numVCopies, totalCopies, iStdWidth, iStdHeight;

  gTileBackground = LoadCel (kTileBackGroundCel, MEMTYPE_CEL);

  FailNULL (gTileBackground);

  // Compute number of copies of this cel (horizontally and vertically)
  // it will take to fill the screen.
  iStdWidth = gTileBackground->ccb_Width;
  iStdHeight = gTileBackground->ccb_Height;
  numHCopies = (kScreenWidth + iStdWidth - 1) / iStdWidth;
  numVCopies = (kScreenHeight + iStdHeight - 1) / iStdHeight;
  totalCopies = numHCopies * numVCopies;

  if (totalCopies > 1)
    {
      int32 i, iRow, iCol, xPos, yPos;
      CCB *tmpCel;

      // Make enough copies of the cel to fill the screen (minus 1 for
      // 'gTileBackground')
      for (i = 1; i < totalCopies; ++i)
        {
          tmpCel
              = MakeNewDupCCB (gTileBackground); // Duplicate the cel exactly
          if (tmpCel == NULL)
            {
              ERR (("Error: couldn't dup all cels %ld of %ld\n", i,
                    totalCopies));
              goto BKCEL_ERROR;
            }

          tmpCel->ccb_NextPtr
              = gTileBackground;          // Put it before 'gTileBackground'
          tmpCel->ccb_Flags &= ~CCB_LAST; // Mark this as not last
          gTileBackground = tmpCel; // Make 'gTileBackground' point to first
        }

      // Now we have enough cels to fill the screen in a chain pointed to by
      // 'gTileBackground'
      xPos = yPos = 0;
      tmpCel = gTileBackground; // Point to first cel in list
      for (iRow = 0; iRow < numVCopies; ++iRow)
        {
          for (iCol = 0; iCol < numHCopies; ++iCol)
            {
              tmpCel->ccb_XPos = Convert32_F16 (xPos);
              tmpCel->ccb_YPos = Convert32_F16 (yPos);
              xPos += iStdWidth;            // Increment horz position
              tmpCel = tmpCel->ccb_NextPtr; // Move to next cel
            }
          xPos = 0;           // Carridge-return
          yPos += iStdHeight; // Line-feed
        }
    }
BKCEL_ERROR:;
}

// this function draws all of the cells

void
DrawEverything ()
{
  WaitVBL (gVBLIOReq, 1);
  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen], gTileBackground);

  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen], gListCel);
  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen], gOptionsCel);

  FailErr (LHDraw (gList, gScreen.sc_BitmapItems[gScreen.sc_curScreen]));

  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen],
                  gSaverNameCel->tc_CCB);
  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen],
                  gSaverSpaceCel->tc_CCB);
  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen], gSpaceUsedCel);
  DrawScreenCels (gScreen.sc_Screens[gScreen.sc_curScreen], gSpaceFreeCel);

  DisplayScreen (gScreen.sc_Screens[gScreen.sc_curScreen], 0);
}

// this routine creates the list of files

void
CreateFileList (void)
{
  Item rootItem;
  Directory *dir;
  DirectoryEntry de;
  Item ioReqItem;
  int32 result;
  TagArg ioReqTags[2];

  ListCell *cellP;
  Rect listRect;
  GrafCon gc;

  rootItem = OpenDiskFile ("/NVRAM");
  CHECKRESULT ("OpenDiskFile", rootItem);

  // open an item for the directory

  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *)rootItem;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), ioReqTags);
  CHECKRESULT ("CreateItem", ioReqItem);

  // open the root directory

  dir = OpenDirectoryItem (rootItem);
  if (!dir)
    kprintf ("Could not open a directory for item %d\n", rootItem);

  // the location of the list

  listRect.rect_XLeft = kListCelX + 5;
  listRect.rect_YTop = kListCelY + 63;
  listRect.rect_XRight = listRect.rect_XLeft + 140;
  listRect.rect_YBottom = listRect.rect_YTop + 7 * gFontHeight;

  FailErr (LHCreateListHandler (
      &listRect,
      kMapCCBsAtDrawTime | kAutoScrollV | kClipCellsToBounds, // list flags
      kNoWrapCellSelection | kSelectOneOnly
          | kAlwaysSelectOne,                       // selection flags
      kHiliteByCCB | 2 | kFont3DO | kScaleCellCCBs, // default cell flags
      &gList));

  gList->lh_HiliteColor = MakeRGB15 (30, 0, 30);

  SetFGPen (&gc, MakeRGB15 (30, 30, 30));
  SetBGPen (&gc, MakeRGB15 (0, 0, 0));

  while ((result = ReadDirectory (dir, &de)) >= 0)
    {
      FileRecord *fr;
      TextCel *tc;

      // Add this to the file list

      FailErr (LHCreateTiledCell (gList, &cellP, 140, gFontHeight));
      LHAddCell (gList, NULL, cellP); // NULL == add it to the end

      // allocate new record and attach it to the user cell

      fr = (FileRecord *)USER_ALLOCMEM (sizeof (FileRecord), MEMTYPE_ANY);

      FailNULL (fr);

      strncpy (fr->name, de.de_FileName, FILESYSTEM_MAX_NAME_LEN);

      fr->size = de.de_BlockCount * de.de_BlockSize;

      LHSetCellUserValue (gList, cellP, (int32)fr);

#if 1
      tc = CreateTextCel (gFileFont, TC_FORMAT_8BPPCEL, 140, gFontHeight);

      if (!tc)
        kprintf ("could not create cel! Wahh! \n");

      EraseTextInCel (tc);
      SetTextCelMargins (tc, 5, 2);

      SetTextCelColor (tc, 0,
                       MakeRGB15 (kNotSoBright, kNotSoBright, kNotSoBright));
      UpdateTextInCel (tc, TRUE, fr->name);

      LHSetCellCCB (gList, cellP, tc->tc_CCB, true);

      LHSetCellHiliteCCB (gList, cellP, gHighlightCel, true);

#else
      LHSetCellText (gList, cellP, &gc, 8, (void *)fr->name);
#endif

      LHSetCellFormat (gList, cellP,
                       kCellJustLeft,   // horiz justification
                       kCellJustCenter, // vert justification
                       5,               // hOffset
                       5);              // vOffset

      cellP->lcn_FontDesc.lcn_Font3DO = gFileFont;
    }

  CloseDirectory (dir);
}

#define kKilobyte 1024

void
CreateSpaceCels ()
{
  gSaverNameCel
      = CreateTextCel (gFileFont, TC_FORMAT_8BPPCEL, 150, gFontHeight);

  EraseTextInCel (gSaverNameCel);
  SetTextCelMargins (gSaverNameCel, 5, 2);

  SetTextCelColor (gSaverNameCel, 0,
                   MakeRGB15 (kNotSoBright, kNotSoBright, kNotSoBright));

  UpdateTextInCel (gSaverNameCel, TRUE, "Built-in Memory");

  SetTextCelCoords (gSaverNameCel, (kListCelX + 5) << 16,
                    (kListCelY + 5) << 16);

  gSaverSpaceCel
      = CreateTextCel (gFileFont, TC_FORMAT_8BPPCEL, 150, gFontHeight);

  EraseTextInCel (gSaverSpaceCel);
  SetTextCelMargins (gSaverSpaceCel, 5, 2);

  SetTextCelColor (gSaverSpaceCel, 0,
                   MakeRGB15 (kNotSoBright, kNotSoBright, kNotSoBright));
  SetTextCelCoords (gSaverSpaceCel, (kListCelX + 5) << 16,
                    (kListCelY + 5 + 16 * 2) << 16);
}

void
UpdateSpaceCels ()
{
  char spaceString[255];
  int32 freeSize;
  int32 KUsed, KFree;
  Point quad[4], quad2[4];

  freeSize = GetFreeSize ();

  EraseTextInCel (gSaverSpaceCel);

  KUsed = freeSize * 100 / 32768;
  KFree = 100 - KUsed;

  sprintf (spaceString, "%d%% Used, %d%% Free", KUsed, KFree);

  UpdateTextInCel (gSaverSpaceCel, TRUE, spaceString);

  // width = 150 - 30 = 120

  quad[0].pt_X = kListCelX + 5 + 10;
  quad[0].pt_Y = kListCelY + 5 + gFontHeight + 2;

  quad[1].pt_X = quad[0].pt_X + (KUsed * 120 / 100);
  quad[1].pt_Y = quad[0].pt_Y;

  quad[2].pt_X = quad[1].pt_X;
  quad[2].pt_Y = quad[0].pt_Y + 12;

  quad[3].pt_X = quad[0].pt_X;
  quad[3].pt_Y = quad[2].pt_Y;

  MapCel (gSpaceUsedCel, quad);

  // this isn't confusing

  quad2[0].pt_X = quad[1].pt_X + 1;
  quad2[0].pt_Y = quad[1].pt_Y;

  quad2[1].pt_X = quad[0].pt_X + 120;
  quad2[1].pt_Y = quad2[0].pt_Y;

  quad2[2].pt_X = quad2[1].pt_X;
  quad2[2].pt_Y = quad2[0].pt_Y + 12;

  quad2[3].pt_X = quad2[0].pt_X;
  quad2[3].pt_Y = quad2[2].pt_Y;

  MapCel (gSpaceFreeCel, quad2);
}

//
// calculate the free space
//
int32
GetFreeSize ()
{
  int32 totalSpace;
  ListCell *cellP = NULL;
  FileRecord *fr;

  totalSpace = 132 + 20; // volume info + anchor

  cellP = NULL;

  while (LHGetNextCell (gList, &cellP))
    {
      fr = (FileRecord *)LHGetCellUserValue (gList, cellP);

      totalSpace += fr->size + 64; // 64 bytes overhead per file
    }

  return totalSpace;
}

//
// this function call lmadm to repair and defragment NVRAM
//
bool
CleanupNVRAM (void)
{
  Item lmadmItem;

  lmadmItem = LoadProgram ("lmadm -a ram 3 0 NVRAM");
  if (lmadmItem < 0)
    {
      kprintf ("lmadm could not be run");
      return false;
    }

  WaitSignal (SIGF_DEADTASK);
  return true;
}
