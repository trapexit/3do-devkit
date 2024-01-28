
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: slideshow24.c,v 1.13 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/slideshow24
|||	slideshow24 - Version of slideshow that supports displays of 24-bit 3DO
|||	    images.
|||
|||	  Synopsis
|||
|||	    slideshow24 \<imagelist> [-a \<n>]
|||
|||	  Description
|||
|||	    Loads and displays image files which may contain custom VDLs. This
version
|||	    has been modified to support 24-bit images. These modifications are
|||	    preceded by **** 24-BIT ***** comments.
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
|||	    This program is designed to demonstrate image quality differences
between
|||	    standard CLUT images, custom CLUT images, and 24-bit images. The
program
|||	    determines the image type and submits any custom VDLs.
|||
|||	  Arguments
|||
|||	    imagelist                    Name of file listing 24-bit image
names, one
|||	                                 name per line.
|||
|||	    -a                           Set auto-show mode, displaying each
image
|||	                                 for n seconds
|||
|||	  Caveats
|||
|||	    Current implementation causes the screen to flash as you switch
between
|||	    16-bit and 24-bit images.
|||
|||	  Associated Files
|||
|||	    slideshow24.c, slideshow24.make, slideshow24.h, loadfile24.h,
loadfile24
|||	    .c, vdl24util.c,vdl24util.h
|||
|||	  Location
|||
|||	    examples/Graphics/Slideshow_24bit
|||
**/

#include "debug3do.h"
#include "displayutils.h"
#include "graphics.h"
#include "io.h"
#include "parse3do.h"
#include "stdio.h"
#include "string.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#include "getvideoinfo.h"

#include "loadfile24.h"
#include "slideshow24.h"
#include "vdl24util.h"
#include "vdlutil.h"

/*	Function Prototypes */

int32 ParseArgs (int32 argc, char *argv[]);
int32 ParseImageList (char *filename);
int32 LoadImageBuffer (char *filename, int32 whichBuffer);
int32 HandleControlPad (void);
void LoadNextImage (void);
void LoadPreviousImage (void);
int32 Initialize (void);
int32 DisplayCurrentScreen (void);
void Usage (void);
void Cleanup (void);

/**** 24-BIT ****/
Err DisableV (uint32 which);
Err EnableV (uint32 which);
Err DisableH (uint32 which);
Err EnableH (uint32 which);
void ToggleBit15 (void);
void ClearBit15 (void);
Item CreateVDLWrapper (int32 vdlType, int32 length, void *dataPtr);

/*	Global variables */

ubyte *gImage[NUM_BUFFERS] = { NULL, NULL }; /* background image buffers */
int32 gCurrentImage = 0;                     /* current image # */
int32 gImageCount = 0;                       /* # of images we show */
int32 gImageVisTime = 4 * ONE_SECOND; /* default autoshow = 4 sec per image */
bool gAutoShowFlag = FALSE;           /* auto-show the images? */
bool gNextImageLoaded = FALSE;        /* is the next buffer loaded? */
bool gPrevImageLoaded = FALSE;        /* is the previous buffer loaded? */
char *gFilenames[256];                /* list of image file names */
ScreenContext *gScreenContext = NULL; /* screen context structures */
Item gVRAMIOReq = -1;                 /* I/O request for SPORT calls */
Item gVBLIOReq = -1; /* I/O request used for vertical blank waiting */
ImageCC gImageHeader[NUM_BUFFERS];
int32 gImageSize[NUM_BUFFERS];     /* Total number of bytes in image */
int32 gImageVDLLines[NUM_BUFFERS]; /* Number of lines in image's VDL? */
int32 gImageIndex[NUM_BUFFERS];    /* index in gFilenames for this buffer */

Item gVDLItem[NUM_SCREENS + 2]; /* actual VDL items */
VDLEntry *gVDLPtrArray;         /* our raw VDL */
Item gSystemVDL[NUM_SCREENS];   /* Items for system's default VDL */

/**** 24-BIT ****/
uint32 gMaxImageSize;              /* size for a 32-bit image */
uint32 gMinImageSize;              /* size for a 16-bit image */
int32 gPrevBuff = -1;              /* which image buffer is current */
int32 gVAVG[NUM_BUFFERS];          /* enable vertical averaging */
int32 gHAVG[NUM_BUFFERS];          /* enabling horizontal averaging */
int32 gDisplayType = DI_TYPE_NTSC; /* what type of video is being displayed */
int32 gMyScreenWidth
    = NTSC_SCREEN_WIDTH; /* Screen Width in current RUNTINE environment */
int32 gMyScreenHeight
    = NTSC_SCREEN_HEIGHT; /* Screen Height in current RUNTINE environment */

/*	Main functions */

int
main (int argc, char *argv[])
{
  int32 showImageTime; /* time each image is shown when in automatic */

  printf ("slideshow24\n");

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
      if (DisplayCurrentScreen () < 0)
        goto DONE;

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

  printf ("end of slideshow24.\n");

  return 0;
}

int32
ParseImageList (char *filename)
{
  int32 retValue = -1;
  int32 fileSize;
  int32 bufferSize;
  char *scriptBuffer = NULL;
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
      PRT (("Can't find image list %s.\n", filename));
      goto DONE;
    }
  PRT (("Found %s (%d bytes long).\n", filename, fileSize));

  /* allocate a buffer large enough to read in the file, zero it out, and read
   * away */
  scriptBuffer = (char *)AllocMem (fileSize + 1, 0);
  if (scriptBuffer == NULL)
    {
      PRT (("Not enough memory to read image list.\n"));
      goto DONE;
    }
  memset (scriptBuffer, 0, fileSize + 1);
  retValue = ReadFile (filename, fileSize, (int32 *)scriptBuffer, 0);
  if (retValue < 0)
    {
      DIAGNOSE_SYSERR (retValue, ("Error reading image list.\n"));
      goto DONE;
    }

  /* now parse through the file, grabbing a pointer to each file name we find
   */
  gImageCount = -1;

  bufferSize = fileSize + 1;

  scanChar = scriptBuffer;
  imageFilename = scanChar;
  while (bufferSize-- > 0)
    {
      if ((*scanChar == '\r') || (*scanChar == '\n') || (*scanChar == '\0'))
        /*
                This character is a line terminator -- change it to a null and
           see if the last run of chars specified a valid file name
        */
        {
          int32 lineLength;

          *scanChar = '\0';
          lineLength = strlen (imageFilename);
          if ((lineLength == 0) || (*imageFilename == '\r'))
            /*	empty line, do nothing */
            ;

          else if (imageFilename[0] == '-')
            {
              if ((imageFilename[1] == 'a') || (imageFilename[1] == 'A'))
                {
                  int32 seconds = strtol (&imageFilename[3], NULL, 0);

                  gAutoShowFlag = TRUE;
                  gImageVisTime = seconds * ONE_SECOND;
                  PRT (("Auto-show new image every %d seconds.\n", seconds));
                }
              else
                PRT (("Don't understand %s option.\n", imageFilename));
            }
          else if (GetFileSize (imageFilename) <= 0)
            {
              PRT (("Can't find %s...\n", imageFilename));
            }
          else
            {
              PRT (("Found %s.\n", imageFilename));
              gImageCount++;
              gFilenames[gImageCount]
                  = imageFilename; /*	Remember the buffer location of the
                                      filename */
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
        Parse the command line parameters.
        If a filename is specified, build the list of image filenames from it.
        Set the automatic display options.

        argc	the number of arguments in the command line
        argv	pointer to the list of argument strings

        Returns -1 if the command line is invalid (e.g., image list
        not found, invalid option), otherwise 0.
*/
{
  int32 retValue = -1;
  int32 argIndex;
  Boolean haveFilename = FALSE;
  char *cmdLineParam;

  /* If there's no image list name, use the default file. */
  if (argc < 2)
    {
      if (ParseImageList ((gDisplayType == DI_TYPE_NTSC)
                              ? "imagelist24.txt"
                              : "palimagelist24.txt")
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
          /* this param  should be the name of the image list, make sure we
           * don't */
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

/**** 24-BIT ****/
int32
LoadImageBuffer (char *filename, int32 whichBuffer)
/*
        Load an image from disk into an available buffer
*/
{
  int32 retValue = -1;

  VdlChunk *rawVDLPtr = NULL;
  Item oldVDL;

  /* Load the image; If it contains a custom VDL, it's returned in rawVDLPtr.
   */
  if ((loadfile24 (filename, gImage[whichBuffer], gMaxImageSize, MEMTYPE_ANY,
                   &rawVDLPtr, &gImageHeader[whichBuffer], gMyScreenWidth,
                   gMyScreenHeight))
      < 0)
    {
      PRT (("loadfile24 failed\n"));
      goto DONE;
    }
  if (gImageHeader[whichBuffer].bitsperpixel <= 16)
    gImageSize[whichBuffer] = gMinImageSize;
  else
    gImageSize[whichBuffer] = gMaxImageSize;

  /* Point our raw VDL to the specified screen */
  InitVDL24 (gVDLPtrArray, gScreenContext->sc_Bitmaps[whichBuffer],
             gDisplayType);

  gImageVDLLines[whichBuffer] = 0;
  if ((rawVDLPtr != NULL) && (rawVDLPtr->chunk_size))
    {
      int32 vdlLines = rawVDLPtr->vdlcount;

      /* Incorporate the custom VDL into the screen's VDL */
      MergeVDL24 (&(rawVDLPtr->vdl[0]), (VDL_REC *)gVDLPtrArray, vdlLines,
                  gDisplayType);

      /* Dispose the custom VDL; it's no longer needed */
      FreeMem (rawVDLPtr, rawVDLPtr->chunk_size);

      gImageVDLLines[whichBuffer] = vdlLines;

      /*
              We need a VDL item in order to activate the VDL;
              the size argument is in int32s.
      */
      gVDLItem[whichBuffer]
          = CreateVDLWrapper (VDLTYPE_FULL, (vdlLines * sizeof (VDL_REC) / 4),
                              (void *)gVDLPtrArray);
      if (gVDLItem[whichBuffer] < 0)
        {
          DIAGNOSE_SYSERR (gVDLItem[whichBuffer], ("Can't CreateVDL\n"));
          goto DONE;
        }
    }
  else
    /* Use the default VDL for the screen if there's no custom VDL. */
    {
      gVDLItem[whichBuffer] = gSystemVDL[whichBuffer];
    }

  /* Activate the VDL; the previously-used VDL is returned */
  oldVDL = SetVDL (gScreenContext->sc_Screens[whichBuffer],
                   gVDLItem[whichBuffer]);
  if (oldVDL < 0)
    {
      DIAGNOSE_SYSERR (oldVDL, ("Can't SetVDL\n"));
      goto DONE;
    }

  /* Dispose the previously-used VDL; it's no longer needed */
  DisposeOldVDL (oldVDL);

  gHAVG[whichBuffer] = TRUE;
  gVAVG[whichBuffer] = TRUE;

  retValue = 0;

DONE:
  return retValue;
}

int32
HandleControlPad (void)
/*
        Respond to the user's control pad input:

        - Right and down arrows mean show next image in list
        - Left shift button means toggle horizontal averaging
        - Right shift button means toggle vertical averaging
        - Left and up arrows mean show previous image in list
        - Start button means quit the program
        - B button means show the other buffer
        - C button means toggle automatic display mode

        Returns -1 if the user pressed the start button to quit,
        otherwise 0.
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

  /* Right shift button, toggle vertical averaging */
  else if (controlBits & ControlRightShift)
    {
      if (gVAVG[gScreenContext->sc_curScreen])
        {
          gVAVG[gScreenContext->sc_curScreen] = FALSE;
          if (!DisableV (gScreenContext->sc_curScreen))
            {
              PRT (("Vertical Averaging disabled\n"));
            }
          else
            {
              PRT (("Can't disable Vertical Averaging\n"));
            }
        }
      else
        {
          gVAVG[gScreenContext->sc_curScreen] = TRUE;
          if (!EnableV (gScreenContext->sc_curScreen))
            {
              PRT (("Vertical Averaging enabled\n"));
            }
          else
            {
              PRT (("Can't enable Vertical Averaging\n"));
            }
        }
      goto DONE;
    }

  /* Left shift button, toggle horizontal averaging */
  else if (controlBits & ControlLeftShift)
    {
      if (gHAVG[gScreenContext->sc_curScreen])
        {
          gHAVG[gScreenContext->sc_curScreen] = FALSE;
          if (!DisableH (gScreenContext->sc_curScreen))
            {
              PRT (("Horizontal Averaging disabled\n"));
            }
          else
            {
              PRT (("Can't disable Horizontal Averaging\n"));
            }
        }
      else
        {
          gHAVG[gScreenContext->sc_curScreen] = TRUE;
          if (!EnableH (gScreenContext->sc_curScreen))
            {
              PRT (("Horizontal Averaging enabled\n"));
            }
          else
            {
              PRT (("Can't enable Horizontal Averaging\n"));
            }
        }
      goto DONE;
    }

  /* A button, toggle bit 15 of each image pixel */
  else if (controlBits & ControlA)
    {
      ToggleBit15 ();
      PRT ((" Bit 15 toggled\n"));
    }

  /* X button, clear bit 15 of each image pixel */
  else if (controlBits & ControlX)
    {
      ClearBit15 ();
      PRT ((" Bit 15 cleared\n"));
    }

  /* B button, display the other buffer */
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

DONE:
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
    {
      LoadImageBuffer (gFilenames[gCurrentImage],
                       gScreenContext->sc_curScreen);
      gImageIndex[gScreenContext->sc_curScreen] = gCurrentImage;
    }

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
    {
      LoadImageBuffer (gFilenames[gCurrentImage],
                       gScreenContext->sc_curScreen);
      gImageIndex[gScreenContext->sc_curScreen] = gCurrentImage;
    }

  gNextImageLoaded = FALSE; /*	the "next" image must be loaded  */
  gPrevImageLoaded = TRUE;  /*	the "previous" image is still loaded */
}

/**** 24-BIT ****/
int32
InitScreenVDLs (ScreenContext *sc, int32 displayType)
/*
        Initialize the VDLs for each screen
*/
{
  int32 retValue = -1;
  int32 screenIndex;

  /* allocate memory for the raw VDL, initialize it.  Fail if we can't get the
   * memory */
  if (AllocateVDL24 (&gVDLPtrArray, sc->sc_Bitmaps[0], displayType) < 0)
    goto DONE;

  /* assign the standard VDL to both screens */
  for (screenIndex = 0; screenIndex < sc->sc_nScreens; screenIndex++)
    {
      /* first, init the raw VDL we hang onto to point to the correct screen */
      InitVDL24 (gVDLPtrArray, sc->sc_Bitmaps[screenIndex], displayType);

      /* the size argument is in int32s */
      gVDLItem[screenIndex] = SubmitVDL (
          gVDLPtrArray, (GetVDLSize (displayType) / 4), VDLTYPE_FULL);
      if (gVDLItem[screenIndex] < 0)
        {
          DIAGNOSE_SYSERR (gVDLItem[screenIndex], ("Can't SubmitVDL\n"));
          goto DONE;
        }

      /* got the 'real' vdl, now make it current */
      gSystemVDL[screenIndex]
          = SetVDL (sc->sc_Screens[screenIndex], gVDLItem[screenIndex]);
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

/**** 24-BIT ****/
/* Utility function for copying screen item info to a ScreenContext */
Screen *
ScreenToScreenContext (int32 screenIndex, ScreenContext *sc)
{
  Screen *pScreen;

  pScreen = (Screen *)LookupItem (sc->sc_Screens[screenIndex]);
  if (pScreen == NULL)
    {
      PRT (("Can't locate screen\n"));
      goto DONE;
    }
  sc->sc_BitmapItems[screenIndex] = pScreen->scr_TempBitmap->bm.n_Item;
  sc->sc_Bitmaps[screenIndex] = pScreen->scr_TempBitmap;

DONE:
  return pScreen;
}

/**** 24-BIT ****/
/* VERY SIMILAR TO OpenGraphicsNTSCPAL */
int32
CreateScreens (ScreenContext *sc)
{
  int32 retValue = -1; /* value returned by THIS function */
  int32 result;        /* value returned by some internal calls */
  uint32 nScreens;
  int32 width;
  int32 height;
  int32 displayType;
  int32 pageSize;
  Item screenGroupItem;
  uint32 bitmapWidth[MAXSCREENS];
  uint32 bitmapHeight[MAXSCREENS];
  ubyte *bitmap[NUM_SCREENS];
  int32 *vdl[NUM_SCREENS];

  TagArg screenGroupTags24[] =
      /*
              Tags for creating a ScreenGroup.  The -1 entries are
         placeholders. Don't change the order without changing the indexes used
              below.
      */
      {
        CSG_TAG_DISPLAYHEIGHT,
        (void *)-1,
        CSG_TAG_SCREENCOUNT,
        (void *)-1,
        CSG_TAG_SCREENHEIGHT,
        (void *)-1,
        CSG_TAG_BITMAPCOUNT,
        (void *)1,
        CSG_TAG_BITMAPWIDTH_ARRAY,
        (void *)-1,
        CSG_TAG_BITMAPHEIGHT_ARRAY,
        (void *)-1,
        CSG_TAG_BITMAPBUF_ARRAY,
        (void *)-1,
        CSG_TAG_DISPLAYTYPE,
        (void *)-1,
        CSG_TAG_DONE,
        (void *)0,
      };

  int32 vdlSize = 2 * 40 * sizeof (int32);
  ubyte *screenBuffer;

  /* Check for bad arguments */
  if (sc == NULL)
    goto DONE;

  nScreens = sc->sc_nScreens;
  if ((nScreens < 1) || (nScreens > MAXSCREENS))
    goto DONE;

  result = OpenGraphicsFolio ();
  if (result < 0)
    {
      DIAGNOSE_SYSERR (result, ("Can't open graphics folio\n"));
      goto DONE;
    }

  displayType = GetDisplayType ();
  width = GetScreenWidth (displayType);
  height = GetScreenHeight (displayType);

  gMyScreenWidth = width;
  gMyScreenHeight = height;
  gDisplayType = displayType;

  {
    int32 screenIndex;

    for (screenIndex = 0; screenIndex < nScreens; screenIndex++)
      {
        bitmapWidth[screenIndex] = width;
        bitmapHeight[screenIndex] = height;
      }
  }

  /* Now fill out the ScreenContext, create the ScreenGroup and add it to the
   * display mechanism */
  sc->sc_curScreen = 0;

  pageSize = GetPageSize (MEMTYPE_VRAM);

  sc->sc_nFrameBufferPages = (width * height * 2 + pageSize - 1) / pageSize;
  sc->sc_nFrameByteCount = sc->sc_nFrameBufferPages * pageSize;

  /* Allocate two screen bitmaps */
  screenBuffer
      = (ubyte *)AllocMem ((int)(2 * sc->sc_nFrameBufferPages * pageSize),
                           MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);
  if (screenBuffer == NULL)
    {
      PRT (("Can't allocate screen memory\n"));
      goto DONE;
    }
  bitmap[0] = screenBuffer;
  bitmap[1] = screenBuffer + (sc->sc_nFrameBufferPages * pageSize);

  /* Create and add the ScreenGroup */
  screenGroupTags24[0].ta_Arg = (void *)height;
  screenGroupTags24[1].ta_Arg = (void *)sc->sc_nScreens;
  screenGroupTags24[2].ta_Arg = (void *)height;
  screenGroupTags24[4].ta_Arg = (void *)bitmapWidth;
  screenGroupTags24[5].ta_Arg = (void *)bitmapHeight;
  screenGroupTags24[6].ta_Arg = (void *)bitmap;
  screenGroupTags24[7].ta_Arg = (void *)displayType;

  screenGroupItem
      = CreateScreenGroup (&(sc->sc_Screens[0]), screenGroupTags24);
  if (screenGroupItem < 0)
    {
      DIAGNOSE_SYSERR (screenGroupItem, ("Can't create ScreenGroup\n"));
      goto DONE;
    }

  screenGroupItem = AddScreenGroup (screenGroupItem, NULL);
  if (screenGroupItem < 0)
    {
      DIAGNOSE_SYSERR (screenGroupItem, ("Can't add ScreenGroup"));
      goto DONE;
    }

  /* Use the screen group items to fill out more of the ScreenContext */
  {
    int32 screenIndex;

    for (screenIndex = 0; screenIndex < nScreens; screenIndex++)
      {
        if (ScreenToScreenContext (screenIndex, sc) == NULL)
          goto DONE;
        EnableHAVG (sc->sc_Screens[screenIndex]);
        EnableVAVG (sc->sc_Screens[screenIndex]);
      }
  }

  if (InitScreenVDLs (sc, displayType) < 0)
    goto DONE;

  /*
          Now... for "simplicity's" sake we'll create two new screens with
     24-bit VDLs.  Rather than allocating new bitmaps for the screens, we'll
          simply repurpose the two we already have - all we need is a
          couple of new VDLs.

          Allocate VDL memory: 2 blocks of 40 int32s each.
          One VDL is for the odd-field display, the other is for the even.
          They are full color tables because the hardware reloads
          the CLUTs with default ramps after each field.  For 24bit,
          this would cause a bad display.
  */
  vdl[0] = (int32 *)AllocMem (vdlSize, MEMTYPE_ANY);
  if (vdl[0] == NULL)
    {
      PRT (("Can't allocate VDL memory\n"));
      goto DONE;
    }
  vdl[1] = vdl[0] + 40;

  nScreens = sc->sc_nScreens += 2;

  /* Both screens use the same bitmap buffer in 24-bit mode */
  bitmap[0] = screenBuffer;
  bitmap[1] = screenBuffer;

  /* Create and add screen group */
  screenGroupItem
      = CreateScreenGroup (&sc->sc_Screens[nScreens - 2], screenGroupTags24);
  if (screenGroupItem < 0)
    {
      DIAGNOSE_SYSERR (screenGroupItem, ("Can't create 2nd ScreenGroup\n"));
      goto DONE;
    }

  screenGroupItem = AddScreenGroup (screenGroupItem, NULL);
  if (screenGroupItem < 0)
    {
      DIAGNOSE_SYSERR (screenGroupItem, ("Can't add 2nd ScreenGroup"));
      goto DONE;
    }

  /*
          So...  after all of that we have a single, double height screen.  now
     get a simple vdl for this screen.  We want each to point to the start of
          the single large bitmap we allocated
  */
  {
    int32 screenIndex;
    int32 vdlIndex = 0;
    Item oldVDL;

    for (screenIndex = (nScreens - 2); screenIndex < nScreens; screenIndex++)
      {
        if (ScreenToScreenContext (screenIndex, sc) == NULL)
          goto DONE;

        InitVDL480 (vdl[vdlIndex], sc->sc_Bitmaps[screenIndex], vdlIndex,
                    displayType);

        gVDLItem[screenIndex]
            = CreateVDLWrapper (VDLTYPE_FULL, 40, (void *)vdl[vdlIndex]);
        if (gVDLItem[screenIndex] < 0)
          {
            DIAGNOSE_SYSERR (gVDLItem[screenIndex], ("CreateVDL failed\n"));
            goto DONE;
          }

        oldVDL = SetVDL (sc->sc_Screens[screenIndex], gVDLItem[screenIndex]);
        if (oldVDL < 0)
          {
            DIAGNOSE_SYSERR (oldVDL, ("Can't SetVDL\n"));
            goto DONE;
          }

        DisposeOldVDL (oldVDL);

        vdlIndex++;
      }
  }

  // free up the raw vdl data
  FreeMem (vdl[0], vdlSize);

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

  gVBLIOReq = CreateVBLIOReq ();
  if (gVBLIOReq < 0)
    {
      DIAGNOSE_SYSERR (gVBLIOReq, ("Can't get VBL IOReq\n"));
      goto DONE;
    }

  gVRAMIOReq = CreateVRAMIOReq ();
  if (gVRAMIOReq < 0)
    {
      DIAGNOSE_SYSERR (gVRAMIOReq, ("Can't get VRAM IOReq\n"));
      goto DONE;
    }

  /*	Allocate memory for the screenContext */
  gScreenContext
      = (ScreenContext *)AllocMem (sizeof (ScreenContext), MEMTYPE_ANY);
  if (gScreenContext == NULL)
    {
      PRT (("Can't allocate memory for ScreenContext\n"));
      goto DONE;
    }
  gScreenContext->sc_nScreens = 2;

  if (CreateScreens (gScreenContext) < 0)
    goto DONE;

  // now allocate some memory for the image buffers
  {
    int32 screenIndex;

    gMinImageSize = (int32)(gScreenContext->sc_nFrameByteCount);
    gMaxImageSize = 2 * gMinImageSize;

    for (screenIndex = 0; screenIndex < NUM_BUFFERS; screenIndex++)
      {

        gImage[screenIndex] = (ubyte *)AllocMem (gMaxImageSize, MEMTYPE_ANY);
        if (gImage[screenIndex] == NULL)
          {
            PRT (("Can't allocate gImage[%d]", screenIndex));
            goto DONE;
          }

        // initialize it to black
        memset (gImage[screenIndex], 0, gMaxImageSize);
      }
  }

  if (InitControlPad (2) < 0)
    {
      PRT (("Can't initialize control pad\n"));
      goto DONE;
    }

  retValue = 0;

DONE:
  return retValue;
}

/**** 24-BIT ****/
Err
DisableV (uint32 screenIndex)
{
  Err result = 0;
  TagArg DVAVGTags[] = {
    CREATEVDL_TAG_VAVG,
    (void *)0,
    CREATEVDL_TAG_DONE,
    (void *)0,
  };

  if (gImageSize[screenIndex] == gMinImageSize)
    result = ModifyVDL (gVDLItem[screenIndex], DVAVGTags);
  else
    result = ModifyVDL (gVDLItem[2], DVAVGTags)
             | ModifyVDL (gVDLItem[3], DVAVGTags);

  return result;
}

/**** 24-BIT ****/
Err
EnableV (uint32 screenIndex)
{
  Err result = 0;
  TagArg EVAVGTags[] = {
    CREATEVDL_TAG_VAVG,
    (void *)1,
    CREATEVDL_TAG_DONE,
    (void *)0,
  };

  if (gImageSize[screenIndex] == gMinImageSize)
    result = ModifyVDL (gVDLItem[screenIndex], EVAVGTags);
  else
    result = ModifyVDL (gVDLItem[2], EVAVGTags)
             | ModifyVDL (gVDLItem[3], EVAVGTags);

  return result;
}

/**** 24-BIT ****/
Err
DisableH (uint32 screenIndex)
{
  Err result = 0;
  TagArg DHAVGTags[] = {
    CREATEVDL_TAG_HAVG,
    (void *)0,
    CREATEVDL_TAG_DONE,
    (void *)0,
  };

  if (gImageSize[screenIndex] == gMinImageSize)
    result = ModifyVDL (gVDLItem[screenIndex], DHAVGTags);
  else
    result = ModifyVDL (gVDLItem[2], DHAVGTags)
             | ModifyVDL (gVDLItem[3], DHAVGTags);

  return result;
}

/**** 24-BIT ****/
Err
EnableH (uint32 screenIndex)
{
  Err result = 0;
  TagArg EHAVGTags[] = {
    CREATEVDL_TAG_HAVG,
    (void *)1,
    CREATEVDL_TAG_DONE,
    (void *)0,
  };

  if (gImageSize[screenIndex] == gMinImageSize)
    result = ModifyVDL (gVDLItem[screenIndex], EHAVGTags);
  else
    result = ModifyVDL (gVDLItem[2], EHAVGTags)
             | ModifyVDL (gVDLItem[3], EHAVGTags);

  return result;
}

/**** 24-BIT ****/
void
DoBit15 (bool doToggle)
{
  uint32 *p;
  uint32 n;

  n = gImageSize[gScreenContext->sc_curScreen] >> 2;
  if (gImageSize[gScreenContext->sc_curScreen] == gMinImageSize)
    p = (uint32 *)gScreenContext->sc_Bitmaps[gScreenContext->sc_curScreen]
            ->bm_Buffer;
  else
    p = (uint32 *)gScreenContext->sc_Bitmaps[2]->bm_Buffer;
  if (doToggle)
    {
      while (n--)
        *p++ ^= 0x80008000;
    }
  else
    {
      while (n--)
        *p++ &= 0x7fff7fff;
    }
}

/**** 24-BIT ****/
void
ToggleBit15 (void)
{
  DoBit15 (TRUE);
}

/**** 24-BIT ****/
void
ClearBit15 (void)
{
  DoBit15 (FALSE);
}

void
Cleanup (void)
/*
        Dispose all global resources used by the program.  This mirrors
        the Initialize function:

        - Disposes the image array memory.
        - Kills the control pad utility.
        - Disposes the screengroups, vdls, and ScreenContext
                and closes the graphics folio.
        - Disposes the I/O Request used for vertical blank waiting.
        - Disposes the I/O Request used for SPORT operations.

        We use FreeMem rather than UnloadImage to dispose the image memory
        because we used loadfile24 rather than LoadImage.
*/
{
  int32 imageIndex;

  for (imageIndex = 0; imageIndex < NUM_BUFFERS; imageIndex++)
    FreeMem (gImage[imageIndex], (int32)(gScreenContext->sc_nFrameByteCount));

  KillControlPad ();

  if (gScreenContext)
    //		CloseGraphics( gScreenContext );
    {
      Item screenGroup;
      Screen *screen;
      int32 screenIndex;

      for (screenIndex = 0; screenIndex <= 2; screenIndex += 2)
        {
          if (gScreenContext->sc_Screens[screenIndex] > 0)
            {
              if ((screen = (Screen *)LookupItem (
                       gScreenContext->sc_Screens[screenIndex]))
                  != NULL)
                {
                  screenGroup = screen->scr_ScreenGroupPtr->sg.n_Item;
                  RemoveScreenGroup (screenGroup);
                  DeleteScreenGroup (screenGroup);
                }
            }
        }

      DeleteVDL (gSystemVDL[0]);
      DeleteVDL (gSystemVDL[1]);

      FreeMem (gScreenContext, sizeof (ScreenContext));
    }

  CloseGraphicsFolio ();

  DeleteVBLIOReq (gVBLIOReq);
  DeleteVRAMIOReq (gVRAMIOReq);
}

/**** 24-BIT ****/
int32
DisplayCurrentScreen (void)
{
  int32 retValue = 0;
  int32 currScreen = gScreenContext->sc_curScreen;

  if (gPrevBuff == currScreen)
    goto DONE;

  if (gImageSize[currScreen] == gMinImageSize)
    {
      if (gImageVDLLines[currScreen])
        {
          PRT ((" 16 bit Image with %3d line Custom VDL  %s \n",
                gImageVDLLines[currScreen],
                gFilenames[gImageIndex[currScreen]]));
        }
      else
        {
          PRT ((" 16 bit Image with default VDL          %s \n",
                gFilenames[gImageIndex[currScreen]]));
        }

      memcpy (gScreenContext->sc_Bitmaps[currScreen]->bm_Buffer,
              gImage[currScreen], gImageSize[currScreen]);

      /*	Tell the 3DO display hardware to show this screen every frame.
       */
      retValue = DisplayScreen (gScreenContext->sc_Screens[currScreen], 0);
    }
  else
    {
      PRT ((" 24 bit Image with 24 bit VDL           %s \n",
            gFilenames[gImageIndex[currScreen]]));

      // 24-bit mode doesn't double buffer
      memcpy (gScreenContext->sc_Bitmaps[2]->bm_Buffer, gImage[currScreen],
              gImageSize[currScreen]);
      retValue = DisplayScreen (gScreenContext->sc_Screens[2],
                                gScreenContext->sc_Screens[3]);
    }

  if (retValue < 0)
    {
      DIAGNOSE_SYSERR (retValue, ("DisplayScreen failed\n"));
      goto DONE;
    }

  gPrevBuff = currScreen;

  retValue = 0;

DONE:
  return retValue;
}

void
Usage (void)
/*	Show the user the acceptable command line formats */
{
  PRT (("\nUsage:\nslideshow [imagelist] [-a [n]]\n"));
  PRT (("   imagelist is a text file listing the names of\n"));
  PRT (("   the image files to display. The text file\n"));
  PRT (("   \"imagelist24.txt\" is used if none is specified.\n"));
  PRT (("   when displaying NTSC video.  The name\n"));
  PRT (("   \"palimagelist24.txt\" is used if none is specified.\n"));
  PRT (("   when displaying PAL video.\n"));
  PRT (("   Options:\n"));
  PRT (("   -a [n]  Auto-show a new image every n seconds.\n"));
  PRT (("           4 is used if n isn't specified.\n"));
}

Item
CreateVDLWrapper (int32 vdlType, int32 length, void *dataPtr)
/*
        Convenience routine for creating a VDL item, given VDL type, size in
   int32s, and address of the VDL data.
*/
{
  TagArg VDLTags[] = {
    CREATEVDL_TAG_VDLTYPE, (void *)0, CREATEVDL_TAG_DISPLAYTYPE, (void *)0,
    CREATEVDL_TAG_LENGTH,  (void *)0, CREATEVDL_TAG_HEIGHT,      (void *)0,
    CREATEVDL_TAG_DATAPTR, (void *)0, CREATEVDL_TAG_DONE,        (void *)0
  };
  Item VDLItem;

  VDLTags[0].ta_Arg = (void *)vdlType;
  VDLTags[1].ta_Arg = (void *)gDisplayType;
  VDLTags[2].ta_Arg = (void *)length;
  VDLTags[3].ta_Arg = (void *)gMyScreenHeight;
  VDLTags[4].ta_Arg = (void *)dataPtr;

  VDLItem = CreateVDL (VDLTags);

  return VDLItem;
}
