
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jsinteractivesound.c,v 1.14 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/jsinteractivesound
|||	jsinteractivesound - Demonstrates using sound effects (a Jumpstart
|||	    program).
|||
|||	  Synopsis
|||
|||	    jsinteractivesound
|||
|||	  Description
|||
|||	    UFO game with a sound-effect.
|||
|||	  Associated Files
|||
|||	    jsinteractivesound.make
|||
|||	  Location
|||
|||	    examples/Jumpstart/Jumpstart2/jsinteractivesound
|||
**/

#include "audio.h"
#include "celutils.h"
#include "debug3do.h"
#include "displayutils.h"
#include "init3do.h"
#include "parse3do.h"
#include "stdio.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#include "effectshandler.h"

#define UFO_MINDISTANCE 256
#define UFO_MAXDISTANCE 8000

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define LEFT_EDGE 10
#define RIGHT_EDGE 310
#define TOP_EDGE 10
#define BOTTOM_EDGE 230

#define LASER_LEFT 0
#define LASER_RIGHT 319
#define LASER_BOTTOM 210
/* (the laser top isn't constant -- it's at the calculated focal point) */

/*
        Scale a length for distance.  This is used to
        determine an object's virtual size.  The function
        result is inversely proportional to the object's
        distance, returning the input value as its maximum value
        at the closest possible distance ( UFO_MINDISTANCE,
        i.e., 256, or 1 << 8 )
*/
#define PROJECTED(_length) (((_length) << 8) / gDistance)

ScreenContext *gScreenContext;
Item gDisplayItem = -1; /* result of CreateBasicDisplay */

int32 gDistance = UFO_MAXDISTANCE;
int32 gScore = 0;
int32 gMisses = 0;
pTSampleInfo gZapEffect = NULL;

ubyte *gBackgroundImage = NULL;
CCB *gUFOCel = NULL;
CCB *gCockpitCel = NULL;
CCB *gCrosshairsCel = NULL;
CCB *gLaserCel = NULL;
CCB *gOptionsCel = NULL;

Item gVBLIOReq; /* I/O request for waiting for vertical blank */

pTMixerInfo gMixerInfo = NULL; /*	a mixer for audio output */

bool
Zap (CCB *aTarget, Coord X, Coord Y)
/*
        Play the zap sound effect, show the laser firing at the
        focal point, and detect if that point is within the projected
        target bounds.
*/
{
  static TagArg taStartArgs[]
      = { { AF_TAG_FREQUENCY, (void *)0x8000 },
          { AF_TAG_AMPLITUDE, (void *)MAXDSPAMPLITUDE },
          { 0, 0 } };

  static Point quad[4] = { { 0, 0 },
                           { 0, 0 },
                           { LASER_RIGHT, LASER_BOTTOM },
                           { LASER_LEFT, LASER_BOTTOM } };

  /*
              Map the topLeft and topRight corners of the laser
              to the focal point.
      */
  quad[0].pt_X = quad[1].pt_X = X;
  quad[0].pt_Y = quad[1].pt_Y = Y;

  MapCel (gLaserCel, quad);

  StartInstrument (gZapEffect->si_Player, taStartArgs);

  /*
              Detect if (X, Y) is inside
              the target, first converting from absolute
              to target-relative coordinates.
      */
  X -= ConvertF16_32 (aTarget->ccb_XPos);
  Y -= ConvertF16_32 (aTarget->ccb_YPos);
  return ((X >= 0) && (X < PROJECTED (aTarget->ccb_Width)) && (Y >= 0)
          && (Y < PROJECTED (aTarget->ccb_Height)));
}

#define kContinuousMask                                                       \
  (ControlA | ControlB | ControlC | ControlRight | ControlLeft | ControlDown  \
   | ControlUp)

int32
DoOptions (void)
/*
        Allow user to choose game options from a special screen
        via the control pad.  The UFO is specially positioned, scaled
        and displayed in the foreground.

        Control pad input is handled as follows:

                Start button means resume playing the game.
                C button means quit the game.

        Returns -1 if the user chose Quit by pressing button C, otherwise 0.

        In a real title, you would have more options, perhaps on multiple
        screens.  This function is really just a minimal shell for options-
        handling logic.
*/
{
  int32 xUFO, yUFO, hdxUFO, vdyUFO;
  uint32 controlBits;
  int32 retValue = 0;

  /*	Save the current UFO position and scale */
  xUFO = gUFOCel->ccb_XPos;
  yUFO = gUFOCel->ccb_YPos;
  hdxUFO = gUFOCel->ccb_HDX;
  vdyUFO = gUFOCel->ccb_VDY;

  /*	Set the "options" UFO position and scale to half-size */
  gUFOCel->ccb_XPos = (176 << 16);
  gUFOCel->ccb_YPos = (67 << 16);
  gUFOCel->ccb_HDX = 1 << 19;
  gUFOCel->ccb_VDY = 1 << 15;

  /*	Draw the options screen */
  DrawImage (gScreenContext->sc_Screens[gScreenContext->sc_curScreen],
             gBackgroundImage, gScreenContext);
  DrawCels (gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen],
            gUFOCel);
  DrawCels (gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen],
            gOptionsCel);
  DisplayScreen (gScreenContext->sc_Screens[gScreenContext->sc_curScreen], 0);

  while (TRUE)
    {
      DoControlPad (1, &controlBits, (ControlA | ControlB | ControlC));

      if (controlBits & ControlStart)
        {
          /* Restore the entry values of the UFO position and scale */
          gUFOCel->ccb_XPos = xUFO;
          gUFOCel->ccb_YPos = yUFO;
          gUFOCel->ccb_HDX = hdxUFO;
          gUFOCel->ccb_VDY = vdyUFO;

          goto DONE;
        }
      else
        {
          if (controlBits & ControlA)
            {
            }

          if (controlBits & ControlB)
            {
            }

          if (controlBits & ControlC)
            /*	Quit the program */
            {
              retValue = -1;
              goto DONE;
            }
        }

      WaitVBL (gVBLIOReq, 1);
    }

DONE:
  return retValue;
}

int32
HandleControlPad (void)
/*
        Respond to control pad input:

                Start button means get game options.
                A button means fire the laser at the UFO.
                B button means move the crosshairs faster.
                Arrows move the crosshairs in the corresponding direction.


        Returns -1 if the user chose Quit from the game options,
        otherwise 0.
*/
{
  int32 controlMove;
  uint32 controlBits;
  int32 retValue = 0;

  DoControlPad (1, &controlBits, kContinuousMask);

  if (controlBits & ControlStart)
    {
      retValue = DoOptions ();
      goto DONE;
    }
  else
    {
      if (controlBits & ControlA)
        {
          gLaserCel->ccb_Flags &= ~CCB_SKIP;
          if (Zap (gUFOCel,
                   ConvertF16_32 (gCrosshairsCel->ccb_XPos)
                       + (gCrosshairsCel->ccb_Width >> 1),
                   ConvertF16_32 (gCrosshairsCel->ccb_YPos)
                       + (gCrosshairsCel->ccb_Width >> 1)))
            /*
                    The player center of the crosshairs is on target, so we hit
               it. Increment the score and start a new target approach.
            */
            {
              gScore += gDistance; /* The player's a better shot if he hits the
                                      UFO when its further away */
              gDistance = UFO_MAXDISTANCE;
            }
        }

      if (controlBits & ControlB)
        /*	Move the crosshairs faster */
        controlMove = Convert32_F16 (6);
      else
        controlMove = Convert32_F16 (2);

      if (controlBits & ControlC)
        /*	No functionality implemented */
        {
        }

      /*	Arrows move the crosshairs within the limits of the screen */
      if ((controlBits & ControlRight)
          && (gCrosshairsCel->ccb_XPos
              < Convert32_F16 (DISPLAY_WIDTH - gCrosshairsCel->ccb_Width)))
        gCrosshairsCel->ccb_XPos += controlMove;

      if ((controlBits & ControlLeft) && (gCrosshairsCel->ccb_XPos > 0))
        gCrosshairsCel->ccb_XPos -= controlMove;

      if ((controlBits & ControlDown)
          && (gCrosshairsCel->ccb_YPos
              < Convert32_F16 (DISPLAY_HEIGHT - gCrosshairsCel->ccb_Height)))
        gCrosshairsCel->ccb_YPos += controlMove;

      if ((controlBits & ControlUp) && (gCrosshairsCel->ccb_YPos > 0))
        gCrosshairsCel->ccb_YPos -= controlMove;
    }

DONE:
  return retValue;
}

void
TargetAction (CCB *aTarget)
/*
        Animate the UFO:

        - Determine if it's still approaching.
        - Bounce off the edges of the screen.
        - Set the sound effect volume and balance to correspond to the location
                of the target.
*/
{
  static int32 iDeltaX = 8 << 24;
  static int32 iDeltaY = 12 << 24;

  int32 iTestX;
  int32 iTestY;

  int32 iTest;
  int32 Balance;
  int32 Volume;

  if (gDistance < UFO_MINDISTANCE)
    {
      gDistance = UFO_MAXDISTANCE;
      gMisses++;
    }
  else
    gDistance -= 60; /* Continue the target approach */

  /* Make the target larger as it approaches */
  aTarget->ccb_HDX = (1 << 28) / gDistance;
  aTarget->ccb_VDY = (1 << 24) / gDistance;

  /* Calculate the new x position and bounce off the sides */
  aTarget->ccb_XPos += (iDeltaX / gDistance);
  iTestX = ConvertF16_32 (aTarget->ccb_XPos);
  iTest = iTestX + PROJECTED (aTarget->ccb_Width);
  if (iTestX > (RIGHT_EDGE - aTarget->ccb_Width))
    {
      aTarget->ccb_XPos = Convert32_F16 (RIGHT_EDGE - aTarget->ccb_Width);
      iDeltaX *= -1;
    }
  else if (iTestX < LEFT_EDGE)
    {
      aTarget->ccb_XPos = Convert32_F16 (LEFT_EDGE);
      iDeltaX *= -1;
    }

  /* Make the zap louder the closer the UFO is */
  iTest -= ConvertF16_32 (aTarget->ccb_XPos);
  Balance = ((aTarget->ccb_XPos + (iTest << 15)) >> 2) / 160;
  Volume = ((UFO_MAXDISTANCE - gDistance) << 15) / UFO_MAXDISTANCE;
  ehSetChannelLevels (gMixerInfo, gZapEffect->si_LeftGainKnob,
                      gZapEffect->si_RightGainKnob, Volume, Balance);

  aTarget->ccb_YPos += (iDeltaY / gDistance);
  iTestY = ConvertF16_32 (aTarget->ccb_YPos);
  if (iTestY > (BOTTOM_EDGE - aTarget->ccb_Height))
    {
      aTarget->ccb_YPos = ConvertF16_32 (BOTTOM_EDGE - aTarget->ccb_Height);
      iDeltaY *= -1;
    }
  else if (iTestY < TOP_EDGE)
    {
      aTarget->ccb_YPos = ConvertF16_32 (TOP_EDGE);
      iDeltaY *= -1;
    }

  return;
}

int32
Initialize (void)
/*
        Allocate and prepare all of the program's global resources.
        These are:

                - A VBL I/O Request used to wait for the vertical blank.
                - A single screen context for handling 2 screens.
                - The control pad utility.
                - A background image.
                - Numerous cels for each of the game's graphic objects.
                - A sound effect for the laser "zap".

        Returns 0 if all operations are performed successfully, otherwise a
        negative value.
*/
{
  int32 retValue = -1;

  /* Create the I/O request we'll use for VBL waiting */
  gVBLIOReq = CreateVBLIOReq ();
  if (gVBLIOReq < 0)
    {
      PRT (("Can't get a VBL I/O Request\n"));
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

  if (OpenAudioFolio ())
    {
      PRT (("Can't open the audio folio\n"));
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
      PRT (("Can't load the sky image\n"));
      goto DONE;
    }

  if ((gUFOCel = LoadCel ("jsdata/art/jsufo.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the UFO cel\n"));
      goto DONE;
    }

  if ((gCockpitCel = LoadCel ("jsdata/art/cockpit.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the cockpit cel\n"));
      goto DONE;
    }

  if ((gCrosshairsCel = LoadCel ("jsdata/art/crosshairs.cel", MEMTYPE_CEL))
      == NULL)
    {
      PRT (("Can't load the crosshairs cel\n"));
      goto DONE;
    }

  if ((gLaserCel = LoadCel ("jsdata/art/laser.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the laser cel\n"));
      goto DONE;
    }

  if ((gOptionsCel = LoadCel ("jsdata/art/options.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the options cel\n"));
      goto DONE;
    }

  /* Link the UFO, laser, crosshairs, and cockpit cels: */

  LinkCel (gUFOCel, gLaserCel);
  LinkCel (gLaserCel, gCrosshairsCel);
  LinkCel (gCrosshairsCel, gCockpitCel);
  LAST_CEL (gCockpitCel);

  gCrosshairsCel->ccb_XPos
      = Convert32_F16 ((DISPLAY_WIDTH / 2) - gCrosshairsCel->ccb_Width);
  gCrosshairsCel->ccb_YPos
      = Convert32_F16 ((DISPLAY_HEIGHT / 2) - gCrosshairsCel->ccb_Height);

  SetFlag (gLaserCel->ccb_Flags, CCB_SKIP);

  /*
          Create a mixer info for the sound effects.
  */
  if (ehNewMixerInfo (&gMixerInfo, 4, "mixer4x2.dsp") < 0)
    goto DONE;

  /*	Load the zap sound effect */
  if (ehLoadSoundEffect (&gZapEffect, gMixerInfo, "jsdata/sound/zap.aiff", 0)
      < 0)
    goto DONE;

  retValue = DoOptions ();

DONE:
  return retValue;
}

void
Cleanup (void)
/*
        Dispose all global resources used by the program.  This mirrors
        the Initialize function:

        - Disposes of the cels.
        - Disposes of the background image.
        - Disposes the screen context.
        - Disposes the I/O request used for VBL waiting.
        - Kills the control pad utility.
        - Disposes the sound effect.
        - Disposes the mixer info.
        - Closes the audio folio.
*/
{
  UnloadCel (gUFOCel);
  UnloadCel (gCockpitCel);
  UnloadCel (gCrosshairsCel);
  UnloadCel (gLaserCel);
  UnloadCel (gOptionsCel);

  UnloadImage (gBackgroundImage);

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);

  DeleteVBLIOReq (gVBLIOReq);

  KillControlPad ();

  ehDisposeSampleInfo (gZapEffect);
  ehDisposeMixerInfo (gMixerInfo);
  CloseAudioFolio ();
}

int
main (int argc, char **argv)
{
  printf ("jsinteractivesound\n");

  if (Initialize () < 0)
    goto DONE;

  while (HandleControlPad () >= 0)
    {
      gScreenContext->sc_curScreen = 1 - gScreenContext->sc_curScreen;

      TargetAction (gUFOCel);

      DrawImage (gScreenContext->sc_Screens[gScreenContext->sc_curScreen],
                 gBackgroundImage, gScreenContext);
      DrawCels (gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen],
                gUFOCel);
      DisplayScreen (gScreenContext->sc_Screens[gScreenContext->sc_curScreen],
                     0);

      gLaserCel->ccb_Flags |= CCB_SKIP;
    }

DONE:
  if (gScreenContext)
    FadeToBlack (gScreenContext, 60);

  /*
          This is a placeholder for what might be a closing
          screen in a real title.
  */
  if (gScore + gMisses)
    {
      printf ("You got %ld points and ", gScore);
      if (gMisses)
        printf ("let only %ld", gMisses);
      else
        printf ("didn't let any");
      printf (" get away!\n");
    }
  printf ("Thank you for playing jsinteractivesound.\n");

  Cleanup ();

  return 0;
}
