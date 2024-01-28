
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ufo.c,v 1.18 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ufo
|||	ufo - A basic player/target game.
|||
|||	  Synopsis
|||
|||	    ufo
|||
|||	  Description
|||
|||	    Demonstrates the programming of a basic player/target game which is
|||	    controlled via the control pad.
|||
|||	  Associated Files
|||
|||	    ufo.c, ufo.make, playbgndmusic.c, playbgndmusic.make
|||
|||	  Location
|||
|||	    examples/Jumpstart/Ufo
|||
**/

#include "animutils.h"
#include "audio.h"
#include "celutils.h"
#include "debug3do.h"
#include "displayutils.h"
#include "filefunctions.h"
#include "graphics.h"
#include "stdio.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#include "effectshandler.h"

/*
        NTSC screen bounds
*/
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define DISPLAY_XCENTER 160
#define DISPLAY_YCENTER 120

/*
        The UFO should be constrained to be within these
        bounds so that it remains visible in the area
        above the cockpit.
*/
#define UFO_BOUNDS_LEFT 20
#define UFO_BOUNDS_TOP 20
#define UFO_BOUNDS_RIGHT 299
#define UFO_BOUNDS_BOTTOM 149

/*
        Macros for screen-handling
*/
#define SCREEN_INDEX gScreenContext->sc_curScreen
#define CURRENT_SCREEN gScreenContext->sc_Screens[SCREEN_INDEX]
#define CURRENT_BITMAPITEM gScreenContext->sc_BitmapItems[SCREEN_INDEX]

/*
        Standard audio playback rate factor
*/
#define STD_AUDIO_RATE 0x8000

/*
        Some game parameters
*/
#define LASER_COUNT 15
#define MAX_DISTANCE 8000
#define MIN_DISTANCE 256
#define HOVER_RANGE 500

/* Macro for perspective calculation */
#define PROJECTED(length) (((length) << 8) / gDistance)

ScreenContext *gScreenContext;
Item gDisplayItem = -1; /* result of CreateBasicDisplay */

int32 gLaserCount = 0;
bool gHoverMode = FALSE;
int32 gBoomCount = 0;
int32 gDistance = MAX_DISTANCE;
int32 gVelocity = -50;

int32 gScore = 0;
int32 gMisses = 0;

ubyte *gBackgroundImage = NULL;
CCB *gUFOCel = NULL;
CCB *gCockpitCel = NULL;
CCB *gCockpitRedCel = NULL;
CCB *gCockpitGreenCel = NULL;
CCB *gCockpitFireCel = NULL;
CCB *gCrosshairsCel = NULL;
CCB *gLaserCel = NULL;
CCB *gBlipCel = NULL;
CCB *gOptionsCel = NULL;
CCB *gPleaseWaitCel = NULL;
ANIM *gExplosionAnim;
CCB *gExplosionCel = NULL;

pTMixerInfo gMixerInfo = NULL;
pTSampleInfo gWhirrEffect = NULL;
pTSampleInfo gExplosionEffect = NULL;
pTSampleInfo gZapEffect = NULL;

Err gWhirrErr = -1;
Item gMusicTask;
Item gVBLIOReq;

uint32
randbits (int32 shift)
{
  shift = 31 - shift;
  return ((uint32)rand () >> shift);
}

#define FIRST_GREEN 1
#define LAST_GREEN FIRST_GREEN + 14
#define FIRST_RED 16
#define LAST_RED FIRST_RED + 11

int32
UpdateDashboard ()
/*
        Handle the game behavior related to the cockpit dashboard:

        - Cycle the crosshairs colors;
        - Position the blip cel;
        - Draw the laser if we're firing;
        - Draw the crosshairs and everything in the cockpit.

        Returns 0, but you can add functionality to determine if
        the player died, and return a negative value when that
        happens.
*/
{
  static int32 sine[16]
      = { 0,     2753,  5502,  8241,  10965, 13670, 16351, 19003,
          21621, 24201, 26739, 29229, 31668, 34051, 36373, 38632 };
  static int32 cosine[16]
      = { 65536, 65478, 65304, 65015, 64612, 64094, 63463, 62720,
          61866, 60903, 59832, 58656, 57376, 55995, 54515, 52938 };
  static int32 cycle = 0;

  int32 i, dist;
  int32 x, y;
  int16 *p;
  int16 temp;

  /*
          Cycle the crosshairs colors.
  */
  p = (int16 *)gCrosshairsCel->ccb_PLUTPtr;
  temp = p[FIRST_GREEN];
  for (i = FIRST_GREEN; i < LAST_GREEN; i++)
    p[i] = p[i + 1];
  p[LAST_GREEN] = temp;

  if (!cycle)
    {
      temp = p[FIRST_RED];
      for (i = FIRST_RED; i < LAST_RED; i++)
        p[i] = p[i + 1];
      p[LAST_RED] = temp;
      cycle = 3;
    }
  else
    --cycle;

  /*
          Determine the blip cel position based on the location of the target.
  */
  i = (int32)PROJECTED (gUFOCel->ccb_Width);
  i = (int32)(i >> 1) + (int32)ConvertF16_32 (gUFOCel->ccb_XPos);
  i = ((i - DISPLAY_XCENTER) << 1) / 21;
  dist = gDistance / 307;
  if (i < 0)
    {
      x = -1 * dist * sine[i * -1];
      y = dist * cosine[i * -1];
    }
  else
    {
      x = dist * sine[i];
      y = dist * cosine[i];
    }
  gBlipCel->ccb_XPos = Convert32_F16 (246) + x + gCockpitCel->ccb_XPos;
  gBlipCel->ccb_YPos = Convert32_F16 (205) - y + gCockpitCel->ccb_YPos;

  /*
          Draw the laser if the player is firing.
  */
  if (gLaserCount == LASER_COUNT)
    DrawCels (CURRENT_BITMAPITEM, gLaserCel);

  /*
              Draw the crosshairs and everything in the cockpit.
      */

  LinkCel (gCrosshairsCel, gCockpitCel);
  LinkCel (gCockpitCel, gBlipCel);
  LAST_CEL (gBlipCel);

  DrawCels (CURRENT_BITMAPITEM, gCrosshairsCel);

  if (gLaserCount)
    {
      --gLaserCount;
      DrawCels (CURRENT_SCREEN, gCockpitRedCel);
    }

  if (gHoverMode)
    DrawCels (CURRENT_SCREEN, gCockpitGreenCel);

  if (!randbits (8))
    DrawCels (CURRENT_SCREEN, gCockpitFireCel);

  /*
              if ( DetermineIfPlayerDied() )
                      return -1.
      */

  return 0;
}

bool
Zap (CCB *targetCel, Coord X, Coord Y)
/*
        This function "focuses" the laser to ( X, Y ), plays the zap sound
   effect and detects whether the laser has hit the target by testing whether
   the specified coordinates are within the target bounds, taking perspective
        into account.

        targetCel	pointer to the target's CCB
        X			horizontal coordinate of the point of impact
        Y			vertical coordinate of the point of impact

        Returns TRUE if target was hit, FALSE if not

        Call when user fires the laser ( via ControlA ).
*/
{
  static TagArg taStartArgs[] = { { AF_TAG_RATE, (void *)STD_AUDIO_RATE },
                                  { AF_TAG_AMPLITUDE, (void *)0x4000 },
                                  { 0, 0 } };

  static Point Corner[4] = { { 0, 0 }, { 0, 0 }, { 319, 210 }, { 0, 210 } };

  Corner[0].pt_X = Corner[1].pt_X = X;
  Corner[0].pt_Y = Corner[1].pt_Y = Y;

  MapCel (gLaserCel, Corner);

  StartInstrument (gZapEffect->si_Player, taStartArgs);

  X -= ConvertF16_32 (targetCel->ccb_XPos);
  Y -= ConvertF16_32 (targetCel->ccb_YPos);
  return ((X >= 0) && (X < PROJECTED (targetCel->ccb_Width)) && (Y >= 0)
          && (Y < PROJECTED (targetCel->ccb_Height)));
}

int32
DoOptions (void)
/*
        This function displays the options screen and handles control
        pad button presses until the user resumes playing or quits:

        - ControlStart		Resume playing the game.

        - ControlC			Quit.

        Returns 0 if user resumes, a negative number if
        he quits.

        Call at end of initialization or in response to ControlStart during
        playing.
*/
{
  uint32 button;
  int32 status;
  int32 xUFO, yUFO, hdxUFO, vdyUFO;
  TagArg taStartArgs[] = { { AF_TAG_RATE, (void *)STD_AUDIO_RATE },
                           { AF_TAG_AMPLITUDE, (void *)0x3000 },
                           { 0, 0 } };

  /*
              Save current UFO cel position and scale.
      */
  xUFO = gUFOCel->ccb_XPos;
  yUFO = gUFOCel->ccb_YPos;
  hdxUFO = gUFOCel->ccb_HDX;
  vdyUFO = gUFOCel->ccb_VDY;

  /*
              Draw the UFO at a special position and size for the options
     screen.
      */
  gUFOCel->ccb_XPos = Convert32_F16 (176);
  gUFOCel->ccb_YPos = Convert32_F16 (67);
  gUFOCel->ccb_HDX = 1 << 19;
  gUFOCel->ccb_VDY = 1 << 15;

  SCREEN_INDEX = 1 - SCREEN_INDEX;
  DrawImage (CURRENT_SCREEN, gBackgroundImage, gScreenContext);
  LinkCel (gCockpitCel, gOptionsCel);
  LinkCel (gOptionsCel, gUFOCel);
  LAST_CEL (gUFOCel);
  DrawCels (CURRENT_BITMAPITEM, gCockpitCel);
  DisplayScreen (CURRENT_SCREEN, 0);

  /*
          Stop the whirring while we're getting the options.
  */
  if (gWhirrErr == 0)
    StopInstrument (gWhirrEffect->si_Player, NULL);

  WaitVBL (gVBLIOReq, 1);
  while (1)
    {
      status = DoControlPad (
          1, &button, ControlLeft | ControlRight | ControlUp | ControlDown);
      if (button & ControlStart)
        /*
                Resume playing the game.
        */
        {

          /*
                  Restore UFO cel position and scale.
          */
          gUFOCel->ccb_XPos = xUFO;
          gUFOCel->ccb_YPos = yUFO;
          gUFOCel->ccb_HDX = hdxUFO;
          gUFOCel->ccb_VDY = vdyUFO;

          /*
                  Restart the whirring.
          */
          gWhirrErr = StartInstrument (gWhirrEffect->si_Player, taStartArgs);

          return 0;
        }
      else
        {
          if (button & ControlA)
            /*
                    Unimplemented.
            */
            {
            }

          if (button & ControlB)
            /*
                    Unimplemented.
            */
            {
            }

          if (button & ControlC)
            /*
                    Quit.
            */
            {
              return -1;
            }
        }

      WaitVBL (gVBLIOReq, 1);
    }
}

int32
PlayerAction (void)
/*
        Get the control pad state and respond to button presses:

        - Start	button means get game options and return.
        - A	button means fire laser and detect start explosion if target is
   hit.
        - B	button means toggle hover mode.
        - Left button means aim crosshairs to the left
        - Right	button means aim crosshairs to the right
        - Up button means aim crosshairs higher
        - Down button means aim crosshairs lower

        Returns 0 if user is continuing to play, a negative number if
        he quits (from the option screen).

        Call at the start of the main loop
*/
{
  static TagArg taStartArgs[]
      = { { AF_TAG_RATE, (void *)STD_AUDIO_RATE },
          { AF_TAG_AMPLITUDE, (void *)MAXDSPAMPLITUDE },
          { 0, 0 } };

  int32 moveIncrement = Convert32_F16 (3);

  int32 Balance;
  int32 Volume;
  int32 status;
  uint32 button;

  status = DoControlPad (1, &button,
                         ControlLeft | ControlRight | ControlUp | ControlDown);

  if (button & ControlStart)
    /*
            Get the player options.
    */
    return DoOptions ();
  else
    {
      if ((button & ControlA) && !gLaserCount)
        /*
                Player wants to fire and we're not still firing.
        */
        {
          gLaserCount = LASER_COUNT;
          if (Zap (gUFOCel,
                   ConvertF16_32 (gCrosshairsCel->ccb_XPos)
                       + (gCrosshairsCel->ccb_Width >> 1),
                   ConvertF16_32 (gCrosshairsCel->ccb_YPos)
                       + (gCrosshairsCel->ccb_Height >> 1)))
            /*
                    The target was hit.
            */
            {

              /*
                      Play the explosion sound effect.

                              Sound originates from the crosshairs.
                              Volume gets louder as the target gets closer.
              */
              Balance = ((gCrosshairsCel->ccb_XPos
                          + (gCrosshairsCel->ccb_Width >> 1))
                         >> 2)
                        / DISPLAY_XCENTER;
              Volume = ((MAX_DISTANCE - gDistance) << 15) / 3000;
              ehSetChannelLevels (
                  gMixerInfo, gExplosionEffect->si_LeftGainKnob,
                  gExplosionEffect->si_RightGainKnob, Volume, Balance);
              StartInstrument (gExplosionEffect->si_Player, taStartArgs);

              /*
                      Play the explosion for its full duration at the point of
                 impact and at the same scale as the target.
              */
              gBoomCount = (int32)gExplosionAnim->num_Frames;
              gExplosionCel->ccb_XPos
                  = gCrosshairsCel->ccb_XPos
                    + (gCrosshairsCel->ccb_Width << 15)
                    - (gExplosionCel->ccb_Width * gUFOCel->ccb_VDY >> 1);
              gExplosionCel->ccb_YPos
                  = gCrosshairsCel->ccb_YPos
                    + (gCrosshairsCel->ccb_Height << 15)
                    - (gExplosionCel->ccb_Height * gUFOCel->ccb_VDY >> 1);
              gExplosionCel->ccb_HDX = gUFOCel->ccb_HDX;
              gExplosionCel->ccb_VDY = gUFOCel->ccb_VDY;

              /*
                      Add to the player's score; the farther away the target,
                 the higher the points.
              */
              gScore += gDistance;

              /*
                      Initialize for the next target approach.
              */
              gDistance = MAX_DISTANCE;
              if (gVelocity > 0)
                gVelocity *= -1;
              gUFOCel->ccb_XPos = (int32)Convert32_F16 (UFO_BOUNDS_LEFT
                                                        + (int32)randbits (8));
              gUFOCel->ccb_YPos = (int32)Convert32_F16 (UFO_BOUNDS_TOP
                                                        + (int32)randbits (9));
            }
        }

      if (button & ControlB)
        /*
                Toggle hover mode.
        */
        {
          gHoverMode = !gHoverMode;
        }

      if (button & ControlC)
        /*
                Unimplemented.
        */
        {
        }

      /*
              Move the crosshairs if it's not at the edge of the screen.
      */
      if ((button & ControlRight)
          && (gCrosshairsCel->ccb_XPos
              < Convert32_F16 (DISPLAY_WIDTH - gCrosshairsCel->ccb_Width)))
        gCrosshairsCel->ccb_XPos += moveIncrement;

      if ((button & ControlLeft) && (gCrosshairsCel->ccb_XPos > 0))
        gCrosshairsCel->ccb_XPos -= moveIncrement;

      if ((button & ControlDown)
          && (gCrosshairsCel->ccb_YPos
              < Convert32_F16 (DISPLAY_HEIGHT - gCrosshairsCel->ccb_Height)))
        gCrosshairsCel->ccb_YPos += moveIncrement;

      if ((button & ControlUp) && (gCrosshairsCel->ccb_YPos > 0))
        gCrosshairsCel->ccb_YPos -= moveIncrement;
    }

  return 0;
}

void
TargetAction (CCB *targetCel)
/*
        This function animates the target and
        updates the number of misses when it goes out of
        range.  If the user has triggered hover mode (via ControlB),
        the target slows down when it reaches a nominal distance from
        the cockpit.  It reverses direction when it reaches the minimum
        distance, and continues to retreat until it goes out of range.

        targetCel	pointer to the target CCB to be animated

        Call from main loop after player input is handled
*/
{
  static int32 iDeltaX = Convert32_F16 (1);
  static int32 iDeltaY = 3 << 15;

  int32 iTest;

  int32 Balance;
  int32 Volume;

  if (gDistance > MAX_DISTANCE)
    /*
            Target is out of range - the player missed it.
            Reverse the velocity and start at the maximum distance and a random
            position for a new approach.
    */
    {
      gMisses++;
      gDistance = MAX_DISTANCE;
      gVelocity *= -1;

      targetCel->ccb_XPos
          = (int32)Convert32_F16 (UFO_BOUNDS_LEFT + (int32)randbits (8));
      targetCel->ccb_YPos
          = (int32)Convert32_F16 (UFO_BOUNDS_TOP + (int32)randbits (9));
    }
  else if (gDistance < MIN_DISTANCE)
    /*
            Target got as near as possible without being hit.
            Reverse the velocity and start at the minimum distance to begin
            its retreat.
    */
    {
      gDistance = MIN_DISTANCE;
      gVelocity *= -1;
    }

  if ((gDistance < HOVER_RANGE) && gHoverMode)
    /*
            Hovering starting or continuing
    */
    {
      gDistance += (gVelocity >> 5);
      targetCel->ccb_YPos += iDeltaY >> 1;
      targetCel->ccb_XPos += iDeltaX >> 1;
    }
  else
    /* Normal approach ( not hovering ) */
    {
      gDistance += gVelocity;
      iTest = PROJECTED (MIN_DISTANCE);
      targetCel->ccb_YPos += (iDeltaY >> 7) * iTest;
      targetCel->ccb_XPos += (iDeltaX >> 7) * iTest;

      if (!randbits (8))
        iDeltaX *= -1;

      if (!randbits (8))
        iDeltaY *= -1;
    }

  /*
              Scale based on distance.
      */
  //	targetCel->ccb_HDX = (1<<28) / gDistance;
  //	targetCel->ccb_VDY = (1<<24) / gDistance;
  targetCel->ccb_HDX = PROJECTED (1 << 20);
  targetCel->ccb_VDY = PROJECTED (Convert32_F16 (1));

  /*
              Keep the UFO within the established bounds.
      */
  iTest = ConvertF16_32 (targetCel->ccb_YPos)
          + PROJECTED (targetCel->ccb_Height);
  if (targetCel->ccb_YPos <= Convert32_F16 (UFO_BOUNDS_TOP))
    {
      targetCel->ccb_YPos = Convert32_F16 (UFO_BOUNDS_TOP);
      iDeltaY *= -1;
    }
  else if (iTest >= UFO_BOUNDS_BOTTOM)
    {
      targetCel->ccb_YPos -= Convert32_F16 (iTest - UFO_BOUNDS_BOTTOM);
      iDeltaY *= -1;
    }

  iTest
      = ConvertF16_32 (targetCel->ccb_XPos) + PROJECTED (targetCel->ccb_Width);
  if (targetCel->ccb_XPos <= Convert32_F16 (UFO_BOUNDS_LEFT))
    {
      targetCel->ccb_XPos = Convert32_F16 (UFO_BOUNDS_LEFT);
      iDeltaX *= -1;
    }
  else if (iTest >= UFO_BOUNDS_RIGHT)
    {
      targetCel->ccb_XPos -= Convert32_F16 (iTest - UFO_BOUNDS_RIGHT);
      iDeltaX *= -1;
    }

  /*
          Play the whirring sound effect:

                  Sound originates from the target.
                  Volume gets louder as the target gets closer.
  */
  iTest -= ConvertF16_32 (targetCel->ccb_XPos);
  Balance = ((targetCel->ccb_XPos + (iTest << 15)) >> 2) / DISPLAY_XCENTER;
  Volume = ((MAX_DISTANCE - gDistance) << 15) / MAX_DISTANCE;
  ehSetChannelLevels (gMixerInfo, gWhirrEffect->si_LeftGainKnob,
                      gWhirrEffect->si_RightGainKnob, Volume, Balance);

  DrawCels (CURRENT_BITMAPITEM, gUFOCel);

  /*
              If the target is exploding, advance to the next frame
              of the explosion animation.
      */
  if (gBoomCount)
    {
      gBoomCount--;
      if (gBoomCount)
        {
          GetAnimCel (gExplosionAnim, Convert32_F16 (1));
          DrawCels (CURRENT_BITMAPITEM, gExplosionCel);
        }
      else
        {
          gExplosionAnim->cur_Frame = 0;
        }
    }

  return;
}

void
UnifyAnimation (ANIM *pAnim)
/*
        Ensure that all frames of the animation use the same CCB,
        so we can quickly make universal changes to the position
        and scale.
*/
{
  int32 frameIndex;
  CCB *theCCB;

  theCCB = pAnim->pentries[0].af_CCB;
  for (frameIndex = 0; frameIndex < pAnim->num_Frames; frameIndex++)
    {
      pAnim->pentries[frameIndex].af_CCB = theCCB;
    }
}

void
Cleanup (void)
/*
        This function performs application termination in the reverse order of
        initialization.

        Call in main just before exiting.
*/
{
  if (gMusicTask >= 0)
    DeleteItem (gMusicTask);

  ehDisposeSampleInfo (gZapEffect);
  ehDisposeSampleInfo (gExplosionEffect);
  ehDisposeSampleInfo (gWhirrEffect);

  UnloadAnim (gExplosionAnim);
  UnloadCel (gCockpitFireCel);
  UnloadCel (gCockpitGreenCel);
  UnloadCel (gCockpitRedCel);
  UnloadCel (gBlipCel);
  UnloadCel (gLaserCel);
  UnloadCel (gCrosshairsCel);
  UnloadCel (gPleaseWaitCel);
  UnloadCel (gOptionsCel);
  UnloadCel (gCockpitCel);
  UnloadImage (gBackgroundImage);

  KillControlPad ();

  ehDisposeMixerInfo (gMixerInfo);

  CloseAudioFolio ();

  if (gDisplayItem >= 0)
    DeleteBasicDisplay (gScreenContext);

  DeleteVBLIOReq (gVBLIOReq);
}

int32
Initialize (void)
/*
       Perform game-wide initialization:

       - Get a VBLIOReq for waiting.
       - Open the graphics and audio folios.
       - Initialize the mixer for the sound effects.
       - Initialize the control pad handler.
       - Pre-load all graphics elements and sound effects.
       - Launch the background music-playing task.
       - Allow user to specify game options.

       Returns 0 if successful or an error code (a negative value)

       Call prior to main loop
*/
{
  int32 Result = -1;

  /*
          Get a VBLIOReq to use for waiting.
  */
  gVBLIOReq = CreateVBLIOReq ();
  if (gVBLIOReq < 0)
    {
      DIAGNOSE_SYSERR (gVBLIOReq, ("Can't get a VBL IOReq\n"));
      goto DONE;
    }

  /*
          Initialize system resources and utilities
  */

  /*	Allocate memory for the screenContext */
  gScreenContext
      = (ScreenContext *)AllocMem (sizeof (ScreenContext), MEMTYPE_ANY);
  if (gScreenContext == NULL)
    {
      PRT (("Can't allocate memory for ScreenContext\n"));
      goto DONE;
    }

  if ((gDisplayItem = CreateBasicDisplay (gScreenContext, DI_TYPE_DEFAULT, 2))
      < 0)
    {
      PRT (("Can't initialize display\n"));
      goto DONE;
    }

  if (OpenAudioFolio ())
    {
      PRT (("Can't open the audio folio\n"));
      goto DONE;
    }

  if (ehNewMixerInfo (&gMixerInfo, 4, "mixer4x2.dsp") < 0)
    goto DONE;

  Result = InitControlPad (2);
  if (Result < 0)
    {
      PRT (("Can't initialize the control pad\n"));
      goto DONE;
    }

  /*
              Load the graphic elements for the initial screen
      */

  if ((gBackgroundImage
       = (ubyte *)LoadImage ("ufoart/sky.imag", NULL, NULL, gScreenContext))
      == NULL)
    {
      PRT (("Can't load the background image\n"));
      goto DONE;
    }

  if ((gUFOCel = LoadCel ("ufoart/ufo.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the UFO cel\n"));
      goto DONE;
    }

  if ((gCockpitCel = LoadCel ("ufoart/cockpit.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the cockpit cel\n"));
      goto DONE;
    }

  if ((gOptionsCel = LoadCel ("ufoart/options.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the options cel\n"));
      goto DONE;
    }

  if ((gPleaseWaitCel = LoadCel ("ufoart/pleasewait.cel", MEMTYPE_CEL))
      == NULL)
    {
      PRT (("Can't load the PleaseWait cel\n"));
      goto DONE;
    }

  /*
              Set the initial target position and scale to half-size.
      */

  gUFOCel->ccb_XPos = Convert32_F16 (176);
  gUFOCel->ccb_YPos = Convert32_F16 (67);
  gUFOCel->ccb_HDX = 1 << 19;
  gUFOCel->ccb_VDY = 1 << 15;

  /*
          Draw the "Please Wait" screen
  */

  SCREEN_INDEX = 1;
  DrawImage (CURRENT_SCREEN, gBackgroundImage, gScreenContext);

  LinkCel (gCockpitCel, gOptionsCel);
  LinkCel (gOptionsCel, gUFOCel);
  LinkCel (gUFOCel, gPleaseWaitCel);
  LAST_CEL (gPleaseWaitCel);

  DisplayScreen (CURRENT_SCREEN, 0);

  /*
              Load the rest of the graphics elements
      */

  if ((gCrosshairsCel = LoadCel ("ufoart/crosshairs.cel", MEMTYPE_CEL))
      == NULL)
    {
      PRT (("Can't load the crosshairs cel\n"));
      goto DONE;
    }

  if ((gLaserCel = LoadCel ("ufoart/laser.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the laser cel\n"));
      goto DONE;
    }

  if ((gBlipCel = LoadCel ("ufoart/blip.cel", MEMTYPE_CEL)) == NULL)
    {
      PRT (("Can't load the radar blip cel\n"));
      goto DONE;
    }

  if ((gCockpitRedCel = LoadCel ("ufoart/cockpitred.cel", MEMTYPE_CEL))
      == NULL)
    {
      PRT (("Can't load the red light cel\n"));
      goto DONE;
    }

  if ((gCockpitGreenCel = LoadCel ("ufoart/cockpitgreen.cel", MEMTYPE_CEL))
      == NULL)
    {
      PRT (("Can't load the green light cel\n"));
      goto DONE;
    }

  if ((gCockpitFireCel = LoadCel ("ufoart/cockpitfire.cel", MEMTYPE_CEL))
      == NULL)
    {
      PRT (("Can't load the fire light cel\n"));
      goto DONE;
    }

  /*
          Load the explosion animation, and initialize the explosion cel
          to the animation's first frame.
  */
  if ((gExplosionAnim = LoadAnim ("ufoart/boom.anim", MEMTYPE_CEL)) == 0)
    {
      PRT (("Can't load the explosion animation\n"));
      goto DONE;
    }
  UnifyAnimation (gExplosionAnim);
  gExplosionCel = GetAnimCel (gExplosionAnim, 0);

  /*
          Load the sound effects
  */

  if (ehLoadSoundEffect (&gWhirrEffect, gMixerInfo, "ufosound/whirr.aiff", 0)
      < 0)
    goto DONE;

  if (ehLoadSoundEffect (&gExplosionEffect, gMixerInfo,
                         "ufosound/explosion.aiff", 1)
      < 0)
    goto DONE;

  if (ehLoadSoundEffect (&gZapEffect, gMixerInfo, "ufosound/zap.aiff", 2) < 0)
    goto DONE;

  /*
              Launch the background music-playing task
      */

  {
    int32 taskPriority = (int32)KernelBase->kb_CurrentTask->t.n_Priority + 1;

    if ((gMusicTask = LoadProgramPrio ("playbgndmusic", taskPriority)) < 0)
      {
        PRT (("Can't launch the background music-playing task\n"));
        goto DONE;
      }
  }

  PRT (("All file access complete...\n"));

  LAST_CEL (gBlipCel);
  LAST_CEL (gCockpitCel);
  LAST_CEL (gCockpitFireCel);
  LAST_CEL (gCockpitGreenCel);
  LAST_CEL (gCockpitRedCel);
  LAST_CEL (gCrosshairsCel);
  LAST_CEL (gExplosionCel);
  LAST_CEL (gLaserCel);
  LAST_CEL (gOptionsCel);
  LAST_CEL (gPleaseWaitCel);
  LAST_CEL (gUFOCel);

  /*
              More initialization for the crosshairs.
      */
  {
    // Position the crosshairs near the center of the screen
    gCrosshairsCel->ccb_XPos
        = (Coord)Convert32_F16 (DISPLAY_XCENTER - gCrosshairsCel->ccb_Width);
    gCrosshairsCel->ccb_YPos
        = (Coord)Convert32_F16 (DISPLAY_YCENTER - gCrosshairsCel->ccb_Height);

    {
      int16 *p = (int16 *)gCrosshairsCel->ccb_PLUTPtr;
      int32 i;

      // Brute-force technique for changing the red band of the color-cycling
      // palette to green.
      for (i = FIRST_RED; i <= LAST_RED; i++)
        p[i] >>= 5;
    }
  }

  /*	Finally, go to the options screen. */
  return DoOptions ();

DONE:
  return -1;
}

int
main (int argc, char **argv)
/*
        Main application logic.

        After successfully initialization, enter a loop in which
        player input is detected and the target and dashboard are
        animated accordingly.
*/
{
  printf ("ufo\n");

  if (Initialize () < 0)
    goto DONE;

  while (PlayerAction () >= 0)
    {
      SCREEN_INDEX = 1 - SCREEN_INDEX;
      DrawImage (CURRENT_SCREEN, gBackgroundImage, gScreenContext);
      TargetAction (gUFOCel);
      if (UpdateDashboard () < 0)
        break;
      DisplayScreen (CURRENT_SCREEN, 0);
    }

DONE:
  if (gScreenContext)
    FadeToBlack (gScreenContext, 60);

  /*
          This is a placeholder for code that would handle the
          end-of-game screen.
  */
  PRT (("You scored %ld points and ", gScore));
  if (gMisses)
    {
      PRT (("let only %d", gMisses));
    }
  else
    {
      PRT (("didn't let any"));
    }
  PRT ((" get away!\n"));
  PRT (("Thank you for playing ufo.\n"));

  Cleanup ();

  return 0;
}
