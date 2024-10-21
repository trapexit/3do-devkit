
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: animationvdl.c,v 1.20 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/animationvdl
|||	animationvdl - Manipulates an animation against a background image.
|||
|||	  Synopsis
|||
|||	    animationvdl \<imageName> \<animName>
|||
|||	  Description
|||
|||	    Moves and distorts cels against a background image.
|||
|||	    The program loads a background image and an animation and enters a
loop in
|||	    which each cel of the animation is displayed against the background
image
|||	    and manipulated in response to control pad input.
|||
|||	  Arguments
|||
|||	    imageName                    Filename of image to use as
background.
|||
|||	    animName                     Filename of animation to display in
|||	                                 foreground
|||
|||	  Associated Files
|||
|||	    animationvdl.make
|||
|||	    The data files are:seafloor.img Default image
filenamefish.animDefault
|||	    animation filename
|||
|||	  Location
|||
|||	    examples/Graphics/Slideshow
|||
**/

#include "animutils.h"
#include "celutils.h"
#include "debug3do.h"
#include "displayutils.h"
#include "graphics.h"
#include "io.h"
#include "mem.h"
#include "string.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#include "vdlutil.h"

/***** Constants and macro definitions: *****/

#define IMAGE_DEFAULT_FILENAME "seafloor.imag"
#define ANIM_DEFAULT_FILENAME "fish.anim"

#define NUM_SCREENS 2
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define FRAME_INCREMENT (1 << 16)

/***** Prototypes: *****/

void DrawBackground (Bitmap *bitmap);
int32 HandleControlPad (CCB *ccb);
int32 Initialize (void);
void MoveCCB (CCB *aCCB, int32 xPos, int32 yPos);
void Cleanup (void);
void UnifyAnimation (ANIM *pAnim);
void WriteVDLCtlWords (VDLDisplayCtlWord dmaWord, int32 *activeVDLPtr);

/***** Global variables: *****/

/* I/O request for SPORT calls */
Item gVRAMIOReq;

/* These are needed to display graphics */
ScreenContext *gScreenContext = NULL;
Item gDisplayItem = -1; /* result of CreateBasicDisplay */
int32 gScreenSelect = 0;

int32 gXPos = 0;
int32 gYPos = 0;

int32 xMovePt = 0;
int32 yMovePt = 0;

int32 xDistPt = 0;
int32 yDistPt = 0;

bool gSetPMode = FALSE;
bool gReverse = FALSE;

ubyte *gBackgroundImage = NULL;
ANIM *gAnim = NULL;
CCB *gCel = NULL;

char gImageName[128];
char gAnimName[128];

VDLDisplayCtlWord gDmaWord;
Item gVDLItems[NUM_SCREENS];            // actual VDL items
VDLEntry *gRawVDLPtrArray[NUM_SCREENS]; // our VDLs

/***** Function definitions: *****/

int
main (int argc, char *argv[])
{

  printf (("animationvdl\n"));

  /*
          Extract the imagefile name, and celfile name from
          the arguments passed into the program.
  */
  {
    int32 nameLength;

    strcpy (gImageName, IMAGE_DEFAULT_FILENAME);
    strcpy (gAnimName, ANIM_DEFAULT_FILENAME);

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
          strcpy (gAnimName, argv[2]);
      }
  }

  /*  Allocate global resources */
  if (Initialize ())
    goto DONE;

  while (TRUE)
    {
      /* React to the controlPad */
      if (HandleControlPad (gCel) < 0)
        goto DONE;

      gCel = GetAnimCel (gAnim, FRAME_INCREMENT);

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
      gCel->ccb_Flags &= ~CCB_POVER_MASK;
      if (gSetPMode)
        {
          SetFlag (gCel->ccb_Flags, PMODE_ONE);
        }
      else
        {
          gCel->ccb_Flags |= PMODE_PDC; /* use P-mode from pixel decoder */
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
      DrawCels (gScreenContext->sc_BitmapItems[gScreenSelect], gCel);

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

  printf ("end of animationvdl\n");

  return 0;
}

/*
        Allocates the gBackgroundImage buffer.  If a fileName is specified,
        the routine loads the file, otherwise, merely zeroes the buffer.
        Returns 0 if all operations succeed, otherwise, -1.
*/
int32
InitBackgroundImage (char *fileName)
{
  int32 retValue = -1;
  VdlChunk *rawVDLPtr = NULL;
  int32 screenIndex;
  Item oldVDL;
  int32 errorCode;

  /* Allocate and initialize memory for the raw VDL. */
  for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
    {
      errorCode = AllocateVDL (&gRawVDLPtrArray[screenIndex],
                               gScreenContext->sc_Bitmaps[screenIndex],
                               DI_TYPE_NTSC);
      if (errorCode != 0)
        {
          DIAGNOSE_SYSERR (errorCode, ("Can't allocate rawVDL\n"));
          goto DONE;
        }
    }

  gBackgroundImage
      = (ubyte *)AllocMem ((int)(gScreenContext->sc_nFrameByteCount),
                           MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);
  if (gBackgroundImage == NULL)
    {
      PRT (("Can't allocate gBackgroundImage\n"));
      goto DONE;
    }

  SetVRAMPages (gVRAMIOReq, gBackgroundImage, 0,
                gScreenContext->sc_nFrameBufferPages, -1);

  /* Load the image into the buffer; if it has a custom VDL, it's returned in
   * rawVDLPtr. */
  if (LoadImage (fileName, gBackgroundImage, &rawVDLPtr, gScreenContext)
      == NULL)
    {
      PRT (("LoadImage failed\n"));
      goto DONE;
    }

  /* if the image has a custom VDL, merge its data into our raw vdl */
  if ((rawVDLPtr) && (rawVDLPtr->chunk_size))
    {
      int32 vdlLines = rawVDLPtr->vdlcount;

      /* incorporate the custom VDL into the screen's VDL */
      for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
        MergeVDL (&rawVDLPtr->vdl[0], (VDL_REC *)gRawVDLPtrArray[screenIndex],
                  vdlLines, DI_TYPE_NTSC);

      LONGWORD (gDmaWord) = ((VDL_REC *)(gRawVDLPtrArray[0]))->displayControl;

      /* Dispose of the custom VDL; it's no longer needed */
      FreeMem (rawVDLPtr, rawVDLPtr->chunk_size);
    }

  for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
    {
      /* Validate the screen's VDL; the size argument is in int32s */
      gVDLItems[screenIndex]
          = SubmitVDL (gRawVDLPtrArray[screenIndex],
                       (GetVDLSize (DI_TYPE_NTSC) / 4), VDLTYPE_FULL);
      if (gVDLItems[screenIndex] < 0)
        {
          DIAGNOSE_SYSERR (gVDLItems[screenIndex], ("SubmitVDL failed\n"));
          goto DONE;
        }

      /* Activate the new VDL; the previously-used VDL is returned. */
      oldVDL = SetVDL (gScreenContext->sc_Screens[screenIndex],
                       gVDLItems[screenIndex]);
      if (oldVDL < 0)
        {
          DIAGNOSE_SYSERR (oldVDL, ("SetVDL failed\n"));
          goto DONE;
        }

      /* Delete the previously-used VDL; it's no longer needed */
      DeleteVDL (oldVDL);
    }

  retValue = 0;

DONE:
  return retValue;
}

void
UnifyAnimation (ANIM *pAnim)
/*
        Make all frames of the animation use the same CCB.
        This facilitates uniform setting of position and
        scale.
*/
{
  int frameIndex;
  CCB *theCCB;

  theCCB = pAnim->pentries[0].af_CCB;
  for (frameIndex = 0; frameIndex < pAnim->num_Frames; frameIndex++)
    {
      pAnim->pentries[frameIndex].af_CCB = theCCB;
    }
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
      PRT (("Can't get a VRAM IOReq\n"));
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
  gScreenContext->sc_curScreen = 0;

  PRT (("Initializing control pad.\n"));
  if (InitControlPad (2) < 0)
    {
      PRT (("Can't initialize the control pad\n"));
      goto DONE;
    }

  /* Load the background image */
  PRT (("Loading the background image\n"));
  if (InitBackgroundImage (gImageName) < 0)
    goto DONE;

  /* Load the animation from which we'll get the foreground cels */
  PRT (("Loading the animation\n"));
  if ((gAnim = LoadAnim (gAnimName, MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the animation\n"));
      goto DONE;
    }

  UnifyAnimation (gAnim);
  gCel = GetAnimCel (gAnim, 0);
  if (gCel == NULL)
    {
      PRT (("Can't get the first cel\n"));
      goto DONE;
    }
  LAST_CEL (gCel);

  /* Compare cel resizing and distortion with this bit on:
          SetFlag( gCel->ccb_Flags, CCB_MARIA);
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

        - Disposes the animation.
        - Disposes the background image.
        - Disposes the screen context.
        - Disposes the VDL array.
        - Kills the control pad utility.
        - Deletes the I/O request for SPORT transfers.

        You shouldn't dispose the cel because its memory is
        disposed with the animation.
*/
{
  UnloadAnim (gAnim); /* UnloadAnim checks for a NULL parameter */

  if (gBackgroundImage && gScreenContext)
    FreeMem (gBackgroundImage, (int32)(gScreenContext->sc_nFrameByteCount));

  {
    int32 screenIndex;

    /* Free the memory allocated for the raw VDL. */
    for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
      FreeMem (gRawVDLPtrArray[screenIndex], -1);
  }

  KillControlPad ();

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);

  DeleteVRAMIOReq (gVRAMIOReq);
}

void
MoveCCB (CCB *aCCB, int32 xPos, int32 yPos)
/* Convenience routine to move a cel to the specified int32 coordinates */
{
  aCCB->ccb_XPos = Convert32_F16 (xPos);
  aCCB->ccb_YPos = Convert32_F16 (yPos);
}

void
SetQuad (Point *r, Coord left, Coord top, Coord right, Coord bottom)
/* Convenience routine to set a quad given the four corners */
{
  r[0].pt_X = r[3].pt_X = left;
  r[0].pt_Y = r[1].pt_Y = top;
  r[1].pt_X = r[2].pt_X = right;
  r[2].pt_Y = r[3].pt_Y = bottom;
}

#define kContinuousMask (ControlRight | ControlLeft | ControlUp | ControlDown)

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

        ccb		pointer to the ccb to manipulate and display

        Returns -1 if user wants to quit, otherwise 0.
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

  // button A diddles with the VDL control word bits.
  if (controlBits & ControlA)
    {

      // *******
      // ******* Use with caution!!!  Several of the bits cause a nasty system
      // meltdown!!!!!
      // *******

      if (controlBits & ControlRight)
        {
          ShowAnotherField (gDmaWord, TRUE);
        }
      else if (controlBits & ControlLeft)
        {
          ShowAnotherField (gDmaWord, FALSE);
        }
      else if (controlBits & ControlUp)
        {
          SetCtlField (&gDmaWord, TRUE);
        }
      else if (controlBits & ControlDown)
        {
          SetCtlField (&gDmaWord, FALSE);
        }

      // write the changes if both button A and B are down
      if ((controlBits & ControlA) && (controlBits & ControlB))
        {
          WriteVDLCtlWords (gDmaWord, (int32 *)gRawVDLPtrArray[0]);
          WriteVDLCtlWords (gDmaWord, (int32 *)gRawVDLPtrArray[1]);
          PRT (("VDL control word written: [%X]\n", LONGWORD (gDmaWord)));
        }

      goto DONE;
    }

  /* If the B button is pressed, toggle P-mode between always 1 and
   * pixel-dependent */
  if (controlBits & ControlB)
    {
      if (controlBits & ControlRight)
        {
          static int32 state = B0POS_PPMP;
          switch (state)
            {
            case B0POS_0:
              state = B0POS_1;
              PRT (("CEControl B0POS = B0POS_1\n"));
              break;

            case B0POS_1:
              state = B0POS_PPMP;
              PRT (("CEControl B0POS = B0POS_PPMP\n"));
              break;

            case B0POS_PPMP:
              state = B0POS_PDC;
              PRT (("CEControl B0POS = B0POS_PDC\n"));
              break;

            case B0POS_PDC:
              state = B0POS_0;
              PRT (("CEControl B0POS = B0POS_0\n"));
              break;
            }

          SetCEControl (
              gScreenContext->sc_BitmapItems[0],
              (gScreenContext->sc_Bitmaps[0]->bm_CEControl & ~B0POS_MASK)
                  | state,
              B0POS_MASK);
          SetCEControl (
              gScreenContext->sc_BitmapItems[1],
              (gScreenContext->sc_Bitmaps[0]->bm_CEControl & ~B0POS_MASK)
                  | state,
              B0POS_MASK);
          goto DONE;
        }
      else if (controlBits & ControlLeft)
        {
          static int32 b15state = B15POS_0;
          switch (b15state)
            {
            case B15POS_0:
              b15state = B15POS_1;
              PRT (("CEControl B15POS = B15POS_1\n"));
              break;

            case B15POS_1:
              b15state = B15POS_PDC;
              PRT (("CEControl B15POS = B15POS_PDC\n"));
              break;

            case B15POS_PDC:
              b15state = B15POS_0;
              PRT (("CEControl B15POS = B15POS_0\n"));
              break;

            case B0POS_PDC:
              b15state = B0POS_0;
              PRT (("CEControl = B0POS_0\n"));
              break;
            }

          SetCEControl (
              gScreenContext->sc_BitmapItems[0],
              (gScreenContext->sc_Bitmaps[0]->bm_CEControl & ~B15POS_MASK)
                  | b15state,
              B15POS_MASK);
          SetCEControl (
              gScreenContext->sc_BitmapItems[1],
              (gScreenContext->sc_Bitmaps[0]->bm_CEControl & ~B15POS_MASK)
                  | b15state,
              B15POS_MASK);
          goto DONE;
        }
      else if (controlBits & ControlUp)
        {
          --yMovePt;
        }
      else if (controlBits & ControlDown)
        {
          ++yMovePt;
        }
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

      SetQuad (aQuad, gXPos, gYPos, gXPos + gCel->ccb_Width + xMovePt,
               gYPos + gCel->ccb_Height + yMovePt);

      aQuad[2].pt_X += xDistPt;
      aQuad[2].pt_Y += yDistPt;

      MapCel (gCel, aQuad);

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

      MoveCCB (gCel, gXPos, gYPos);
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

void
WriteVDLCtlWords (VDLDisplayCtlWord dmaWord, int32 *activeVDLPtr)
/*
        Open heart surgery...  Change the display control word for an active
   VDL and activate it.  Return the old VDL so the caller can destroy it.
*/
{
  uint32 scanLineEntry;
  int32 screenIndex;
  Item oldVDL = -1;
  VDL_REC *activeVDLArrayPtr = (VDL_REC *)activeVDLPtr;

  // write the control word into each VDL entry
  for (scanLineEntry = 0; scanLineEntry < DISPLAY_HEIGHT; scanLineEntry++)
    ((VDL_REC *)(&activeVDLArrayPtr[scanLineEntry]))->displayControl
        = LONGWORD (dmaWord);

  // hand our raw vdls to the system, let it validate and copy them for it's
  // own
  //  use
  for (screenIndex = 0; screenIndex < NUM_SCREENS; screenIndex++)
    {
      // SubmitVDL wants it's size arg in WORDS
      gVDLItems[screenIndex]
          = SubmitVDL (gRawVDLPtrArray[screenIndex],
                       (GetVDLSize (DI_TYPE_NTSC) / 4), VDLTYPE_FULL);
      if (gVDLItems[screenIndex] < 0)
        {
          DIAGNOSE_SYSERR (gVDLItems[screenIndex], ("SubmitVDL failed\n"));
          goto DONE;
        }

      // activate the new VDL
      oldVDL = SetVDL (gScreenContext->sc_Screens[screenIndex],
                       gVDLItems[screenIndex]);
      if (oldVDL < 0)
        {
          DIAGNOSE_SYSERR (gVDLItems[screenIndex], ("SetVDL failed\n"));
          goto DONE;
        }

      // Even though we've already told the system to use the the newly
      // modified,
      //  the hardware is still using the old one.  because the system sets a
      //  screen to black if DeleteVDL is called for an active VDL, tell the
      //  hardware to use the new one before we toss the old one
    DONE:
      DisplayScreen (gScreenContext->sc_Screens[gScreenSelect], 0);
      if (oldVDL >= 0)
        DeleteVDL (oldVDL);
    }
}

void
Usage (char *progName)
{
  PRT (("usage: animationvdl [imagefile] [animfile]\n"));
  PRT (("   Button A plus right/left arrow cycles 'focus' through the bits "
        "in\n"));
  PRT (("   the VDL control word.\n"));
  PRT (("   Button A plus up/down cycles through the possible values for "
        "the\n"));
  PRT (("      VDL control word bit in 'focus'.\n"));
  PRT ((
      "   Button A and B AT THE SAME TIME write the VDL control word into\n"));
  PRT (("      the current screen VDL.\n"));
  PRT (("\n"));
  PRT (
      ("   Button B and left arrow cycles through Cel Engine control word\n"));
  PRT (("      (CECONTROL) B15POS_MASK values.\n"));
  PRT (("   Button B and right arrow cycles through Cel Engine control word "
        "bit\n"));
  PRT (("      (CECONTROL) B0POS_PDC values.\n"));
  PRT (("\n"));
  PRT (("   Button C and right/left/up/down arrows stretches the cel or "
        "anim.\n"));
  PRT (("   Arrows by themselves move the cel.\n"));
}
