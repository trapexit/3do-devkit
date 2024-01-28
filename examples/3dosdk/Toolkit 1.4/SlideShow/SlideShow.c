/*
        File:		SlideShow.c

        Contains:	Load and show image files.

        Written by:	eric carlson

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 5/09/94	crm		removed
   explicit references to GrafBase struct timing changed from VBL interrupts to
   audio clock
                 <8>	 1/18/94	JAY		explicitely type last
   argument to MergeVDL call. Seems to be causing 1.6v2 armcc some problems <7>
   8/20/93	JAY		unclear <6>	 8/11/93	JAY
   changed VRAMIOReq to gVRAMIOReq and some other minor changes ec did. <5>
   7/30/93	JAY		add io parameter to SetVRAMPages() and
   CopyVRAMPages() for Dragon Tail release <4>	 6/23/93	ec
   Updated for 4B1: Removed JOURNALING code, changed parameters to SubmitVDL.
   Major changes to the way we deal with VDLs as the system now makes it's own
   copy (under the fence) when we do a 'SubmitVDL'. Now do a 'DeleteVDL' after
   submitting a new one as each VDL is tied to a specific file/image. <3>
   4/7/93	JAY		MEMTYPE_SPRITE is now MEMTYPE_CEL in Cardinal2
                 <2>	  4/5/93	JAY		remove /remote/. add
   ChangeInitialDirectory() <1>	 3/18/93	JAY		first checked
   in 2/17/93		ec		Added VDL diddling options. 2/15/93
   ec		1.1 release, updated for Magneto 6
                                11/16/92	ec		1.0 release

        To Do:
*/

#include "Portfolio.h"
#include "debug.h"
#include "filefunctions.h"

#include "Init3DO.h"
#include "Parse3DO.h"
#include "UMemory.h"
#include "Utils3DO.h"

#include "OurVDL.h"
#include "SlideShow.h"

/*
 * Definitions
 */
#define kVersion "1.1.2"
#define kTicksPerSecond 240 // clock ticks per second

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
void usage (void);

/*
 * Data Declarations
 */
ubyte *gBackPic[kBufferCount]; // background image buffers
int32 gCurrentImage = 0,       // current image #
    gImageCount = 0,           // # of images we show
    gShowImageTime,            // length each image is shown in autoshow
    gImageVisTime = kTicksPerSecond * 4; // default autoshow = 4 sec per image
Boolean gAutoShowFlag = FALSE,           // autoshow?
    gNextBuffGood,                       // next buffer is good?
    gPrevBuffGood;            // have we shown the first image yet? [HACK}
char *gFileNames[256];        // list of image file names
ScreenContext gScreenCntx;    // screen context structures
Item gVDLItems[kScreenCount]; // actual VDL items
VDLEntry *gVDLPtrArray;       // our raw VDL
Item gVRAMIOReq;              // Item for Set/CopyVRAMPages

/*
 * Main Routines
 */
int
main (int argc, char *argv[])
{
  int32 retValue, screenCount;

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
  for (screenCount = 0; screenCount < kScreenCount; screenCount++)
    ShowNextImage ();

  // and point to the first buffer/image
  gCurrentImage = 0;
  gScreenCntx.sc_curScreen = 0;
  gNextBuffGood = TRUE;
  gPrevBuffGood = FALSE;
  gShowImageTime = GetAudioTime () + gImageVisTime;

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
          if (gShowImageTime < GetAudioTime ())
            {
              ShowNextImage ();
              gShowImageTime = GetAudioTime () + gImageVisTime;
            }
        }
    }

DONE:

  // we don't REALLY want to fade to back, but we need the pause so RJ's menu
  // program doesn't
  //  see the user holding down the quit key
  FadeToBlack (&gScreenCntx, 56);

  ShutDown ();
  kprintf ("\nSlideShow has completed!  Thanks for shopping with us!\n");
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
      kprintf ("Can't find script file %s.\n", scriptFileName);
      return 1;
    }
  kprintf ("Found %s, %ld bytes long...\n", scriptFileName, fileSize);

  // allocate a buffer large enough to read in the file, zero it out, and read
  // away
  scriptBuffer = (char *)ALLOCMEM (fileSize + 1, 0);
  if (scriptBuffer == NULL)
    {
      kprintf ("Not enough memory to read script file.");
      return 1;
    }
  memset (scriptBuffer, 0, fileSize + 1);
  result = ReadFile (scriptFileName, fileSize, (int32 *)scriptBuffer, 0);
  if (result < 0)
    {
      kprintf ("Error %ld reading script file...\n", result);
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
                  gImageVisTime
                      = strtol (&fileName[3], NULL, 0) * kTicksPerSecond;
                  kprintf ("autoshow mode, new image every %ld seconds\n",
                           gImageVisTime / kTicksPerSecond);
                }
              else
                kprintf ("Don't understand %s option.\n", fileName);
            }
          else if (GetFileSize (fileName) <= 0)
            kprintf ("Hmm, can't find %s...\n", fileName);
          else
            {
              kprintf ("Found %s...\n", fileName);
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
      kprintf ("No valid image names!  What's going on here?");
      return 1;
    }

  // bump the image count so it is 1 based, get on with life
  kprintf ("\n");
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
  kprintf ("%s %s %s\n", progName, kVersion, kBuildDate);
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
              gImageVisTime = strtol (cmdLineParam, NULL, 0) * kTicksPerSecond;
              kprintf (
                  "Auto running mode, each image is shown for %ld seconds.\n",
                  gImageVisTime / kTicksPerSecond);
              break;

            default:
              kprintf ("### Unknown option: %s\n", cmdLineParam);
              usage ();
              return FALSE;
            }
        }
    }

  return TRUE;
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

  /*
   * Extract the imagefile name from the arguments passed into the program.
   *	NOTE: the imagefile name is string copied into a local variable so
   *	that it will start on a longword boundry. Any strings that are passed
   *  to the O/S must start on a longword boundry.
   */
  strcpy (imageFileName, fileName);

  kprintf ("Loading file %s...\n", imageFileName);

  // Load the background image and save a copy of it into the non-visible
  // buffer
  if (LoadImage (imageFileName, gBackPic[whichBuffer], &rawVDLPtr,
                 &gScreenCntx)
      == NULL)
    {
      kprintf ("LoadImage failed\n");
      goto DONE;
    }

  // first, init the raw VDL we hang onto to point to the correct screen
  InitVDL (gVDLPtrArray, gScreenCntx.sc_Bitmaps[whichBuffer]);

  // if we loaded a raw VDL, process it, shove it into memory, and toss the
  //  memory allocated for us by LoadImage
  if ((rawVDLPtr != NULL) && (rawVDLPtr->chunk_size))
    {
      // move the new data into video ram
      MergeVDL ((int32 *)&(rawVDLPtr->vdl[0]), (int32 *)gVDLPtrArray);

      // done with that, free up the memory
      Free (rawVDLPtr);
    }

  // our version of the VDL has been updated, hand it off to the system...
  oldVDL = gVDLItems[whichBuffer];
  gVDLItems[whichBuffer]
      = SubmitVDL (gVDLPtrArray, (kVDLSize / 4), VDLTYPE_FULL);
  if (gVDLItems[whichBuffer] < 0)
    {
      kprintf ("Error:  SubmitVDL() failed with error code %ld\n",
               gVDLItems[whichBuffer]);
      PrintfSysErr (gVDLItems[whichBuffer]);
      goto DONE;
    }

  // activate the new VDL, remember the current VDL so we can toss it when it
  // is no longer
  //  active
  oldVDL
      = SetVDL (gScreenCntx.sc_Screens[whichBuffer], gVDLItems[whichBuffer]);
  if (oldVDL < 0)
    {
      kprintf ("Error: SetVDL() failed with error code %ld\n", oldVDL);
      PrintfSysErr (oldVDL);
      goto DONE;
    }

  // and finally, toss the old VDL as it isn't being used anymore
  DeleteVDL (oldVDL);

  // made it this far?  all must be well in the world... joy
  sucessfulLoad = TRUE;

DONE:
  return sucessfulLoad;
}

/*
 * Returns non-FALSE if user didn't press START, else returns FALSE to quit
 */
Boolean
DoJoystick (void)
{
#define JOYCONTINUOUS                                                         \
  (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT | JOYFIREA | JOYFIREB)

  Boolean retValue = TRUE;
  uint32 joyBits;
  static uint32 gOldJoyBits = 0; // joy bits last time we checked

  joyBits = ReadControlPad (JOYCONTINUOUS);

  /*
   * If the user has pressed the START button on the joystick
   *  then return FALSE
   */
  if (joyBits & JOYSTART)
    {
      retValue = FALSE;
      goto DONE;
    }

  if (joyBits & JOYFIREA)
    {
      // we don't like continuous presses
      if (joyBits == gOldJoyBits)
        goto DONE;

      goto DONE;
    }

  // if FIRE B is pressed, display the other buffer
  if ((joyBits & JOYFIREB) && !(gOldJoyBits & JOYFIREB))
    {
      gScreenCntx.sc_curScreen = 1 - gScreenCntx.sc_curScreen;
      goto DONE;
    }

  // the select button (Button C) is pressed toggle autoshow
  if (joyBits & JOYSELECT)
    {
      gAutoShowFlag = !gAutoShowFlag;

      kprintf ("Auto-show mode is ");
      if (gAutoShowFlag)
        kprintf ("TRUE\n");
      else
        kprintf ("FALSE\n");

      goto DONE;
    }

  // direction keys switch picture and buffer
  if (joyBits & (JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT))
    {
      if ((joyBits & JOYUP) && !(gOldJoyBits & JOYUP))
        {
          ShowPrevImage ();
        }
      else if ((joyBits & JOYLEFT) && !(gOldJoyBits & JOYLEFT))
        {
          ShowPrevImage ();
        }
      else if ((joyBits & JOYDOWN) && !(gOldJoyBits & JOYDOWN))
        {
          ShowNextImage ();
        }
      else if ((joyBits & JOYRIGHT) && !(gOldJoyBits & JOYRIGHT))
        {
          ShowNextImage ();
        }
    }

DONE:
  gOldJoyBits = joyBits;

  return (retValue);

#undef JOYCONTINUOUS
}

void
ShowPrevImage (void)
{
  // show the prevous image.
  gCurrentImage--;
  if (gCurrentImage < 0)
    gCurrentImage = gImageCount - 1;

  gScreenCntx.sc_curScreen--;
  if (gScreenCntx.sc_curScreen < 0)
    gScreenCntx.sc_curScreen = kScreenCount - 1;

  // don't actually load another image unless we're showing the first one
  if (!gPrevBuffGood)
    LoadScreen (gFileNames[gCurrentImage], gScreenCntx.sc_curScreen);

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

  gScreenCntx.sc_curScreen++;
  if (gScreenCntx.sc_curScreen >= kScreenCount)
    gScreenCntx.sc_curScreen = 0;

  // don't actually load another image unless the next buffer isn't valid
  if (!gNextBuffGood)
    LoadScreen (gFileNames[gCurrentImage], gScreenCntx.sc_curScreen);

  gNextBuffGood = FALSE;
  gPrevBuffGood = TRUE;
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
  int32 errorCode;

  if (ChangeInitialDirectory (NULL, NULL, false) < 0)
    {
      DIAGNOSTIC ("Cannot set initial working directory ");
      return FALSE;
    }

  gVRAMIOReq = GetVRAMIOReq ();

  // ask the libs for some screens
  if (!OpenGraphics (&gScreenCntx, kScreenCount))
    {
      kprintf ("Cannot open graphics");
      return FALSE;
    }
  if (!OpenSPORT ())
    {
      kprintf ("Cannot open DMA channel");
      return FALSE;
    }
  if (!OpenMacLink ())
    {
      // complain but don't die, we might be running without the mac
      kprintf ("Cannot communicate with Mac");
    }

  // allocate memory for the raw VDL, initialize it.  Fail if we can't get the
  // memory
  if (AllocateVDL (&gVDLPtrArray, gScreenCntx.sc_Bitmaps[0]) != noErr)
    goto DONE;

  // assign the standard VDL to both screens
  for (screenNdx = 0; screenNdx < kBufferCount; screenNdx++)
    {
      // SubmitVDL wants it's size arg in WORDS
      gVDLItems[screenNdx]
          = SubmitVDL (gVDLPtrArray, (kVDLSize / 4), VDLTYPE_FULL);
      if (gVDLItems[screenNdx] < 0)
        {
          kprintf ("Error: SubmitVDL() failed with error code %ld\n",
                   gVDLItems[0]);
          PrintfSysErr (gVDLItems[screenNdx]);
          goto DONE;
        }

      // got the 'real' vdl, now make it current
      errorCode
          = SetVDL (gScreenCntx.sc_Screens[screenNdx], gVDLItems[screenNdx]);
      if (errorCode < 0)
        {
          kprintf ("Error: SetVDL() failed with error code %ld\n", errorCode);
          PrintfSysErr (errorCode);
          goto DONE;
        }
    }

  // now allocate some memory for the images buffers
  for (screenNdx = 0; screenNdx < kBufferCount; screenNdx++)
    {
      gBackPic[screenNdx]
          = (ubyte *)ALLOCMEM ((int32)(gScreenCntx.sc_nFrameByteCount),
                               MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

      if (gBackPic[screenNdx] == NULL)
        {
          kprintf ("unable to allocate gBackPic");
          goto DONE;
        }
      // initialize it to back
      SetVRAMPages (gVRAMIOReq, gBackPic[screenNdx], 0,
                    gScreenCntx.sc_nFrameBufferPages, -1);
    }

  // a little status message and we're done
  kprintf ("VRAM page size = %ld bytes\n", GetPageSize (MEMTYPE_VRAM));

  // will be using the audio clock for timing
  OpenAudioFolio ();

  return TRUE;

DONE:
  return FALSE;
}

Boolean
DisplayCurrentScreen (void)
{
  int32 retValue;
  int32 currScreen = gScreenCntx.sc_curScreen;

  // SPORT the background background image into the current screen buffer.
  CopyVRAMPages (gVRAMIOReq, gScreenCntx.sc_Bitmaps[currScreen]->bm_Buffer,
                 gBackPic[currScreen], gScreenCntx.sc_nFrameBufferPages, -1);

  /*
   * Point the 3DO screen to the screen we just filled. The 3DO
   *	hardware will now draw this screen to the TV every VBlank.
   */
  retValue = DisplayScreen (gScreenCntx.sc_Screens[currScreen], 0);
  if (retValue < 0)
    {
      kprintf ("DisplayScreen() failed, error=%ld\n", retValue);
      return FALSE;
    }

  return TRUE;
}

void
usage (void)
{
  kprintf (("\nUsage:\n\tSlideShow scriptFileName\n"));
  kprintf (("Where <scriptFileName> is a text file with the names of the "
            "images to show\n"));
  kprintf (("the image files to load from the Mac.  The text file "
            "\"ImageList\" is \n"));
  kprintf ("used if none is specified.\n");
  kprintf ("options:\n");
  kprintf ("\t-a<n>\tshow a new image every <n> seconds.\n");
  kprintf ("\t-r file  - Make a recording to file\n");
  kprintf ("\t-p file  - Playback a recording from file\n");
  exit (1);
}
