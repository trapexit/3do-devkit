/*******************************************************************************************
 *	File: SANMMaker.c
 *
 *	Contains:	Think C source file for program which converts a folder
 *of 3DO cels to a file containing a streamed anim.
 *
 *				NOTE:  Assumes that only cel files are in the
 *folder. Assumes that all cels in an anim share the same CCB.
 *
 *	Written by Jay London
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/19/93	jb		Add gLoopOption and code to handle new
 *checkbox item in the dialog filter proc. Optionally output a looping chunk in
 *the output stream ONLY if this option is selected.
 *	10/8/93		jb		Fix the way we get the arrow cursor by
 *referencing &arrow, the global.
 *	5/21/93		JML		Combined AA and Regular versions into
 *one AA version now has just one PLUT for the AA cels Added status dialog and
 *error checking 4/23/93		JML		New Today.
 *
 *******************************************************************************************/

#include <Controls.h>
#include <Dialogs.h>
#include <Errors.h>
#include <Memory.h>
#include <Menus.h>
#include <OSUtils.h>
#include <Packages.h>
#include <Quickdraw.h>
#include <Resources.h>
#include <StandardFile.h>
#include <ToolUtils.h>
#include <Types.h>
#include <pascal.h>

#include "ImageFile.h"
#include "OperaHW.h"
#include "SANMSubscriber.h"
#include "StandardGetFolder.h"
#include "rsmsg.h"

#ifndef _WaitNextEvent
#define _WaitNextEvent 0xA860
#endif
#ifndef _Unimplemented
#define _Unimplemented 0xA89F
#endif

/* The value of global variables are not preserved between invocations
   of the plug-in, since the plug-in's resource file is closed. To keep
   values around longer, make them part of the following structure. */

#define NIL ((unsigned long)0)
#define kStatusDLOGID 128
#define kErrorDlogID 129

#define kPromptStringID 128
#define kGetFilePromptID 1
#define kPutFilePromptID 2

#define kErrorStringID 129
#define kBadAACelID 1
#define kReadErrID 2
#define kWriteErrID 3
#define kCreateErrID 4
#define kUserCancelledID 5
#define kOutOfMemErrID 6
#define kNotAllCelsID 7

#define PUTFILE_DIALOG_ID 2007
#define CEL_FILE_TYPE '3DOc'
#define SANM_CHUNK_TYPE 'SANM'
#define STREAM_FILE_TYPE '3DOs'
#define SANM_HEADER_TYPE 'AHDR'
#define SANM_CCB_TYPE 'ACCB'
#define SANM_FRAME_TYPE 'FRME'
#define SANM_PLUT_TYPE 'APLT'
#define CREATOR_3DO '3DOe'
#define MAX_BLOCKSIZE 32768

HFileInfo gMyCPB;       // for the PBGetCatInfo call
Str255 gFileName;       // place to hold file name
long gFrames, gChannel; // used in put file dialog to get user input
DialogPtr gMyDialog;    // dialog used to give status during conversion
Boolean gAntiAlias;     // Does streamed anim have anti-alias cels also
Boolean gLoopOption; // Save dialog option to insert/not insert loop in stream
Boolean gWneAvail;   // Is WaitNextEvent available
SFReply gSFReply;
short gOutRefNum; // File refnum for SANM file we create and write to
short gNumFiles, gProcessedFiles;

/***** center a dialog box/window on the main screen *****/

void centerWindow (theResType, theResID) ResType theResType;
short theResID;

{
  Rect **theResource;
  Rect screenRect;
  GrafPtr theScreen;
  short resourceWidth;
  short resourceHeight;

  if ((theResType != 'DLOG') && (theResType != 'ALRT')
      && (theResType != 'WIND'))
    return;

  theResource = (Rect **)GetResource (theResType, theResID);
  if (theResource == NIL)
    return;

  if ((theResType == 'WIND')
      && ((**theResource).top
          != 0)) /* allow windows to have fixed locations */
    return;

  GetWMgrPort (&theScreen);
  screenRect = theScreen->portRect;
  screenRect.top += MBarHeight;

  resourceWidth = (**theResource).right - (**theResource).left;
  resourceHeight = (**theResource).bottom - (**theResource).top;

  (**theResource).left
      = screenRect.left
        + (((screenRect.right - screenRect.left) - resourceWidth) / 2);
  (**theResource).right = (**theResource).left + resourceWidth;

  (**theResource).top
      = screenRect.top
        + (((screenRect.bottom - screenRect.top) - resourceHeight) / 3);
  (**theResource).top = ((**theResource).top <= (MBarHeight + 4))
                            ? (MBarHeight + 4)
                            : (**theResource).top;
  (**theResource).bottom = (**theResource).top + resourceHeight;
}

/***************************************************************************************
 *	ROUTINE:	FrameButton - draws outline around a button to indicate
 *default button INPUT:		Pointer to the dialog and the item number
 *	OUTPUT:		None
 *
 ***************************************************************************************/

void
FrameButton (DialogPtr thedialog, int itemNum)

{
  Rect box;
  short itemType;
  Handle item;

  GetDItem (thedialog, itemNum, &itemType, &item, &box);
  SetPort (thedialog);
  PenSize (3, 3);
  InsetRect (&box, -4, -4);
  FrameRoundRect (&box, 16, 16);
}

void
ErrorAlert (int MessageID, OSErr error, Boolean deleteOutFile,
            Boolean closeInFile)
{
  DialogPtr AlertDLOG;
  short ItemHit, type;
  Rect box;
  Handle hndl;
  Str255 prompt, processStr, numStr;
  OSErr err;

  // Read error string resource
  GetIndString (prompt, kErrorStringID, MessageID);
  if ((MessageID == kReadErrID) || (MessageID == kBadAACelID))
    ParamText (prompt, gMyCPB.ioNamePtr, NIL, NIL);
  else if (MessageID == kNotAllCelsID)
    {
      NumToString (gProcessedFiles, processStr);
      NumToString (gNumFiles, numStr);
      ParamText (prompt, processStr, numStr, NIL);
    }
  else
    ParamText (prompt, NIL, NIL, NIL);

  if (deleteOutFile)
    { // Close and Delete the file we created for SANM
      FSClose (gOutRefNum);
      (void)FSDelete (gSFReply.fName, gSFReply.vRefNum);
    }

  if (closeInFile) // Close the open file we were reading from
    err = PBClose ((ParmBlkPtr)&gMyCPB, false);

  InitCursor ();
  centerWindow ('DLOG', kErrorDlogID);
  SysBeep (0);
  AlertDLOG = GetNewDialog (kErrorDlogID, NIL, (WindowPtr)-1);
  FrameButton (AlertDLOG, 1);
  ModalDialog (NIL, &ItemHit);
  DisposDialog (AlertDLOG);
  exit (error);
}

/***************************************************************************************
 *	ROUTINE:	DisplayMessage
 *	INPUT:		A message record.
 *	OUTPUT:		None
 *	NOTES:		This routine puts a message on the screen either in the
 *counter display box or in an error box, if an error message is specified.
 *
 ***************************************************************************************/

void
DisplayMessage (long total, long current)

{
#define updateItem 2
  Rect updaterect;
  GrafPtr savedport;
  short type;
  Handle hndl;
  Rect box;

  if (gMyDialog == NIL)
    {
      centerWindow ('DLOG', kStatusDLOGID);
      gMyDialog = GetNewDialog (kStatusDLOGID, NIL, (WindowPtr)-1);
      DrawDialog (gMyDialog);
    }
  GetPort (&savedport);
  SetPort (gMyDialog);
  GetDItem (gMyDialog, updateItem, &type, &hndl, &box);
  updaterect = box;
  PenSize (1, 1);
  FrameRect (&updaterect);

  if (total != 0)
    {
      updaterect.left += 1;
      updaterect.top += 1;
      updaterect.bottom -= 1;
      updaterect.right
          = updaterect.left
            + ((updaterect.right - 1 - updaterect.left) * current) / total;
      FillRect (&updaterect, gray);
    }
  InvalRect (&updaterect);
  SetPort (savedport);
}

/***************************************************************************************
 *	ROUTINE:	RemoveMessage
 *	INPUT:		None
 *	OUTPUT:		None
 *	NOTES:		This routine de-allocates the space for the counter in
 *case we forget to call ClearCounter. *
 ***************************************************************************************/

void
RemoveMessage ()
{
  DisposDialog (gMyDialog);
  gMyDialog = NIL;
}

/* Centers a dialog template 1/3 of the way down on the main screen. */

void CenterDialog (dt)

    DialogTHndl dt;

#define menuHeight 20

{
  Rect r;
  short width;
  short height;

  width = qd.screenBits.bounds.right;
  height = qd.screenBits.bounds.bottom;

  r = (**dt).boundsRect;
  OffsetRect (&r, -r.left, -r.top);
  OffsetRect (&r, (width - r.right) / 2,
              (height - r.bottom - menuHeight) / 3 + menuHeight);
  (**dt).boundsRect = r;
}

#undef menuHeight

pascal short
myDlg (short Item, DialogPtr theDialog)
{
#define framesItem 11
#define channelItem 12
#define loopOptionBox 15

  short type;
  Handle hndl;
  Rect box;
  Str255 framesStr, channelStr;

  if (Item == loopOptionBox)
    {
      gLoopOption = !gLoopOption;
      GetDItem (theDialog, loopOptionBox, &type, &hndl, &box);
      SetCtlValue ((ControlHandle)hndl, gLoopOption);
      SetDItem (theDialog, loopOptionBox, type, hndl, &box);
    }

  if (Item == putSave)
    {
      GetDItem (theDialog, framesItem, &type, &hndl, &box);
      GetIText (hndl, framesStr);
      StringToNum (framesStr, &gFrames);
      GetDItem (theDialog, channelItem, &type, &hndl, &box);
      GetIText (hndl, channelStr);
      StringToNum (channelStr, &gChannel);
      if ((gChannel < 0) || (gChannel > MAX_CHANNELS - 1) || (gFrames < 1)
          || (gFrames > 60))
        {
          SysBeep (0);
          Item = 100;
        }
    }
  return Item;
}

short
SaveDialog (void)
{
  Point where;
  DialogTHndl dt;
  SFTypeList typeList;
  OSErr error;
  short refnum;
  Str255 prompt;

  // Center StandardFile dialog
  dt = (DialogTHndl)GetResource ('DLOG', PUTFILE_DIALOG_ID);
  HNoPurge ((Handle)dt);
  CenterDialog (dt);
  where.v = (**dt).boundsRect.top;
  where.h = (**dt).boundsRect.left;
  HPurge ((Handle)dt);

  // Read prompt string resource
  GetIndString (prompt, kPromptStringID, kPutFilePromptID);

  // Put up the StandardFile dialog
  SFPPutFile (where, prompt, "\p", (ProcPtr)myDlg, &gSFReply,
              PUTFILE_DIALOG_ID, NIL);
  if (!gSFReply.good)
    return 0;

  // Delete any old file with the specified name, create a brand new one,
  // and open the new file for writing.
  (void)FSDelete (gSFReply.fName, gSFReply.vRefNum);
  error = Create (gSFReply.fName, gSFReply.vRefNum, CREATOR_3DO,
                  STREAM_FILE_TYPE);
  if (error != noErr)
    return error;

  error = FSOpen (gSFReply.fName, gSFReply.vRefNum, &refnum);
  if (error != noErr)
    return error;
  return refnum;
}

short
GetNumFilesInDirectory (long dirIDToSearch, short vRefNum)
{
  short int index = 1;
  OSErr err;
  short numFiles = 0;

  do
    {
      gMyCPB.ioFDirIndex = index; // set up the index
      gMyCPB.ioVRefNum = vRefNum; // set up vrefnum
      gMyCPB.ioDirID
          = dirIDToSearch; // GetCatInfo returns ioFlNum in this field
      err = PBGetCatInfo ((CInfoPBPtr)&gMyCPB, false);

      if (err == noErr)
        {
          // bit 4 is set if it is a folder
          if (((gMyCPB.ioFlAttrib >> 4) & 0x01) != 1)
            numFiles += 1;
          index += 1;
        }
    }
  while (err == noErr);
  return numFiles;
}

OSErr
SetupCPBForIndexedFile (long dirIDToSearch, short vRefNum, short index)
{
  OSErr err;

  gMyCPB.ioFDirIndex = index;     // set up the index
  gMyCPB.ioVRefNum = vRefNum;     // set up vrefnum
  gMyCPB.ioDirID = dirIDToSearch; // GetCatInfo returns ioFlNum in this field
  //	err= PBHGetFInfo((HParmBlkPtr) &gMyCPB, false);
  err = PBGetCatInfo ((CInfoPBPtr)&gMyCPB, false);
  if (gMyCPB.ioFlFndrInfo.fdType != CEL_FILE_TYPE)
    return -1;
  gMyCPB.ioDirID = dirIDToSearch; // reset this field so PBHOpen works !!!
  return err;
}

/*************************************************************************
 * This routine contains the giant switch statement to handle all events
 *************************************************************************/

Boolean
HandleEvent ()
{
#define escapeKey 0x1B

  OSErr res;
  EventRecord myEvent;
  char theChar;

  if (gWneAvail)
    {
      res = WaitNextEvent (everyEvent, &myEvent, 15, 0);
    }
  else
    {
      SystemTask ();
      res = GetNextEvent (everyEvent, &myEvent);
    }

  switch (myEvent.what)
    {
    case keyDown:
    case autoKey:
      theChar = myEvent.message & charCodeMask;
      if ((myEvent.modifiers & cmdKey) != 0)
        {
          if (theChar == '.') // user hit cmd-period, exit program
            return true;
        }
      else if (theChar == escapeKey)
        return true; // user hit the esc key
      break;
    } /* end switch */
  return false;
}

/********************************************************************
 *	WaitNextEvent Available routine
 **
 ********************************************************************/
Boolean
TrapAvailable (short tNumber, long tType)
{
  /* Check and see if the trap exists. */
  /* On 64K ROM machines, tType will be ignored. */

  return (NGetTrapAddress (tNumber, tType) != GetTrapAddress (_Unimplemented));
}

long
main (void)
{
  short inrefnum;
  StreamAnimHeader sanmHdr;
  OSErr error;
  long numBytes, numFRMEBytes;
  long dirID;
  spritehdr CCBHdr, *CCBHdrPtr;
  StreamAnimCCB sanmCCB;
  StreamAnimFrame sanmFrame;
  short i, vRefNum;
  void *PDATBuffer, *PLUTBuffer, *fillBuffer;
  StandardFileReply mySFReply;
  Point where;
  Str255 prompt;
  DialogTHndl dt;
  long blockSize = 0;
  StreamMarkerChunk markerChunk;
  Cursor watchCursor;
  Boolean userCancelled = false;

  /*-------------------------------------------------------------------------+
 | Initialize all that toolbox stuff.                                       |
 +-------------------------------------------------------------------------*/
  InitGraf (&thePort);
  InitFonts ();
  InitWindows ();
  FlushEvents (everyEvent, 0);
  InitCursor ();
  InitMenus ();
  TEInit ();
  InitDialogs (0L);

  gWneAvail = TrapAvailable (_WaitNextEvent, ToolTrap);
  gMyDialog = NIL;
  gMyCPB.ioNamePtr = gFileName;
  gMyCPB.ioVRefNum = 0;
  gLoopOption = false;
  gProcessedFiles = 0;

  watchCursor = **GetCursor (4);

  gOutRefNum = SaveDialog (); // ask the user to give a file name to create the
                              // SANM file
  if (gOutRefNum <= 0)
    ErrorAlert (kCreateErrID, gOutRefNum, false,
                false); // user cancelled or couldn't create file

  // ask the user for the directory which contains the cels

  // Read prompt string resource
  GetIndString (prompt, kPromptStringID, kGetFilePromptID);
  // Center StandardFile dialog
  dt = (DialogTHndl)GetResource ('DLOG', rGetFolderDialog);
  HNoPurge ((Handle)dt);
  CenterDialog (dt);
  where.v = (**dt).boundsRect.top;
  where.h = (**dt).boundsRect.left;
  HPurge ((Handle)dt);

  /* Get the folder containing the cels */
  StandardGetFolder (where, prompt, &mySFReply);
  dirID = mySFReply.sfFile.parID;
  vRefNum = mySFReply.sfFile.vRefNum;
  gNumFiles = GetNumFilesInDirectory (
      dirID, vRefNum); // how many cel files are in this directory?

  /* Fill in the SANM Header Chunk and write it out */
  numBytes = sizeof (StreamAnimHeader);
  sanmHdr.chunkType = SANM_CHUNK_TYPE;
  sanmHdr.chunkSize = numBytes;
  sanmHdr.time = 0;
  sanmHdr.channel = gChannel;
  sanmHdr.subChunkType = SANM_HEADER_TYPE;
  sanmHdr.version = 0;
  if (gAntiAlias)
    {
      sanmHdr.animType = STREAM_AA_ANIM_TYPE;
      sanmHdr.numFrames = gNumFiles / 2;
    }
  else
    {
      sanmHdr.animType = STREAM_ANIM_TYPE;
      sanmHdr.numFrames = gNumFiles;
    }
  sanmHdr.frameRate = gFrames;
  error = FSWrite (gOutRefNum, (long *)&numBytes, &sanmHdr);
  if (error != noErr)
    ErrorAlert (kWriteErrID, error, true, false);
  blockSize += numBytes;

  SetCursor (&watchCursor); // set the watch cursor
  for (i = 1; i <= gNumFiles; i++)
    {
      DisplayMessage (gNumFiles, i);

      userCancelled = HandleEvent ();
      if (userCancelled)
        goto END;

      if (SetupCPBForIndexedFile (dirID, vRefNum, i) == 0)
        {
          gProcessedFiles++;
          error = PBHOpen ((HParmBlkPtr)&gMyCPB, false);
          if (error != noErr)
            ErrorAlert (kReadErrID, error, true, true);
          inrefnum = gMyCPB.ioFRefNum;
          if (error == noErr)
            {
              if (i == 1)
                {
                  /* Read in the CCB Header and write it out to the SANM file
                   */
                  CCBHdrPtr = &CCBHdr;
                  error = ReadAChunk (inrefnum, tccbHdr, (void **)&CCBHdrPtr);
                  if (error != noErr)
                    ErrorAlert (kReadErrID, error, true, true);
                  memcpy (&sanmCCB.ccb_Flags, &CCBHdr.scb_Flags,
                          sizeof (spritehdr) - 12);
                  numBytes = sizeof (StreamAnimCCB);
                  sanmCCB.chunkType = SANM_CHUNK_TYPE;
                  sanmCCB.chunkSize = numBytes;
                  sanmCCB.channel = gChannel;
                  sanmCCB.time = 0;
                  sanmCCB.subChunkType = SANM_CCB_TYPE;
                  error = FSWrite (gOutRefNum, (long *)&numBytes, &sanmCCB);
                  if (error != noErr)
                    ErrorAlert (kWriteErrID, error, true, true);
                  blockSize += numBytes;
                }

              if (gAntiAlias && (i == 2))
                {
                  /* Read in the CCB Header and write it out to the SANM file
                   */
                  CCBHdrPtr = &CCBHdr;
                  error = ReadAChunk (inrefnum, tccbHdr, (void **)&CCBHdrPtr);
                  if (error != noErr)
                    ErrorAlert (kReadErrID, error, true, true);
                  memcpy (&sanmCCB.ccb_Flags, &CCBHdr.scb_Flags,
                          sizeof (spritehdr) - 12);
                  numBytes = sizeof (StreamAnimCCB);
                  sanmCCB.chunkType = SANM_CHUNK_TYPE;
                  sanmCCB.chunkSize = numBytes;
                  sanmCCB.channel = gChannel + 1;
                  sanmCCB.time = 0;
                  sanmCCB.subChunkType = SANM_CCB_TYPE;
                  error = FSWrite (gOutRefNum, (long *)&numBytes, &sanmCCB);
                  if (error != noErr)
                    ErrorAlert (kWriteErrID, error, true, true);
                  blockSize += numBytes;

                  // read the AA cels' plut, we'll use the same plut for all
                  // frames
                  sanmCCB.chunkType = SANM_CHUNK_TYPE;
                  sanmCCB.channel = gChannel + 1;
                  sanmCCB.time = 0;
                  sanmCCB.subChunkType = SANM_PLUT_TYPE;
                  PLUTBuffer = NIL;
                  error = ReadAChunk (inrefnum, tPLUT, &PLUTBuffer);
                  if (error == eofErr)
                    ErrorAlert (kBadAACelID, error, true, true);
                  if (error != noErr)
                    ErrorAlert (kReadErrID, error, true, true);
                  sanmCCB.chunkSize
                      = GetPtrSize (PLUTBuffer) + sizeof (StreamAnimPLUT);
                  numBytes = sizeof (StreamAnimPLUT);
                  error = FSWrite (gOutRefNum, (long *)&numBytes, &sanmCCB);
                  if (error != noErr)
                    ErrorAlert (kWriteErrID, error, true, true);
                  blockSize += numBytes;
                  numBytes = GetPtrSize (PLUTBuffer);
                  error = FSWrite (gOutRefNum, (long *)&numBytes, PLUTBuffer);
                  if (error != noErr)
                    ErrorAlert (kWriteErrID, error, true, true);
                  blockSize += numBytes;
                  DisposPtr (PLUTBuffer);
                  PLUTBuffer = NIL;
                }

              /* Read in the PDAT data and write out the SANM Frame chunk */
              numFRMEBytes = sizeof (StreamAnimFrame);
              sanmFrame.chunkType = SANM_CHUNK_TYPE;
              sanmFrame.subChunkType = SANM_FRAME_TYPE;
              sanmFrame.duration = (240 / gFrames);
              if (gAntiAlias)
                {
                  if ((i % 2) == 1)
                    {
                      sanmFrame.channel = gChannel;
                      sanmFrame.time = (240 / gFrames) * (i / 2);
                    }
                  else
                    {
                      sanmFrame.channel = gChannel + 1;
                      sanmFrame.time = (240 / gFrames) * ((i - 1) / 2);
                    }
                }
              else
                {
                  sanmFrame.channel = gChannel;
                  sanmFrame.time = (240 / gFrames) * (i - 1);
                }

              PDATBuffer = NIL;
              error = ReadAChunk (inrefnum, tpixels, &PDATBuffer);
              if (error != noErr)
                ErrorAlert (kReadErrID, error, true, true);
              numFRMEBytes += GetPtrSize (PDATBuffer);

              if (gAntiAlias && ((i % 2) == 0))
                { // always check the AA cel for a plut
                  PLUTBuffer = NIL;
                  error = ReadAChunk (inrefnum, tPLUT, &PLUTBuffer);
                  if (error != noErr)
                    ErrorAlert (kBadAACelID, error, true, true);
                  DisposPtr (PLUTBuffer);
                  PLUTBuffer = NIL;
                }
              else
                { // each cel in a non-AA anim can have its own plut
                  PLUTBuffer = NIL;
                  error = ReadAChunk (inrefnum, tPLUT, &PLUTBuffer);
                  if (error == noErr) // not always a PLUT
                    numFRMEBytes += GetPtrSize (PLUTBuffer);
                }

              if (blockSize + numFRMEBytes > MAX_BLOCKSIZE)
                { // we must fill in the 32K block
                  numBytes = MAX_BLOCKSIZE - blockSize;
                  fillBuffer = NewPtrClear (numBytes);
                  if (fillBuffer == NIL)
                    ErrorAlert (kOutOfMemErrID, error, true, true);
                  *(long *)fillBuffer = FILLER_CHUNK_TYPE;
                  *(long *)((char *)fillBuffer + 4) = numBytes;
                  error = FSWrite (gOutRefNum, (long *)&numBytes, fillBuffer);
                  if (error != noErr)
                    ErrorAlert (kWriteErrID, error, true, true);
                  DisposePtr (fillBuffer);
                  blockSize = 0;
                }

              sanmFrame.chunkSize = numFRMEBytes;
              numBytes = sizeof (StreamAnimFrame);
              error = FSWrite (gOutRefNum, (long *)&numBytes, &sanmFrame);
              if (error != noErr)
                ErrorAlert (kWriteErrID, error, true, true);
              blockSize += numBytes;

              numBytes = GetPtrSize (PDATBuffer);
              error = FSWrite (gOutRefNum, (long *)&numBytes, PDATBuffer);
              if (error != noErr)
                ErrorAlert (kWriteErrID, error, true, true);
              DisposPtr (PDATBuffer);
              blockSize += numBytes;

              if (PLUTBuffer != NIL)
                {
                  numBytes = GetPtrSize (PLUTBuffer);
                  error = FSWrite (gOutRefNum, (long *)&numBytes, PLUTBuffer);
                  DisposPtr (PLUTBuffer);
                  blockSize += numBytes;
                }
            }
          error = PBClose ((ParmBlkPtr)&gMyCPB, false);
        }
    }

  /* If the user selected the "loop" checkbox in the Save dialog,
   * then output a loop chunk to loop back to the beginning of the
   * stream.
   */
  if (gLoopOption)
    {
      // Now we add a Marker chunk to seek back to the beginning
      numBytes = sizeof (StreamMarkerChunk);
      markerChunk.chunkType = CTRL_CHUNK_TYPE;
      markerChunk.subChunkType = MARKER_CHUNK_TYPE;
      markerChunk.chunkSize = numBytes;
      markerChunk.time = (240 / gFrames) * gNumFiles;
      markerChunk.channel = 0;
      markerChunk.marker = 0;
      error = FSWrite (gOutRefNum, (long *)&numBytes, &markerChunk);
      if (error != noErr)
        ErrorAlert (kWriteErrID, error, true, false);
      blockSize += numBytes;
    }

  // Now we must pad the last block to 32K
  numBytes = MAX_BLOCKSIZE - blockSize;
  fillBuffer = NewPtrClear (numBytes);
  if (fillBuffer == NIL)
    ErrorAlert (kOutOfMemErrID, error, true, false);
  *(long *)fillBuffer = FILLER_CHUNK_TYPE;
  *(long *)((char *)fillBuffer + 4) = numBytes;
  error = FSWrite (gOutRefNum, (long *)&numBytes, fillBuffer);
  DisposePtr (fillBuffer);
  if (error != noErr)
    ErrorAlert (kWriteErrID, error, true, false);

END:
  FSClose (gOutRefNum);
  if (userCancelled)
    (void)FSDelete (gSFReply.fName, gSFReply.vRefNum);
  if (gNumFiles > 0)
    RemoveMessage ();
  SetCursor (&arrow); // set the arrow cursor
  if (!userCancelled
      && (gProcessedFiles
          != gNumFiles)) // We found files in the folder that were not cels
    ErrorAlert (kNotAllCelsID, 0, false, false);
}