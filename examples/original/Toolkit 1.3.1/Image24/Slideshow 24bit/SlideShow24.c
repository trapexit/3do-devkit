/*
        File:		SlideShow24.c

        Contains:	Load and show image files.

        Written by:	eric carlson

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <7>	2/3/94		DSM		Updated for 24 bit
   display modes <6>	 8/11/93	JAY		changed VRAMIOReq to
   gVRAMIOReq and some other minor changes ec did. <5>	 7/30/93	JAY
   add io parameter to SetVRAMPages() and CopyVRAMPages() for Dragon Tail
   release
                 <4>	 6/23/93	ec		Updated for 4B1:
   Removed JOURNALING code, changed parameters to SubmitVDL. Major changes to
   the way we deal with VDLs as the system now makes it's own copy (under the
   fence) when we do a 'SubmitVDL'. Now do a 'DeleteVDL' after submitting a new
   one as each VDL is tied to a specific file/image.
                 <3>	  4/7/93	JAY		MEMTYPE_SPRITE is now
   MEMTYPE_CEL in Cardinal2
                 <2>	  4/5/93	JAY		remove /remote/. add
   ChangeInitialDirectory() <1>	 3/18/93	JAY		first checked
   in 2/17/93		ec		Added VDL diddling options. 2/15/93
   ec		1.1 release, updated for Magneto 6
                                11/16/92	ec		1.0 release

        To Do:
*/

#include "Portfolio.h"
#include "debug.h"
#include "event.h"
#include "filefunctions.h"

#include "Init3DO.h"
#include "Parse3DO.h"
#include "UMemory.h"
#include "Utils3DO.h"

#include "OurVDL.h"
#include "SlideShow24.h"
#include "loadfile24.h"

/*
 * Definitions
 */
#define kVersion "1.1.2"

/*
 * Function Prototypes
 */
int32 ParseArgs (int32 argc, char *argv[]);
int32 ParseScriptFile (char *scriptFileName);
int32 LoadScreen (char *fileName, int32 whichBuffer);
Boolean DoJoystick (void);
void ShowNextImage (void);
void ShowPrevImage (void);
Boolean Init (void);
Boolean OpenMacLink (void);
Boolean OpenSPORT (void);
Boolean DisplayCurrentScreen (void);
int32 ReadPad (void);
void usage (void);
Err DisableV (uint32 which);
Err EnableV (uint32 which);
Err DisableH (uint32 which);
Err EnableH (uint32 which);
void ReportMemoryUsage (void);
/*
 * Data Declarations
 */
ubyte *gBackPic[kBufferCount]; // background image buffers
ImageCC picHeader[kBufferCount];
int32 picSize[kBufferCount];      // Total number of bytes in image
int32 picHasVDL[kBufferCount];    // Boolean flag for VDL presence
int32 picIndex[kBufferCount];     // file name index for this buffer
int32 gCurrentImage = 0,          // current image #
    gImageCount = 0,              // # of images we show
    gShowImageTime,               // length each image is shown in autoshow
    gImageVisTime = 60 * 4;       // default autoshow = 4 sec per image
Boolean gAutoShowFlag = FALSE,    // autoshow?
    gNextBuffGood,                // next buffer is good?
    gPrevBuffGood;                // have we shown the first image yet? [HACK}
char *gFileNames[256];            // list of image file names
ScreenContext gScreenCntx;        // screen context structures
Item gVDLItems[kScreenCount + 2]; // actual VDL items
VDLEntry *gVDLPtrArray;           // our raw VDL
Item gVRAMIOReq;                  // Item for Set/CopyVRAMPages
Item defaultVDL[kScreenCount];    // Items for System's default VDL
Boolean g24BitMode = false;

uint32 maxImageSize;      // size for a 32 bit image
uint32 minImageSize;      // size for a 16 bit image
int32 curbuf = -1;        // which image buffer is current
int32 prevbuf = -1;       // which image buffer is current
int32 VAVG[kBufferCount]; // enabling vertical averaging
int32 HAVG[kBufferCount]; // enabling vertical averaging
/*   TagArgs */

TagArg EVAVGTags[] = {
  CREATEVDL_TAG_VAVG,
  (void *)1,
  CREATEVDL_TAG_DONE,
  (void *)0,
};

TagArg DVAVGTags[] = {
  CREATEVDL_TAG_VAVG,
  (void *)0,
  CREATEVDL_TAG_DONE,
  (void *)0,
};

TagArg EHAVGTags[] = {
  CREATEVDL_TAG_HAVG,
  (void *)1,
  CREATEVDL_TAG_DONE,
  (void *)0,
};

TagArg DHAVGTags[] = {
  CREATEVDL_TAG_HAVG,
  (void *)0,
  CREATEVDL_TAG_DONE,
  (void *)0,
};

/*
 * Main Routines
 */
int
main (int argc, char *argv[])
{
  int32 retValue, bufCount;

  //	Init the machine and get ready to run.
  if (!Init ())
    goto DONE;

  // parse the params, bail if we don't like them
  if (!ParseArgs (argc, argv))
    goto DONE;

  // load up the two frame buffers with the first two pictures, but don't show
  // them yet or
  //  we'll flash from one to the other
  gCurrentImage = -1;
  gScreenCntx.sc_curScreen = -1;
  gNextBuffGood = FALSE;
  for (bufCount = 0; bufCount < kBufferCount; bufCount++)
    ShowNextImage ();

  // and point to the first buffer/image
  curbuf = 0;
  gCurrentImage = 0;
  gScreenCntx.sc_curScreen = 0;
  gNextBuffGood = TRUE;
  gPrevBuffGood = FALSE;
  gShowImageTime = TICKCOUNT () + gImageVisTime;

  while (TRUE)
    {
      // React to the joystick
      if (NOT DoJoystick ())
        goto DONE;

      // make sure we show whatever changes might have happened
      if (!DisplayCurrentScreen ())
        retValue = FALSE;

      // are we in autoshow mode???
      if (gAutoShowFlag)
        {
          // check the time to see if it's time to do another round
          if (gShowImageTime < TICKCOUNT ())
            {
              ShowNextImage ();
              gShowImageTime = TICKCOUNT () + gImageVisTime;
            }
        }
    }

DONE:

  // we don't REALLY want to fade to black, but we need the pause so RJ's menu
  // program doesn't
  //  see the user holding down the quit key
  FadeToBlack (&gScreenCntx, 56);

  ShutDown ();
  printf ("\nSlideShow24 has completed!\n");
  return ((int)retValue);
}

int32
ParseScriptFile (char *scriptFileName)
{
  int32 fileSize, bufferSize, result, lineLen;
  char *scriptBuffer = NULL;
  char *scanChar, *fileName;

  // determine file size, alloc memory (grab an extra byte so the last file
  // name can
  //	have a null terminator), and read the file into memory
  fileSize = GetFileSize (scriptFileName);
  if (fileSize <= 0)
    {
      printf ("Can't find script file %s.\n", scriptFileName);
      return 1;
    }
  printf ("Found %s, %d bytes long (%x)...\n", scriptFileName, fileSize,
          &fileSize);

  // allocate a buffer large enough to read in the file, zero it out, and read
  // away
  scriptBuffer = (char *)ALLOCMEM (fileSize + 1, 0);
  if (scriptBuffer == NULL)
    {
      printf ("Not enough memory to read script file.");
      return 1;
    }
  memset (scriptBuffer, 0, fileSize + 1);
  result = ReadFile (scriptFileName, fileSize, (int32 *)scriptBuffer, 0);
  if (result < 0)
    {
      printf ("Error %d reading script file...\n", result);
      return result;
    }

  // now parse through the file, grabbing a pointer to each file name we find
  gImageCount = -1;
  bufferSize = fileSize;
  scanChar = scriptBuffer;
  fileName = scanChar;
  while (bufferSize-- > 0)
    {
      // if this char is a return, change it to a null and see if the last run
      // of
      //  chars specified a valid file name
      if ((*scanChar == '\r') || (*scanChar == '\n') || (*scanChar == '\0'))
        {
          *scanChar = '\0';
          lineLen = strlen (fileName);
          if ((lineLen == 0) || (*fileName == '\r'))
            ; // empty line, do nothing
          else if (fileName[0] == '-')
            {
              if ((fileName[1] == 'a') || (fileName[1] == 'A'))
                {
                  gAutoShowFlag = TRUE;
                  gImageVisTime = strtol (&fileName[3], NULL, 0) * 60;
                  if (gImageVisTime == 0)
                    gImageVisTime = 180;
                  printf ("autoshow mode, new image every %d seconds\n",
                          gImageVisTime / 60);
                }
              else
                printf ("Don't understand %s option.\n", fileName);
            }
          else if (GetFileSize (fileName) <= 0)
            printf ("Hmm, can't find %s...\n", fileName);
          else
            {
              printf ("Found %s...\n", fileName);
              gImageCount++;
              gFileNames[gImageCount] = fileName;
            }

          //  bump the buffer pointer and remember the start of this file name
          scanChar++;
          --bufferSize;
          fileName = scanChar;
        }
      scanChar++;
    }

  // hope that we have at least one valid image name by now
  if (gImageCount < 0)
    {
      printf ("No valid image names!  What's going on here?");
      return 1;
    }

  // bump the image count so it is 1 based, get on with life
  printf ("\n");
  gImageCount++;
  return noErr;
}

/*
 * Parse the command line parameters.  When finished with this routine we
 *  will know how many files to loop through and the auto slide show options
 */
int32
ParseArgs (int32 argc, char **argv)
{
  int32 argNdx;
  Boolean haveFileName = FALSE;
  char *progName, *cmdLineParam;

  // print our name, sanity check the param count.  we must have at least the
  // script
  //  file name
  progName = *argv;
  printf ("%s %s %s\n", progName, kVersion, kBuildDate);
  if (argc < 2)
    {
      if (ParseScriptFile ("imagelist") != noErr)
        return FALSE;
      haveFileName = TRUE;
    }

  for (argNdx = 1; argNdx < argc; argNdx++)
    {
      cmdLineParam = argv[argNdx];

      if ((*cmdLineParam != '-') && (*cmdLineParam != '+'))
        {
          // this param  should be the name of the script file, make sure we
          // don't
          //  already have one
          if (haveFileName)
            {
              usage ();
              return FALSE;
            }
          if (ParseScriptFile (argv[argNdx]) != noErr)
            return FALSE;
          haveFileName = TRUE;
        }
      else
        {
          switch (cmdLineParam[1])
            {
            case 'A':
            case 'a':
              gAutoShowFlag = TRUE;
              cmdLineParam = argv[++argNdx];
              gImageVisTime = strtol (cmdLineParam, NULL, 0) * 60;
              if (gImageVisTime == 0)
                gImageVisTime = 180;
              printf (
                  "Auto running mode, each image is shown for %d seconds.\n",
                  gImageVisTime / 60);
              break;

            default:
              printf ("### Unknown option: %s\n", cmdLineParam);
              usage ();
              return FALSE;
            }
        }
    }
  if (haveFileName == FALSE)
    {
      if (ParseScriptFile ("imagelist") != noErr)
        return FALSE;
      haveFileName = TRUE;
    }

  return TRUE;
}

void
printVDL_ENTRY (VDLEntry *vdle)
{
  printf ("DMAControlWord	 %x \n", *vdle++);
  printf ("DMAWord2			 %x \n", *vdle++);
  printf ("DMAWord3 		%x \n", *vdle++);
  printf ("DMADisplayWord 	%x \n", *vdle++);
}
void
printVDL (VDL *v)
{

  printf ("VDLEntry     %x \n", v->vdl_DataPtr);
  printf ("vdl_Type     %x \n", v->vdl_Type);
  printf ("vdl_DataSize %x \n", v->vdl_DataSize);
  printf ("vdl_Height %x \n", v->vdl_Height);
  printf ("vdl_Flags %x \n", v->vdl_Flags);
  printf ("vdl_Offset %x \n", v->vdl_Offset);
  printVDL_ENTRY ((v->vdl_DataPtr));
}

/*
 * Load an image from disk into the non-visible buffer
 */
int32
LoadScreen (char *fileName, int32 whichBuffer)
{
  int32 sucessfulLoad = FALSE;
  VdlChunk *rawVDLPtr = NULL;
  char imageFileName[64];
  Item oldVDL;
  int32 bpp, vdllines; // bits per pixel
  VDL *oldVDLptr;

  /*
   * Extract the imagefile name from the arguments passed into the program.
   *	NOTE: the imagefile name is string copied into a local variable so
   *	that it will start on a longword boundry. Any strings that are passed
   *  to the O/S must start on a longword boundry.
   */
  strcpy (imageFileName, fileName);

  if ((loadfile24 (imageFileName, gBackPic[whichBuffer], maxImageSize,
                   MEMTYPE_ANY, &rawVDLPtr, &picHeader[whichBuffer]))
      < 0)
    {
      printf ("LoadImage failed\n");
      goto DONE;
    }
  bpp = picHeader[whichBuffer].bitsperpixel;
  if (bpp <= 16)
    picSize[whichBuffer] = minImageSize;
  else
    picSize[whichBuffer] = maxImageSize;

  // first, init the raw VDL we hang onto to point to the correct screen
  InitVDL (gVDLPtrArray, gScreenCntx.sc_Bitmaps[whichBuffer]);

  // if we loaded a raw VDL, process it, shove it into memory, and toss the
  //  memory allocated for us by LoadImage
  picHasVDL[whichBuffer] = FALSE;
  vdllines = kScreenHeight;
  if ((rawVDLPtr != NULL) && (rawVDLPtr->chunk_size))
    {
      // move the new data into video ram
      vdllines = rawVDLPtr->vdlcount;
      MergeVDL ((VDLEntry *)&(rawVDLPtr->vdl[0]), gVDLPtrArray, vdllines);

      // done with that, free up the memory
      FREEMEM (rawVDLPtr, rawVDLPtr->chunk_size);
      picHasVDL[whichBuffer] = vdllines;
      // our version of the VDL has been updated, hand it off to the system...
      oldVDL = gVDLItems[whichBuffer];
      gVDLItems[whichBuffer] = SubmitVDL (
          gVDLPtrArray, ((vdllines * sizeof (VDL_REC)) / 4), VDLTYPE_FULL);
      if (gVDLItems[whichBuffer] < 0)
        {
          printf ("Error:  SubmitVDL() failed with error code %d\n",
                  gVDLItems[whichBuffer]);
          PrintfSysErr (gVDLItems[whichBuffer]);
          goto DONE;
        }
    }
  else
    {
      gVDLItems[whichBuffer] = defaultVDL[whichBuffer];
    }

  oldVDL
      = SetVDL (gScreenCntx.sc_Screens[whichBuffer], gVDLItems[whichBuffer]);
  if (oldVDL < 0)
    {
      printf ("Error: SetVDL() failed with error code %d\n", oldVDL);
      PrintfSysErr (oldVDL);
      goto DONE;
    }

  // and finally, toss the old VDL as it isn't being used anymore
  oldVDLptr = (VDL *)LookupItem (oldVDL);
  if ((oldVDL != defaultVDL[0]) && (oldVDL != defaultVDL[1]))
    DeleteVDL (oldVDL);

  HAVG[whichBuffer] = TRUE;
  VAVG[whichBuffer] = TRUE;
  sucessfulLoad = TRUE;

DONE:
  return sucessfulLoad;
}

bool EventUtilityInited = 0;

/*
 * Returns non-FALSE if user didn't press START, else returns FALSE to quit
 */
Boolean
DoJoystick (void)
{

  Boolean retValue = TRUE;
  uint32 joyBits;
  Err err;
  ControlPadEventData padData;
  if ((err = GetControlPad (1, 0, &padData)) < 0)
    {
      PrintfSysErr (err);
    }
  else if (err == 1)
    {
      joyBits = (long)padData.cped_ButtonBits;
    }
  else
    {
      return (retValue);
    }

  /*
   * If the user has pressed the START button on the joystick
   *  then return FALSE
   */
  if (joyBits & ControlStart)
    {
      retValue = FALSE;
      goto DONE;
    }

  if (joyBits & ControlRightShift)
    {
      if (VAVG[curbuf])
        {
          VAVG[curbuf] = FALSE;
          if (!DisableV (curbuf))
            printf (" Vertical Averaging Disabled \n");
          else
            printf (" Failed to Disable Vertical Averaging\n");
        }
      else
        {
          VAVG[curbuf] = TRUE;
          if (!EnableV (curbuf))
            printf (" Vertical Averaging Enabled \n");
          else
            printf (" Failed to Enable Vertical Averaging\n");
        }
      goto DONE;
    }

  if (joyBits & ControlLeftShift)
    {
      if (HAVG[curbuf])
        {
          HAVG[curbuf] = FALSE;
          if (!DisableH (curbuf))
            printf (" Horizontal Averaging Disabled \n");
          else
            printf (" Failed to Disable Horizontal Averaging\n");
        }
      else
        {
          HAVG[curbuf] = TRUE;
          if (!EnableH (curbuf))
            printf (" Horizontal Averaging Enabled \n");
          else
            printf (" Failed to Enable Horizontal Averaging\n");
        }
      goto DONE;
    }

  // if FIRE B is pressed, display the other buffer
  if ((joyBits & ControlB))
    {
      curbuf = 1 - curbuf;
      goto DONE;
    }

  // the select button (Button C) is pressed toggle autoshow
  if (joyBits & ControlC)
    {
      gAutoShowFlag = !gAutoShowFlag;

      printf ("Auto-show mode is ");
      if (gAutoShowFlag)
        printf ("TRUE\n");
      else
        printf ("FALSE\n");

      goto DONE;
    }

  // the A button (Button A) is pressed
  if (joyBits & ControlA)
    {
      ReportMemoryUsage ();
    }

  // direction keys switch picture and buffer
  if (joyBits & (ControlUp | ControlDown | ControlLeft | ControlRight))
    {
      if ((joyBits & ControlUp))
        {
          ShowPrevImage ();
        }
      else if ((joyBits & ControlLeft))
        {
          ShowPrevImage ();
        }
      else if ((joyBits & ControlDown))
        {
          ShowNextImage ();
        }
      else if ((joyBits & ControlRight))
        {
          ShowNextImage ();
        }
    }

DONE:

  return (retValue);
}

void
ShowPrevImage (void)
{
  // show the prevous image.
  gCurrentImage--;
  if (gCurrentImage < 0)
    gCurrentImage = gImageCount - 1;

  curbuf--;
  if (curbuf < 0)
    curbuf = kBufferCount - 1;

  // don't actually load another image unless we're showing the first one
  if (!gPrevBuffGood)
    {
      LoadScreen (gFileNames[gCurrentImage], curbuf);
      picIndex[curbuf] = gCurrentImage;
    }
  gNextBuffGood = TRUE;
  gPrevBuffGood = FALSE;
}

void
ShowNextImage (void)
{
  // next image, next buffer
  gCurrentImage++;
  if (gCurrentImage >= gImageCount)
    gCurrentImage = 0;

  curbuf++;
  if (curbuf >= kBufferCount)
    curbuf = 0;

  // don't actually load another image unless the next buffer isn't valid
  if (!gNextBuffGood)
    {
      LoadScreen (gFileNames[gCurrentImage], curbuf);
      picIndex[curbuf] = gCurrentImage;
    }

  gNextBuffGood = FALSE;
  gPrevBuffGood = TRUE;
}

uint32 bmWidths[kScreenCount] = { kScreenWidth, kScreenWidth };
uint32 bmHeights[kScreenCount] = { kScreenHeight, kScreenHeight };
ubyte *bitMapBuffs[kScreenCount];
TagArg screenTags24[] = {
  CSG_TAG_DISPLAYHEIGHT,
  (void *)kScreenHeight,
  CSG_TAG_SCREENCOUNT,
  (void *)kScreenCount,
  CSG_TAG_SCREENHEIGHT,
  (void *)kScreenHeight,
  CSG_TAG_BITMAPCOUNT,
  (void *)1,
  CSG_TAG_BITMAPWIDTH_ARRAY,
  (void *)bmWidths,
  CSG_TAG_BITMAPHEIGHT_ARRAY,
  (void *)bmHeights,
  CSG_TAG_BITMAPBUF_ARRAY,
  (void *)bitMapBuffs,
  CSG_TAG_SPORTBITS,
  (void *)(0),
  CSG_TAG_DONE,
  (void *)0,
};

int32
CreateScreens (void)
{
  Item screenGroupItem;
  long screenNdx;
  int32 *vdl1, *vdl2, errorCode = 0;
  Screen *screen;
  ubyte *buffPtr;

  //	Allocate VDL memory: 2 x 3 lines of 40 longs each
  vdl1 = (int32 *)ALLOCMEM (2 * (3 * 40 * sizeof (int32)), MEMTYPE_ANY);
  vdl2 = vdl1 + (30 * 4);
  if ((!vdl1) || (!vdl2))
    {
      printf ("Error - unable to allocate VDL memory\n");
      return -1;
    }

  gScreenCntx.sc_nFrameBufferPages
      = (kScreenWidth * kScreenHeight * 2 + GrafBase->gf_VRAMPageSize - 1)
        / GrafBase->gf_VRAMPageSize;
  gScreenCntx.sc_nScreens = 2;
  gScreenCntx.sc_nFrameByteCount
      = gScreenCntx.sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize;
  gScreenCntx.sc_curScreen = 0;

  //	Allocate memory for the screen bitmap
  buffPtr = (ubyte *)ALLOCMEM (
      (int)(2 * gScreenCntx.sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize),
      MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);
  if (buffPtr == NULL)
    {
      printf ("Error - unable to allocate screen memory\n");
      return -1;
    }

  // the standard clut vld uses one 152k bitmap per screen
  bitMapBuffs[0] = buffPtr;
  bitMapBuffs[1]
      = buffPtr
        + (gScreenCntx.sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize);
  screenTags24[7].ta_Arg = (void *)GETBANKBITS (GrafBase->gf_ZeroPage);
  screenGroupItem
      = CreateScreenGroup (&(gScreenCntx.sc_Screens[0]), screenTags24);
  if (screenGroupItem < 0)
    {
      printf ("Cannot create screen group: %x", screenGroupItem);
      PrintfSysErr (screenGroupItem);
      return -1;
    }

  for (screenNdx = 0; screenNdx < kScreenCount; screenNdx++)
    {
      screen = (Screen *)LookupItem (gScreenCntx.sc_Screens[screenNdx]);
      if (screen == NULL)
        {
          printf ("Failed in LookupItem for screengroup...\n");
          return -1;
        }
      gScreenCntx.sc_BitmapItems[screenNdx]
          = screen->scr_TempBitmap->bm.n_Item;
      gScreenCntx.sc_Bitmaps[screenNdx] = screen->scr_TempBitmap;
      EnableHAVG (gScreenCntx.sc_Screens[screenNdx]);
      EnableVAVG (gScreenCntx.sc_Screens[screenNdx]);
    }

  // allocate memory for the raw VDL, initialize it.  Fail if we can't get the
  // memory
  if (AllocateVDL (&gVDLPtrArray, gScreenCntx.sc_Bitmaps[0]) != noErr)
    return -1;

  for (screenNdx = 0; screenNdx < kScreenCount; screenNdx++)
    {
      // first, init the raw VDL we hang onto to point to the correct screen
      InitVDL (gVDLPtrArray, gScreenCntx.sc_Bitmaps[screenNdx]);

      // SubmitVDL wants it's size arg in WORDS
      gVDLItems[screenNdx]
          = SubmitVDL (gVDLPtrArray, (kVDLSize / 4), VDLTYPE_FULL);
      if (gVDLItems[screenNdx] < 0)
        {
          printf ("Error: SubmitVDL() failed with error code %d\n",
                  gVDLItems[0]);
          PrintfSysErr (gVDLItems[screenNdx]);
          return -1;
        }

      // got the 'real' vdl, now make it current
      errorCode
          = SetVDL (gScreenCntx.sc_Screens[screenNdx], gVDLItems[screenNdx]);
      if (errorCode < 0)
        {
          printf ("Error: SetVDL() failed with error code %d\n", errorCode);
          PrintfSysErr (errorCode);
          return -1;
        }
      else
        {
          defaultVDL[screenNdx] = errorCode;
        } // Save the System default VDL's
    }

  // now... for 'simplicity' sake we'll create two new screens with 24-bit
  //  VDLs.  Rather than allocating new bitmaps for the screens, we'll
  //  simply repurpose the two we already have - all we need is new a
  //  couple of new VDLs
  gScreenCntx.sc_nScreens += 2;

  // both screens use the same bitmap buffer in 24-bit mode
  bitMapBuffs[0] = buffPtr;
  bitMapBuffs[1] = buffPtr;

  //	create the screen group
  screenGroupItem
      = CreateScreenGroup (&(gScreenCntx.sc_Screens[2]), screenTags24);
  if (screenGroupItem < 0)
    {
      printf ("Failed in CreateScreenGroup: %x\n", screenGroupItem);
      PrintfSysErr (screenGroupItem);
      return -1;
    }
  AddScreenGroup (screenGroupItem, NULL);

  for (screenNdx = 2; screenNdx < 4; screenNdx++)
    {
      screen = (Screen *)LookupItem (gScreenCntx.sc_Screens[screenNdx]);
      if (screen == 0)
        {
          printf ("Failed in CreateScreenGroup: %x\n", screenGroupItem);
          PrintfSysErr (screenGroupItem);
          return -1;
        }
      gScreenCntx.sc_BitmapItems[screenNdx]
          = screen->scr_TempBitmap->bm.n_Item;
      gScreenCntx.sc_Bitmaps[screenNdx] = screen->scr_TempBitmap;
    }

  // So...  after all of that we have a single, double height screen.  now call
  // out
  //  and get a simple vdl for this screen.  we want to point each to the start
  //  of the single large bitmap we allocated
  InitVDL480 (vdl1, gScreenCntx.sc_Bitmaps[2], 0);
  InitVDL480 (vdl2, gScreenCntx.sc_Bitmaps[3], 1);

  // link them together, terminate with the magic postdisplay in grafbase
  vdl1[83] = (int32)vdl2;
  vdl1[123] = (int32)GrafBase->gf_VDLPostDisplay;

  gVDLItems[2] = SubmitVDL ((VDLEntry *)vdl1, 30 * 4, VDLTYPE_FULL);
  if (gVDLItems[2] < 0)
    {
      printf ("Error in SubmitVDL  (%lx)\n", gVDLItems[2]);
      PrintfSysErr (gVDLItems[2]);
      return -1;
    }

  gVDLItems[3] = SubmitVDL ((VDLEntry *)vdl2, 30 * 4, VDLTYPE_FULL);
  if (gVDLItems[3] < 0)
    {
      printf ("Error in SubmitVDL  (%lx)\n", gVDLItems[3]);
      PrintfSysErr (gVDLItems[3]);
      return -1;
    }
  (void)SetVDL (gScreenCntx.sc_Screens[2], gVDLItems[2]);
  (void)SetVDL (gScreenCntx.sc_Screens[3], gVDLItems[3]);
  // free up the raw vdl data
  FREEMEM (vdl1, 6 * 40 * 4);
  return noErr;
}

/*
 * This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns non-FALSE if all is well, FALSE if error
 */
Boolean
Init (void)
{
  int32 screenNdx;
  Err err;

  if (ChangeInitialDirectory (NULL, NULL, false) < 0)
    {
      DIAGNOSTIC ("Cannot set initial working directory ");
      return FALSE;
    }

  if (OpenGraphicsFolio () < 0)
    {
      printf ("Failed to open the graphics folio");
      return FALSE;
    }

  if (!OpenSPORT ())
    {
      printf ("Cannot open DMA channel");
      return FALSE;
    }
  if (!OpenMacLink ())
    {
      // complain but don't die, we might be running without the mac
      printf ("Cannot communicate with Mac");
    }

  gVRAMIOReq = GetVRAMIOReq ();

  if (CreateScreens () != 0)
    return false;

  // now allocate some memory for the images buffers
  for (screenNdx = 0; screenNdx < kBufferCount; screenNdx++)
    {

      /* gBackPic[screenNdx] = (ubyte *)ALLOCMEM(
                                                              (int32)(gScreenCntx.sc_nFrameByteCount),
                                                              GETBANKBITS(GrafBase->gf_ZeroPage)
                                                              |
         MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

      */
      maxImageSize = 2 * (int32)(gScreenCntx.sc_nFrameByteCount);
      minImageSize = (int32)(gScreenCntx.sc_nFrameByteCount);
      gBackPic[screenNdx] = (ubyte *)ALLOCMEM (maxImageSize, MEMTYPE_DRAM);

      if (gBackPic[screenNdx] == NULL)
        {
          printf ("unable to allocate gBackPic[%d]", screenNdx);
          goto DONE;
        }
      // initialize it to black
      // SetVRAMPages( gVRAMIOReq, gBackPic[screenNdx], 0,
      // gScreenCntx.sc_nFrameBufferPages, -1);
      memset (gBackPic[screenNdx], 0,
              2 * (int32)(gScreenCntx.sc_nFrameByteCount));
    }

  if ((err = InitEventUtility (1, 0, 1)) < 0)
    {
      PrintfSysErr (err);
      goto DONE;
    }
  else
    {
      //			printf( "ReadControlPad Inited\n" );
      EventUtilityInited = true;
    }

  return TRUE;

DONE:
  return FALSE;
}

Err
DisableV (uint32 which)
{
  Err result = 0;
  if (picSize[which] == minImageSize)
    {
      result = ModifyVDL (gVDLItems[which], DVAVGTags);
    }
  else
    {
      result = ModifyVDL (gVDLItems[2], DVAVGTags);
      result |= ModifyVDL (gVDLItems[3], DVAVGTags);
    }
  return result;
}

Err
EnableV (uint32 which)
{
  Err result = 0;
  if (picSize[which] == minImageSize)
    {
      result = ModifyVDL (gVDLItems[which], EVAVGTags);
    }
  else
    {
      result = ModifyVDL (gVDLItems[2], EVAVGTags);
      result |= ModifyVDL (gVDLItems[3], EVAVGTags);
    }
  return result;
}

Err
DisableH (uint32 which)
{
  Err result = 0;
  if (picSize[which] == minImageSize)
    {
      result = ModifyVDL (gVDLItems[which], DHAVGTags);
    }
  else
    {
      result = ModifyVDL (gVDLItems[2], DHAVGTags);
      result |= ModifyVDL (gVDLItems[3], DHAVGTags);
    }
  return result;
}

Err
EnableH (uint32 which)
{
  Err result = 0;
  if (picSize[which] == minImageSize)
    {
      result = ModifyVDL (gVDLItems[which], EHAVGTags);
    }
  else
    {
      result = ModifyVDL (gVDLItems[2], EHAVGTags);
      result |= ModifyVDL (gVDLItems[3], EHAVGTags);
    }
  return result;
}

Boolean
DisplayCurrentScreen (void)
{
  int32 retValue;

  if (prevbuf == curbuf)
    return TRUE; // already showing this buffer
  if (picSize[curbuf] == minImageSize)
    {

      memcpy (gScreenCntx.sc_Bitmaps[curbuf]->bm_Buffer, gBackPic[curbuf],
              picSize[curbuf]);
      /*
       * Point the 3DO screen to the screen we just filled. The 3DO
       *	hardware will now draw this screen to the TV every VBlank.
       */
      retValue = DisplayScreen (gScreenCntx.sc_Screens[curbuf], 0);
      if (retValue < 0)
        {
          printf ("DisplayScreen() failed, error=%d\n", retValue);
          return FALSE;
        }
      if (picHasVDL[curbuf])
        printf (" 16 bit Image with %3d line Custom VDL  %s \n",
                picHasVDL[curbuf], gFileNames[picIndex[curbuf]]);
      else
        printf (" 16 bit Image with default VDL          %s \n",
                gFileNames[picIndex[curbuf]]);
    }
  else
    {
      // 24bit mode doesn't double buffer
      memcpy (gScreenCntx.sc_Bitmaps[2]->bm_Buffer, gBackPic[curbuf],
              picSize[curbuf]);
      retValue = DisplayScreen (gScreenCntx.sc_Screens[2],
                                gScreenCntx.sc_Screens[3]);
      if (retValue < 0)
        {
          printf ("DisplayScreen() failed, error=%d\n", retValue);
          return FALSE;
        }
      printf (" 24 bit Image with 24 bit VDL           %s \n",
              gFileNames[picIndex[curbuf]]);
    }

  prevbuf = curbuf;
  return TRUE;
}

void
usage (void)
{
  printf (("\nUsage:\n\tSlideShow scriptFileName\n"));
  printf (("Where <scriptFileName> is a text file with the names of the "
           "images to show\n"));
  printf (("the image files to load from the Mac.  The text file "
           "\"ImageList\" is \n"));
  printf ("used if none is specified.\n");
  printf ("options:\n");
  printf ("\t-a<n>\tshow a new image every <n> seconds.\n");
  exit (1);
}
