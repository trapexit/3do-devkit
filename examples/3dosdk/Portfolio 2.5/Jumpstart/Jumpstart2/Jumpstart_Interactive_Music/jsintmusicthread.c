
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: jsintmusicthread.c,v 1.18 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/jsintmusicthread
|||	jsintmusicthread - Demonstrates starting background music as a thread (a
**/

#include "types.h"
#include "graphics.h"
#include "task.h"
#include "audio.h"
#include "stdio.h"
#include "debug3do.h"
#include "parse3do.h"
#include "displayutils.h"

#include "event.h"
#include "controlpad.h"

#include "effectshandler.h"
#include "loopstereosoundfile.h"

/* Include additional headers used by the thread */
#include "music.h"

#define     CURRENT_TASK_PRIORITY   KernelBase->kb_CurrentTask->t.n_Priority

#define	UFO_MINDISTANCE	256
#define UFO_MAXDISTANCE	8000
#define UFO_HOVER_DISTANCE 500

#define	DISPLAY_WIDTH   320
#define	DISPLAY_HEIGHT  240
#define VISIBLE_INSET	20

#define	LEFT_EDGE	10
#define RIGHT_EDGE	310
#define TOP_EDGE	10
#define BOTTOM_EDGE	230

#define LASER_LEFT		0
#define LASER_RIGHT		319
#define LASER_BOTTOM	210
/* (the laser top isn't constant -- it's at the calculated focal point) */

#define LASER_COUNT 15

/*
	Scale a length for distance.  This is used to
	determine an object's virtual size.  The function
	result is inversely proportional to the object's
	distance, returning the input value as its maximum value
	at the closest possible distance ( UFO_MINDISTANCE,
	i.e., 256, or 1 << 8 )
*/
#define PROJECTED( _length )	( ((_length) << 8) / gDistance )

/* Convenience macro for frac20 conversion */
#define Convert32_F20( x )	( (x) << 20 )

ScreenContext	*gScreenContext;
Item			gDisplayItem = -1;					/* result of CreateBasicDisplay */

int32 gLaserCount = 0;
bool gHoverMode = FALSE;
int32 gBoomCount = 0;
int32 gDistance = UFO_MAXDISTANCE;
int32 gVelocity = 50;

int32 gScore = 0;
int32 gMisses = 0;

ubyte* gBackgroundImage = NULL;
CCB* gUFOCel = NULL;
CCB* gCockpitCel = NULL;
CCB* gCrosshairsCel = NULL;
CCB* gLaserCel = NULL;
CCB* gBlipCel = NULL;
CCB* gOptionsCel = NULL;
CCB* gPleaseWaitCel = NULL;
ANIM* gExplosionAnim = NULL;
CCB* gExplosionCel = NULL;

pTMixerInfo gMixerInfo = NULL;
pTSampleInfo gWhirrEffect = NULL;
pTSampleInfo gExplosionEffect = NULL;
pTSampleInfo gZapEffect = NULL;
Item gBackgroundMusic = -1;

Item gVBLIOReq;     /* I/O request for VBL timing calls */

/***** This section contains the thread which plays the background music *****/

void JSPlayBgndMusic(void)
/*
	This function is identical to the main function in jsplaybgndmusic.c, with
	the full soundfile pathname specified and the return statement omitted.
*/
{
    int32 Result;

    if ( (Result = OpenAudioFolio()) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't open the audio folio\n") );
        goto CLEANUP;
        }

    /*
		Play the stereo sample in an endless loop (the "JSData" directory is expected
		to be present in the executable's directory)
	*/
	LoopStereoSoundFile( "jsdata/sound/backgroundmusic.aiff" );

CLEANUP:
    CloseAudioFolio();
}

/***** ( End of thread section ) *****/


uint32 randbits(uint32 shift)
	/*
		Return an unsigned random value
		with a maximum of sigBits significant bits.
	*/
    {
    shift = 31 - shift;
    return (rand() >> shift);
    }


int32 UpdateDashboard( void )
	/*
		Draw the graphics objects associated with the cockpit.
		These are:

			- The crosshairs
			- The cockpit
			- The radar blip
			- The laser, if firing
	*/
    {
    static int32 sine[16] =
		{
		0,
		2753,
		5502,
		8241,
		10965,
		13670,
		16351,
		19003,
		21621,
		24201,
		26739,
		29229,
		31668,
		34051,
		36373,
		38632
		};
    static int32 cosine[16] =
		{
		65536,
		65478,
		65304,
		65015,
		64612,
		64094,
		63463,
		62720,
		61866,
		60903,
		59832,
		58656,
		57376,
		55995,
		54515,
		52938
		};

    int32 i, dist;
    int32 x, y;

	/* Calculate the blip location */
	i = (PROJECTED( gUFOCel->ccb_Width ) >> 1) + ConvertF16_32(gUFOCel->ccb_XPos);
	i = ((i - 160) << 1) / 21;
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
	gBlipCel->ccb_XPos = Convert32_F16(246) + x + gCockpitCel->ccb_XPos;
	gBlipCel->ccb_YPos = Convert32_F16(205) - y + gCockpitCel->ccb_YPos;

    if (gLaserCount == LASER_COUNT)
		DrawCels(gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen], gLaserCel);

    LinkCel( gCrosshairsCel, gCockpitCel );
	LinkCel( gCockpitCel, gBlipCel );
	LAST_CEL( gBlipCel );
	DrawCels(gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen], gCrosshairsCel);

    if (gLaserCount)
		--gLaserCount;

    return 0;
    }

bool Zap(CCB* aTarget, Coord X, Coord Y)
	/*
		Play the zap sound effect, show the laser firing at the
		focal point, and detect if that point is within the projected
		target bounds.
	*/
    {

		/* Make the laser beams converge at the focal point */
		{
		static Point quad[4] =
			{
			{0, 0},
			{0, 0},
			{LASER_RIGHT, LASER_BOTTOM},
			{LASER_LEFT, LASER_BOTTOM}
			};
	
		quad[0].pt_X = quad[1].pt_X = X;
		quad[0].pt_Y = quad[1].pt_Y = Y;
	
		MapCel(gLaserCel, quad);
		}

		/* Play the zap sound effect */
		{
		static TagArg taStartArgs[] =
			{
			{AF_TAG_FREQUENCY, (void*)0x8000},
			{AF_TAG_AMPLITUDE, (void*)0x4000}, /* play at mid-range */
			{0, 0}
			};
	
		StartInstrument(gZapEffect->si_Player, taStartArgs);
		}
	
    /*
		Detect if (X, Y) is inside
		the target, first converting from absolute
		to target-relative coordinates.
	*/
	X -= ConvertF16_32(aTarget->ccb_XPos);
    Y -= ConvertF16_32(aTarget->ccb_YPos);
    return ( (X >= 0) && (X < PROJECTED(aTarget->ccb_Width))
			&&  (Y >= 0) &&  (Y < PROJECTED(aTarget->ccb_Height)) );
    }


#define     kContinuousMask     ( ControlA | ControlB | ControlC | ControlRight | ControlLeft | ControlDown | ControlUp )

int32 DoOptions(void)
/*
	Allow user to choose game options from a special screen
	via the control pad.  The UFO is specially positioned, scaled
	and displayed in the foreground.
	
	Control pad input is handled as follows:

		- Start button means resume playing the game.
		- C button means quit the game.


	Returns -1 if the user chose Quit by pressing button C, otherwise 0.

	In a real title, you would have more options, perhaps on multiple
	screens.  This function is really just a minimal shell for options-
	handling logic.
*/
{
    int32 retValue = 0;
	int32 xUFO, yUFO, hdxUFO, vdyUFO;
    uint32  controlBits;

    xUFO = gUFOCel->ccb_XPos;
    yUFO = gUFOCel->ccb_YPos;
    hdxUFO = gUFOCel->ccb_HDX;
    vdyUFO = gUFOCel->ccb_VDY;

    gUFOCel->ccb_XPos = Convert32_F16(176);
    gUFOCel->ccb_YPos = Convert32_F16(67);
    gUFOCel->ccb_HDX = Convert32_F20(1) >> 1;	/*	Half-size */
    gUFOCel->ccb_VDY = Convert32_F16(1) >> 1;	/*	Half-size */

	LinkCel( gCockpitCel, gOptionsCel);
	LinkCel( gOptionsCel, gUFOCel );
	LAST_CEL( gUFOCel );

    DrawImage(gScreenContext->sc_Screens[gScreenContext->sc_curScreen], gBackgroundImage, gScreenContext);
    DrawCels(gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen], gCockpitCel);
    DisplayScreen(gScreenContext->sc_Screens[gScreenContext->sc_curScreen], 0);

    /*	Stop the whirring while the options screen is displayed */
	StopInstrument(gWhirrEffect->si_Player, NULL);

    while (TRUE)
		{
		DoControlPad(1, &controlBits, 0);
	
		if ( controlBits & ControlStart )
			{
			gUFOCel->ccb_XPos = xUFO;
			gUFOCel->ccb_YPos = yUFO;
			gUFOCel->ccb_HDX = hdxUFO;
			gUFOCel->ccb_VDY = vdyUFO;
	
   			{
			TagArg taStartArgs[] =
				{
				{AF_TAG_FREQUENCY, (void*)0x8000},
				{AF_TAG_AMPLITUDE, (void*)0x3000},
				{0, 0}
				};

			StartInstrument(gWhirrEffect->si_Player, taStartArgs);
			}
	
			goto DONE;
			}
		else
			{
			if ( controlBits & ControlA )
				{
				}
	
			if ( controlBits & ControlB )
				{
				}
		
			if ( controlBits & ControlC )
				{
					retValue = -1;
					goto DONE;
				}
			}
	
		WaitVBL( gVBLIOReq, 1 );
	
		}

DONE:
	return retValue;
}

int32 HandleControlPad(void)
	/*
		Respond to control pad input:
	
			- Start button means get game options.
			- A button means fire the laser at the UFO.
			- B button means toggle hover mode (the UFO will linger in
				at the end of its approach.
			- Arrows move the crosshairs in the corresponding direction.
	
	
		Returns -1 if the user chose Quit from the game options,
		otherwise 0.
	*/
    {
	int32 retValue = 0;
    int32 controlMove = Convert32_F16(3);
    uint32 controlBits;

    DoControlPad( 1, &controlBits, (ControlA | ControlRight | ControlLeft | ControlDown | ControlUp) );

    if ( controlBits & ControlStart )
		{
		retValue = DoOptions();
		goto DONE;
		}
    else
		{
		if ( controlBits & ControlA && !gLaserCount)
			{
			gLaserCount = LASER_COUNT;
			if (Zap(gUFOCel, ConvertF16_32(gCrosshairsCel->ccb_XPos)+(gCrosshairsCel->ccb_Width >> 1), ConvertF16_32(gCrosshairsCel->ccb_YPos)+(gCrosshairsCel->ccb_Height >> 1)))
				/*
					Make the explosion sound and animation emanate from the point of impact
				*/
				{
				int32 Balance;
				int32 Volume;
				static TagArg taStartArgs[] =
					{
					{AF_TAG_FREQUENCY, (void*)0x4000},
					{AF_TAG_AMPLITUDE, (void*)MAXDSPAMPLITUDE},
					{0, 0}
					};
				
				Balance = ((gCrosshairsCel->ccb_XPos + (gCrosshairsCel->ccb_Width >> 1)) >> 2) / 160;
				Volume = Convert32_F16((UFO_MAXDISTANCE - gDistance) >> 1) / 3000;
				ehSetChannelLevels(gMixerInfo, gExplosionEffect->si_LeftGainKnob, gExplosionEffect->si_RightGainKnob,
									Volume, Balance);
				StartInstrument(gExplosionEffect->si_Player, taStartArgs);
		
				gBoomCount = gExplosionAnim->num_Frames;
				gExplosionCel->ccb_XPos = gCrosshairsCel->ccb_XPos + Convert32_F16(gCrosshairsCel->ccb_Width >> 1) - ( (gExplosionCel->ccb_Width >> 1) * gUFOCel->ccb_VDY );
				gExplosionCel->ccb_YPos = gCrosshairsCel->ccb_YPos + Convert32_F16(gCrosshairsCel->ccb_Height >> 1) - ( (gExplosionCel->ccb_Height >> 1) * gUFOCel->ccb_VDY );
				gExplosionCel->ccb_HDX = gUFOCel->ccb_HDX;
				gExplosionCel->ccb_VDY = gUFOCel->ccb_VDY;
		
				gScore += gDistance;
				gDistance = UFO_MAXDISTANCE;
				if (gVelocity > 0)
					gVelocity *= -1;
		
				gUFOCel->ccb_XPos = Convert32_F16(20 + randbits(8));
				gUFOCel->ccb_YPos = Convert32_F16(20 + randbits(9));
				}
			}
	
		if ( controlBits & ControlB )
			/*	Toggle hover mode */
			gHoverMode = !gHoverMode;
	
		if ( controlBits & ControlC )
			/*	No functionality implemented */
			{
			}
	
		if ( (controlBits & ControlRight) && (gCrosshairsCel->ccb_XPos < Convert32_F16(DISPLAY_WIDTH-gCrosshairsCel->ccb_Width)) )
			gCrosshairsCel->ccb_XPos += controlMove;
	
		if ( (controlBits & ControlLeft) && (gCrosshairsCel->ccb_XPos > 0))
			gCrosshairsCel->ccb_XPos -= controlMove;
	
		if ( (controlBits & ControlDown) && (gCrosshairsCel->ccb_YPos < Convert32_F16(DISPLAY_HEIGHT-gCrosshairsCel->ccb_Height)) )
			gCrosshairsCel->ccb_YPos += controlMove;
	
		if ( (controlBits & ControlUp) && (gCrosshairsCel->ccb_YPos > 0) )
			gCrosshairsCel->ccb_YPos -= controlMove;
		}

DONE:
	return retValue;
}

void TargetAction(CCB* aTargetCCB)
	/*
		Animate the UFO: If it's still exploding, play the explosion's next frame,
		otherwise display the UFO (after a nominal post-explosion delay).
	*/
    {
    static int32 iDeltaX = Convert32_F16(1);
    static int32 iDeltaY = Convert32_F16(3) >> 1;

    int32 iTest;

    if (gDistance > UFO_MAXDISTANCE)
		{
		gDistance = UFO_MAXDISTANCE;
		gVelocity = -gVelocity;
		gMisses++;
	
		aTargetCCB->ccb_XPos = Convert32_F16(VISIBLE_INSET + randbits(8));
		aTargetCCB->ccb_YPos = Convert32_F16(VISIBLE_INSET + randbits(9));
		}
    else if (gDistance < UFO_MINDISTANCE)
		{
		gDistance = UFO_MINDISTANCE;
		gVelocity = -gVelocity;
		}

    if ( gHoverMode && (gDistance < UFO_HOVER_DISTANCE) )
		{
		gDistance += (gVelocity >> 5);
		aTargetCCB->ccb_YPos += iDeltaY >> 1;
		aTargetCCB->ccb_XPos += iDeltaX >> 1;
		}
    else
		{
		gDistance += gVelocity;
		iTest = PROJECTED( UFO_MINDISTANCE );
		aTargetCCB->ccb_YPos += (iDeltaY >> 7) * iTest;
		aTargetCCB->ccb_XPos += (iDeltaX >> 7) * iTest;
	
		if (!randbits(8))
			iDeltaX *= -1;
	
		if (!randbits(8))
			iDeltaY *= -1;
		}

    aTargetCCB->ccb_HDX = PROJECTED(Convert32_F20(1));
    aTargetCCB->ccb_VDY = PROJECTED(Convert32_F16(1));

    iTest = ConvertF16_32(aTargetCCB->ccb_YPos) + PROJECTED( aTargetCCB->ccb_Height );
    if (aTargetCCB->ccb_YPos <= Convert32_F16(VISIBLE_INSET))
		{
		aTargetCCB->ccb_YPos = Convert32_F16(VISIBLE_INSET);
		iDeltaY *= -1;
		}
    else if (iTest >= 149)
		{
		aTargetCCB->ccb_YPos -= Convert32_F16(iTest - 149);
		iDeltaY *= -1;
		}

    iTest = ConvertF16_32(aTargetCCB->ccb_XPos) + PROJECTED( aTargetCCB->ccb_Width );
    if (aTargetCCB->ccb_XPos <= Convert32_F16(VISIBLE_INSET))
		{
		aTargetCCB->ccb_XPos = Convert32_F16(VISIBLE_INSET);
		iDeltaX *= -1;
		}
    else if (iTest >= 299)
		{
		aTargetCCB->ccb_XPos -= Convert32_F16(iTest - 299);
		iDeltaX *= -1;
		}

	{
	int32 Balance;
	int32 Volume;

	iTest -= ConvertF16_32(aTargetCCB->ccb_XPos);
	Balance = ( (aTargetCCB->ccb_XPos + (Convert32_F16(iTest) >> 1)) >> 2 ) / 160;
	Volume = Convert32_F16( (UFO_MAXDISTANCE - gDistance) >> 1 ) / UFO_MAXDISTANCE;
	ehSetChannelLevels(gMixerInfo, gWhirrEffect->si_LeftGainKnob, gWhirrEffect->si_RightGainKnob, Volume, Balance);
	}

    DrawCels(gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen], gUFOCel);

    if (gBoomCount)
	{
	gBoomCount--;
	if (gBoomCount)
	    {
	    GetAnimCel(gExplosionAnim, Convert32_F16(1));
	    DrawCels(gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen], gExplosionCel);
	    }
	else
	    {
	    gExplosionAnim->cur_Frame = 0;
	    }
	}

    return;
    }


void UnifyAnimation(ANIM* pAnim)
/*
	Ensure that all frames of the animation use the same CCB,
	to facilitate making uniform changes to the position
	and scale.
*/
{
    int frameIndex;
    CCB* theCCB;

    theCCB = pAnim->pentries[0].af_CCB;
    for (frameIndex = 0; frameIndex < pAnim->num_Frames; frameIndex++)
    {
    	pAnim->pentries[frameIndex].af_CCB = theCCB;
    }
}



int32 Initialize(void)
	/*
		Allocate and prepare all of the program's global resources.
		These are:
	
			- A VBL I/O Request used to wait for the vertical blank.
			- A single ScreenContext for handling 2 screens.
			- The graphics folio.
			- The audio folio.
			- The control pad utility.
			- A background image.
			- Numerous cels for each of the game's graphic objects.
			- A MixerInfo for the sound effects.
			- Numerous sound effects to play on demand.
			- The thread which plays the background music.
	
		Returns 0 if all operations are performed successfully, otherwise a
		negative value.
	*/
    {
 	int32 retValue = -1;

    /* Create the I/O request we'll use for VBL waiting */
	gVBLIOReq = CreateVBLIOReq();
	if (gVBLIOReq < 0)
	{
		PRT(("Can't get a VBL I/O Request\n"));
		goto DONE;
	}

	/*	Allocate memory for the screenContext */
	gScreenContext = (ScreenContext *) AllocMem( sizeof(ScreenContext), MEMTYPE_ANY );
	if (gScreenContext == NULL)
		{
		PRT(("Can't allocate memory for ScreenContext\n"));
		goto DONE;
		}
 
	/* Open the graphics folio and set up the screen context */
	if ( (gDisplayItem = CreateBasicDisplay(gScreenContext, DI_TYPE_DEFAULT, 2)) < 0 )
    	{
        PRT( ("Can't initialize display\n") );
		goto DONE;
    	}
    gScreenContext->sc_curScreen = 0;

    if ( OpenAudioFolio() )
		{
		PRT(("Can't open the audio folio\n"));
		goto DONE;
		}

	if ( InitControlPad( 2 ) <  0 )
		{
		PRT( ("Can't initialize the control pad\n") );
		goto DONE;
		}

    if ((gBackgroundImage = (ubyte *) LoadImage("jsdata/art/sky.imag", NULL, NULL, gScreenContext)) == NULL)
		{
		PRT(("Can't load the sky image\n"));
		goto DONE;
		}

    if ((gUFOCel = LoadCel("jsdata/art/jsufo.cel", MEMTYPE_CEL)) == NULL)
		{
		PRT(("Can't load the UFO cel\n"));
		goto DONE;
		}

    if ((gCockpitCel = LoadCel("jsdata/art/cockpit.cel", MEMTYPE_CEL)) == NULL)
		{
		PRT(("Can't load the cockpit cel\n"));
		goto DONE;
		}

    if ((gOptionsCel = LoadCel("jsdata/art/options.cel", MEMTYPE_CEL)) == NULL)
		{
		PRT(("Can't load the options cel\n"));
		goto DONE;
		}

    if ((gPleaseWaitCel = LoadCel("jsdata/art/pleasewait.cel", MEMTYPE_CEL)) == NULL)
		{
		PRT(("Can't load the PleaseWait cel\n"));
		goto DONE;
		}

    if ((gBackgroundMusic = CreateThread("JSPlayBgndMusic", CURRENT_TASK_PRIORITY + 1, JSPlayBgndMusic, 4000)) < 0)
        {
        PRT(("Can't spawn thread for SoundHandler\n"));
		goto DONE;
        }

    LinkCel( gUFOCel, gCockpitCel );
    LinkCel( gCockpitCel, gOptionsCel );
    LinkCel( gOptionsCel, gPleaseWaitCel );
    LAST_CEL( gPleaseWaitCel );

    gUFOCel->ccb_XPos = Convert32_F16(176);
    gUFOCel->ccb_YPos = Convert32_F16(67);
    gUFOCel->ccb_HDX = 1<<19;
    gUFOCel->ccb_VDY = 1<<15;

    DrawImage(gScreenContext->sc_Screens[gScreenContext->sc_curScreen], gBackgroundImage, gScreenContext);
    DrawCels(gScreenContext->sc_BitmapItems[gScreenContext->sc_curScreen], gCockpitCel);
    DisplayScreen(gScreenContext->sc_Screens[gScreenContext->sc_curScreen], 0);

    /* We're done with the PleaseWait cel, so dispose it */
	UnloadCel(gPleaseWaitCel);
	gPleaseWaitCel = NULL;

    if ((gCrosshairsCel = LoadCel("jsdata/art/crosshairs.cel", MEMTYPE_CEL)) == NULL)
		{
		PRT(("Can't load the crosshairs cel\n"));
		goto DONE;
		}

    if ((gLaserCel = LoadCel("jsdata/art/laser.cel", MEMTYPE_CEL)) == NULL)
		{
		PRT(("Can't load the laser cel\n"));
		goto DONE;
		}

    if ((gBlipCel = LoadCel("jsdata/art/blip.cel", MEMTYPE_CEL)) == NULL)
		{
		PRT(("Can't load the radar blip cel\n"));
		goto DONE;
		}

    if ( (gExplosionAnim = LoadAnim("jsdata/art/boom.anim", MEMTYPE_CEL) ) == 0)
		{
		PRT(("Can't load the explosion animation\n"));
		goto DONE;
		}
	/*
		Make all of the animation's frames reference the same CCB to
		facilitate uniform positioning and scaling.
	*/
	UnifyAnimation(gExplosionAnim);
    gExplosionCel = GetAnimCel(gExplosionAnim, 0);

	/*
		Create a mixer info for the sound effects.
	*/
	if ( ehNewMixerInfo(&gMixerInfo, 4, "mixer4x2.dsp") < 0 )
		goto DONE;
		
    /*
		Load the sound effects
	*/
	if ( ehLoadSoundEffect(&gWhirrEffect, gMixerInfo, "jsdata/sound/whirr.aiff", 0) < 0 )
		{
		PRT(("Can't load the whirr sound effect\n"));
		goto DONE;
		}

	if ( ehLoadSoundEffect(&gExplosionEffect, gMixerInfo, "jsdata/sound/explosion.aiff", 1) < 0 )
		{
		PRT(("Can't load the explosion sound effect\n"));
		goto DONE;
		}

	if ( ehLoadSoundEffect(&gZapEffect, gMixerInfo, "jsdata/sound/zap.aiff", 2) < 0 )
		{
		PRT(("Can't load the zap sound effect\n"));
		goto DONE;
		}

    LAST_CEL( gBlipCel );
    LAST_CEL( gCockpitCel );
    LAST_CEL( gCrosshairsCel );
    LAST_CEL( gExplosionCel );
    LAST_CEL( gLaserCel );
    LAST_CEL( gOptionsCel );
    LAST_CEL( gUFOCel );

    gCrosshairsCel->ccb_XPos = Convert32_F16((DISPLAY_WIDTH >> 1) - gCrosshairsCel->ccb_Width);
    gCrosshairsCel->ccb_YPos = Convert32_F16((DISPLAY_HEIGHT >> 1) - gCrosshairsCel->ccb_Height);

    retValue = DoOptions();

DONE:
	return retValue;
}


void Cleanup( void )
/*
	Dispose all global resources used by the program.  This mirrors
	the Initialize function:

		- Disposes the cels.
		- Disposes the animation.
		- Disposes the background image.
		- Disposes the ScreenContext.
		- Disposes the I/O request used for VBL waiting.
		- Kills the control pad utility.
		- Disposes the sound effects.
		- Disposes the mixerInfo.
		- Closes the audio folio.
		- Kills the background music-playing thread.
*/
{
    UnloadAnim(gExplosionAnim);

    UnloadCel(gUFOCel);
    UnloadCel(gCockpitCel);
    UnloadCel(gCrosshairsCel);
    UnloadCel(gLaserCel);
    UnloadCel(gBlipCel);
    UnloadCel(gOptionsCel);
    UnloadCel(gPleaseWaitCel);

    UnloadImage(gBackgroundImage);

	if ( gDisplayItem >= 0 )
		DeleteBasicDisplay( gScreenContext );
	
	DeleteVBLIOReq( gVBLIOReq );
    KillControlPad();

	ehDisposeSampleInfo( gWhirrEffect );		
	ehDisposeSampleInfo( gExplosionEffect );		
	ehDisposeSampleInfo( gZapEffect );		
	ehDisposeMixerInfo( gMixerInfo );
	CloseAudioFolio();

	DeleteThread(gBackgroundMusic);
}

int main(int argc, char** argv)
{
    printf( "jsintmusicthread\n" );
	
	if ( Initialize() < 0 )
		goto DONE;
		
	while ( HandleControlPad() >= 0 )
	{
	    gScreenContext->sc_curScreen = 1 - gScreenContext->sc_curScreen;
	    DrawImage(gScreenContext->sc_Screens[gScreenContext->sc_curScreen], gBackgroundImage, gScreenContext);
	    TargetAction(gUFOCel);
	    if ( UpdateDashboard() < 0 )
			break;
	    DisplayScreen(gScreenContext->sc_Screens[gScreenContext->sc_curScreen], 0);
	}

DONE:
	if ( gScreenContext )
		FadeToBlack(gScreenContext, 60);

	/*
		This is a placeholder for code that would handle the
		end-of-game screen.
	*/
	printf("You got %ld points and ", gScore);
	if ( gMisses )
		printf("let only %ld", gMisses);
	else
		printf("didn't let any");
	printf(" get away!\n");
    printf("Thank you for playing jsintmusicthread.\n");

    Cleanup();

	return 0;
}
