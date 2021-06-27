
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: aaplayer.c,v 1.13 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/aaplayer
|||	aaplayer - Loads and plays back an anti-aliased animation, using timer
|||	    mechanisms.
|||
|||	  Synopsis
|||
|||	    aaplayer \<imagefile> \<animfile>
|||
|||	  Description
|||
|||	    Demonstrates loading and playing an anti-aliased animation and a
|||	    background image. Allows the user to change the animation's frame rate,
|||	    and the four corners of the animation's cels, and to toggle displaying the
|||	    matte cel.
|||
|||	  Arguments
|||
|||	    imagefile                    Name of 3DO image.
|||
|||	    animfile                     Name of 3DO cel or animation.
|||
|||	  Associated Files
|||
|||	    aaplayer.c, aaplayer.h, aaerror.h
|||
|||	  Location
|||
|||	    examples/Graphics/aaplayer
|||
**/


#include "aaplayer.h"


void Usage ( void ) {

  PRT (( "C                     toggles between moving the cel, and warping the corners\n"));
  PRT (( "d-pad                 moves cel around or warps the corners\n"));
  PRT (( "B + D-pad             scales the cel\n"));
  PRT (( "Left or Right Shift   cycles through current corner\n"));
  PRT (( "A + up or down        toggles display of matte cel\n"));
  PRT (( "A + left or right     changes anims frame rate\n"));
  PRT (( "Stop                  exits\n"));

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
**	    controlpad, and creates an error text and a couple of IO req items
**
**	  Arguments
**
**	    sc	                 pointer to a screen context structure
**
**	    nScreens             the number of screens to create
**
**	  Return Value
**
**	    The function returns 0 if successful or an error code (a negative value)
**	    if an error occurs.
**
**	  Implementation
**
**	    Called prior to main loop
**
*********************/
static int32 Initialize ( ScreenContext *sc, int32 nScreens ) {

	int32		status = 0;

			/*
			   create an error text item to return opera style errors
			*/
	status = CreateItem ( MKNODEID(KERNELNODE, ERRORTEXTNODE), AAErrorTags );
	if ( status < 0 )
		goto CLEANUP;

	gAAErrorItem = status;


			/*
			   open the graphics folio, fill in the screen context structure
			   for nScreens screens
			*/
	if ( ! CreateBasicDisplay ( sc, DI_TYPE_NTSC, (int) nScreens ) ) {
		status = AAGRAPHICS_ERR;
		goto CLEANUP;
	}

	sc->sc_curScreen = 0;


			/*
			   get an io req item for SPORT transfers, and for using the VBL timer
			*/
	status = CreateVRAMIOReq ();
	if ( status < 0 )
		goto CLEANUP;

	gVramIOReq = status;


	status = CreateVBLIOReq ();
	if ( status < 0 )
		goto CLEANUP;

	gVblIOReq = status;

			/*
				get the item number of the system timer, called "timer" and an io req for doing timer
				queries so we can adjust the frame rate of the animation.
			*/
	status = OpenNamedDevice ( "timer", 0 );
	if ( status < 0 )
		goto CLEANUP;

	gTimerDevice = status;


	status = CreateIOReq ( 0, 0, gTimerDevice, 0 );
	if ( status < 0 )
		goto CLEANUP;

	gTimerReq = status;


			/*
			   init structures and resources for querying the controlpad
			*/
	status = InitControlPad ( 1 );
	if ( status < 0 )
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
**	    a 3DO anim or cel, drops into a main loop where it queries the control
**	    pad and reacts to it, double buffers, and renders all the artwork
**
**	  Arguments
**
**	    argc	         number of command line arguments
**
**	    argv	   	 	 the command line arguments
**
**	  Return Value
**
**	    The function exits with  0.
**
*********************/
int main ( int32 argc, char *argv[] ) {

	ScreenContext		sc;
	ANIM				*anim = NULL;
	ubyte				*background = NULL;

	uint32				button;

	CCB					*ccb = NULL, *alphaCCB = NULL, *realCCB;

	int32				status = 0;

	IOReq				*pIOReq;
	struct timeval		lastTime;


			/*
			   Do some idiot checking, print what the buttons do
			*/
	if ( argc != 3 ) {
		ERR (( "Usage:: %s <imagefile> <celfile>\n", argv[0] ));
		exit ( 0 );
	}

	PRT (( "%s %s\n", argv[0], VERSION ));
	Usage ();

	if ( ( status = Initialize ( &sc, 2 ) ) < 0 ) {
		PrintfSysErr ( status );
		exit ( 0 );
	}

			/*
			   Load in the background 3DO image.  Let LoadImage () allocate a buffer
			   for us (second parameter)... and we don't care about the VDL with
			   the image ( third parameter)
			*/
	background = ( ubyte * ) LoadImage ( argv[1], NULL, NULL, &sc );
	if ( background == NULL ) {
		status = AALOADIMAGE_ERR;
		goto CLEANUP;
	}

			/*
			   Load in the 3DO animation.  As an animation is a collection of cels,
			   this will work even if we are just loading a cel... it will just be
			   an anim with 1 frame.  Specify that we want the data loaded into memory
			   dedicated to the cel engine.

			   This gets sorta tricky in this case, because we are loading an anti-aliased
			   animation.  An anti-aliased animation consists of two cels per frame of animation.
			   The one cel is the actual cel, with the second being the matte cel that
			   is used to dimish the background and create the anti-aliasing effect.
			   This means that each frame consists of two cels, one is the matte cel,
			   the second is the actual frame.  This means that we have to handle this differently
			   from normal animations.

			   How you bring it in will determine what you do with them. If you load in a AA cel
			   with LoadCel(), it will return you a pointer to the matte ccb with the actual ccb chained to
			   it.  Then you can just call DrawCels() (You want the matte cel drawn first).

			   Or, if you bring in the AA cel with LoadAnim(), then you get two ccbs in the
			   animation, the first being the cel, and the second being the matte.  Chain
			   them together in the right order and then draw them.

			   If you call LoadAnim() on an anti-aliased animation, then you get a the
			   ccb's and you have to chain each pair together.
			*/
	anim = LoadAnim ( argv[2], MEMTYPE_CEL );
	if ( anim == NULL ) {
		status = AALOADANIM_ERR;
		goto CLEANUP;
	}


			/*
			   Do some idiot checking... do we have any frames in the anim?
			*/
	if ( anim->num_Frames > 0 ) {
		PRT (( "%ld cels found in %s\n", anim->num_Frames, argv[2] ));
	} else {
		status = AANUMFRAMES_ERR;
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
			   animation.  This ensures that we have good data, rather than garbage.

			   In this case, we also need a second CCB for the alpha channel.

			*/
	ccb = ( CCB * ) AllocMem ( sizeof ( CCB ), MEMTYPE_CEL);
	if ( ccb == NULL ) {
		status = AAALLOC_ERR;
		goto CLEANUP;
	}

	alphaCCB = ( CCB * ) AllocMem ( sizeof ( CCB ), MEMTYPE_CEL );
	if ( alphaCCB == NULL ) {
		status = AAALLOC_ERR;
		goto CLEANUP;
	}

	memcpy ( ccb, GetAnimCel ( anim, 1 << 16 ), sizeof ( CCB ) );
	memcpy ( alphaCCB, GetAnimCel ( anim, 1 << 16 ), sizeof ( CCB ) );


			/*
				chain the two dummy cels together, alpha channel first so that it will
				be drawn first
			*/
	alphaCCB->ccb_NextPtr = ccb;
	alphaCCB->ccb_Flags &= ~CCB_LAST;

	ccb->ccb_NextPtr = NULL;
	ccb->ccb_Flags |= CCB_LAST;

	anim->cur_Frame = 0;


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

			   Given the way the animation is kept in the anti-aliased anim, we can't really
			   use this field to adjust for frame rate, as we could in normal animations.  That's
			   because each true frame consists of a pair of cels, and GetAnimCel() doesn't have
			   enough intelligence in it to figure out how to return the proper cel that
			   you are expecting.
			*/
	gFrameIncr = 1 << 16;
	gMoveIncr = MOVE_DELTA << 16;


			/*
				Rather than using the frame increment to GetAnimCel(), we are going to be using
				a timer to figure out if its time to advance the animation, given a desired frame
				rate.

				Here, we figure out what time it is now, so we can figure frame timing differences
			*/
	pIOReq = (IOReq *) LookupItem ( gTimerReq );
	(void) GetTime ( pIOReq, &lastTime );

			/*
			   Drop into the main loop.  We stay here until the user quits
			   by hitting stop on the control pad.

			   In this loop, we react to the control pad, double buffer by toggling
			   what the current screen is, and draw the background and the animation.
			*/
	while ( true ) {

			/*
			   Query the control pad
			*/
		status = DoControlPad ( 1, &button, CONTROL_ALL );
		if ( status < 0 )
			break;


			/*
			   react to any button presses
			*/
		if ( ! HandleButton ( button, alphaCCB ) )
			break;


			/*
				check to see if its time to advance to the next animation frame.  If so,
				the grab the next frames
			*/
		if ( AdvanceFrame ( pIOReq, &lastTime ) > 0) {

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

			realCCB = GetAnimCel ( anim, gFrameIncr );
			ccb->ccb_SourcePtr = realCCB->ccb_SourcePtr;
			ccb->ccb_PLUTPtr = ccb->ccb_PLUTPtr;

			realCCB = GetAnimCel ( anim, gFrameIncr );
			alphaCCB->ccb_SourcePtr = realCCB->ccb_SourcePtr;
			alphaCCB->ccb_PLUTPtr = ccb->ccb_PLUTPtr;
		}

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

		CopyVRAMPages ( gVramIOReq, sc.sc_Bitmaps [ sc.sc_curScreen ]->bm_Buffer,
								background, sc.sc_nFrameBufferPages, ~0 );

		DrawCels ( sc.sc_BitmapItems [ sc.sc_curScreen ], alphaCCB );

			/*
			   This queues up the screen we just drew to be displayed at
			   the next vertical blank.  The function returns immediately, however
			*/
		DisplayScreen ( sc.sc_Screens [ sc.sc_curScreen ], 0 );

	}

CLEANUP:
	if ( status < 0 )
		PrintfSysErr ( status );

	FadeToBlack ( &sc, 20 );

	FreeMem(ccb,sizeof(CCB));
	FreeMem(alphaCCB,sizeof(CCB));
	if ( background )			UnloadImage ( background );
	if ( anim )					UnloadAnim ( anim );

	DeleteVBLIOReq ( gVblIOReq );
	DeleteVRAMIOReq ( gVramIOReq );
	DeleteItem ( gAAErrorItem );
	DeleteIOReq ( gTimerReq );
	CloseNamedDevice( gTimerDevice );

	KillControlPad ();

	CloseGraphics ( &sc );

	PRT (( "Exiting\n" ));

	exit ( 0 );

}



/*********************
**	HandleButton - handles events off of the control pad
**
**	  Synopsis
**
**	    static bool HandleButton ( uint32 button, CCB *alphaCCB )
**
**	  Description
**
**	    This function takes an event to from the control pad and reacts to it.
**	    according to the following logic.
**
**	    ControlX					quits the program
**	    ControlB + DPAD				scales the cel
**	    ControlA + up/down			toggles drawing of matte cel
**	    ControlA + left/right		changes frame rate of animation
**	    ControlC					toggles between moving the cel and warping the corners
**	    Left/RightShift				cycles through which corner to warp
**	    DPAD						moves the cel or warps a corner, depending on mode
**
**	  Arguments
**
**	    button	         which buttons are down in this event
**
**	    ccb			 pointer to the cel whose positioning we want to modify
**
**
**	  Return Value
**
**	    The function returns nothing
**
**	  Caveats
**
**	   Whoa!  Tons of stuff.  If you were doing this yourself, how would you do it?
**
**	   The idea is to try to minimize the calls to MapCel ().  Why would you want to
**	   minimize those calls?  Because MapCel() is slow.  There are 8 fields in the CCB
**	   which determine the region in which the cel engines render a cel.  MapCel() sets
**	   all 8 of those fields.  Six of those, require divides, which are slower than
**	   molasses on a cold winters day.  Most position changes for a cel don't
**	   depend on all 8 of those fields... some can stay the same... so you end up
**	   wasting a lot of time doing unnecessary math.  This is most generally the
**	   case when you are doing simple moving and scaling of the cel (ie. adding no
**	   perspective).  Most of the time, you can pretty much figure out what the
**	   ccb scaling factors are and do the math yourself.  The formulas are (given
**	   that the order of points is clockwise, starting at the upper left):
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
**	   Keep in mind what base the ccb fields are.  XPos, YPos, VDX, and VDY are 16.16
**	   HDX, HDY, HDDX and HDDY are 12.20.
**
**	   Secondly, there is MapP2Cel(), which works on cels whose dimensions are powers of
**	   two.  There is also FastMapCel() and InitFastMapCel() which are faster
**	   than MapCel(), but provide less precision.
**
*********************/
static bool HandleButton ( uint32 button, CCB *alphaCCB ) {

	CCB		*ccb;

		/*
			in this case, we have to adjust the positions of two cels, identically.
			Because we've linked the matte and true cels together, we can do all
			the calculations on one, and then just reset the CCB values of the other.
		*/
	ccb = alphaCCB->ccb_NextPtr;

	if ( button == ControlX )
		return false;


	if ( button & ControlB ) {

		if ( button & ControlUp ) {
			gP[2].pt_Y -= MOVE_DELTA;
			gP[3].pt_Y -= MOVE_DELTA;

			ccb->ccb_VDY = ( ( gP[3].pt_Y - gP[0].pt_Y) << 16 ) / ccb->ccb_Height;

		}


		if ( button & ControlDown )  {
			gP[2].pt_Y += MOVE_DELTA;
			gP[3].pt_Y += MOVE_DELTA;

			ccb->ccb_VDY = ( ( gP[3].pt_Y - gP[0].pt_Y) << 16 ) / ccb->ccb_Height;

		}


		if ( button & ControlRight ) {
			gP[1].pt_X += MOVE_DELTA;
			gP[2].pt_X += MOVE_DELTA;

			ccb->ccb_HDX = ( ( gP[1].pt_X - gP[0].pt_X) << 20 ) / ccb->ccb_Width;

		}


		if ( button & ControlLeft ) {
			gP[1].pt_X -= MOVE_DELTA;
			gP[2].pt_X -= MOVE_DELTA;

			ccb->ccb_HDX = ( ( gP[1].pt_X - gP[0].pt_X) << 20 ) / ccb->ccb_Width;
		}

	}

	else if ( button == ControlLeftShift ) {
		if ( --gCorner < 0 )
			gCorner = 3;

		PRT (( "Corner %ld\n", gCorner ));
	}

	else if ( button == ControlRightShift ) {
		if ( ++gCorner > 3 )
			gCorner = 0;

		PRT (( "Corner %ld\n", gCorner ));
	}

	else if ( button == ControlC )  {
			if ( gMoveCorners ) {
				gMoveCorners = false;
				PRT (( "Moving cel mode\n" ));
			}
			else {
				gMoveCorners = true;
				PRT (( "Warp corners mode\n" ));
			}

	}

	else if ( button & ControlA ) {

		if ( button & ControlUp )
			alphaCCB->ccb_Flags |= CCB_SKIP;

		if ( button & ControlDown )
			alphaCCB->ccb_Flags &= ~CCB_SKIP;


		if ( button & ControlLeft ) {
			if ( --gFrameRate <1 )
				gFrameRate = 1;
		}

		if ( button & ControlRight )
			gFrameRate++;
	}

	else {
		if ( gMoveCorners ) {

			if ( button & ControlUp )
				gP [ gCorner ].pt_Y -= MOVE_DELTA;

			if ( button & ControlDown )
				gP [ gCorner ].pt_Y += MOVE_DELTA;

			if ( button & ControlRight )
				gP [ gCorner ].pt_X += MOVE_DELTA;

			if ( button & ControlLeft )
				gP [ gCorner ].pt_X -= MOVE_DELTA;

			if ( button ) {
				switch ( gCorner ) {
					case 0:


							/*
							   Every field in the ccb is changing, so I might as well use MapCel()
							   In all the other cases, I only need to recalculate those fields which
							   have changed due to the changed coordinates.  So I spare myself most
							   of the divides that would take place, and speed up my program just a bit.
							*/
						MapCel ( ccb, gP );
						break;
					case 1:
						ccb->ccb_HDX = ( ( gP[1].pt_X - gP[0].pt_X ) << 20 ) / ccb->ccb_Width;
						ccb->ccb_HDY = ( ( gP[1].pt_Y - gP[0].pt_Y ) << 20 ) / ccb->ccb_Width;

						ccb->ccb_HDDX = ( ( ( gP[2].pt_X - gP[3].pt_X ) - ( gP[1].pt_X - gP[0].pt_X ) ) << 20 ) /
													( ccb->ccb_Height * ccb->ccb_Width );

						ccb->ccb_HDDY = ( ( ( gP[2].pt_Y - gP[3].pt_Y ) - ( gP[1].pt_Y - gP[0].pt_Y ) ) << 20 ) /
													( ccb->ccb_Height * ccb->ccb_Width );
						break;
					case 2:
						ccb->ccb_HDDX = ( ( ( gP[2].pt_X - gP[3].pt_X ) - ( gP[1].pt_X - gP[0].pt_X ) ) << 20 ) /
													( ccb->ccb_Height * ccb->ccb_Width );

						ccb->ccb_HDDY = ( ( ( gP[2].pt_Y - gP[3].pt_Y ) - ( gP[1].pt_Y - gP[0].pt_Y ) ) << 20 ) /
													( ccb->ccb_Height * ccb->ccb_Width );
						break;
					case 3:
						ccb->ccb_VDX = ( ( gP[3].pt_X - gP[0].pt_X ) << 16 ) / ccb->ccb_Height;
						ccb->ccb_VDY = ( ( gP[3].pt_Y - gP[0].pt_Y ) << 16 ) / ccb->ccb_Height;

						ccb->ccb_HDDX = ( ( ( gP[2].pt_X - gP[3].pt_X ) - ( gP[1].pt_X - gP[0].pt_X ) ) << 20 ) /
													( ccb->ccb_Height * ccb->ccb_Width );

						ccb->ccb_HDDY = ( ( ( gP[2].pt_Y - gP[3].pt_Y ) - ( gP[1].pt_Y - gP[0].pt_Y ) ) << 20 ) /
													( ccb->ccb_Height * ccb->ccb_Width );
						break;
					default:
						break;
				}
			}
		}

		else {

			if ( button & ControlUp ) {
				ccb->ccb_YPos -= gMoveIncr;
				gP[0].pt_Y -= MOVE_DELTA;
				gP[1].pt_Y -= MOVE_DELTA;
				gP[2].pt_Y -= MOVE_DELTA;
				gP[3].pt_Y -= MOVE_DELTA;
			}

			if ( button & ControlDown ) {
				ccb->ccb_YPos += gMoveIncr;
				gP[0].pt_Y += MOVE_DELTA;
				gP[1].pt_Y += MOVE_DELTA;
				gP[2].pt_Y += MOVE_DELTA;
				gP[3].pt_Y += MOVE_DELTA;
			}

			if ( button & ControlRight ) {
				ccb->ccb_XPos += gMoveIncr;
				gP[0].pt_X += MOVE_DELTA;
				gP[1].pt_X += MOVE_DELTA;
				gP[2].pt_X += MOVE_DELTA;
				gP[3].pt_X += MOVE_DELTA;
			}

			if ( button & ControlLeft ) {
				ccb->ccb_XPos -= gMoveIncr;
				gP[0].pt_X -= MOVE_DELTA;
				gP[1].pt_X -= MOVE_DELTA;
				gP[2].pt_X -= MOVE_DELTA;
				gP[3].pt_X -= MOVE_DELTA;
			}

		}
	}

		/*
			reset the positioning values of the matte cel's CCB so that the two
			cels will be drawn in identical positions.
		*/
	alphaCCB->ccb_XPos = ccb->ccb_XPos;
	alphaCCB->ccb_YPos = ccb->ccb_YPos;
	alphaCCB->ccb_HDX = ccb->ccb_HDX;
	alphaCCB->ccb_HDY = ccb->ccb_HDY;
	alphaCCB->ccb_VDX = ccb->ccb_VDX;
	alphaCCB->ccb_VDY = ccb->ccb_VDY;
	alphaCCB->ccb_HDDX = ccb->ccb_HDDX;
	alphaCCB->ccb_HDDY = ccb->ccb_HDDY;

	return true;

}

/*********************
**	GetTime - finds the current system time
**
**	  Synopsis
**
**	    static int32 GetTime ( IOReq *pIOReq, struct timeval *tv )
**
**	  Description
**
**	    This function queries the system microsecond timer for the current time
**
**	  Arguments
**
**	    pIOReq	             pointer to the IO request to use in querying the timer
**
**	    tv		             pointer to a timeval structure to contain the current time
**
**	  Return Value
**
**	    The function returns 0 if successful or an error code (a negative value)
**	    if an error occurs.
**
*********************/
static int32 GetTime ( IOReq *pIOReq, struct timeval *tv ) {

	IOInfo					io;

		/*
			initialize the io info structure fields to 0
		*/
	memset ( &io, 0, sizeof ( IOInfo ) );

		/*
			set the fields appropriately.  We want to read from the microsecond timer.
			The read operation should fill in the memory pointed to by tv.

			We send off the io request synchronously, waiting for its completion
		*/
	io.ioi_Command = CMD_READ;
	io.ioi_Unit = TIMER_UNIT_USEC;

	io.ioi_Recv.iob_Buffer = tv;
	io.ioi_Recv.iob_Len = sizeof (struct timeval);

	return DoIO ( gTimerReq, &io );

}


/*********************
**	AdvanceFrame - determines if its time to advance an animation
**
**	  Synopsis
**
**	   static int32 AdvanceFrame ( IOReq *pIOReq, struct timeval *lastTime )
**
**	  Description
**
**	    This function queries the system microsecond timer for the current time, and
**	    compares it to the time of the previous frame.  If the current time exceeds that
**	    time plus the frame's duration, then the function returns true.
**
**	  Arguments
**
**	    pIOReq	             pointer to the IO request to use in querying the timer
**
**	    lastTime             pointer to a timeval structure that contains the time at which
**	    					 the current frame was first displayed
**
**
**	  Return Value
**
**	    The function returns an error code if an error occurs, false if it is not yet time
**	    to display the next frame, or true if it is time to display the next frame
**
*********************/
static int32 AdvanceFrame ( IOReq *pIOReq, struct timeval *lastTime ) {

	struct timeval			curTime;
	int32					status = 0;
	int32					frameDur;

			/*
				Because we are using the microsecond timer, get everything into microseconds.

				If we desire a particular frame rate, then each frame's duration is 1/frame rate,
				multiplied by 1,000,000 to convert from seconds to microseconds.
			*/
	frameDur = 1000000/gFrameRate;

			/*
				get the current time and compare it to the last time.
			*/
	status = GetTime ( pIOReq, &curTime );
	if (status >= 0 ) {

		if ( ( (curTime.tv_sec - lastTime->tv_sec) * 1000000 ) +
					(curTime.tv_usec - lastTime->tv_usec) >= frameDur )
			goto Return;
	}

	return status;

Return:
	status = true;
	lastTime->tv_sec = curTime.tv_sec;
	lastTime->tv_usec = curTime.tv_usec;

	return status;

}
