
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: animsample.c,v 1.13 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/animsample
|||	animsample - Loads and displays an animation and a background.
|||
|||	  Synopsis
|||
|||	    animsample \<imagefile> \<animfile>
|||
|||	  Description
|||
|||	    Demonstrates loading and displaying an animation and a background
image.
|||
|||	    The program allows the user to change the animation's frame rate,
the four
|||	    corners of the animation's cels, and the P-mode.
|||
|||	  Arguments
|||
|||	    imagefile                    Name of 3DO image.
|||
|||	    animfile                     Name of 3DO cel or animation.
|||
|||	  Associated Files
|||
|||	    animsample.c, animsample.h, animerror.h
|||
|||	  Location
|||
|||	    examples/Animsample
|||
**/

#include "animsample.h"

void
Usage (void)
{

  PRT (("C                     toggles between moving cel mode, and warping "
        "cel mode\n"));
  PRT (("d-pad                 moves cel around or warps corners\n"));
  PRT (("B + D-pad             scales the cel\n"));
  PRT (("Left or Right Shift   cycles through current corner\n"));
  PRT (("A + up or down        toggles pmode\n"));
  PRT (("A + left or right     changes anims frame rate\n"));
  PRT (("Stop                  exits\n"));

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
**	    This function opens the graphics and math folios, initializes for 1
**	    controlpad, and creates an error text and a couple of of IO req
*items
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
**	    animsample.h animerror.h controlpad.h controlpad.c
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
  status = CreateItem (MKNODEID (KERNELNODE, ERRORTEXTNODE), AnimErrorTags);
  if (status < 0)
    goto CLEANUP;

  gAnimErrorItem = status;

  /*
     open the graphics folio, fill in the screen context structure
     for nScreens screens
  */
  if (!CreateBasicDisplay (sc, DI_TYPE_NTSC, (int)nScreens))
    {
      status = ANIMGRAPHICS_ERR;
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
     init structures and resources for querying the controlpad
  */
  status = InitControlPad (1);
  if (status < 0)
    goto CLEANUP;

CLEANUP:
  return status;
}

/*********************
**	main - main routine
**
**	  Synopsis
**
**	    int main ( int32 argc, char *argv[] )
**
**	  Description
**
**	    This function initializes the system, loads in a 3DO background and
**	    a 3DO anim or cel, drops into a main loop where it queries the
*control *	    pad and reacts to it, double buffers, and renders all the
*artwork
**
**	  Arguments
**
**	    argc	         number of command line arguments
**
**	    argv		 the command line arguments
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
**	   none
**
**	  Associated Files
**
**	    animsample.h animerror.h controlpad.h controlpad.c
**
**	  See Also
**
**
*********************/
int
main (int32 argc, char *argv[])
{

  ScreenContext sc;
  ANIM *anim = NULL;
  ubyte *background = NULL;

  uint32 button;

  CCB *ccb = NULL, *realCCB;

  int32 status = 0;

  /*
     Do some idiot checking, print what the buttons do
  */
  if (argc != 3)
    {
      ERR (("Usage:: %s <imagefile> <celfile>\n", argv[0]));
      exit (0);
    }

  PRT (("%s %s\n", argv[0], VERSION));
  Usage ();

  if ((status = Initialize (&sc, 2)) < 0)
    {
      PrintfSysErr (status);
      exit (0);
    }

  /*
     Load in the background 3DO image.  Let LoadImage () allocate a buffer
     for us (second parameter)... and we don't care about the VDL with
     the image ( third parameter)
  */
  background = (ubyte *)LoadImage (argv[1], NULL, NULL, &sc);
  if (background == NULL)
    {
      status = ANIMLOADIMAGE_ERR;
      goto CLEANUP;
    }

  /*
     Load in the 3DO animation.  As an animation is a collection of cels,
     this will work even if we are just loading a cel... it will just be
     an anim with 1 frame.  Specify that we want the data loaded into memory
     dedicated to the cel engine.
  */
  anim = LoadAnim (argv[2], MEMTYPE_CEL);
  if (anim == NULL)
    {
      status = ANIMLOADANIM_ERR;
      goto CLEANUP;
    }

  /*
     Do some idiot checking... do we have any frames in the anim?
  */
  if (anim->num_Frames > 0)
    {
      PRT (("%ld cels found in %s\n", anim->num_Frames, argv[2]));
    }
  else
    {
      status = ANIMNUMFRAMES_ERR;
      goto CLEANUP;
    }

  /*
     Allocate some space for a ccb.  We will use this later.  The reason
     why we want to dynamically allocate the ccb* rather than declare
     a ccb and set a pointer to it is future compatibility.  Allocating
     ensures where we get the memory from.  In this case, its DMA accessible
     (very important for the cel engine).  Not doing the allocation means
     that we can't control were the memory comes from... and in the future,
     that might be bad (ie. the code would break)

     Also, fill in the new ccb with data from an actual cel from the
     animation.  This ensures that we have good data, rather than garbage
  */
  ccb = (CCB *)AllocMem (sizeof (CCB), MEMTYPE_CEL);
  if (ccb == NULL)
    {
      status = ANIMALLOC_ERR;
      goto CLEANUP;
    }

  memcpy (ccb, GetAnimCel (anim, 0), sizeof (CCB));

  /*
     Set some global variables that are going to be used to specify
     where to draw the cel.
  */
  gP[0].pt_X = gP[3].pt_X = ccb->ccb_XPos >> 16;
  gP[1].pt_X = gP[2].pt_X = gP[0].pt_X + ccb->ccb_Width;
  gP[0].pt_Y = gP[1].pt_Y = ccb->ccb_YPos >> 16;
  gP[2].pt_Y = gP[3].pt_Y = gP[0].pt_Y + ccb->ccb_Height;

  /*
     gFrameIncr is the value to pass in GetAnimCel() to update the current
     frame counter.  IMPORTANT:  keep in mind that this is a frac16.
  */
  gFrameIncr = 1 << 16;
  gMoveIncr = MOVE_DELTA << 16;

  /*
     Drop into the main loop.  We stay here until the user quits
     by hitting stop on the control pad.

     In this loop, we react to the control pad, double buffer by toggling
     what the current screen is, and draw the background and the animation.
  */
  while (true)
    {

      /*
         Query the control pad
      */
      status = DoControlPad (1, &button, CONTROL_ALL);
      if (status < 0)
        break;

      /*
         react to any button presses
      */
      if (!HandleButton (button, ccb))
        break;

      /*
         Here is a nifty trick.  Remember that ccb we allocated?  Well, we use
         it here.

         GetAnimCel() returns to us a ccb pointing to the current frame.  We
         can write positioning values into the ccb and it writes those into
         the animation.  This is fine... except, when we get back to this
         same frame the next time around, those same values will be written
         into the frame that we get.  This might not be good.... inother
         words, if we didn't reset the positioning values, then the anim would
         suddenly appear where it was a while ago.

         So what we do instead, is have a dummy ccb around, which contains
         all the positioning information in it.  Then we just shift the source
         and plut pointers of this cel to the cels in the animation.

         This way, a lot of the data for the cel is kept in one place and our
         housekeeping is made much easier
      */
      realCCB = GetAnimCel (anim, gFrameIncr);
      ccb->ccb_SourcePtr = realCCB->ccb_SourcePtr;
      ccb->ccb_PLUTPtr = ccb->ccb_PLUTPtr;

      /*
         Toggle the current screen counter.

         Copy the background pic to the current screen.  We could have
         used DrawImage... but it just calls this function... so we
         deal with some extra overhead.

         BTW... this does an implicit WaitVBL before it returns... so
         we don't need to do one ourselves in worrying about synching
         with the electron beam.

         One the other hand, if we used SetVRAMPages(), then we might have
         to.

         Also, we might have chosen to use the Defer family of these functions
         if we had some extra processing to do while we were waiting
         for the vbl.  We don't, so we don't use that function

         We draw the cel with DrawCels().  If we had multiple cels to draw,
         I would have linked them all together through the NextPtr and
         drawn them all with a single call.

         This is substantially faster than calling DrawCels() on each
         cel.

         Also, I don't use DrawScreenCels()... that was designed for
         multiple bitmaps, and we don't have that... so, again, why
         incur some extra overhead?
       */
      sc.sc_curScreen ^= 1;

      CopyVRAMPages (gVramIOReq, sc.sc_Bitmaps[sc.sc_curScreen]->bm_Buffer,
                     background, sc.sc_nFrameBufferPages, ~0);

      DrawCels (sc.sc_BitmapItems[sc.sc_curScreen], ccb);

      /*
         This queues up the screen we just drew to be displayed at
         the next vertical blank.  The function returns immediately, however
      */
      DisplayScreen (sc.sc_Screens[sc.sc_curScreen], 0);
    }

CLEANUP:
  if (status < 0)
    PrintfSysErr (status);

  FadeToBlack (&sc, 20);

  FreeMem (ccb, sizeof (CCB));
  if (background)
    UnloadImage (background);
  if (anim)
    UnloadAnim (anim);

  DeleteVBLIOReq (gVblIOReq);
  DeleteVRAMIOReq (gVramIOReq);
  DeleteItem (gAnimErrorItem);

  KillControlPad ();

  CloseGraphics (&sc);

  PRT (("Exiting\n"));

  exit (0);
}

/*********************
**	HandleButton - handles events off of the control pad
**
**	  Synopsis
**
**	    static bool HandleButton ( uint32 button, CCB *ccb )
**
**	  Description
**
**	    This function takes an event to from the control pad and reacts to
*it. *	    according to the following logic.
**
**	    ControlX					quits the program
**	    ControlB + DPAD				scales the cel
**	    ControlA + up/down			toggles pmode
**	    ControlA + left/right		changes frame rate of animation
**	    ControlC					toggles between moving
*the cel and warping the corners *	    Left/RightShift
*cycles through which corner to warp
**	    DPAD						moves the cel or
*warps a corner, depending on mode
**
**	  Arguments
**
**	    button	         which buttons are down in this event
**
**	    ccb			 pointer to the cel whose positioning we want
*to modify
**
**
**	  Return Value
**
**	    The function returns nothing
**
**
**	  Implementation
**
**
**	  Caveats
**
**	   Whoa!  Tons of stuff.  If you were doing this yourself, how would
*you do it?
**
**	   The idea is to try to minimize the calls to MapCel ().  Why would
*you want to *	   minimize those calls?  Because MapCel() is slow.  There are
*8 fields in the CCB *	   which determine the region in which the cel engines
*render a cel.  MapCel() sets *	   all 8 of those fields.  Six of those,
*require divides, which are slower than *	   molasses on a cold winters
*day.  Most position changes for a cel don't *	   depend on all 8 of those
*fields... some can stay the same... so you end up *	   wasting a lot of
*time doing unnecessary math.  This is most generally the *	   case when
*you are doing simple moving and scaling of the cel (ie. adding no *
*perspective).  Most of the time, you can pretty much figure out what the
**	   ccb scaling factors are and do the math yourself.  The formulas are
*(given *	   that the order of points is clockwise, starting at the upper
*left):
**
**	   x = x0
**	   y = y0
**	   hdx = (x1-x0)/w
**	   hdy = (y1-y0)/w
**
**	   vdx = (x3-x0)/h
**	   vdy = (y3-y0)/h
**
**	   hddx = [(x2-x3) - (x1-x0)]/wh = (hdx1-hdx0)/h
**	   hddy = [(y2-y3) - (y1-y0)]/wh = (hdy1-hdy0)/w
**
**	   where w and h are the pixel dimensions of the cel.
**
**	   Keep in mind what base the ccb fields are.  XPos, YPos, VDX, and VDY
*are 16.16 *	   HDX, HDY, HDDX and HDDY are 12.20.
**
**	   Secondly, there is MapP2Cel(), which works on cels whose dimensions
*are powers of
**	   two.  There is also FastMapCel() and InitFastMapCel() which are
*faster *	   than MapCel(), but provide less precision.
**
**	  Associated Files
**
**	    animsample.h animerror.h controlpad.h controlpad.c
**
**	  See Also
**
**	    MapCel(), MapP2Cel(), FastMapCel(), InitFastMapCel()
*********************/
static bool
HandleButton (uint32 button, CCB *ccb)
{

  if (button == ControlX)
    return false;

  if (button & ControlB)
    {

      if (button & ControlUp)
        {
          gP[2].pt_Y -= MOVE_DELTA;
          gP[3].pt_Y -= MOVE_DELTA;

          ccb->ccb_VDY = ((gP[3].pt_Y - gP[0].pt_Y) << 16) / ccb->ccb_Height;
        }

      if (button & ControlDown)
        {
          gP[2].pt_Y += MOVE_DELTA;
          gP[3].pt_Y += MOVE_DELTA;

          ccb->ccb_VDY = ((gP[3].pt_Y - gP[0].pt_Y) << 16) / ccb->ccb_Height;
        }

      if (button & ControlRight)
        {
          gP[1].pt_X += MOVE_DELTA;
          gP[2].pt_X += MOVE_DELTA;

          ccb->ccb_HDX = ((gP[1].pt_X - gP[0].pt_X) << 20) / ccb->ccb_Width;
        }

      if (button & ControlLeft)
        {
          gP[1].pt_X -= MOVE_DELTA;
          gP[2].pt_X -= MOVE_DELTA;

          ccb->ccb_HDX = ((gP[1].pt_X - gP[0].pt_X) << 20) / ccb->ccb_Width;
        }
    }

  else if (button == ControlLeftShift)
    {
      if (--gCorner < 0)
        gCorner = 3;

      PRT (("Corner %ld\n", gCorner));
    }

  else if (button == ControlRightShift)
    {
      if (++gCorner > 3)
        gCorner = 0;

      PRT (("Corner %ld\n", gCorner));
    }

  else if (button == ControlC)
    {
      if (gMoveCorners)
        {
          gMoveCorners = false;
          PRT (("Moving cel mode\n"));
        }
      else
        {
          gMoveCorners = true;
          PRT (("Warp corners mode\n"));
        }
    }

  else if (button & ControlA)
    {

      if (button & ControlUp)
        {

          /*
             This will tweak the pmode setting in the flags fields.

             CCB_POVER_MASK is a bit mask with 1's in all the three relevant
             bits of the flags, 0's elsewhere.  You can use this &'d with a
             PMODE_ONE or PMODE_ZERO or PMODE_PDC, to find out what is the
             current pmode for the cel.  &=~'d it with the flags, will clear
             the current pmode and allow you to set whichever one you want.

             PMODE_ONE	tells the cel engine to render the cel with the upper
             16 bits of the PIXC PMODE_ZERO	tells the cel engine to render
             the cel with the upper 16 bits of the PIXC PMODE_PDC	tells
             the cel engine to render the cel with whatever pmode comes out of
             the pixel decoder (ie. per pixel pmode)
          */

          ccb->ccb_Flags &= ~CCB_POVER_MASK;
          ccb->ccb_Flags |= PMODE_ONE;
        }

      if (button & ControlDown)
        {
          ccb->ccb_Flags &= ~CCB_POVER_MASK;
          ccb->ccb_Flags |= PMODE_PDC;
        }

      if (button & ControlLeft)
        gFrameIncr -= 0x00001000;

      if (button & ControlRight)
        gFrameIncr += 0x00001000;
    }

  else
    {
      if (gMoveCorners)
        {

          if (button & ControlUp)
            gP[gCorner].pt_Y -= MOVE_DELTA;

          if (button & ControlDown)
            gP[gCorner].pt_Y += MOVE_DELTA;

          if (button & ControlRight)
            gP[gCorner].pt_X += MOVE_DELTA;

          if (button & ControlLeft)
            gP[gCorner].pt_X -= MOVE_DELTA;

          if (button)
            {
              switch (gCorner)
                {
                case 0:

                  /*
                     Every field in the ccb is changing, so I might as well use
                     MapCel() In all the other cases, I only need to
                     recalculate those fields which have changed due to the
                     changed coordinates.  So I spare myself most of the
                     divides that would take place, and speed up my program
                     just a bit.
                  */
                  MapCel (ccb, gP);
                  break;
                case 1:
                  ccb->ccb_HDX
                      = ((gP[1].pt_X - gP[0].pt_X) << 20) / ccb->ccb_Width;
                  ccb->ccb_HDY
                      = ((gP[1].pt_Y - gP[0].pt_Y) << 20) / ccb->ccb_Width;

                  ccb->ccb_HDDX = (((gP[2].pt_X - gP[3].pt_X)
                                    - (gP[1].pt_X - gP[0].pt_X))
                                   << 20)
                                  / (ccb->ccb_Height * ccb->ccb_Width);

                  ccb->ccb_HDDY = (((gP[2].pt_Y - gP[3].pt_Y)
                                    - (gP[1].pt_Y - gP[0].pt_Y))
                                   << 20)
                                  / (ccb->ccb_Height * ccb->ccb_Width);
                  break;
                case 2:
                  ccb->ccb_HDDX = (((gP[2].pt_X - gP[3].pt_X)
                                    - (gP[1].pt_X - gP[0].pt_X))
                                   << 20)
                                  / (ccb->ccb_Height * ccb->ccb_Width);

                  ccb->ccb_HDDY = (((gP[2].pt_Y - gP[3].pt_Y)
                                    - (gP[1].pt_Y - gP[0].pt_Y))
                                   << 20)
                                  / (ccb->ccb_Height * ccb->ccb_Width);
                  break;
                case 3:
                  ccb->ccb_VDX
                      = ((gP[3].pt_X - gP[0].pt_X) << 16) / ccb->ccb_Height;
                  ccb->ccb_VDY
                      = ((gP[3].pt_Y - gP[0].pt_Y) << 16) / ccb->ccb_Height;

                  ccb->ccb_HDDX = (((gP[2].pt_X - gP[3].pt_X)
                                    - (gP[1].pt_X - gP[0].pt_X))
                                   << 20)
                                  / (ccb->ccb_Height * ccb->ccb_Width);

                  ccb->ccb_HDDY = (((gP[2].pt_Y - gP[3].pt_Y)
                                    - (gP[1].pt_Y - gP[0].pt_Y))
                                   << 20)
                                  / (ccb->ccb_Height * ccb->ccb_Width);
                  break;
                default:
                  break;
                }
            }
        }

      else
        {

          if (button & ControlUp)
            {
              ccb->ccb_YPos -= gMoveIncr;
              gP[0].pt_Y -= MOVE_DELTA;
              gP[1].pt_Y -= MOVE_DELTA;
              gP[2].pt_Y -= MOVE_DELTA;
              gP[3].pt_Y -= MOVE_DELTA;
            }

          if (button & ControlDown)
            {
              ccb->ccb_YPos += gMoveIncr;
              gP[0].pt_Y += MOVE_DELTA;
              gP[1].pt_Y += MOVE_DELTA;
              gP[2].pt_Y += MOVE_DELTA;
              gP[3].pt_Y += MOVE_DELTA;
            }

          if (button & ControlRight)
            {
              ccb->ccb_XPos += gMoveIncr;
              gP[0].pt_X += MOVE_DELTA;
              gP[1].pt_X += MOVE_DELTA;
              gP[2].pt_X += MOVE_DELTA;
              gP[3].pt_X += MOVE_DELTA;
            }

          if (button & ControlLeft)
            {
              ccb->ccb_XPos -= gMoveIncr;
              gP[0].pt_X -= MOVE_DELTA;
              gP[1].pt_X -= MOVE_DELTA;
              gP[2].pt_X -= MOVE_DELTA;
              gP[3].pt_X -= MOVE_DELTA;
            }
        }
    }

  return true;
}
