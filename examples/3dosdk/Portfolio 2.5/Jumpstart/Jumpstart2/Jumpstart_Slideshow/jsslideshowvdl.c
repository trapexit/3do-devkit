
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jsslideshowvdl.c,v 1.18 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/jsslideshowvdl
|||	jsslideshowvdl - Loads and displays image files which may contain
custom
|||	    VDLs (a Jumpstart program).
|||
|||	  Synopsis
|||
|||	    jsslideshowvdl \<scriptfilename> [-a \<n>]
|||
|||	  Description
|||
|||	    Slideshow which loads and displays image files which may contain
custom
|||	    VDLs.
|||
|||	    Outline of functionality:
|||
|||	    * Read a list of image filenames
|||
|||	    * Load a pair of buffers with the first two images in the list
|||
|||	    * Show the first image in the list
|||
|||	    * Respond to control pad input:
|||
|||	    * Right and down arrowsshow next image in list
|||
|||	    * Left and up arrowsshow previous image in list
|||
|||	    * Start buttonquit the program
|||
|||	    * B buttonshow the other buffer
|||
|||	    * C buttontoggle automatic display mode
|||
|||	  Arguments
|||
|||	    scriptfilename               Name of file listing images to
display, one
|||	                                 name per line.
|||
|||	    -a [n]                       Set auto-show mode, displaying each
image
|||	                                 for n seconds.
|||
|||	  Associated Files
|||
|||	    jsslideshowvdl.make, jsvdlslideshow.h
|||
|||	    Data file: JSData/ImageList   List of image files to load and
display.
|||	    Each listed filename must resolve to a valid path.
|||
|||	  Location
|||
|||	    examples/Jumpstart/Jumpstart2/jsslideshow
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

/**** VDL ****/
#include "vdlutil.h"

#include "jsslideshowvdl.h"

/*	Function Prototypes */

int32 ParseArgs (int32 argc, char *argv[]);
int32 ParseScriptFile (char *scriptFilename);
int32 LoadImageBuffer (char *filename, int32 whichBuffer);
int32 HandleControlPad (void);
void LoadNextImage (void);
void LoadPreviousImage (void);
int32 Initialize (void);
void DisplayCurrentScreen (void);
void Usage (void);
void Cleanup (void);

/*	Global variables */

ubyte *gImage[NUM_BUFFERS] = { NULL, NULL }; /* background image buffers */
int32 gCurrentImage = 0;                     /* current image # */
int32 gImageCount = 0;                       /* # of images we show */
int32 gImageVisTime = 4 * ONE_SECOND; /* default autoshow = 4 sec per image */
bool gAutoShowFlag = FALSE;           /* auto-show the images? */
bool gNextImageLoaded = FALSE;        /* is the next buffer loaded? */
bool gPrevImageLoaded = FALSE;        /* is the previous buffer loaded? */
char *gFilenames[256];                /* list of image file names */
ScreenContext *gScreenContext;        /* screen context structures */
Item gDisplayItem = -1;               /* result of CreateBasicDisplay */
Item gVRAMIOReq = -1;                 /* I/O request for SPORT calls */
Item gVBLIOReq = -1; /* I/O request used for vertical blank waiting */

/**** VDL ****/
Item gVDLItems[NUM_SCREENS]; /* actual VDL items */
VDLEntry *gVDLPtrArray;      /* our raw VDL */

/*	Main functions */

int
main (int argc, char *argv[])
{
  int32 showImageTime; /* time each image is shown when in automatic */

  /*	Print our name. */
  printf ("jsslideshowvdl\n");

  /* parse the params, bail if we don't like them */
  if (ParseArgs (argc, argv) < 0)
    goto DONE;

  /*  Init the machine and get ready to run. */
  if (Initialize () < 0)
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

  printf ("end of jsslideshowvdl\n");

  return 0;
}

int32
ParseScriptFile (char *scriptFilename)
{
  int32 retValue = -1;
  int32 fileSize;
  int32 bufferSize;
  int32 lineLen;
  char *scriptBuffer = NULL;
  char *scanChar;
  char *filename;

  /*
              Determine file size, alloc memory (grab an extra byte so the last
     line in the file won't require an explicit terminator), and read the file
     into memory
      */
  fileSize = GetFileSize (scriptFilename);
  if (fileSize <= 0)
    {
      PRT (("Can't find script file %s.\n", scriptFilename));
      goto DONE;
    }
  PRT (("Found %s (%d bytes long).\n", scriptFilename, fileSize));

  /* allocate a buffer large enough to read in the file, zero it out, and read
   * away */
  scriptBuffer = (char *)AllocMem (fileSize + 1, 0);
  if (scriptBuffer == NULL)
    {
      PRT (("Not enough memory to read script file.\n"));
      goto DONE;
    }
  memset (scriptBuffer, 0, fileSize + 1);
  retValue = ReadFile (scriptFilename, fileSize, (int32 *)scriptBuffer, 0);
  if (retValue < 0)
    {
      PRT (("Error %d reading script file.\n", retValue));
      goto DONE;
    }

  /* now parse through the file, grabbing a pointer to each file name we find
   */
  gImageCount = -1;

  bufferSize = fileSize + 1;

  scanChar = scriptBuffer;
  filename = scanChar;
  while (bufferSize-- > 0)
    {
      if ((*scanChar == '\r') || (*scanChar == '\n') || (*scanChar == '\0'))
        /*
                This character is a line terminator -- change it to a null and
           see if the last run of chars specified a valid file name
        */
        {
          *scanChar = '\0';
          lineLen = strlen (filename);
          if ((lineLen == 0) || (*filename == '\r'))
            /*	empty line, do nothing */
            ;

          else if (filename[0] == '-')
            {
              if ((filename[1] == 'a') || (filename[1] == 'A'))
                {
                  int32 seconds = strtol (&filename[3], NULL, 0);

                  gAutoShowFlag = TRUE;
                  gImageVisTime = seconds * ONE_SECOND;
                  PRT (("Auto-show new image every %d seconds.\n", seconds));
                }
              else
                PRT (("Don't understand %s option.\n", filename));
            }
          else if (GetFileSize (filename) <= 0)
            {
              PRT (("Can't find %s...\n", filename));
            }
          else
            {
              PRT (("Found %s.\n", filename));
              gImageCount++;
              gFilenames[gImageCount]
                  = filename; /*	Remember the buffer location of the
                                 filename */
            }

          /*  bump the buffer pointer and remember the start of this file name
           */
          scanChar++;
          --bufferSize;
          filename = scanChar;
        }
      scanChar++;
    }

  /* hope that we have at least one valid image name by now */
  if (gImageCount < 0)
    {
      PRT (("No valid image names!"));
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
        Parse the script file and build the list of image filenames.
        Set the automatic display options.

        argc	the number of arguments in the command line
        argv	pointer to the list of argument strings

        Returns -1 if the command line is invalid (e.g., script file
        not found, invalid option), otherwise 0.
*/
{
  int32 retValue = -1;
  int32 argIndex;
  Boolean haveFilename = FALSE;
  char *cmdLineParam;

  /* If there's no script filename, use the default file. */
  if (argc < 2)
    {
      if (ParseScriptFile ("jsdata/imagelist.txt") < 0)
        goto DONE;
      haveFilename = TRUE;
    }

  /* Loop through the remaining parameters */
  for (argIndex = 1; argIndex < argc; argIndex++)
    {
      cmdLineParam = argv[argIndex];

      if ((*cmdLineParam != '-'))
        {
          /* this param  should be the name of the script file, make sure we
           * don't */
          /*  already have one */
          if (haveFilename)
            {
              Usage ();
              goto DONE;
            }
          if (ParseScriptFile (cmdLineParam) < 0)
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

/**** VDL ****/
int32
LoadImageBuffer (char *filename, int32 whichBuffer)
/*
        Load an image from disk into an available buffer
*/
{
  int32 retValue = -1;

  VdlChunk *rawVDLPtr = NULL;
  Item oldVDL;

  PRT (("Loading file %s...\n", filename));

  /*
              Load the image and save a copy of it into the non-visible buffer;
              If it has a custom VDL, it's returned in rawVDLPtr.
      */
  if (LoadImage (filename, gImage[whichBuffer], &rawVDLPtr, gScreenContext)
      == NULL)
    {
      PRT (("LoadImage failed\n"));
      goto DONE;
    }

  /* First, init our raw VDL to point to the correct screen */
  InitVDL (gVDLPtrArray, gScreenContext->sc_Bitmaps[whichBuffer],
           DI_TYPE_NTSC);

  /* if we loaded a raw VDL, incorporate it into our screen VDL and dispose it
   */
  if ((rawVDLPtr != NULL) && (rawVDLPtr->chunk_size))
    {
      int32 vdlLines = rawVDLPtr->vdlcount;

      /* move the new data into video ram */
      MergeVDL (&(rawVDLPtr->vdl[0]), (VDL_REC *)gVDLPtrArray, vdlLines,
                DI_TYPE_NTSC);

      /* done with that, free up the memory */
      FreeMem (rawVDLPtr, rawVDLPtr->chunk_size);
    }

  /* Validate the screen VDL; the size argument is in int32s */
  gVDLItems[whichBuffer] = SubmitVDL (
      gVDLPtrArray, (GetVDLSize (DI_TYPE_NTSC) / 4), VDLTYPE_FULL);
  if (gVDLItems[whichBuffer] < 0)
    {
      DIAGNOSE_SYSERR (gVDLItems[whichBuffer], ("Can't SubmitVDL\n"));
      goto DONE;
    }

  /* Activate the new VDL. The previously-used VDL is returned. */
  oldVDL = SetVDL (gScreenContext->sc_Screens[whichBuffer],
                   gVDLItems[whichBuffer]);
  if (oldVDL < 0)
    {
      DIAGNOSE_SYSERR (oldVDL, ("Can't SetVDL()\n"));
      goto DONE;
    }

  /* Dispose of the previously-used VDL because it's no longer needed */
  DeleteVDL (oldVDL);

  /* Everything succeeded */
  retValue = 0;

DONE:
  return retValue;
}

int32
HandleControlPad (void)
/*
        Respond to the user's control pad input:

        - Right and down arrows mean show next image in list
        - Left and up arrows mean show previous image in list
        - Start button means quit the program
        - B button means show the other buffer
        - C button means toggle automatic display mode

     Returns -1 if the user pressed the start button to quit, otherwise 0.
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
        PRT (("FALSE\n"));
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

/**** VDL ****/
int32
InitScreenVDLs (void)
/*
        Initialize the VDLs for each screen
*/
{
  int32 retValue = -1;
  Item oldVDL = -1;
  int32 screenIndex;

  /* allocate memory for the raw VDL, initialize it.  Fail if we can't get the
   * memory */
  if (AllocateVDL (&gVDLPtrArray, gScreenContext->sc_Bitmaps[0], DI_TYPE_NTSC)
      < 0)
    goto DONE;

  /* assign the standard VDL to both screens */
  for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
    {
      /* Validate the VDL; The size argument is in int32s */
      gVDLItems[screenIndex] = SubmitVDL (
          gVDLPtrArray, (GetVDLSize (DI_TYPE_NTSC) / 4), VDLTYPE_FULL);
      if (gVDLItems[screenIndex] < 0)
        {
          DIAGNOSE_SYSERR (gVDLItems[screenIndex], ("Can't SubmitVDL\n"));
          goto DONE;
        }

      /* Activate the VDL; the previously-used VDL is returned */
      oldVDL = SetVDL (gScreenContext->sc_Screens[screenIndex],
                       gVDLItems[screenIndex]);
      if (oldVDL < 0)
        {
          DIAGNOSE_SYSERR (oldVDL, ("Can't SetVDL\n"));
          goto DONE;
        }

      /* Dispose of the previously-used VDL because it's no longer needed */
      DeleteVDL (oldVDL);
    }

  retValue = 0;

DONE:
  return retValue;
}

/**** VDL ****/
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

  /* Create an I/O request used for SPORT operations */
  gVRAMIOReq = CreateVRAMIOReq ();

  /* Create an I/O request used for vertical blank waiting */
  gVBLIOReq = CreateVBLIOReq ();

  /*	Allocate memory for the screenContext */
  gScreenContext
      = (ScreenContext *)AllocMem (sizeof (ScreenContext), MEMTYPE_ANY);
  if (gScreenContext == NULL)
    {
      PRT (("Can't allocate memory for ScreenContext!\n"));
      goto DONE;
    }

  /* Open the graphics folio and set up the screen context */
  if ((gDisplayItem = CreateBasicDisplay (gScreenContext, DI_TYPE_DEFAULT, 2))
      < 0)
    {
      PRT (("Can't initialize display\n"));
      goto DONE;
    }
  gScreenContext->sc_curScreen = 0;

  PRT (("Initializing control pad.\n"));
  if (InitControlPad (2) < 0)
    {
      PRT (("Can't initialize the control pad!\n"));
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
            PRT (("Can't allocate gImage"));
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
  int32 imageIndex;

  for (imageIndex = 0; imageIndex < NUM_BUFFERS; imageIndex++)
    FreeMem (gImage[imageIndex], (int32)(gScreenContext->sc_nFrameByteCount));

  KillControlPad ();

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);

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
  PRT (("\nUsage:\njsslideshowvdl [scriptfilename] [-a [n]]\n"));
  PRT (("   scriptFilename is a text file listing the names of\n"));
  PRT (("   the image files to load.  The text file\n"));
  PRT (("   \"jsdata/imagelist.txt\" is used if none is specified.\n"));
  PRT (("   options:\n"));
  PRT (("   -a[n]   Auto-show a new image every n seconds.\n"));
  PRT (("           4 is used if n isn't specified.\n"));
}
