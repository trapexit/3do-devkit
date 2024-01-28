
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: slideshow.c,v 1.21 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/slideshow
|||	slideshow - Slideshow program which displays NTSC or PAL images.
|||
|||	  Synopsis
|||
|||	    slideshow \<imagelist> [-a \<n>]
|||
|||	  Description
|||
|||	    Slideshow which loads and displays NTSC or PAL image files which
may
|||	    contain custom VDLs.
|||
|||	    Outline of functionality:
|||
|||	    * Read a list of image filenames
|||
|||	    * Load a pair of buffers with the first two images in the list
|||
|||	    * Show the first image in the list
|||
|||	    * Respond to control pad input
|||
|||	    * Right and down arrows mean show next image in list
|||
|||	    * Left and up arrows mean show previous image in list
|||
|||	    * Start button means quit the program
|||
|||	    * B button means show the other buffer
|||
|||	    * C button means toggle automatic display mode
|||
|||	  Arguments
|||
|||	    imagelist                    Name of file listing NTSC or PAL
images to
|||	                                 display, one name per line.
|||
|||	    -a                           Set auto-show mode, displaying each
image
|||	                                 for n seconds.
|||
|||	  Associated Files
|||
|||	    slideshow.make, slideshow.h
|||
|||	  Location
|||
|||	    examples/Graphics/Slideshow
|||
**/

#include "debug3do.h"
#include "displayutils.h"
#include "form3do.h"
#include "graphics.h"
#include "io.h"
#include "mem.h"
#include "parse3do.h"
#include "string.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#include "vdlutil.h"

#include "getvideoinfo.h"
#include "slideshow.h"

/*	Function Prototypes */

int32 ParseArgs (int32 argc, char *argv[]);
int32 ParseImageList (char *filename);
int32 LoadImageBuffer (char *filename, int32 whichBuffer);
int32 HandleControlPad (void);
void LoadNextImage (void);
void LoadPreviousImage (void);
int32 Initialize (void);
void DisplayCurrentScreen (void);
void Usage (void);
void Cleanup (void);

/*	Global variables */

ScreenContext *gScreenContext = NULL; /* Screen information */
Item gDisplayItem = -1;               /* result of CreateBasicDisplay */
int32 gDisplayType = DI_TYPE_DEFAULT; /* PAL or NTSC video being displayed */
Item gVRAMIOReq = -1;                 /* I/O request for SPORT calls */
Item gVBLIOReq = -1; /* I/O request used for vertical blank waiting */
ubyte *gImage[NUM_BUFFERS] = { NULL, NULL }; /* background image buffers */
Item gVDLItem[NUM_SCREENS] = { -1, -1 };     /* actual VDL items */
Item gSystemVDL[NUM_SCREENS];         /* Items for system's default VDL */
VDLEntry *gVDLPtrArray;               /* our raw VDL */
int32 gCurrentImage = 0;              /* current image index */
int32 gImageCount = 0;                /* number of images to show */
int32 gImageVisTime = 4 * ONE_SECOND; /* default autoshow = 4 sec per image */
bool gAutoShowFlag = FALSE;           /* auto-show the images? */
bool gNextImageLoaded = FALSE;        /* is the next buffer loaded? */
bool gPrevImageLoaded = FALSE;        /* is the previous buffer loaded? */
char *gFilenames[256];                /* list of image file names */

/*	Main functions */

int
main (int argc, char *argv[])
{
  int32 showImageTime; /* time each image is shown when in automatic */

  /*	Print our name. */
  printf ("slideshow\n");

  /*  Init the machine and get ready to run. */
  if (Initialize () < 0)
    goto DONE;

  /* parse the params, bail if we don't like them */
  if (ParseArgs (argc, argv) < 0)
    goto DONE;

  /*	Load up the two frame buffers with the first two pictures, but don't
   * show them yet or */
  /*  we'll flash from one to the other */
  gCurrentImage
      = gImageCount; /*	So first call to LoadNextImage will use index 0 */
  gScreenContext->sc_curScreen
      = 1; /*	So first call to LoadNextImage will use index 0 */
  gNextImageLoaded = FALSE;

  {
    int32 imageIndex;
    for (imageIndex = 0; imageIndex < NUM_BUFFERS; imageIndex++)
      LoadNextImage ();
  }

  /*	Start at the first image and screen */
  gCurrentImage = 0;
  gScreenContext->sc_curScreen = 0;

  gNextImageLoaded = TRUE;  /* The next image in the list is loaded */
  gPrevImageLoaded = FALSE; /* The previous image in the list isn't loaded */

  showImageTime = gImageVisTime;

  while (TRUE)
    {
      /* React to the controlPad */
      if (HandleControlPad () < 0)
        goto DONE;

      /* make sure we show whatever changes might have happened */
      DisplayCurrentScreen ();

      WaitVBL (gVBLIOReq, 1);

      if (gAutoShowFlag)
        /*	We're showing the images automatically */
        {
          /*	Check if the image has been displayed long enough */
          showImageTime--;
          if (!showImageTime)
            {
              LoadNextImage ();
              showImageTime = gImageVisTime;
            }
        }
    }

DONE:

  if (gScreenContext)
    FadeToBlack (gScreenContext, 60);

  Cleanup ();

  printf ("end of slideshow.\n");

  return 0;
}

int32
ParseImageList (char *filename)
{
  int32 retValue = -1;
  int32 fileSize;
  int32 bufferSize;
  int32 lineLen;
  char *listBuffer = NULL;
  char *scanChar;
  char *imageFilename;

  /*
              Determine file size, alloc memory (grab an extra byte so the last
     line in the file won't require an explicit terminator), and read the file
     into memory
      */
  fileSize = GetFileSize (filename);
  if (fileSize <= 0)
    {
      PRT (("Can't find image list file %s\n", filename));
      goto DONE;
    }
  PRT (("Found %s (%d bytes long).\n", filename, fileSize));

  /* allocate a buffer large enough to read in the file, zero it out, and read
   * away */
  listBuffer = (char *)AllocMem (fileSize + 1, 0);
  if (listBuffer == NULL)
    {
      PRT (("Not enough memory to read image list file\n"));
      goto DONE;
    }
  memset (listBuffer, 0, fileSize + 1);
  retValue = ReadFile (filename, fileSize, (int32 *)listBuffer, 0);
  if (retValue < 0)
    {
      PRT (("Error %d reading image list file\n", retValue));
      goto DONE;
    }

  /* now parse through the file, grabbing a pointer to each file name we find
   */
  gImageCount = -1;

  bufferSize = fileSize + 1;

  scanChar = listBuffer;
  imageFilename = scanChar;
  while (bufferSize-- > 0)
    {
      if ((*scanChar == '\r') || (*scanChar == '\n') || (*scanChar == '\0'))
        /*
                This character is a line terminator -- change it to a null and
           see if the last run of chars specified a valid file name
        */
        {
          *scanChar = '\0';
          lineLen = strlen (imageFilename);
          if ((lineLen == 0) || (*imageFilename == '\r'))
            /*	empty line, do nothing */
            ;

          else if (imageFilename[0] == '-')
            {
              if ((imageFilename[1] == 'a') || (imageFilename[1] == 'A'))
                {
                  int32 seconds = strtol (&imageFilename[3], NULL, 0);

                  gAutoShowFlag = TRUE;
                  gImageVisTime = seconds * ONE_SECOND;
                  PRT (("Auto-show new image every %d seconds\n", seconds));
                }
              else
                PRT (("Don't understand %s option\n", imageFilename));
            }
          else if (GetFileSize (imageFilename) <= 0)
            {
              PRT (("Can't find image file %s\n", imageFilename));
            }
          else
            {
              PRT (("Found %s\n", imageFilename));
              gImageCount++;
              gFilenames[gImageCount]
                  = imageFilename; /*	Remember the buffer location of the
                                      image filename */
            }

          /*  bump the buffer pointer and remember the start of this file name
           */
          scanChar++;
          --bufferSize;
          imageFilename = scanChar;
        }
      scanChar++;
    }

  /* hope that we have at least one valid image name by now */
  if (gImageCount < 0)
    {
      PRT (("No valid image names"));
      goto DONE;
    }

  /* bump the image count so it is 1 based, get on with life */
  PRT (("\n"));
  gImageCount++;

  retValue = 0;

DONE:
  return retValue;
}

int32
ParseArgs (int32 argc, char **argv)
/*
        Parse the image list file and build the list of image filenames.
        Set the automatic display options.

        argc	the number of arguments in the command line
        argv	pointer to the list of argument strings

        Returns -1 if the command line is invalid (e.g., image list file
        not found, invalid option), otherwise 0.
*/
{
  int32 retValue = -1;
  int32 argIndex;
  Boolean haveFilename = FALSE;
  char *cmdLineParam;

  /* If there's no image list filename, use the default file. */
  if (argc < 2)
    {
      if (ParseImageList (gDisplayType == DI_TYPE_NTSC ? "imagelist.txt"
                                                       : "palimagelist.txt")
          < 0)
        goto DONE;
      haveFilename = TRUE;
    }

  /* Loop through the remaining parameters */
  for (argIndex = 1; argIndex < argc; argIndex++)
    {
      cmdLineParam = argv[argIndex];

      if ((*cmdLineParam != '-'))
        {
          /* this param  should be the name of the image list file, make sure
           * we don't */
          /*  already have one */
          if (haveFilename)
            {
              Usage ();
              goto DONE;
            }
          if (ParseImageList (cmdLineParam) < 0)
            goto DONE;
          haveFilename = TRUE;
        }
      else
        {
          /*
                  This parameter is an option.
          */
          switch (cmdLineParam[1])
            {
            case 'A':
            case 'a':
              gAutoShowFlag = TRUE;
              cmdLineParam = argv[++argIndex];
              gImageVisTime = strtol (cmdLineParam, NULL, 0) * ONE_SECOND;
              PRT (("Auto-show each image for %d seconds.\n", gImageVisTime));
              break;

            default:
              PRT (("### Unknown option: %s\n", cmdLineParam));
              Usage ();
              goto DONE;
            }
        }
    }

  retValue = 0;

DONE:
  return retValue;
}

/**** 24-BIT ****/
void
DisposeOldVDL (Item oldVDL)
/*
        Unless the old VDL is one of the VDLs we want to retain,
        get rid of it.
*/
{
  if ((oldVDL != gSystemVDL[0]) && (oldVDL != gSystemVDL[1]))
    {
      DeleteVDL (oldVDL);
    }
}

int32
LoadImageBuffer (char *filename, int32 whichBuffer)
/*
        Load an image from disk into an available buffer
*/
{
  int32 retValue = -1;
  char imageFilename[128];

  VdlChunk *rawVDLPtr = NULL;
  Item oldVDL;

  /*
   * Extract the imagefile name from the arguments passed into the program.
   *  NOTE: the imagefile name is string copied into a local variable so
   *  that it will start on a longword boundary. Any strings that are passed
   *  to the O/S must start on a longword boundary.
   */
  strcpy (imageFilename, filename);

  PRT (("Loading file %s\n", imageFilename));

  /* Load the image and save a copy of it into the non-visible buffer */

  if (LoadImage (imageFilename, gImage[whichBuffer], &rawVDLPtr,
                 gScreenContext)
      == NULL)
    {
      PRT (("LoadImage failed\n"));
      goto DONE;
    }

  /* first, init the raw VDL we hang onto to point to the correct screen */
  InitVDL (gVDLPtrArray, gScreenContext->sc_Bitmaps[whichBuffer],
           gDisplayType);

  /* if we loaded a raw VDL, process it, shove it into memory, and toss the */
  /*  memory allocated for us by LoadImage */
  if ((rawVDLPtr != NULL) && (rawVDLPtr->chunk_size))
    {
      int32 vdlLines = rawVDLPtr->vdlcount;

      /* move the new data into video ram */
      MergeVDL (&rawVDLPtr->vdl[0], (VDL_REC *)gVDLPtrArray, vdlLines,
                gDisplayType);

      /* done with that, free up the memory */
      FreeMem (rawVDLPtr, rawVDLPtr->chunk_size);
    }
  else
    {
      gVDLItem[whichBuffer] = gSystemVDL[whichBuffer];
    }

  /* our version of the VDL has been updated, hand it off to the system... */
  gVDLItem[whichBuffer] = SubmitVDL (
      gVDLPtrArray, (GetVDLSize (gDisplayType) / 4), VDLTYPE_FULL);
  if (gVDLItem[whichBuffer] < 0)
    {
      DIAGNOSE_SYSERR (gVDLItem[whichBuffer], ("Can't SubmitVDL\n"));
      goto DONE;
    }

  /* activate the new VDL, remember the current VDL so we can toss it when it
   * is no longer */
  /*  active */
  oldVDL = SetVDL (gScreenContext->sc_Screens[whichBuffer],
                   gVDLItem[whichBuffer]);
  if (oldVDL < 0)
    {
      DIAGNOSE_SYSERR (oldVDL, ("Can't SetVDL\n"));
      goto DONE;
    }

  /* and finally, toss the old VDL as it isn't being used anymore */
  DisposeOldVDL (oldVDL);

  /* Everything succeeded */
  retValue = 0;

DONE:
  return retValue;
}

int32
HandleControlPad (void)
/*
        Respond to the user's control pad input.

        - Right and down arrows mean show next image in list
        - Left and up arrows mean show previous image in list
        - Start button means quit the program
        - B button means show the other buffer
        - C button means toggle automatic display mode

        Returns -1 if the user pressed the start button to quit,
        otherwise, 0.
*/
{
  int32 retValue = 0;
  uint32 controlBits;

  DoControlPad (1, &controlBits, 0); /* no continuous button presses */

  /* if the Start button is pressed, the user wants to quit */
  if (controlBits & ControlStart)
    {
      retValue = -1;
    }

  /* if the A button is pressed É (no action in this interface) */
  else if (controlBits & ControlA)
    {
      ;
    }

  /* if button B is pressed, display the other buffer */
  else if (controlBits & ControlB)
    {
      gScreenContext->sc_curScreen = 1 - gScreenContext->sc_curScreen;
    }

  /* if button C is pressed toggle autoshow */
  else if (controlBits & ControlC)
    {
      gAutoShowFlag = !gAutoShowFlag;

      PRT (("Auto-show mode is "));
      if (gAutoShowFlag)
        {
          PRT (("TRUE\n"));
        }
      else
        {
          PRT (("FALSE\n"));
        }
    }

  else if (controlBits
           & (ControlUp | ControlLeft | ControlDown | ControlRight))
    /* Arrows mean move one image forward or backward */
    {
      if (controlBits & ControlUp)
        {
          LoadPreviousImage ();
        }
      else if (controlBits & ControlLeft)
        {
          LoadPreviousImage ();
        }
      else if (controlBits & ControlDown)
        {
          LoadNextImage ();
        }
      else if (controlBits & ControlRight)
        {
          LoadNextImage ();
        }
    }

  return retValue;
}

void
LoadPreviousImage (void)
/*	Load the prevous image in our list. */
{

  /*	Decrement the list index */
  gCurrentImage--;
  if (gCurrentImage < 0)
    gCurrentImage = gImageCount - 1;

  /*	Toggle the screen index */
  gScreenContext->sc_curScreen = 1 - gScreenContext->sc_curScreen;

  /* don't actually load another image unless we have to */
  if (!gPrevImageLoaded)
    LoadImageBuffer (gFilenames[gCurrentImage], gScreenContext->sc_curScreen);

  gNextImageLoaded = TRUE;  /*	the "next" image is still loaded */
  gPrevImageLoaded = FALSE; /*	the "previous" image must be loaded  */
}

void
LoadNextImage (void)
/* Load the next image in our list */
{

  /* Increment the list index */
  gCurrentImage++;
  if (gCurrentImage >= gImageCount)
    gCurrentImage = 0;

  /*	Toggle the screen index */
  gScreenContext->sc_curScreen = 1 - gScreenContext->sc_curScreen;

  /* don't actually load another image unless we have to */
  if (!gNextImageLoaded)
    LoadImageBuffer (gFilenames[gCurrentImage], gScreenContext->sc_curScreen);

  gNextImageLoaded = FALSE; /*	the "next" image must be loaded  */
  gPrevImageLoaded = TRUE;  /*	the "previous" image is still loaded */
}

int32
InitScreenVDLs (void)
/*
        Initialize the VDLs for each screen
*/
{
  int32 retValue = -1;
  int32 screenIndex;

  /* allocate memory for the raw VDL, initialize it.  Fail if we can't get the
   * memory */
  if (AllocateVDL (&gVDLPtrArray, gScreenContext->sc_Bitmaps[0], gDisplayType)
      < 0)
    goto DONE;

  /* assign the standard VDL to both screens */
  for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
    {
      /* SubmitVDL wants it's size arg in WORDS */
      gVDLItem[screenIndex] = SubmitVDL (
          gVDLPtrArray, (GetVDLSize (gDisplayType) / 4), VDLTYPE_FULL);
      if (gVDLItem[screenIndex] < 0)
        {
          DIAGNOSE_SYSERR (gVDLItem[screenIndex], ("Can't SubmitVDL\n"));
          goto DONE;
        }

      /* got the 'real' vdl, now make it current */
      gSystemVDL[screenIndex] = SetVDL (
          gScreenContext->sc_Screens[screenIndex], gVDLItem[screenIndex]);
      if (gSystemVDL[screenIndex] < 0)
        {
          DIAGNOSE_SYSERR (gSystemVDL[screenIndex], ("Can't SetVDL\n"));
          goto DONE;
        }
    }

  retValue = 0;

DONE:
  return retValue;
}

int32
Initialize (void)
/*
        Allocate and prepare all of the program's global resources.
        These are:

        - A VBL I/O Request used for vertical blank waiting.
        - A VRAM I/O Request used for SPORT operations.
        - A single screen context for handling 2 screens.
        - The control pad utility.
        - The VDLs for each screen.
        - The image array memory, which is initialized to black.

        Returns 0 if all operations are performed successfully, otherwise a
        negative value.
*/
{
  int32 retValue = -1;
  int32 errorCode;

  /* Create an I/O request used for SPORT operations */
  gVRAMIOReq = CreateVRAMIOReq ();

  /* Create an I/O request used for vertical blank waiting */
  gVBLIOReq = CreateVBLIOReq ();

  /*	Allocate memory for the screenContext */
  gScreenContext
      = (ScreenContext *)AllocMem (sizeof (ScreenContext), MEMTYPE_ANY);
  if (gScreenContext == NULL)
    {
      PRT (("Can't allocate memory for ScreenContext\n"));
      goto DONE;
    }

  /* Open the graphics folio so we can get the displayType */
  errorCode = OpenGraphicsFolio ();
  if (errorCode < 0)
    {
      DIAGNOSE_SYSERR (errorCode, ("Can't open the graphics folio\n"));
      goto DONE;
    }

  gDisplayType = GetDisplayType ();
  if (gDisplayType < 0)
    {
      goto DONE;
    }

  if ((gDisplayType == DI_TYPE_PAL1) || (gDisplayType == DI_TYPE_PAL2))
    {
      gDisplayType = DI_TYPE_PAL2;
    }
  else
    {
      gDisplayType = DI_TYPE_NTSC;
    }

  if ((gDisplayItem = CreateBasicDisplay (gScreenContext, gDisplayType, 2))
      < 0)
    {
      PRT (("Can't initialize display\n"));
      goto DONE;
    }

  PRT (("Initializing control pad.\n"));
  if (InitControlPad (2) < 0)
    {
      PRT (("Can't initialize the control pad\n"));
      goto DONE;
    }

  if (InitScreenVDLs () < 0)
    goto DONE;

  {
    int32 screenIndex;

    /* now allocate some memory for the images buffers */
    for (screenIndex = 0; screenIndex < NUM_BUFFERS; screenIndex++)
      {
        gImage[screenIndex] = (ubyte *)AllocMem (
            (int32)(gScreenContext->sc_nFrameByteCount),
            MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

        if (gImage[screenIndex] == NULL)
          {
            PRT (("Can't allocate gImage[%i]\n", screenIndex));
            goto DONE;
          }
        /* initialize it to black */
        SetVRAMPages (gVRAMIOReq, gImage[screenIndex], 0,
                      gScreenContext->sc_nFrameBufferPages, -1);
      }
  }

  /* an informational message and we're done */
  PRT (("VRAM page size = %d bytes\n", GetPageSize (MEMTYPE_VRAM)));

  retValue = 0;

DONE:
  return retValue;
}

void
Cleanup (void)
/*
        Dispose all global resources used by the program.  This mirrors
        the Initialize function:

        - Disposes the screen context.
        - Disposes the image array memory.
        - Disposes the I/O Request used for vertical blank waiting.
        - Disposes the I/O Request used for SPORT operations.
        - Kills the control pad utility.

        We use FreeMem rather than UnloadImage to dispose the image memory
        because we pre-allocated it and supplied a non-NULL dest (buffer
   address) parameter to LoadImage.
*/
{
  {
    int32 screenIndex;

    for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
      {
        DeleteVDL (gVDLItem[screenIndex]);
        DeleteVDL (gSystemVDL[screenIndex]);
      }
  }

  {
    int32 imageIndex;

    for (imageIndex = 0; imageIndex < NUM_BUFFERS; imageIndex++)
      FreeMem (gImage[imageIndex],
               (int32)(gScreenContext->sc_nFrameByteCount));
  }

  KillControlPad ();

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);

  /* Close the graphics folio because we opened it to get
          the display type */
  CloseGraphicsFolio ();

  if (gScreenContext)
    FreeMem (gScreenContext, sizeof (ScreenContext));

  DeleteVBLIOReq (gVBLIOReq);
  DeleteVRAMIOReq (gVRAMIOReq);
}

void
DisplayCurrentScreen (void)
/*	Copy the current image to the screen buffer and start displaying it. */
{
  int32 currScreen = gScreenContext->sc_curScreen;

  /* SPORT the background background image into the current screen buffer. */
  CopyVRAMPages (gVRAMIOReq, gScreenContext->sc_Bitmaps[currScreen]->bm_Buffer,
                 gImage[currScreen], gScreenContext->sc_nFrameBufferPages, -1);

  /*
          Now display the buffer we just copied into.
          The 3DO display hardware updates the TV display
          from this buffer every frame.
  */
  DisplayScreen (gScreenContext->sc_Screens[currScreen], 0);
}

void
Usage (void)
/*	Show the user the acceptable command line formats */
{
  PRT (("\nUsage:\nslideshow [imagelist] [-a [n]]\n"));
  PRT (("   imagelist is a text file listing the names of\n"));
  PRT (("   the image files to load.  The text file\n"));
  PRT (("   \"palimagelist.txt\" is used if none is specified\n"));
  PRT (("   and PAL video is being displayed.  The text file\n"));
  PRT (("   \"imagelist.txt\" is used if none is specified\n"));
  PRT (("   and NTSC video is being displayed.\n"));
  PRT (("   options:\n"));
  PRT (("   -a [n]  Auto-show a new image every n seconds.\n"));
  PRT (("           4 is used if n isn't specified.\n"));
}
