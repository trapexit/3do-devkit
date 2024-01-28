
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: orbit.c,v 1.19 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/3doorbit
|||	3doorbit - Demonstrates incorporating moving animations and sound
effects.
|||
|||	  Synopsis
|||
|||	    3doorbit
|||
|||	  Description
|||
|||	    Demonstrates loading a background image, several animations, and
some
|||	    sound effects, and shows how you can integrate them.
|||
|||	    The 3DO logo spins around and spirals back into place.
|||
|||	  Associated Files
|||
|||	    orbit.c, orbit.h, orbiterror.h
|||
|||	  Location
|||
|||	    examples/Graphics/3DO_Orbit
|||
**/

#include "orbit.h"

void
Usage (void)
{

  PRT (("Stop                 exits\n"));

  return;
}

/*********************
**	Initialize - initialization routine
**
**	  Synopsis
**
**	    static int32 Initialize ( ScreenContext *sc, int32 nScreens )
**
**	  Description
**
**	    This function opens the graphics, audio, and math folios,
*initializes for 1 *	    controlpad, and creates an error text and a couple
*of IO req items
**
**	  Arguments
**
**	    sc	                 pointer to a screen context structure
**
**	    nScreens             the number of screens to create
**
**
**	  Return Value
**
**	    The function returns 0 if successful or an error code (a negative
*value) *	    if an error occurs.
**
**
**	  Implementation
**
**	    Called prior to main loop
**
**	  Caveats
**
**	   none
**
**	  Associated Files
**
**	    orbit.h orbiterror.h orbitsound.c controlpad.h controlpad.c
*effectshandler.c effectshandler.h
**
**	  See Also
**
**
*********************/
static int32
Initialize (ScreenContext *sc, int32 nScreens)
{

  int32 status = 0;

  /*
     create an error text item to return opera style errors
  */
  status = CreateItem (MKNODEID (KERNELNODE, ERRORTEXTNODE), OrbitErrorTags);
  if (status < 0)
    goto CLEANUP;

  gOrbitErrorItem = status;

  /*
     open the graphics folio, fill in the screen context structure
     for nScreens screens
  */
  if (!CreateBasicDisplay (sc, DI_TYPE_NTSC, (int)nScreens))
    {
      status = ORBITGRAPHICS_ERR;
      goto CLEANUP;
    }

  sc->sc_curScreen = 0;

  /*
     get an io req item for SPORT transfers, and for using the VBL timer
  */
  status = CreateVRAMIOReq ();
  if (status < 0)
    goto CLEANUP;

  gVramIOReq = status;

  status = CreateVBLIOReq ();
  if (status < 0)
    goto CLEANUP;

  gVblIOReq = status;

  /*
          open the math folio for doing some trig
  */
  status = OpenMathFolio ();
  if (status < 0)
    goto CLEANUP;

  /*
          open the audio folio and set up for playing soundeffects.  Use the
     convenience functions found in effectshandler.c
  */
  status = OpenAudioFolio ();
  if (status < 0)
    goto CLEANUP;

  status = ehNewMixerInfo (&gAudioCtx, 2, "mixer2x2.dsp");
  if (status < 0)
    {
      status = ORBITAUDIOCTX_ERR;
      goto CLEANUP;
    }

  /*
          Create a cue to use for knowing when a sample has finished playing.

          allocate a signal with which we will be notified that a sound effect
     has stopped
  */

  status = CreateCue (NULL);
  if (status < 0)
    goto CLEANUP;

  gCue = status;

  gCueSignal = GetCueSignal (gCue);

  /*
     init structures and resources for querying the controlpad
  */
  status = InitControlPad (1);
  if (status < 0)
    goto CLEANUP;

CLEANUP:
  return status;
}

/*********************
**	DrawScreen - routine to draw the background, all the cels, and display
*the screen
**
**	  Synopsis
**
**	    static void DrawScreen ( ScreenContext *sc, OrbitObjPtr obj, ubyte
**background )
**
**	  Description
**
**	    This function toggles the screen counter, copies the background
*pic, draws the cels, and *	    displays the screen
**
**	  Arguments
**
**	    sc	                 pointer to a screen context structure
**
**	    obj          	     array of objects containing info about the
*cels
**
**	    background           pointer to the background bitmap
**
**	  Return Value
**
**	    None.
**
**
**	  Implementation
**
**	    Called once per frame update.
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**
**	  See Also
**
**	    CopyVRAMPages(), DrawCels(), DisplayScreen()
**
*********************/
static void
DrawScreen (ScreenContext *sc, OrbitObjPtr obj, ubyte *background)
{

  /*
          toggle the current screen counter
  */
  sc->sc_curScreen ^= 1;

  /*
          copy the background image.  This does an implicit WaitVBL.
  */
  CopyVRAMPages (gVramIOReq, sc->sc_Bitmaps[sc->sc_curScreen]->bm_Buffer,
                 background, sc->sc_nFrameBufferPages, ~0);

  /*
          draw all of the cels at once.  These have been previously linked
     together
  */
  DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], obj[0].obj_CCB);

  /*
     This queues up the screen we just drew to be displayed at
     the next vertical blank.  The function returns immediately, however
  */
  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);

  return;
}

/*********************
**	main - main routine
**
**	  Synopsis
**
**	    int main ( void )
**
**	  Description
**
**	    This function initializes the system, loads in a 3DO background and
**	    all of the graphics objects, loads in sound effects, drops into a
*main loop where it queries the control *	    pad and reacts to it,
*double buffers, and renders all the artwork
**
**	  Arguments
**
**
**	  Return Value
**
**	    The function exits with  0.
**
**
**	  Implementation
**
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**	    orbit.h orbiterror.h orbitsound.c orbitsound.h controlpad.h
*controlpad.c
**
**	  See Also
**
**
*********************/
int
main (int32 argc, char *argv[])
{

  ScreenContext sc;

  OrbitObjPtr obj = NULL;
  ubyte *background = NULL;

  uint32 button;

  int32 status = 0;

  PRT (("%s %s\n", argv[0], VERSION));
  Usage ();

  /*
          initialize the system, open all the folios, get all the necessary
     resources
  */
  if ((status = Initialize (&sc, 2)) < 0)
    {
      PrintfSysErr (status);
      exit (0);
    }

  /*
          load in the background image
  */
  background
      = (ubyte *)LoadImage ("orbitart/background.imag", NULL, NULL, &sc);
  if (background == NULL)
    {
      status = ORBITLOADIMAGE_ERR;
      goto CLEANUP;
    }

  /*
          load in all the aiff files that will be used as sound effects
  */
  status = LoadSounds ();
  if (status < 0)
    goto CLEANUP;

  /*
          load all of the 3doorbit graphics objects
  */
  status = LoadObj (&obj);
  if (status < 0)
    goto CLEANUP;

  /*
          initialize values for the graphics objects
  */
  InitObj (obj);

  /*
          drop into the main loop until the user quits
  */
  while (true)
    {

      /*
         Query the control pad
      */
      status = DoControlPad (1, &button, 0);
      if (status < 0)
        break;

      /*
              if the X button is hit, then exit
      */
      if (button == ControlX)
        break;

      /*
              update the positioning of the objects.  If the animation sequence
         has reached its conclusion, then reset all the objects and start over.
      */
      if (UpdateObj (obj))
        InitObj (obj);

      /*
              render and display all of the objects in their updated positions
      */
      DrawScreen (&sc, obj, background);
    }

CLEANUP:
  if (status < 0)
    PrintfSysErr (status);

  FadeToBlack (&sc, 20);

  UnloadObj (obj);

  KillControlPad ();

  DeleteCue (gCue);
  ehDisposeSampleInfo (gClickSound);
  ehDisposeSampleInfo (gCheerSound);
  ehDisposeMixerInfo (gAudioCtx);
  CloseAudioFolio ();

  CloseMathFolio ();

  CloseGraphics (&sc);

  PRT (("Exiting\n"));

  exit (0);
}

/*********************
**	LoadSounds - loads in sound effects
**
**	  Synopsis
**
**	    static int32 LoadSounds (void)
**
**	  Description
**
**	    This function loads in a couple of sound effects that will be used
*during the program
**
**	  Arguments
**
**	    None.
**
**	  Return Value
**
**	    The function returns 0 if all went well, < 0 on an error.
**
**
**	  Implementation
**
**	    This function must be called before any of the sound effects can be
*played back.
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**	    effectshandler.c effectshandler.h
**
**	  See Also
**
**	    ehLoadSoundEffect()
**
*********************/
static int32
LoadSounds (void)
{

  int32 status = 0;

  status = ehLoadSoundEffect (&gClickSound, gAudioCtx,
                              "orbitsounds/click.aiff", 0);
  if (status < 0)
    {
      status = ORBITLOADSOUND_ERR;
      goto CLEANUP;
    }

  status = ehLoadSoundEffect (&gCheerSound, gAudioCtx,
                              "orbitsounds/cheer.aiff", 1);
  if (status < 0)
    {
      status = ORBITLOADSOUND_ERR;
      goto CLEANUP;
    }

CLEANUP:
  return status;
}

/*********************
**	SetObjCCB - sets the 3doorbit objects' ccbs
**
**	  Synopsis
**
**	    static int32 SetObjCCB ( OrbitObj *obj, int32 iObj )
**
**	  Description
**
**	    This function sets a 3doorbit object CCB with legitimate values
*from the first frame of the animation
**
**	  Arguments
**
**	    None.
**
**	  Return Value
**
**	    The function returns 0 if all went well, < 0 on an error.
**
**
**	  Implementation
**
**	    This function must be called to set values in the CCBs and avoid
*errors.
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**	    orbit.h
**
**	  See Also
**
**
*********************/
static int32
SetObjCCB (OrbitObj *obj, int32 iObj)
{

  int32 status = 0;

  /*
          allocate space for a CCB, then grab the first frame of the animation
     and copy its values into this dummy ccb to make sure that we have
     legitimate values in there.
  */
  obj[iObj].obj_CCB
      = (CCB *)AllocMem (sizeof (CCB), MEMTYPE_CEL | MEMTYPE_FILL | 0x00);
  if (obj[iObj].obj_CCB != NULL)
    memcpy (obj[iObj].obj_CCB, GetAnimCel (obj[iObj].obj_ANIM, 0),
            sizeof (CCB));

  else
    status = ORBITALLOC_ERR;

  return status;
}

/*********************
**	LoadObj - loads in the three graphical objects for 3doorbit
**
**	  Synopsis
**
**	    static int32 LoadObj ( OrbitObjPtr *pObj )
**
**	  Description
**
**	    This function loads in one cel and two anims
**
**	  Arguments
**
**	    None.
**
**	  Return Value
**
**	    The function returns 0 if all went well, < 0 on an error.
**
**
**	  Implementation
**
**	    This function must be called before any graphics can be displayed.
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**	   orbit.h
**
**	  See Also
**
**	    LoadCel(), LoadAnim()
**
*********************/
static int32
LoadObj (OrbitObjPtr *pObj)
{

  int32 iObj = 0, status = 0;
  OrbitObjPtr obj;

  /*
          allocate an array of 3 OrbitObj
  */
  obj = (OrbitObjPtr)AllocMem (3 * sizeof (OrbitObj),
                               MEMTYPE_TRACKSIZE | MEMTYPE_FILL | 0x00);
  if (obj == NULL)
    {
      status = ORBITALLOC_ERR;
      goto CLEANUP;
    }

  /*
          load in the first cel... this will be cel[0] in the list of cels
          passed off to the cel engine
  */
  obj[iObj].obj_CCB = LoadCel ("orbitart/ball.cel", MEMTYPE_CEL);
  if (obj[iObj].obj_CCB == NULL)
    {
      status = ORBITALLOC_ERR;
      goto CLEANUP;
    }
  /*
          actual height of cel, minus all the black border pixels
  */
  obj[iObj].obj_Height = 42;

  /*
          load in the remaining two animations.  Set a dummy ccb for each one
  */
  obj[++iObj].obj_ANIM = LoadAnim ("orbitart/tvd.anim", MEMTYPE_CEL);
  if (obj[iObj].obj_ANIM == NULL)
    {
      status = ORBITALLOC_ERR;
      goto CLEANUP;
    }

  status = SetObjCCB (obj, iObj);
  if (status < 0)
    goto CLEANUP;

  /*
          actual height of cel, minus all the black border pixels
  */
  obj[iObj].obj_Height = 33;

  obj[++iObj].obj_ANIM = LoadAnim ("orbitart/cube3.anim", MEMTYPE_CEL);
  if (obj[iObj].obj_ANIM == NULL)
    {
      status = ORBITALLOC_ERR;
      goto CLEANUP;
    }

  status = SetObjCCB (obj, iObj);
  if (status < 0)
    goto CLEANUP;

  /*
          actual height of cel, minus all the black border pixels
  */
  obj[iObj].obj_Height = 46;

  *pObj = obj;

CLEANUP:
  return status;
}

/*********************
**	UnloadObj - frees up memory associated with orbitobj's
**
**	  Synopsis
**
**	    static void UnloadObj ( OrbitObjPtr obj )
**
**	  Description
**
**	    This function frees up memory associated with orbitobj's
**
**	  Arguments
**
**	    obj						an array of 3doorbit
*objects
**
**	  Return Value
**
**	    None.
**
**
**	  Implementation
**
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**	    orbit.h
**
**	  See Also
**
**	    UnloadCel(), UnloadAnim()
**
*********************/
static void
UnloadObj (OrbitObjPtr obj)
{

  int32 iObj;

  for (iObj = 0; iObj < 3; iObj++)
    {

      if (obj[iObj].obj_ANIM)
        {
          UnloadAnim (obj[iObj].obj_ANIM);
          FreeMem (obj[iObj].obj_CCB, sizeof (CCB));
        }
      else
        UnloadCel (obj[iObj].obj_CCB);
    }

  FreeMem (obj, -1);

  return;
}

/*********************
**	InitObj - initializes values for the beginning of the animation
*sequence
**
**	  Synopsis
**
**	    static void InitObj ( OrbitObjPtr obj )
**
**	  Description
**
**	    This function initializes values for the orbit objects.  Values
*such as the starting *	    positions, speeds, etc.
**
**	  Arguments
**
**	    obj						an array of 3doorbit
*objects
**
**	  Return Value
**
**	    The function returns 0 if all went well, < 0 on an error.
**
**
**	  Implementation
**
**	    This function must be called before any graphics can be displayed.
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**	    orbit.h
**
**	  See Also
**
**
*********************/
static void
InitObj (OrbitObjPtr obj)
{

  int32 iObj;
  frac16 angle;

  /*
          the trig functions are based on lookup tables dividing a circle into
     256 units. We want the objects to originally be 120 degrees apart... which
     translates into 85.3333 of a circle in the operamath units.  As a
     frac16, 85.3333 is (85 << 16) + (1 <<16)/3, which breaks down into 85 <<
     16 + 0x5555, or 0x555555
  */
  angle = 0x555555;

  /*
          link the ccbs for the orbit objects together so that they can all be
     drawn with a single call to draw cels.
  */
  for (iObj = 0; iObj < 3; iObj++)
    {
      if (iObj == 2)
        {
          obj[iObj].obj_CCB->ccb_Flags |= CCB_LAST;
          obj[iObj].obj_CCB->ccb_NextPtr = NULL;
        }
      else
        {
          obj[iObj].obj_CCB->ccb_Flags &= ~CCB_LAST;
          obj[iObj].obj_CCB->ccb_NextPtr = obj[iObj + 1].obj_CCB;
        }

      /*
              set the initial radius, angle, velocity, frame increment, etc for
         the orbit objects.  Watch out for what is a frac16 and an int32.  Get
         a little bit of a speed boost by figuring ahead of time what the
         frac16 representation is of an int32 so we don't have to continually
         do bit shifting during run time
      */
      obj[iObj].obj_radius = RADIUS;
      obj[iObj].obj_theta = MulSF16 (angle, iObj << 16);

      obj[iObj].obj_velocity = VELOCITY;
      obj[iObj].obj_frameIncr = FRAMEINCR;

      /*
              because our calculations are going to be based on the center of
         the cel... we need to know where the upper left hand corner is in
         relation to that.
      */
      obj[iObj].obj_centerX = XCenter - (obj[iObj].obj_CCB->ccb_Width << 15);
      obj[iObj].obj_centerY = YCenter - (obj[iObj].obj_CCB->ccb_Height << 15);

      /*
              This is what happens when your programmer doesn't talk to your
         artist... or when you get artwork from somewhere beyond your control.

              Because the artwork has some black borders, and I want the
         artwork to be drawn right on top of each other (to spaces between)...
         I have to figure out the appropriate offsets and take them into
         consideration.  That is why I needed to know what the real height of
         the artwork is.  Overall, it just makes this program and the logic
         messier
      */
      obj[iObj].obj_Border
          = (obj[iObj].obj_CCB->ccb_Height - obj[iObj].obj_Height) << 15;

      /*
              A bunch of state flags that are going to be used to determine if
         certain things should occur.  For instance, should the large orbit be
         broken and shifted into the spiral orbit.... is the object stopped and
         should we focus on the next object, etc etc.
      */
      obj[iObj].obj_Stopping = false;
      obj[iObj].obj_Stopped = false;
      obj[iObj].obj_WaitingToStop = false;
      obj[iObj].obj_Set = false;
    }

  return;
}

/*********************
**	UpdateObj - updates the positions of all the cels
**
**	  Synopsis
**
**	    static bool UpdateObj ( OrbitObjPtr obj )
**
**	  Description
**
**	    This function drives the animation sequence for this program. Based
*on various states, it
**	    updates the positions of the three cels based on specific
*trajectories. *	    It updates the animations, as well as updating the
*trajectories given *	    the state of the animation.
**
**	    The logic is kinda messed up and confusing... please bear with it.
*Its been a while since I've had *	    my geometry and trig.
**
**	  Arguments
**
**	    obj						an array of 3doorbit
*objects
**
**	  Return Value
**
**	    The function returns false if the sequence is not yet completed,
*true is it is
**
**
**	  Implementation
**
**	    Call this function once during each iteration of the main loop.
**
**	  Caveats
**
**	    None.
**
**	  Associated Files
**
**
**	  See Also
**
**
*********************/
static bool
UpdateObj (OrbitObjPtr obj)
{

  int32 iObj;
  static toStop = 0;
  CCB *ccb, *objCCB;
  OrbitObjPtr object;
  uint32 signals;

  /*
          if all the animations have ended, then play the ending sound, wait
     until its done, then reset everything by returning true;
  */
  if (toStop > 2)
    {

      /*
              set up to monitor the soundeffect, and be notified when it has
         concluded. Start the soundeffect, wait for the cue, then stop the
         instrument
      */
      MonitorAttachment (gCheerSound->si_Attachment, gCue, CUE_AT_END);
      StartInstrument (gCheerSound->si_Player, NULL);
      WaitSignal (gCueSignal);
      StopInstrument (gCheerSound->si_Player, NULL);

      toStop = 0;

      return true;
    }

  /*
          check to see whether the "click" sound effect has ended yet.  We will
     be notified at its conclusion through the cue and its signal.  If we've
     received the signal, then the effect has ended, and we can move onto the
     trajectory of the next object. Clear the signal, and update which object
     we're going to be slowing and stopping.

          If the effect hasn't yet ended, then keep the other objects going
     until it has
  */
  signals = GetCurrentSignals ();
  if (signals & gCueSignal)
    {
      WaitSignal (gCueSignal);
      StopInstrument (gClickSound->si_Player, NULL);
      toStop++;
    }

  /*
          if an object is in a position where it should be stopped, set its
     position, stop it, wait until it gets to the first frame of the animation
     and play the click sound effect
  */
  if (obj[toStop].obj_Stopping && obj[toStop].obj_theta <= HALFCIRCLE)
    {
      obj[toStop].obj_velocity = 0;
      obj[toStop].obj_theta = HALFCIRCLE;

      obj[toStop].obj_CCB->ccb_XPos
          = obj[toStop].obj_centerX
            + MulSF16 (obj[toStop].obj_radius, SinF16 (obj[toStop].obj_theta));

      obj[toStop].obj_CCB->ccb_YPos
          = obj[toStop].obj_centerY
            + MulSF16 (obj[toStop].obj_radius, CosF16 (obj[toStop].obj_theta));

      if (obj[toStop].obj_ANIM)
        {

          /*
                  because GetAnimCel() returns the frame and then increments
             the counter, we actually have to wait until the current frame is 1
             rather than 0. When it is 1, we know that what is currently being
             displayed is the first frame
          */
          if (obj[toStop].obj_ANIM->cur_Frame == 0x10000)
            {
              obj[toStop].obj_Stopped = true;
              obj[toStop].obj_Stopping = false;
              MonitorAttachment (gClickSound->si_Attachment, gCue, CUE_AT_END);
              StartInstrument (gClickSound->si_Player, NULL);
            }
        }

      /*
              there are two anims and one cel in this program... Obviously, for
         the cel, we don't have to do anything special regarding which frame to
         show... so just stop it and make some noise
      */

      else
        {
          obj[toStop].obj_Stopped = true;
          obj[toStop].obj_Stopping = false;
          MonitorAttachment (gClickSound->si_Attachment, gCue, CUE_AT_END);
          StartInstrument (gClickSound->si_Player, NULL);
        }
    }

  /*
          This is the central logic loop for updating an object.  If its
     moving, accelerate it until its speed reaches a predefined maximum
     (oooh... I remember my physics now... the difference between speed and
     velocity!).  Until that time, simply adjust each object's position.

          Once that max speed is reached, then at the appropriate time... reset
     the object's trajectory so that it spirals into place
  */
  for (iObj = 0; iObj < 3; iObj++)
    {

      /*
              if the object is stopped, skip it
      */
      if (obj[iObj].obj_Stopped)
        continue;

      /*
              adjust the angle of the object.  If its less than 0 degrees (
         which in this case, due to the rotated axes (see below), is along the
         positive y axis), make the angle positive.

              Also, if the object was in its last orbit prior to stopping...
         then start it stopping ;-)
      */
      obj[iObj].obj_theta -= obj[iObj].obj_velocity;
      if (obj[iObj].obj_theta < 0)
        {

          if (iObj == toStop && obj[iObj].obj_WaitingToStop)
            {
              obj[iObj].obj_WaitingToStop = false;
              obj[iObj].obj_Stopping = true;
              obj[iObj].obj_theta = 0;
            }

          obj[iObj].obj_theta += FULLCIRCLE;
        }

      /*
              increment the speed of the objects rotation.  If we've reached
         some maximum speed, and the object is somewhere past the top of its
         orbit, then prepare it for stopping
      */
      if (!obj[iObj].obj_Stopping)
        {
          if (obj[iObj].obj_velocity < MAXVELOCITY)
            obj[iObj].obj_velocity
                = MulSF16 (obj[iObj].obj_velocity, ACCELERATOR);
          else
            {
              if (obj[iObj].obj_theta > HALFCIRCLE)
                obj[iObj].obj_WaitingToStop = true;
            }
        }

      /*
              Get the dummy ccb for the object.  If its an ANIM, then redirect
         the source and plut pointer... assuming that the sizes of all the
         frames are the same
      */
      objCCB = obj[iObj].obj_CCB;

      if (obj[iObj].obj_ANIM)
        {
          ccb = GetAnimCel (obj[iObj].obj_ANIM, obj[iObj].obj_frameIncr);

          objCCB->ccb_SourcePtr = ccb->ccb_SourcePtr;
          objCCB->ccb_PLUTPtr = ccb->ccb_PLUTPtr;
        }

      /*
              this is a little backwards.  Normally, x = rcos(theta), y =
         rsin(theta)... but we have them switched here to locate the cels where
         we want them to be... which is in a Y formation... that is, the axes
         are rotated 90 degrees clockwise.
      */
      objCCB->ccb_XPos
          = obj[iObj].obj_centerX
            + MulSF16 (obj[iObj].obj_radius, SinF16 (obj[iObj].obj_theta));

      objCCB->ccb_YPos
          = obj[iObj].obj_centerY
            + MulSF16 (obj[iObj].obj_radius, CosF16 (obj[iObj].obj_theta));

      /*
              big long if statement that basically determines if we should
         reset an objects trajectory.

              If so, then do it so that the object will come to rest at the
         proper spot
      */
      if (iObj == toStop && obj[iObj].obj_Stopping && !obj[iObj].obj_Set
          && !obj[iObj].obj_Stopped)
        {
          object = obj + toStop;

          object->obj_Set = true;

          object->obj_velocity >>= 2;

          if (toStop == 0)
            {

              object->obj_yTop = YCenter + (obj[1].obj_Height << 15)
                                 + (object->obj_Height << 15);
              object->obj_radius = (200 << 15) - (object->obj_yTop >> 1);
              object->obj_centerY = object->obj_yTop + object->obj_radius
                                    - (object->obj_CCB->ccb_Height << 15);
            }
          else
            {
              object->obj_yTop = obj[toStop - 1].obj_yTop
                                 - (obj[toStop - 1].obj_Height << 15)
                                 - (object->obj_Height << 15);

              object->obj_radius = (200 << 15) - (object->obj_yTop >> 1);

              object->obj_centerY = object->obj_yTop + object->obj_radius
                                    - (object->obj_CCB->ccb_Height << 15);

              /*
                      we need this because the artist, when he drew this piece,
                 didn't center the object amidst the background.  Hence, we
                 can't use the logic that we've got in place wholly, because
                 its based on things being centered.

                      Moral of this story.  Talk to your artist... work
                 together... can't we all just get along?
              */
              if (toStop == 1)
                object->obj_centerY -= 0x30000;
            }
        }
    }

  return false;
}
