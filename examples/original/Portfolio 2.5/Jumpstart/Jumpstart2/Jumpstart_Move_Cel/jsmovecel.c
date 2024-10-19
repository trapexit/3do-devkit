
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jsmovecel.c,v 1.17 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/jsmovecel
|||	jsmovecel - Demonstrates moving and distorting cels (a Jumpstart
program).
|||
|||	  Synopsis
|||
|||	    jsmovecel \<imagename> \<celname>
|||
|||	  Description
|||
|||	    Moves and distorts cels against a background image.
|||
|||	    The program loads a background image and a cel, and enters a loop
in which
|||	    the cel is displayed against the background image as it is moved
and
|||	    distorted in response to input from the control pad.
|||
|||	  Arguments
|||
|||	    imagename                    Filename of the image to display as
the
|||	                                 background.
|||
|||	    celname                      Filename of the cel to display in the
|||	                                 foreground.
|||
|||	  Associated Files
|||
|||	    jsmovecel.make, controlpad.h, controlpad.c
|||
|||	  Location
|||
|||	    examples/Jumpstart/Jumpstart2/jsmovecel
|||
**/

#include "celutils.h"
#include "debug3do.h"
#include "displayutils.h"
#include "graphics.h"
#include "string.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

/***** Constants and macro definitions: *****/

#define IMAGE_DEFAULT_FILENAME "jsdata/art/sky.imag"
#define CEL_DEFAULT_FILENAME "jsdata/art/jsufo.cel"

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

/***** Prototypes: *****/

void DrawBackground (Bitmap *bitmap);
int32 HandleControlPad (CCB *ccb);
int32 Initialize (void);
void MoveCCB (CCB *aCCB, int32 xPos, int32 yPos);
void Cleanup (void);

/***** Global variables: *****/

/* I/O request for SPORT calls */
Item gVRAMIOReq;

/* These are needed to display graphics */
ScreenContext *gScreenContext;
Item gDisplayItem = -1; /* result of CreateBasicDisplay */
int32 gScreenSelect = 0;

int32 gXPos = 0;
int32 gYPos = 0;

int32 xMovePt = 0;
int32 yMovePt = 0;

int32 xDistPt = 0;
int32 yDistPt = 0;

Boolean gSetPMode = FALSE;

ubyte *gBackgroundImage = NULL;
CCB *gCcb = NULL;

char gImageName[128], gCelName[128];

/***** Function definitions: *****/

int
main (int argc, char *argv[])
{

  printf ("jsmovecel\n");

  /*
          Extract the imagefile and celfile names from
          the program's command line arguments.
  */
  {
    int32 nameLength;

    strcpy (gImageName, IMAGE_DEFAULT_FILENAME);
    strcpy (gCelName, CEL_DEFAULT_FILENAME);

    if (argc > 1)
      {
        nameLength = strlen (argv[1]);
        if (nameLength > 0)
          strcpy (gImageName, argv[1]);
      }

    if (argc > 2)
      {
        nameLength = strlen (argv[2]);
        if (nameLength > 0)
          strcpy (gCelName, argv[2]);
      }
  }

  /*  Allocate global resources */
  if (Initialize () < 0)
    goto DONE;

  while (TRUE)
    {
      /* React to the controlPad */
      if (HandleControlPad (gCcb) < 0)
        goto DONE;

      /*  If gSetPMode is true (it's toggled by the B button), the active PMode
       *  for this Cel will be forced to 1. This means that whatever settings
       *  were used for PMODE 1 (opaque, translucent, etc.) will be applied to
       *  the entire Cel.
       *  If gSetPMode is false, the Cel will use the PMODE value from the
       * pixel data high bit if it is coded 16 bit, uncoded 16 bit or coded 6
       * bit. If the Cel is not one of the above types, the PMODE pixel will
       * come from the high bit of the PLUT color entry for each color. 8 bit
       * Uncoded Cels will default to zero, as they have no PLUT or PMODE per
       * pixel.
       *
       *  Note: these constants are from hardware.h
       */
      if (gSetPMode)
        {
          gCcb->ccb_Flags &= ~CCB_POVER_MASK;
          gCcb->ccb_Flags |= PMODE_ONE;
        }
      else
        {
          gCcb->ccb_Flags &= ~CCB_POVER_MASK;
          gCcb->ccb_Flags |= PMODE_PDC; /* use P-mode from pixel decoder */
        }

      /*
              "Erase" the screen by copying the background image into the
              current bitmap. This call syncs us up to the vertical blank.
              We should have as short a path as possible from this point
              to the call to DisplayScreen() -- in this case, we have only
              a single call to DrawCels intervening.
       */
      DrawBackground (gScreenContext->sc_Bitmaps[gScreenSelect]);

      /*  Draw (project) the cel into the bitmap. */
      DrawCels (gScreenContext->sc_BitmapItems[gScreenSelect], gCcb);

      /*
              Tell the 3DO display hardware to use this screen to refresh
              the TV display.
      */
      DisplayScreen (gScreenContext->sc_Screens[gScreenSelect], 0);

      /*  Switch to the other screen for our next frame of animation. */
      gScreenSelect = 1 - gScreenSelect;
    }

DONE:
  if (gScreenContext)
    FadeToBlack (gScreenContext, 60);

  Cleanup ();

  printf ("end of jsmovecel\n");

  return 0;
}

int32
Initialize (void)
/*
        Allocate and prepare all of the program's global resources.
        These are:

        - An I/O request for SPORT transfers.
        - A single screen context for handling 2 screens.
        - The event utility used for control pad input.
        - A background image.
        - A cel.

        Returns 0 if all operations are performed successfully, otherwise a
        negative value.
*/

{
  int32 retValue = -1;

  /* Get a VRAM I/O Request to use for all of our SPORT calls */
  gVRAMIOReq = CreateVRAMIOReq ();
  if (gVRAMIOReq < 0)
    {
      PRT (("Can't get a VRAM I/O Request\n"));
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

  /* Open the graphics folio and set up the screen context */
  if ((gDisplayItem = CreateBasicDisplay (gScreenContext, DI_TYPE_DEFAULT, 2))
      < 0)
    {
      PRT (("Can't initialize display\n"));
      goto DONE;
    }

  /* Set the index of the screen we'll be using */
  gScreenContext->sc_curScreen = 0;

  if (InitControlPad (2) < 0)
    {
      PRT (("Can't initialize the control pad\n"));
      goto DONE;
    }

  /* Load the background image */
  if ((gBackgroundImage
       = (ubyte *)LoadImage (gImageName, NULL, NULL, gScreenContext))
      == NULL)
    {
      PRT (("Can't load the background image\n"));
      goto DONE;
    }

  /* Load the foreground cel and ensure that it's the last CCB */
  if ((gCcb = LoadCel (gCelName, MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the foreground cel\n"));
      goto DONE;
    }
  LAST_CEL (gCcb);

  /* Compare cel resizing and distortion with this bit on:
          SetFlag( gCcb->ccb_Flags, CCB_MARIA);
  */
  retValue = 0;

DONE:
  return retValue;
}

void
Cleanup ()
/*
        Dispose all global resources used by the program.  This mirrors
        the Initialize function:

        - Disposes of the cel.
        - Disposes of the background image.
        - Disposes the screen context.
        - Kills the control pad utility.
        - Deletes the I/O request for sport transfers.
*/
{
  UnloadCel (gCcb);               /* UnloadCel checks for a NULL parameter */
  UnloadImage (gBackgroundImage); /* UnloadImage checks for a NULL parameter */

  KillControlPad ();

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);

  DeleteVRAMIOReq (gVRAMIOReq);
}

void
MoveCCB (CCB *aCCB, int32 xPos, int32 yPos)
/*
    Convenience routine to move a cel to the specified int32 coordinates
*/
{
  aCCB->ccb_XPos = Convert32_F16 (xPos);
  aCCB->ccb_YPos = Convert32_F16 (yPos);
}

void
SetQuad (Point *r, Coord left, Coord top, Coord right, Coord bottom)
/*
        Convenience routine to set a quad given the four corners
*/
{
  r[0].pt_X = r[3].pt_X = left;
  r[0].pt_Y = r[1].pt_Y = top;
  r[1].pt_X = r[2].pt_X = right;
  r[2].pt_Y = r[3].pt_Y = bottom;
}

#define kContinuousMask                                                       \
  (ControlA | ControlC | ControlRight | ControlLeft | ControlUp | ControlDown)

int32
HandleControlPad (CCB *ccb)
/*
        Respond to control pad input:

        - All keys used to manipulate the cel are enabled for continuous
   presses.
        - Start button means quit the program.
        - A button and arrows means resize the cel.
        - B button means set toggle P-mode setting between always 1 and
   pixel-dependent.
        - C button and arrows means distort the cel.
        - While just arrows pressed, move the cel in the corresponding
   direction.

        Arguments:

                ccb		pointer to the ccb to manipulate and display

        Returns -1 if user presses start button to quit, otherwise 0.
*/
{
  Point aQuad[4];
  uint32 controlBits;
  int32 retValue = 0;

  DoControlPad (1, &controlBits, kContinuousMask);

  /* If START is pressed, the user wants to quit */
  if (controlBits & ControlStart)
    {
      retValue = -1;
      goto DONE;
    }

  /* While the A button and arrows are pressed, resize the cel */
  if (controlBits & ControlA)
    {
      if (controlBits & ControlRight)
        ++xMovePt;
      else if (controlBits & ControlLeft)
        --xMovePt;

      if (controlBits & ControlUp)
        --yMovePt;
      else if (controlBits & ControlDown)
        ++yMovePt;

      SetQuad (aQuad, gXPos, gYPos, gXPos + ccb->ccb_Width + xMovePt,
               gYPos + ccb->ccb_Height + yMovePt);
      MapCel (gCcb, aQuad);

      goto DONE;
    }

  /* If the B button is pressed, toggle P-mode between always 1 and
   * pixel-dependent */
  if (controlBits & ControlB)
    {
      gSetPMode = !gSetPMode;
      goto DONE;
    }

  /* While the C button and arrows are pressed distort the cel */
  if (controlBits & ControlC)
    {
      if (controlBits & ControlRight)
        {
          ++xDistPt;
        }
      else if (controlBits & ControlLeft)
        {
          --xDistPt;
        }

      if (controlBits & ControlUp)
        {
          --yDistPt;
        }
      else if (controlBits & ControlDown)
        {
          ++yDistPt;
        }

      SetQuad (aQuad, gXPos, gYPos, gXPos + gCcb->ccb_Width + xMovePt,
               gYPos + gCcb->ccb_Height + yMovePt);

      aQuad[2].pt_X += xDistPt;
      aQuad[2].pt_Y += yDistPt;

      MapCel (gCcb, aQuad);

      goto DONE;
    }

  /* While non-opposing arrows are pressed, move the cel appropriately */
  if (controlBits & (ControlUp | ControlDown | ControlLeft | ControlRight))
    {
      if (controlBits & ControlUp)
        {
          if (--gYPos < 0)
            gYPos = 0;
        }
      else if (controlBits & ControlDown)
        {
          if (++gYPos > DISPLAY_HEIGHT)
            gYPos = DISPLAY_HEIGHT;
        }

      if (controlBits & ControlLeft)
        {
          if (--gXPos < 0)
            gXPos = 0;
        }
      else if (controlBits & ControlRight)
        {
          if (++gXPos > DISPLAY_WIDTH)
            gXPos = DISPLAY_WIDTH;
        }

      MoveCCB (gCcb, gXPos, gYPos);
    }

DONE:
  return retValue;
}

void
DrawBackground (Bitmap *bitmap)
/* Copy the background picture to the specified bitmap, using SPORT */
{
  CopyVRAMPages (gVRAMIOReq, bitmap->bm_Buffer, gBackgroundImage,
                 gScreenContext->sc_nFrameBufferPages, -1);
}
