/*
        File:		JSBasicSlideShow.c

        Contains:	Load and display image files.

        Files used:

                $boot/JSData/basicImageList		-	list of image
   filenames to load and display $boot/JSData/Art/Å.img			-
   images listed in the above file

        Written by:	eric carlson
                                ( Freely adapted for JumpStart by Charlie
   Eckhaus )

        Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <3>		1/5/94		CDE Removed
   ChangeInitialDirectory; Added $boot to filenames; Removed -r, -p options
   messages from Usage() since they were unimplemented; Cleaned up includes,
   prototypes, changed some function types. <2>		1/3/94		CDE
   Replaced direct joypad access with event broker interface.
                <1>		12/21/93	CDE		Derived from
   SlideShow.c; removed VDL handling.

*/

#include "Portfolio.h"
#include "UMemory.h"
#include "Utils3DO.h"
#include "event.h"

#include "JSBasicSlideShow.h"

/*
 * Definitions
 */
#define kVersion "1.0"

/*
 * Function Prototypes
 */
Boolean ParseArgs (int32 argc, char *argv[]);
int32 ParseScriptFile (char *scriptFileName);
Boolean LoadScreen (char *fileName, int32 whichBuffer);
Boolean DoControlPad (void);
void ShowNextImage (void);
void ShowPrevImage (void);
Boolean Init (void);
Boolean DisplayCurrentScreen (void);
void Usage (void);
void Cleanup (void);

/*
 * Data Declarations
 */
ubyte *gBackPic[kBufferCount] = { NULL, NULL };
// background image buffers
int32 gCurrentImage = 0,       // current image #
    gImageCount = 0,           // # of images we show
    gShowImageTime,            // length each image is shown in autoshow
    gImageVisTime = 60 * 4;    // default autoshow = 4 sec per image
Boolean gAutoShowFlag = FALSE, // autoshow?
    gNextBuffGood,             // next buffer is good?
    gPrevBuffGood;             // have we shown the first image yet? [HACK}
char *gFileNames[256];         // list of image file names
ScreenContext gScreenContext;  // screen context structures
Item gVRAMIOReq;               // I/O request for SPORT calls

/*
 * Main Routines
 */
int
main (int argc, char *argv[])
{
  int32 screenCount;

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
  gScreenContext.sc_curScreen = -1;
  gNextBuffGood = FALSE;
  for (screenCount = 0; screenCount < kScreenCount; screenCount++)
    ShowNextImage ();

  // and point to the first buffer/image
  gCurrentImage = 0;
  gScreenContext.sc_curScreen = 0;
  gNextBuffGood = TRUE;
  gPrevBuffGood = FALSE;
  gShowImageTime = TICKCOUNT () + gImageVisTime;

  while (TRUE)
    {
      // React to the joystick
      if (NOT DoControlPad ())
        goto DONE;

      // make sure we show whatever changes might have happened
      DisplayCurrentScreen ();

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

  FadeToBlack (&gScreenContext, 60);
  printf ("\nJSBasicSlideShow has completed.  Thanks for shopping with us!\n");

  Cleanup ();
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
  printf ("Found %s, %d bytes long...\n", scriptFileName, fileSize);

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

  bufferSize = fileSize + 1;

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
  return FALSE;
}

/*
 * Parse the command line parameters.  When finished with this routine we
 *  will know how many files to loop through and the auto slide show options.
 * Returns TRUE if all is well, FALSE otherwise.
 */
Boolean
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
      if (ParseScriptFile ("$boot/JSData/basicimagelist") != noErr)
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
              Usage ();
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
              printf (
                  "Auto running mode, each image is shown for %d seconds.\n",
                  gImageVisTime);
              break;

            default:
              printf ("### Unknown option: %s\n", cmdLineParam);
              Usage ();
              return FALSE;
            }
        }
    }

  return TRUE;
}

/*
 * Load an image from disk into the non-visible buffer
 */
Boolean
LoadScreen (char *fileName, int32 whichBuffer)
{
  Boolean successfulLoad = FALSE;
  char imageFileName[64];

  /*
   * Extract the imagefile name from the arguments passed into the program.
   *	NOTE: the imagefile name is string copied into a local variable so
   *	that it will start on a longword boundry. Any strings that are passed
   *  to the O/S must start on a longword boundry.
   */
  strcpy (imageFileName, fileName);

  printf ("Loading file %s...\n", imageFileName);

  // Load the background image and save a copy of it into the non-visible
  // buffer

  if (LoadImage (imageFileName, gBackPic[whichBuffer], (VdlChunk **)NULL,
                 &gScreenContext)
      == NULL)
    {
      printf ("LoadImage failed\n");
      goto DONE;
    }

  // made it this far?  all must be well in the world... joy
  successfulLoad = TRUE;

DONE:
  return successfulLoad;
}

uint32
JSGetControlPad (uint32 continuousMask)
/*
        Get the current control pad bits, masking out undesirable continuous
        button presses.
*/
{
  static uint32 gOldBits = 0;

  int32 aResult;
  ControlPadEventData aControlPadEvent;
  uint32 newBits;
  uint32 maskedBits;

  aResult = GetControlPad (1, FALSE, &aControlPadEvent);
  if (aResult < 0)
    {
      printf ("Error: GetControlPad() failed with error code %d\n", aResult);
      PrintfSysErr (aResult);
      return 0;
    }
  newBits = aControlPadEvent.cped_ButtonBits;

  maskedBits = newBits ^ gOldBits;

  maskedBits &= newBits;
  maskedBits |= (newBits & continuousMask);

  gOldBits = newBits;
  return maskedBits;
}

Boolean
DoControlPad (void)
/*
        Handle the controlPad using the event broker.

        Returns FALSE if user pressed START to quit, otherwise TRUE.

 */
{
  uint32 joyBits;

  joyBits = JSGetControlPad (0); // no continuous button presses

  // if the Start button is pressed, the user wants to quit
  if (joyBits & ControlStart)
    {
      return FALSE;
    }

  // if the A button is pressed É (no action in this interface)
  if (joyBits & ControlA)
    {
      goto DONE;
    }

  // if button B is pressed, display the other buffer
  if (joyBits & ControlB)
    {
      gScreenContext.sc_curScreen = 1 - gScreenContext.sc_curScreen;
      goto DONE;
    }

  // if button C is pressed toggle autoshow
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

  // direction keys switch picture and buffer
  if (joyBits & (ControlUp | ControlLeft | ControlDown | ControlRight))
    {
      if (joyBits & ControlUp)
        {
          ShowPrevImage ();
        }
      else if (joyBits & ControlLeft)
        {
          ShowPrevImage ();
        }
      else if (joyBits & ControlDown)
        {
          ShowNextImage ();
        }
      else if (joyBits & ControlRight)
        {
          ShowNextImage ();
        }
    }

DONE:
  return TRUE;
}

void
ShowPrevImage (void)
{
  // show the prevous image.
  gCurrentImage--;
  if (gCurrentImage < 0)
    gCurrentImage = gImageCount - 1;

  gScreenContext.sc_curScreen--;
  if (gScreenContext.sc_curScreen < 0)
    gScreenContext.sc_curScreen = kScreenCount - 1;

  // don't actually load another image unless we're showing the first one
  if (!gPrevBuffGood)
    LoadScreen (gFileNames[gCurrentImage], gScreenContext.sc_curScreen);

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

  gScreenContext.sc_curScreen++;
  if (gScreenContext.sc_curScreen >= kScreenCount)
    gScreenContext.sc_curScreen = 0;

  // don't actually load another image unless the next buffer isn't valid
  if (!gNextBuffGood)
    LoadScreen (gFileNames[gCurrentImage], gScreenContext.sc_curScreen);

  gNextBuffGood = FALSE;
  gPrevBuffGood = TRUE;
}

/*
 * This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns TRUE if all is well, FALSE if error
 */
Boolean
Init (void)
{
  int32 aScreenIndex;

  gVRAMIOReq = GetVRAMIOReq (); // Create the I/O request which we'll use for
                                // all of our SPORT calls.

  // ask the libs for some screens
  if (!OpenGraphics (&gScreenContext, kScreenCount))
    {
      printf ("Can't open graphics\n");
      return FALSE;
    }

  if (!OpenSPORT ())
    {
      printf ("Can't open SPORT I/O\n");
      return FALSE;
    }

  if (InitEventUtility (1, 0, LC_Observer) < 0)
    {
      printf ("Can't initialize the event broker\n");
      return FALSE;
    }

  // now allocate some memory for the images buffers
  for (aScreenIndex = 0; aScreenIndex < kBufferCount; aScreenIndex++)
    {
      gBackPic[aScreenIndex] = (ubyte *)AllocMem (
          (int32)(gScreenContext.sc_nFrameByteCount),
          GETBANKBITS (GrafBase->gf_ZeroPage) | MEMTYPE_STARTPAGE
              | MEMTYPE_VRAM | MEMTYPE_CEL);

      if (gBackPic[aScreenIndex] == NULL)
        {
          printf ("Can't allocate gBackPic");
          goto DONE;
        }
      // initialize it to back
      SetVRAMPages (gVRAMIOReq, gBackPic[aScreenIndex], 0,
                    gScreenContext.sc_nFrameBufferPages, -1);
    }

  // a little status message and we're done
  printf ("VRAM page size = %d bytes\n", GrafBase->gf_VRAMPageSize);

  return TRUE;

DONE:
  return FALSE;
}

Boolean
DisplayCurrentScreen (void)
{
  int32 retValue;
  int32 currScreen = gScreenContext.sc_curScreen;

  // SPORT the background background image into the current screen buffer.
  CopyVRAMPages (gVRAMIOReq, gScreenContext.sc_Bitmaps[currScreen]->bm_Buffer,
                 gBackPic[currScreen], gScreenContext.sc_nFrameBufferPages,
                 -1);

  /*
   * Point the 3DO screen to the screen we just filled. The 3DO
   *	hardware will now draw this screen to the TV every VBlank.
   */
  retValue = DisplayScreen (gScreenContext.sc_Screens[currScreen], 0);
  if (retValue < 0)
    {
      printf ("DisplayScreen() failed, error=%d\n", retValue);
      return FALSE;
    }

  return TRUE;
}

void
Usage (void)
{
  printf (("\nUsage:\n\tJSBasicSlideShow scriptFileName\n"));
  printf (("Where <scriptFileName> is a text file with the names of the "
           "images to show\n"));
  printf (("the image files to load from the Mac.  The text file "
           "\"$boot/JSData/basicImageList\" is \n"));
  printf ("used if none is specified.\n");
  printf ("options:\n");
  printf ("\t-a<n>\tshow a new image every <n> seconds.\n");
  exit (1);
}

void
Cleanup (void)
/*
        Free global data and system resources
*/
{
  int32 aScreenIndex;

  for (aScreenIndex = 0; aScreenIndex < kBufferCount; aScreenIndex++)
    FreeMem (gBackPic[aScreenIndex],
             (int32)(gScreenContext.sc_nFrameByteCount));

  KillEventUtility ();
  DeleteIOReq (gVRAMIOReq);
  ShutDown ();
}
