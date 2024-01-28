
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jsanimation.c,v 1.15 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/jsanimation
|||	jsanimation - Demonstrates how to play an animation (a Jumpstart
program).
|||
|||	  Synopsis
|||
|||	    jsanimation
|||
|||	  Description
|||
|||	    Example for displaying a cel at a random position, waiting for
control pad
|||	    input, and playing an explosion animation.
|||
|||	  Associated Files
|||
|||	    jsanimation.c, jsanimation.make
|||
|||	  Location
|||
|||	    examples/Jumpstart/Jumpstart2/jumpstartanimation
|||
|||	    The data files are:JSData/Art/sky.imgsky
imageJSData/Art/jsufo.celUFO cel
|||	    JSData/Art/boom.animexplosion animation
|||
**/

#include "debug3do.h"
#include "displayutils.h"
#include "graphics.h"
#include "operamath.h"
#include "parse3do.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define FRAME_INCREMENT 1 << 16

ScreenContext *gScreenContext;
Item gDisplayItem = -1; /* result of CreateBasicDisplay */

ubyte *gBackgroundImage = NULL;
CCB *gUFO_CCB = NULL;
ANIM *gExplosionAnim;
CCB *gExplosionCCB = NULL;
int32 gBoomCount = 0;
int32 gPostBoomDelay = 0;

void
UnifyAnimation (ANIM *pAnim)
/*
        Ensure that all frames of the animation use the same CCB,
        to facilitate making uniform changes to the position
        and scale.
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

                - A single screen context for handling 2 screens.
                - The event utility used for control pad input.
                - A background image.
                - A cel.

        Returns 0 if all operations are performed successfully, otherwise a
        negative value.
*/
{
  int32 retValue = -1;

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

  if (InitControlPad (2) < 0)
    {
      PRT (("Can't initialize the control pad\n"));
      goto DONE;
    }

  if ((gBackgroundImage = (ubyte *)LoadImage ("jsdata/art/sky.imag", NULL,
                                              NULL, gScreenContext))
      == NULL)
    {
      PRT (("Can't load the background image\n"));
      goto DONE;
    }

  if ((gUFO_CCB = LoadCel ("jsdata/art/jsufo.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the foreground cel\n"));
      goto DONE;
    }
  LAST_CEL (gUFO_CCB);

  if ((gExplosionAnim = LoadAnim ("jsdata/art/boom.anim", MEMTYPE_CEL)) == 0)
    {
      PRT (("Can't load the explosion animation\n"));
      goto DONE;
    }
  UnifyAnimation (gExplosionAnim);
  gExplosionCCB = GetAnimCel (gExplosionAnim, 0);
  LAST_CEL (gExplosionCCB);

  retValue = 0;

DONE:
  return retValue;
}

uint32
Random (uint32 n)
/*
        Return a random number between 0 and n - 1
        The return value has 16 bits of significance,
        and should be treated as unsigned.
*/
{
  uint32 i, j, k;

  i = (uint32)rand () << 1;
  j = i & 0x0000FFFF;
  k = i >> 16;
  return ((((j * n) >> 16) + k * n) >> 16);
}

Coord
CalcCenterCoord (int32 aCoord, int32 sideLength)
/*
        Convenience function to return the center location of a rectangle side
   starting at aCoord, with the specified length.
*/
{
  return aCoord + sideLength / 2;
}

Point
CalcCCBCenterPoint (CCB *aCCB)
/*
        Convenience function to return the center location of a cel in a Point
   structure.
*/
{
  Point aPoint;

  aPoint.pt_X
      = CalcCenterCoord (ConvertF16_32 (aCCB->ccb_XPos), aCCB->ccb_Width);
  aPoint.pt_Y
      = CalcCenterCoord (ConvertF16_32 (aCCB->ccb_YPos), aCCB->ccb_Height);

  return aPoint;
}

void
MoveCCB (CCB *aCCB, int32 xPos, int32 yPos)
/*
    Convenience function to move a cel to the specified int32 coordinates
*/
{
  aCCB->ccb_XPos = Convert32_F16 (xPos);
  aCCB->ccb_YPos = Convert32_F16 (yPos);
}

void
CenterCCB (CCB *aCCB, Coord xCenter, Coord yCenter)
/*
        Move a CCB so that its center is (xCenter, yCenter).
*/
{
  int32 aXPos;
  int32 aYPos;

  aXPos = xCenter - aCCB->ccb_Width / 2;
  aYPos = yCenter - aCCB->ccb_Height / 2;

  MoveCCB (aCCB, aXPos, aYPos);
}

#define kContinuousMask (ControlA)

int32
HandleControlPad (void)
/*
        Respond to control pad input:

          - The A key is enabled for continuous presses.
          - Start button means quit the program.
          - A button and arrows means fire (explode the cel).

        Returns -1 if the user pressed the start button to quit,
        otherwise 0.
*/
{
  uint32 controlBits;
  Point aUFOCenter;
  int32 retValue = 0;

  DoControlPad (1, &controlBits, kContinuousMask);

  if (controlBits & ControlStart)
    /*
                If the user has pressed the START button,
                the game is over.
    */
    {
      retValue = -1;
      goto DONE;
    }

  /* ControlA triggers the explosion */
  if (controlBits & ControlA)
    {
      if (!gBoomCount) /* Don't fire until the previous explosion is finished
                        */
        {
          gBoomCount = gExplosionAnim->num_Frames;

          /* Position the center of the explosion at the center of the UFO */
          aUFOCenter = CalcCCBCenterPoint (gUFO_CCB);
          CenterCCB (gExplosionCCB, aUFOCenter.pt_X, aUFOCenter.pt_Y);

          /* Get a random center for the next UFO */
          CenterCCB (gUFO_CCB, (Coord)Random (DISPLAY_WIDTH),
                     (Coord)Random (DISPLAY_HEIGHT));
        }
    }

DONE:
  return retValue;
}

void
TargetAction (CCB *aTarget)
/*
        Animate the UFO: If it's still exploding, play the explosion's next
   frame, otherwise display the UFO (after a nominal post-explosion delay).
*/
{
  if (gBoomCount)
    {
      gBoomCount--;
      if (gBoomCount)
        {
          GetAnimCel (gExplosionAnim, FRAME_INCREMENT);
          DrawCels (
              gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen],
              gExplosionCCB);
        }
      else
        {
          gExplosionAnim->cur_Frame
              = 0; /* Reset explosion animation to its first frame */
          gPostBoomDelay = gExplosionAnim->num_Frames
                           * 5; /* Wait awhile before showing the target */
        }
    }
  else
    {
      if (gPostBoomDelay)
        gPostBoomDelay--;
      if (!gPostBoomDelay)
        DrawCels (gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen],
                  aTarget);
    }
}

void
Cleanup (void)
/*
        Dispose all global resources used by the program.  This mirrors
        the Initialize function:

                - Disposes of the cel.
                - Disposes of the background image.
                - Disposes of the explosion animation.
                - Disposes the screen context.
                - Kills the control pad utility.

        All of the UnloadÉ calls check for a NULL parameter.
*/
{
  KillControlPad ();

  UnloadAnim (gExplosionAnim);
  UnloadCel (gUFO_CCB);
  UnloadImage (gBackgroundImage);

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);
}

int
main (int argc, char **argv)
{
  printf ("jsanimation\n");

  if (Initialize () < 0)
    goto DONE;

  gScreenContext->sc_curScreen = 0;

  while (HandleControlPad () >= 0)
    {
      /* Draw the background image */
      DrawImage (gScreenContext->sc_Screens[gScreenContext->sc_curScreen],
                 gBackgroundImage, gScreenContext);

      /* Handle the drawing of the UFO cel and the explosion */
      TargetAction (gUFO_CCB);

      /*
              Display the current screen.

              DisplayScreen waits for the next vertical blank, then
              tells the display hardware to use the specified screen as the
              display buffer. The hardware will continue showing
              this buffer on the TV every frame until another call
              to DisplayScreen specifies a different buffer.
      */
      DisplayScreen (gScreenContext->sc_Screens[gScreenContext->sc_curScreen],
                     0);

      /* Toggle the current screen */
      gScreenContext->sc_curScreen = 1 - gScreenContext->sc_curScreen;
    }

DONE:
  if (gScreenContext)
    FadeToBlack (gScreenContext, 60);

  Cleanup ();

  printf ("end of jsanimation.\n");

  return 0;
}
