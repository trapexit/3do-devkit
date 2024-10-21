/* $Id: $ */

/****************************************************************************/

/* Copyright (C) 1993-1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */

/****************************************************************************/

#include "debug.h"
#include "directory.h"
#include "directoryfunctions.h"
#include "event.h"
#include "filefunctions.h"
#include "filesystem.h"
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
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#include "FontLib.h"
#include "TextLib.h"

#include "st.h"
#include "stlists.h"
#include "stmem.h"
#include "storagetuner.h"

/*****************************************************************************/

CCB *LoadCel (char *filename, uint32 memTypeBits);
void UnloadCel (CCB *cel);

/*****************************************************************************/

// #define TRACE(s)   kprintf("ST: %s\n", s)
#define TRACE(s) ;

#define NUMSCREENS 2
#define kTileBackGroundCel "Background Cel"
#define kListCel "List Cel"
#define kHighlightCel "Highlight Cel"
#define kOptionsCel "Options Cel"
#define kAlertCel "Alert Cel"
#define kHelpCel "Help Cel"

#define kScreenWidth 320
#define kScreenHeight 240
#define kListCelX 115
#define kListCelY 18
#define kOptionsCelX 20
#define kOptionsCelY 18
#define kNotSoBright 27
#define kHorizontalMargin 2
#define kVerticalMargin 2
#define kTotalWidth 175
#define kCellWidth 130
#define kSizeWidth 45

typedef struct FileRecord
{
  char name[FILESYSTEM_MAX_NAME_LEN];
  char sizeString[12];
  uint32 size;
  uint32 type;
  char *comment;
} FileRecord;

#define kCommentLength 256

typedef struct Comment
{
  uint32 languageCode;
  char comment[kCommentLength];
} Comment;

static Comment gComment;
static Item vblIO;
static Item screens[NUMSCREENS];
static uint32 curScreen;
static ListHandler *gList;
static FontDescriptor *gFileFont;
static int32 gFontHeight;
static CCB *gBackgroundCel;
static CCB *gListCel;
static CCB *gHighlightCel;
static CCB *gOptionsCel;
static CCB *gAlertCel;
static CCB *gHelpCel;
static TextCel *gSaverNameCel;
static TextCel *gSaverSpaceCel;
static TextCel *gCommentCel;
static TextCel *gFileNameCel;
static TextCel *gFileSizeCel;
static List memToFree;
static bool cleanupNVRAM;
static bool deleteScreen;

/*****************************************************************************/

#define STD_FULL_INTENSITY                                                    \
  0x00F8 // full intensity R,G,B in standard system CLUT entry
#define NUM_FADE_STEPS 15

static void
FadeToBlack (Item *screens)
{
  int32 i, j, k;
  ubyte color;
  int32 index;
  uint32 colorEntries[32];

  for (i = NUM_FADE_STEPS - 1; i >= 0; i--)
    {
      WaitVBL (vblIO, 1);
      k = (i * STD_FULL_INTENSITY) / NUM_FADE_STEPS;

      for (index = 0; index < 32; index++)
        {
          color = (uint8)((k * index) / 31);
          colorEntries[index]
              = MakeCLUTColorEntry (index, color, color, color);
        }

      for (j = 0; j < NUMSCREENS; j++)
        SetScreenColors (screens[j], colorEntries, 32);
    }
}

/*****************************************************************************/

/* calculate the free space */
static int32
GetUsedSize (void)
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

/*****************************************************************************/

static bool
GetCommentData (char *fileName)
{
  ulong NVRTType = 0x4E565254;
  Item fileItem;
  int32 result;
  IOInfo fileInfo;
  uint32 chunkHeader[2];
  Item ioReqItem;
  bool working = TRUE;
  uint32 fileOffset = 0;
  bool foundIt = FALSE;

  fileItem = OpenDiskFile (fileName);
  ioReqItem = CreateIOReq (NULL, 0, fileItem, 0);

  while (working)
    {
      memset (&fileInfo, 0, sizeof (fileInfo)); // clear the parameter block

      fileInfo.ioi_Command = CMD_READ;
      fileInfo.ioi_Recv.iob_Buffer = &chunkHeader[0];
      fileInfo.ioi_Recv.iob_Len = 2 * sizeof (uint32);
      fileInfo.ioi_Offset = fileOffset;

      result = DoIO (ioReqItem, &fileInfo);

      if (result < 0)
        {
          working = FALSE; // stop trying
        }
      else
        {
          // check to see if this is the correct chunk

          if (chunkHeader[0] == NVRTType)
            {
              // read in the comment data

              memset (&fileInfo, 0,
                      sizeof (fileInfo)); // clear the parameter block
              fileInfo.ioi_Command = CMD_READ;
              fileInfo.ioi_Recv.iob_Buffer = &gComment;
              fileInfo.ioi_Recv.iob_Len
                  = (chunkHeader[1] - 8);           // read chunk date
              fileInfo.ioi_Offset = fileOffset + 8; // start past chunk header

              result = DoIO (ioReqItem, &fileInfo);

              // check for the proper language

              if (gComment.languageCode == 0) // english
                {
                  foundIt = TRUE;
                  working = FALSE;
                }
            }

          // advance the file to the next chunk
          fileOffset += chunkHeader[1];
        }
    }

  DeleteIOReq (ioReqItem);
  result = CloseDiskFile (fileItem);

  return foundIt;
}

/*****************************************************************************/

static void
UpdateCommentCel (void)
{
  ListCell *lc;
  FileRecord *fr;

  lc = LHGetSelectedCell (gList);

  if (!lc)
    return;

  fr = (FileRecord *)LHGetCellUserValue (gList, lc);

  if (fr->type == 0x33444f46) // type = '3DOF'
    {
      char fileName[256];

      sprintf (fileName, "/NVRAM/%s", fr->name);

      if (GetCommentData (fileName))
        {
          EraseTextInCel (gCommentCel);
          UpdateTextInCel (gCommentCel, TRUE, gComment.comment);
        }
      else
        {
          EraseTextInCel (gCommentCel);
          UpdateTextInCel (gCommentCel, TRUE, "No comment available.");
        }
    }
  else
    {
      EraseTextInCel (gCommentCel);
      UpdateTextInCel (gCommentCel, TRUE, fr->comment);
    }
}

/*****************************************************************************/

static void
FullDisposeCell (ListHandler *listP, ListCell *cellP)
{
  FileRecord *fr;

  // delete the cels
  fr = (FileRecord *)LHGetCellUserValue (listP, cellP);

  // delete the file record
  STFreeMem (fr);

  // Ébefore disposing of the current
  LHDisposeCell (cellP);
}

/*****************************************************************************/

static void
LHDeleteTiledCell (ListHandler *list, ListCell *deleteCell)
{
  ListCell *previousCell;
  ListCell *nextCell;

  if (deleteCell == NULL)
    {
      TRACE ("passing nil to delete cell");
      return;
    }

  previousCell = deleteCell->lcn_Prev; // get the previous Cell
  nextCell = deleteCell->lcn_Next;     // get the next Cell

  LHDeleteCell (list, deleteCell);

  FullDisposeCell (list, deleteCell);

  // retile cells below the deleted one
  if (nextCell)
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
        {
          int32 scrollV = 0;

          LHSelectCell (gList, lc, true);

          // make sure selected cell is visible

          if (lc->lcn_Bounds.rect_YBottom > list->lh_Bounds.rect_YBottom)
            {
              scrollV
                  = list->lh_Bounds.rect_YBottom - lc->lcn_Bounds.rect_YBottom;
            }
          else if (lc->lcn_Bounds.rect_YTop < list->lh_Bounds.rect_YTop)
            {
              scrollV = list->lh_Bounds.rect_YTop - lc->lcn_Bounds.rect_YTop;
            }

          if (scrollV)
            LHOffsetList (list, 0, scrollV, TRUE);
        }
    }
}

/*****************************************************************************/

static void
UpdateSpaceCels (void)
{
  char spaceString[255];
  int32 usedSize;
  int32 KUsed, KFree, free;

  usedSize = GetUsedSize ();

  free = 32 * 1024 - usedSize;

  EraseTextInCel (gSaverSpaceCel);

  KUsed = usedSize / 1024;

  KFree = 32 - KUsed;

  // sprintf(spaceString, "%dK Used, %dK Free", KUsed, KFree);
  sprintf (spaceString, "%d Used, %d Free", usedSize, 32 * 1024 - usedSize);

  UpdateTextInCel (gSaverSpaceCel, TRUE, spaceString);
}

/*****************************************************************************/

static void
DeleteSelectedFile (void)
{
  ListCell *lc;
  ControlPadEventData cpe;

  lc = LHGetSelectedCell (gList);

  if (!lc) // return if the list is empty
    return;

  // put up a confirmation

  curScreen = 1 - curScreen;

  DrawScreenCels (screens[curScreen], gAlertCel);
  DisplayScreen (screens[curScreen], 0);

  // wait until a or b is hit
  while (GetControlPad (1, true, &cpe) && !(cpe.cped_ButtonBits & ControlA)
         && !(cpe.cped_ButtonBits & ControlC))
    {
    }

  // if a is hit, delete the file
  if (cpe.cped_ButtonBits & ControlA)
    {
      char fileName[256];
      FileRecord *fr;

      fr = (FileRecord *)LHGetCellUserValue (gList, lc);

      sprintf (fileName, "/NVRAM/%s", (*fr).name);

      DeleteFile (fileName);
      cleanupNVRAM = TRUE;

      LHDeleteTiledCell (gList, lc);

      UpdateSpaceCels ();
      UpdateCommentCel ();
    }
}

/*****************************************************************************/

/* this routine tiles a cel across the screen */

static Err
CreateBackgroundCels (void)
{
  int32 numHCopies, numVCopies, totalCopies, iStdWidth, iStdHeight;

  TRACE ("Loading Background\n");

  /* Compute number of copies of this cel (horizontally and vertically)
   * it will take to fill the screen.
   */
  iStdWidth = gBackgroundCel->ccb_Width;
  iStdHeight = gBackgroundCel->ccb_Height;
  numHCopies = (kScreenWidth + iStdWidth - 1) / iStdWidth;
  numVCopies = (kScreenHeight + iStdHeight - 1) / iStdHeight;
  totalCopies = numHCopies * numVCopies;

  if (totalCopies > 1)
    {
      int32 i, iRow, iCol, xPos, yPos;
      CCB *last;
      CCB *ccb;

      // Make enough copies of the cel to fill the screen
      last = gBackgroundCel;
      for (i = 1; i < totalCopies; ++i)
        {
          TRACE ("Duping background cell");
          ccb = (CCB *)STAllocMem (sizeof (CCB), MEMTYPE_CEL);
          if (!ccb)
            return (STORAGETUNER_ERR_NOMEM);

          *ccb = *gBackgroundCel;
          ccb->ccb_NextPtr = NULL;
          ccb->ccb_Flags |= CCB_LAST; // Mark this as not last
          last->ccb_NextPtr = ccb;
          last->ccb_Flags &= ~CCB_LAST;
          last = ccb;
        }

      // Now we have enough cels to fill the screen in a chain pointed to by
      // 'gBackgroundCel'
      xPos = yPos = 0;
      ccb = gBackgroundCel; // Point to first cel in list
      for (iRow = 0; iRow < numVCopies; ++iRow)
        {
          for (iCol = 0; iCol < numHCopies; ++iCol)
            {
              ccb->ccb_XPos = Convert32_F16 (xPos);
              ccb->ccb_YPos = Convert32_F16 (yPos);
              xPos += iStdWidth;      // Increment horz position
              ccb = ccb->ccb_NextPtr; // Move to next cel
            }
          xPos = 0;           // Carridge-return
          yPos += iStdHeight; // Line-feed
        }
    }

  return (0);
}

/*****************************************************************************/

/* this routine cleans up the background
 * the background is a cel and several duplicates
 */
static void
DeleteBackgroundCels (void)
{
  CCB *ccb;
  CCB *next;

  if (gBackgroundCel)
    {
      ccb = gBackgroundCel->ccb_NextPtr;
      while (ccb)
        {
          next = ccb->ccb_NextPtr;
          STFreeMem (ccb);
          ccb = next;
        }
    }
}

/*****************************************************************************/

// custom draw proc for the list manager
// the only difference between this proc
// and the standard proc is that this one
// looks to see if the regular cel points
// to another cel. If it does, this adjusts
// the following cel

static Err
CustomDrawProc (ListHandler *listP, ListCell *cellP, Item bitMapItem)
{
  bool hilited = (cellP->lcn_CellFlags & kCellHilite) != 0;

  // draw the cell's cel(s) if they exist
  if (hilited && (cellP->lcn_CellFlags & kHiliteByCCB))
    if (cellP->lcn_HiliteCCB)
      DrawCels (bitMapItem, cellP->lcn_HiliteCCB);

  // draw the cell's text if it exists
  if (cellP->lcn_Text)
    {
      FileRecord *fr;
      int32 textX, textY;

      textX = cellP->lcn_Bounds.rect_XLeft;
      textY = cellP->lcn_Bounds.rect_YTop;

      fr = (FileRecord *)LHGetCellUserValue (listP, cellP);

      EraseTextInCel (gFileNameCel);
      UpdateTextInCel (gFileNameCel, TRUE, (char *)cellP->lcn_Text);
      SetTextCelCoords (gFileNameCel, textX << 16, textY << 16);

      DrawScreenCels (screens[curScreen], gFileNameCel->tc_CCB);

      EraseTextInCel (gFileSizeCel);
      UpdateTextInCel (gFileSizeCel, TRUE, (char *)fr->sizeString);
      SetTextCelCoords (gFileSizeCel, (textX + kCellWidth) << 16, textY << 16);

      DrawScreenCels (screens[curScreen], gFileSizeCel->tc_CCB);
    }

  return 0;
}

/*****************************************************************************/

typedef struct FileMapping
{
  char *fm_ActualName;
  char *fm_MappedName;
  char *fm_Comment;
} FileMapping;

static FileMapping mappings[] = {
  { "acd.playlist", "Audio CD Programs", NULL },
  { "pcd.playlist", "Photo CD Playlist", NULL },
  { "CNBTESTSAVE", "Crash And Burn", "Tournament Settings" },
  { "MonsterManorData", "Monster Manor", "Game Levels" },
  { "stellar7.fame", "Stellar 7 Scores", "Saved Scores" },
  { "stellar7.dat", "Stellar 7", "Stellar 7 Preferences" },
  { "cpubach.dta", "CPU Bach", "CPU Bach Preferences" },
  { "swc_highscore", "SW Commander", "Super Wing Commander Saved Game" },
  { "maddog", "Mad Dog McCree", NULL },
  { "bc.save", "Battle Chess", NULL },
  { "jrock", "Johnny Rock", NULL },
  { "tim.cfg", "Incredible Machine", "Saved Preferences" },
  { "EA_JMFB94", "John Madden Game", "Saved Game" },
  { "EA_JMFB94 greats", "John Madden Scores", "Saved Scores" },
  { NULL },
};

/* how about doing a binary search here...? */
static char *
MapFileName (char *originalName, char **comment)
{
  uint32 i;
  Node *node;

  *comment = "No comment available";

  if ((originalName[0] == 's') && (originalName[1] == 'g'))
    {
      /* a horde game... */

      node = (Node *)STAllocMem (
          sizeof (Node) + strlen (originalName) + 10 + 1, MEMTYPE_ANY);
      if (node)
        {
          AddTail (&memToFree, node);
          node->n_Name = (char *)&node[1];
          strcpy (node->n_Name, "The Horde ");
          strcat (node->n_Name, &originalName[2]);

          return (node->n_Name);
        }

      return (originalName);
    }

  i = 0;
  while (mappings[i].fm_ActualName)
    {
      if (strcmp (originalName, mappings[i].fm_ActualName) == 0)
        {
          if (mappings[i].fm_Comment)
            *comment = mappings[i].fm_Comment;

          return mappings[i].fm_MappedName;
        }
      i++;
    }

  return (originalName);
}

/*****************************************************************************/

/* this routine creates the list of files */

static Err
CreateFileList (void)
{
  Err err;
  Item rootItem;
  Directory *dir;
  DirectoryEntry de;
  Item ioReqItem;
  int32 result;
  TagArg tags[2];
  ListCell *cellP;
  Rect listRect;

  err = OpenDiskFile ("/NVRAM");
  if (err >= 0)
    {
      rootItem = err;

      tags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
      tags[0].ta_Arg = (void *)rootItem;
      tags[1].ta_Tag = TAG_END;
      err = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), tags);
      if (err >= 0)
        {
          ioReqItem = err;

          dir = OpenDirectoryItem (rootItem);
          if (dir)
            {
              // the location of the list

              listRect.rect_XLeft = kListCelX + 5;
              listRect.rect_YTop = kListCelY + 63 - gFontHeight;
              listRect.rect_XRight = listRect.rect_XLeft + kTotalWidth;
              listRect.rect_YBottom = listRect.rect_YTop + 7 * gFontHeight;

              TRACE ("about to create list handler!");

              err = LHCreateListHandler (
                  &listRect,
                  kMapCCBsAtDrawTime | kAutoScrollV
                      | kClipCellsToBounds, // list flags
                  kNoWrapCellSelection | kSelectOneOnly
                      | kAlwaysSelectOne, // selection flags
                  kHiliteByCCB | 2 | kFont3DO
                      | kScaleCellCCBs, // default cell flags
                  &gList);
              if (err >= 0)
                {
                  LHSetDrawProc (gList, &CustomDrawProc);

                  err = 0;

                  while ((result = ReadDirectory (dir, &de)) >= 0)
                    {
                      FileRecord *fr;
                      char *mappedName;
                      void *fakeGrafCon = NULL;

                      // Add this to the file list

                      TRACE ("about to create tiled cel!");

                      err = LHCreateTiledCell (gList, &cellP, kCellWidth,
                                               gFontHeight);
                      if (err < 0)
                        break;

                      TRACE ("adding cell!");

                      LHAddCell (gList, NULL,
                                 cellP); // NULL == add it to the end

                      // allocate new record and attach it to the user cell

                      TRACE ("creating file record!");

                      fr = (FileRecord *)STAllocMem (sizeof (FileRecord),
                                                     MEMTYPE_ANY);
                      if (!fr)
                        {
                          err = STORAGETUNER_ERR_NOMEM;
                          break;
                        }

                      strncpy (fr->name, de.de_FileName,
                               FILESYSTEM_MAX_NAME_LEN);

                      fr->size = de.de_BlockCount * de.de_BlockSize;

                      LHSetCellUserValue (gList, cellP, (int32)fr);

                      fr->type = de.de_Type;

                      TRACE ("Mapping file name");

                      mappedName = MapFileName (fr->name, &(fr->comment));

                      sprintf (fr->sizeString, "%d", fr->size + 64);

                      TRACE ("LHSetCellText");

                      LHSetCellText (gList, cellP, (GrafCon *)&fakeGrafCon, 8,
                                     mappedName);

                      TRACE ("LHSetCellHiliteCCB");

                      LHSetCellHiliteCCB (gList, cellP, gHighlightCel, true);

                      TRACE ("LHSetCellFormat");

                      LHSetCellFormat (gList, cellP,
                                       kCellJustLeft,   // horiz justification
                                       kCellJustCenter, // vert justification
                                       2,               // hOffset
                                       5);              // vOffset
                    }
                }
              CloseDirectory (dir);
            }
          DeleteItem (ioReqItem);
        }
      CloseDiskFile (rootItem);
    }

  return (err);
}

/*****************************************************************************/

static void
DeleteFileList (ListHandler *listP)
{
  ListCell *cellP;
  ListCell *nextCellP;

  if (listP)
    {
      cellP = listP->lh_ListAnchor;
      while (cellP)
        {
          // get the next one
          nextCellP = cellP;
          LHGetNextCell (listP, &nextCellP);

          FullDisposeCell (listP, cellP);

          cellP = nextCellP;
        }

      listP->lh_ListAnchor = NULL;
      listP->lh_CellCount = 0;

      LHDeleteListHandler (listP);
    }
}

/*****************************************************************************/

/* this function draws all of the cells */

static void
DrawEverything (void)
{
  Screen *sp;
  Item screen;

  screen = screens[curScreen];

  WaitVBL (vblIO, 1);
  DrawScreenCels (screen, gBackgroundCel);
  DrawScreenCels (screen, gListCel);
  DrawScreenCels (screen, gOptionsCel);

  sp = (Screen *)LookupItem (screen);
  LHDraw (gList, sp->scr_TempBitmap->bm.n_Item);

  DrawScreenCels (screen, gSaverNameCel->tc_CCB);
  DrawScreenCels (screen, gSaverSpaceCel->tc_CCB);
  DrawScreenCels (screen, gCommentCel->tc_CCB);
  DisplayScreen (screen, 0);
}

/*****************************************************************************/

static Err
CreateDrawStringCel (void)
{
  gFileNameCel = CreateTextCel (gFileFont, TC_FORMAT_LEFT_JUSTIFY, kCellWidth,
                                gFontHeight);
  if (!gFileNameCel)
    return (STORAGETUNER_ERR_NOMEM);

  SetTextCelMargins (gFileNameCel, kHorizontalMargin, kVerticalMargin);
  SetTextCelColor (gFileNameCel, 0,
                   MakeRGB15 (kNotSoBright, kNotSoBright, kNotSoBright));

  gFileSizeCel = CreateTextCel (gFileFont, TC_FORMAT_LEFT_JUSTIFY, kSizeWidth,
                                gFontHeight);
  SetTextCelMargins (gFileSizeCel, kHorizontalMargin, kVerticalMargin);
  SetTextCelColor (gFileSizeCel, 0,
                   MakeRGB15 (kNotSoBright, kNotSoBright, kNotSoBright));

  return (0);
}

/*****************************************************************************/

static Err
CreateCommentCel (void)
{
  gCommentCel
      = CreateTextCel (gFileFont, TC_FORMAT_WORDWRAP, 175, gFontHeight * 2);
  if (!gCommentCel)
    return (STORAGETUNER_ERR_NOMEM);

  EraseTextInCel (gCommentCel);
  SetTextCelMargins (gCommentCel, kHorizontalMargin, kVerticalMargin);

  SetTextCelCoords (gCommentCel, (kListCelX + 5) << 16,
                    (kListCelY + 5 * 5 + 9 * gFontHeight) << 16);

  UpdateCommentCel ();

  return (0);
}

/*****************************************************************************/

static Err
CreateSpaceCels (void)
{
  gSaverNameCel = CreateTextCel (gFileFont, TC_FORMAT_LEFT_JUSTIFY,
                                 kTotalWidth, gFontHeight);
  if (!gSaverNameCel)
    return (STORAGETUNER_ERR_NOMEM);

  EraseTextInCel (gSaverNameCel);
  SetTextCelMargins (gSaverNameCel, kHorizontalMargin, kVerticalMargin);

  SetTextCelColor (gSaverNameCel, 0,
                   MakeRGB15 (kNotSoBright, kNotSoBright, kNotSoBright));

  UpdateTextInCel (gSaverNameCel, TRUE, "Built-in Storage");

  SetTextCelCoords (gSaverNameCel, (kListCelX + 5) << 16,
                    (kListCelY + 5) << 16);

  gSaverSpaceCel = CreateTextCel (gFileFont, TC_FORMAT_LEFT_JUSTIFY,
                                  kTotalWidth, gFontHeight);

  EraseTextInCel (gSaverSpaceCel);
  SetTextCelMargins (gSaverSpaceCel, kHorizontalMargin, kVerticalMargin);

  SetTextCelColor (gSaverSpaceCel, 0,
                   MakeRGB15 (kNotSoBright, kNotSoBright, kNotSoBright));
  SetTextCelCoords (gSaverSpaceCel, (kListCelX + 5) << 16,
                    (kListCelY + 5 + 16 * 1) << 16);

  UpdateSpaceCels ();

  return (0);
}

/*****************************************************************************/

static void
DeleteSpaceCels (void)
{
  DeleteTextCel (gSaverSpaceCel);
  DeleteTextCel (gSaverNameCel);
}

/*****************************************************************************/

/* invoke the system's NVRAM cleanup utility to defrag and clean things up */
static bool
CleanupNVRAM (void)
{
  Item task;

  task = LoadProgram ("$c/lmadm -a ram 3 0 NVRAM");
  if (task < 0)
    {
      TRACE ("lmadm could not be run\n");
      return FALSE;
    }

  WaitSignal (SIGF_DEADTASK);

  return TRUE;
}

/*****************************************************************************/

static Err
CreateTunerScreen (Item screenGroup)
{
  Err err;
  Screen *screen;
  int32 i;
  TagArg tags[2];
  ScreenGroup *sg;

  sg = (ScreenGroup *)CheckItem (screenGroup, NODE_GRAPHICS, TYPE_SCREENGROUP);
  if (sg)
    {
      i = 0;
      SCANLIST (&sg->sg_ScreenList, screen, Screen)
      {
        screens[i] = screen->scr.n_Item;
        i++;

        if (i == 2)
          break;
      }

      if (i < 2)
        return (STORAGETUNER_ERR_BADSCREENCOUNT);

      return (0);
    }

  tags[0].ta_Tag = CSG_TAG_SCREENCOUNT;
  tags[0].ta_Arg = (void *)NUMSCREENS;
  tags[1].ta_Tag = CSG_TAG_DONE;

  err = CreateScreenGroup (screens, tags);
  if (err >= 0)
    {
      screenGroup = err;

      err = AddScreenGroup (screenGroup, NULL);
      if (err >= 0)
        {
          for (i = 0; i < NUMSCREENS; i++)
            {
              EnableHAVG (screens[i]);
              EnableVAVG (screens[i]);
            }

          deleteScreen = TRUE;

          return 0;
        }
      DeleteScreenGroup (screenGroup);
    }

  return err;
}

/*****************************************************************************/

static void
DeleteTunerScreen (void)
{
  Item screenGroup;
  Screen *screen;

  if (deleteScreen)
    {
      screen = (Screen *)LookupItem (screens[0]);
      if (screen)
        {
          screenGroup = screen->scr_ScreenGroupPtr->sg.n_Item;
          RemoveScreenGroup (screenGroup);
          DeleteScreenGroup (screenGroup);
        }
    }
}

/*****************************************************************************/

static void
DeleteStorageTuner (void)
{
  Node *node;

  DeleteItem (vblIO);

  KillEventUtility ();

  DeleteBackgroundCels ();

  UnloadCel (gBackgroundCel);
  UnloadCel (gListCel);
  UnloadCel (gOptionsCel);
  UnloadCel (gHighlightCel);
  UnloadCel (gAlertCel);
  UnloadCel (gHelpCel);

  UnloadFont (gFileFont);

  DeleteFileList (gList);
  DeleteSpaceCels ();

  DeleteTextCel (gCommentCel);
  DeleteTextCel (gFileNameCel);
  DeleteTextCel (gFileSizeCel);

  DeleteTunerScreen ();

  while (TRUE)
    {
      node = RemHead (&memToFree);
      if (!node)
        break;

      STFreeMem (node);
    }
}

/*****************************************************************************/

static Err
CreateStorageTuner (STParms *stp)
{
  Err err;

  STInitMem (stp->stp_MemoryLists);

  err = CreateTunerScreen (stp->stp_ScreenGroup);
  if (err >= 0)
    {
      err = GetVBLIOReq ();
      if (err >= 0)
        {
          vblIO = err;

          err = InitEventUtility (1, 0, LC_Observer);
          if (err >= 0)
            {
              gBackgroundCel = LoadCel (kTileBackGroundCel, MEMTYPE_CEL);
              gListCel = LoadCel (kListCel, MEMTYPE_CEL);
              gOptionsCel = LoadCel (kOptionsCel, MEMTYPE_CEL);
              gHighlightCel = LoadCel (kHighlightCel, MEMTYPE_CEL);
              gAlertCel = LoadCel (kAlertCel, MEMTYPE_CEL);
              gHelpCel = LoadCel (kHelpCel, MEMTYPE_CEL);

              if (!gBackgroundCel || !gListCel || !gOptionsCel
                  || !gHighlightCel || !gAlertCel || !gHelpCel)
                {
                  return (STORAGETUNER_ERR_CANTLOADCELS);
                }

              gListCel->ccb_XPos = (kListCelX << 16);
              gListCel->ccb_YPos = (kListCelY << 16);
              gOptionsCel->ccb_XPos = (kOptionsCelX << 16);
              gOptionsCel->ccb_YPos = (kOptionsCelY << 16);
              gAlertCel->ccb_XPos = (0 << 16);
              gAlertCel->ccb_YPos = (0 << 16);
              gAlertCel->ccb_HDX = (1 << 20); // full size
              gAlertCel->ccb_VDY = (1 << 16); // full size
              gHelpCel->ccb_XPos = (20 << 16);
              gHelpCel->ccb_YPos = (20 << 16);

              /* we don't care if this one fails, we'll just get a
               * lame background
               */
              CreateBackgroundCels ();

              TRACE ("About to load font");

              gFileFont = LoadFont ((char *)"Sans Serif 14", MEMTYPE_ANY);
              if (!gFileFont)
                {
                  return (STORAGETUNER_ERR_CANTLOADFONT);
                }

              gFontHeight = gFileFont->fd_charHeight + 2;

              err = CreateFileList ();
              if (err >= 0)
                {
                  err = CreateSpaceCels ();
                  if (err >= 0)
                    {
                      err = CreateCommentCel ();
                      if (err >= 0)
                        {
                          err = CreateDrawStringCel ();
                          if (err >= 0)
                            {
                              DrawEverything ();
                              return (0);
                            }
                        }
                    }
                }
            }
        }
    }

  return err;
}

/*****************************************************************************/

int
main (int32 argc, STParms *stp)
{
  ControlPadEventData controlPadEvent;
  int32 result;
  Err err;
  STParms parms;

  if (argc != 0)
    {
      /* started as a stand-alone executable, we ain't got nuttin' in this
       * world... */
      parms.stp_ScreenGroup = -1;
      parms.stp_MemoryLists = NULL;
      stp = &parms;
    }

  vblIO = -1;
  curScreen = 0;
  gList = NULL;
  gFileFont = NULL;
  gBackgroundCel = NULL;
  gListCel = NULL;
  gHighlightCel = NULL;
  gOptionsCel = NULL;
  gAlertCel = NULL;
  gHelpCel = NULL;
  gSaverNameCel = NULL;
  gSaverSpaceCel = NULL;
  gCommentCel = NULL;
  gFileNameCel = NULL;
  gFileSizeCel = NULL;
  cleanupNVRAM = FALSE;
  deleteScreen = FALSE;
  InitList (&memToFree, NULL);

  err = OpenGraphicsFolio ();
  if (err >= 0)
    {
      err = CreateStorageTuner (stp);
      if (err >= 0)
        {
          while (TRUE)
            {
              result = GetControlPad (1, FALSE, &controlPadEvent);

              if ((controlPadEvent.cped_ButtonBits & ControlX)
                  && (result == 1))
                break;

              if ((controlPadEvent.cped_ButtonBits & ControlDown)
                  && (result == 1))
                {
                  LHSelectNextCellByDirection (gList, kDown);
                  UpdateCommentCel ();
                }

              if ((controlPadEvent.cped_ButtonBits & ControlUp)
                  && (result == 1))
                {
                  LHSelectNextCellByDirection (gList, kUp);
                  UpdateCommentCel ();
                }

              if ((controlPadEvent.cped_ButtonBits & ControlA)
                  && (result == 1))
                DeleteSelectedFile ();

              if ((controlPadEvent.cped_ButtonBits & ControlC)
                  && (result == 1))
                {
                  ControlPadEventData cpe;

                  // show the help screen
                  curScreen = 1 - curScreen;

                  DrawScreenCels (screens[curScreen], gHelpCel);
                  DisplayScreen (screens[curScreen], 0);

                  // wait while c is down
                  while (!GetControlPad (1, false, &cpe)
                         && (cpe.cped_ButtonBits & ControlC))
                    {
                    }
                }

              curScreen = 1 - curScreen;
              DrawEverything ();
            }

          FadeToBlack (screens);

          if (cleanupNVRAM)
            CleanupNVRAM ();
        }

      DeleteStorageTuner ();

      //      only available in Portfolio 2.0
      //      CloseGraphicsFolio();
    }

  if (err >= 0)
    err = 0;

  return ((int)err);
}
