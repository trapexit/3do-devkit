
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bounce.c,v 1.18 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/bounce
|||	bounce - Interactive component animation with sound.
|||
|||	  Synopsis
|||
|||	    bounce
|||
|||	  Description
|||
|||	    This is a small demo program where each of the elements of the 3DO logo
|||	    bounce around a 3D room. The logo elements distort upon contact with the
|||	    walls and floor. Shadows of the logo elements follow along with the
|||	    objects. The room can be lit at two different levels.
|||
|||	    The code which supports the generation of sound is bracketed with: #if
|||	    ENABLE_SOUND... #endif.
|||
|||	  Associated Files
|||
|||	    bounce.c, bounce.make, bounce.h, bounce_sound.c, bounce_sound.hThe data
|||	    files are:BounceFolder/Art/wallo.img     wall, light on image
|||	    BounceFolder/Art/wallf.img     wall, light off image
|||	    BounceFolder/Art/globe.anim    globe animationBounceFolder/Art/globes.cel
|||	      globe shadow celBounceFolder/Art/tv.anim tv animation
|||	    BounceFolder/Art/tvs.animtv shadow animationBounceFolder/Art/cube.celcube
|||	    celBounceFolder/Art/cubes.cel     cube shadow celBounceFolder/Art/ball.cel
|||	    ball celBounceFolder/Art/balls.cel     ball shadow cel
|||	    BounceFolder/Sound/bird.aiff  ball-tv collision sound
|||	    BounceFolder/Sound/3do.aiff    ball-globe collision sound
|||	    BounceFolder/Sound/interactive.aiff tv-cube collision sound
|||	    BounceFolder/Sound/multiplayer.aiff  cube-globe collision sound
|||	    BounceFolder/Sound/ballbnce.aiff     ball bounce sound
|||	    BounceFolder/Sound/tvbnce.aiff tv bounce sound
|||	    BounceFolder/Sound/cubebnce.aiff    cube bounce sound
|||	    BounceFolder/Sound/globebnce.aiff    globe bounce sound
|||
|||	    If PAL video is being displayed, the folder PalBounceFolder is used
|||	    instead of BounceFolder.
|||
|||	  Location
|||
|||	    examples/3DObounce
|||
**/
#define ENABLE_SOUND	1

#include "types.h"
#include "graphics.h"
#include "mem.h"
#include "audio.h"
#include "debug3do.h"
#include "displayutils.h"
#include "parse3do.h"

#include "event.h"
#include "controlpad.h"

#include "bounce.h"
#include "getvideoinfo.h"

#if ENABLE_SOUND
#include "bounce_sound.h"
#endif

#define MIN_XPOS		0
#define MIN_YPOS		0
#define MAX_XPOS		270
#define MAX_YPOS		200

#define MIN_LEFT		20
#define MIN_TOP			0
#define MAX_RIGHT		300
#define	MAX_BOTTOM		215

typedef enum TQuadCornerTag
{
	kQuadTL,
	kQuadTR,
	kQuadBR,
	kQuadBL
} TQuadIndex;

#define REFLECTION_TOP		190
#define REFLECTION_BOTTOM	200

#define BALL_INIT_XPOS		150
#define BALL_INIT_YPOS		100
#define BALL_XSPEED			2
#define BALL_BOUNCE_XMIN	0
#define BALL_BOUNCE_XMAX	MAX_XPOS
#define BALL_BOUNCE_YMIN	-50
#define BALL_BOUNCE_YMAX	190
#define BALLSHADOW_XMIN		40
#define BALLSHADOW_XMAX		280
#define BALLSHADOW_YMIN		-40
#define BALLSHADOW_YMAX		180

#define CUBE_INIT_XPOS		150
#define CUBE_INIT_YPOS		15
#define CUBE_XSPEED			1
#define CUBE_BOUNCE_XMIN	10
#define CUBE_BOUNCE_XMAX	265
#define CUBE_BOUNCE_YMIN	0
#define CUBE_BOUNCE_YMAX	190
#define CUBESHADOW_XMIN		40
#define CUBESHADOW_XMAX		280
#define CUBESHADOW_YMIN		40
#define CUBESHADOW_YMAX		180

#define GLOBE_INIT_XPOS		50
#define GLOBE_INIT_YPOS		50
#define GLOBE_XSPEED		3
#define GLOBE_BOUNCE_XMIN	0
#define GLOBE_BOUNCE_XMAX	MAX_XPOS
#define GLOBE_BOUNCE_YMIN	0
#define GLOBE_BOUNCE_YMAX	190
#define GLOBESHADOW_XMIN	40
#define GLOBESHADOW_XMAX	280
#define GLOBESHADOW_YMIN	40
#define GLOBESHADOW_YMAX	180
#define GLOBE_NOBOUNCE_XMIN	50
#define GLOBE_NOBOUNCE_XMAX	220

#define TV_INIT_XPOS		145
#define TV_INIT_YPOS		60
#define TV_XSPEED			3
#define TV_BOUNCE_XMIN		0
#define TV_BOUNCE_XMAX		MAX_XPOS
#define TV_BOUNCE_YMIN		40
#define TV_BOUNCE_YMAX		190
#define TVSHADOW_XMIN		40
#define TVSHADOW_XMAX		280
#define TVSHADOW_YMIN		40
#define TVSHADOW_YMAX		190

#define THIS_INSET		5
#define OTHER_INSET		15

/* General BETWEEN macro */
#define BETWEEN( _left, _middle, _right ) \
	( (_left <= _middle) && (_middle <= _right) )
	
/* Collision subtest for objects moving vertically */
#define HORZ_OVERLAP( _thisCcb, _thisXPos, _otherCcb, _otherXPos ) \
( (_thisXPos+THIS_INSET <= _otherXPos+(_otherCcb->ccb_Width-OTHER_INSET)) && \
	(_thisXPos+(_thisCcb->ccb_Width-THIS_INSET) >= _otherXPos+OTHER_INSET) )

/* Collision subtest for objects moving horizontally */
#define VERT_OVERLAP( _thisCcb, _thisYPos, _otherCcb, _otherYPos ) \
( (_thisYPos+THIS_INSET <= _otherYPos+(_otherCcb->ccb_Height-OTHER_INSET)) && \
	(_thisYPos+(_thisCcb->ccb_Height-THIS_INSET) >= _otherYPos+OTHER_INSET) )

/* condition which must be met for a collision while this object is moving upward */
#define UP_COLLISION( _thisCcb, _thisXPos, _thisYPos, _otherCcb, _otherXPos, _otherYPos ) \
( HORZ_OVERLAP(_thisCcb, _thisXPos, _otherCcb, _otherXPos) && \
	BETWEEN(_otherYPos+(_otherCcb->ccb_Height-OTHER_INSET), _thisYPos+THIS_INSET, _otherYPos+(_otherCcb->ccb_Height-THIS_INSET)) ) 

/* condition which must be met for a collision while this object is moving downward */
#define DOWN_COLLISION( _thisCcb, _thisXPos, _thisYPos, _otherCcb, _otherXPos, _otherYPos ) \
( HORZ_OVERLAP(_thisCcb, _thisXPos, _otherCcb, _otherXPos) && \
	BETWEEN(_otherYPos+THIS_INSET, _thisYPos+(_thisCcb->ccb_Height-THIS_INSET), _otherYPos+OTHER_INSET) )

/* condition which must be met for a collision while this object is moving to the left */
#define LEFT_COLLISION( _thisCcb, _thisXPos, _thisYPos, _otherCcb, _otherXPos, _otherYPos ) \
( BETWEEN(_otherXPos+(_otherCcb->ccb_Width-OTHER_INSET), _thisXPos+THIS_INSET, _otherXPos+(_otherCcb->ccb_Width-THIS_INSET)) && \
	VERT_OVERLAP(_thisCcb, _thisYPos, _otherCcb, _otherYPos) )

/* condition which must be met for a collision while this object is moving to the right */
#define RIGHT_COLLISION( _thisCcb, _thisXPos, _thisYPos, _otherCcb, _otherXPos, _otherYPos ) \
( BETWEEN(_otherXPos+THIS_INSET, _thisXPos+(_thisCcb->ccb_Width-THIS_INSET), _otherXPos+OTHER_INSET) && \
	VERT_OVERLAP(_thisCcb, _thisYPos, _otherCcb, _otherYPos) )
	
/* Function Prototypes */

int32 InitGraphicObjects( void );
int32 HandleControlPad( void );


/* Global variables */

/*
	Switches
*/
bool    gBouncing   = FALSE;	/* Are the graphic objects moving? */
bool    gLightOn    = TRUE;		/* Is the light shining on the wall? */
bool    gCollision  = FALSE;	/* Are collisions enabled? */

/*
	Graphic object positions
*/
int32    gGlobeXPos  = GLOBE_INIT_XPOS;
int32    gGlobeYPos  = GLOBE_INIT_YPOS;

int32    gCubeXPos   = CUBE_INIT_XPOS;
int32    gCubeYPos   = CUBE_INIT_YPOS;

int32    gTvXPos     = TV_INIT_XPOS;
int32    gTvYPos     = TV_INIT_YPOS;

int32    gBallXPos   = BALL_INIT_XPOS;
int32    gBallYPos   = BALL_INIT_YPOS;

/*
	Each object is loaded as an animation
*/
ANIM    *gBallAnim;
ANIM    *gBallSAnim;
ANIM    *gBallRAnim;
ANIM    *gGlobeAnim;
ANIM    *gGlobeSAnim;
ANIM    *gGlobeRAnim;
ANIM    *gTvAnim;
ANIM    *gTvSAnim;
ANIM    *gTvRAnim;
ANIM    *gCubeAnim;
ANIM    *gCubeSAnim;
ANIM    *gCubeRAnim;

frac16  gBallFrameIncr;
frac16  gGlobeFrameIncr;
frac16  gTvFrameIncr;
frac16  gCubeFrameIncr;

/* In each frame, a cel is obtained from its animation */
CCB*    gBallCcb = NULL;
CCB*    gBallSCcb = NULL;
CCB*    gBallRCcb = NULL;

CCB*    gGlobeCcb = NULL;
CCB*    gGlobeSCcb = NULL;
CCB*    gGlobeRCcb = NULL;

CCB*    gTvCcb = NULL;
CCB*    gTvSCcb = NULL;
CCB*    gTvRCcb = NULL;

CCB*    gCubeCcb = NULL;
CCB*    gCubeSCcb = NULL;
CCB*    gCubeRCcb = NULL;

/* Background images for light off and on, respectively */
ubyte   *gWallOffPict = NULL;
ubyte   *gWallOnPict = NULL;

/* We need this to display graphics */
static	ScreenContext *gScreenContext = NULL;

/* Type of video being displayed */
int32	gDisplayType;

/* Basic display created */
Item	gDisplayItem = -1;

/* An IOReq for SPORT operations */
Item    gVRAMIOReq;


/*
	Utility functions
*/

void LinkBounce()
	/* Link the cels for the bouncing display */
	{
	if ( gLightOn )
		{
		LinkCel(gBallSCcb, gBallRCcb);
		LinkCel(gBallRCcb, gBallCcb);
		LinkCel(gBallCcb, gTvSCcb);
		
		LinkCel(gTvSCcb, gTvRCcb);
		LinkCel(gTvRCcb, gTvCcb);
		LinkCel(gTvCcb, gCubeSCcb);
	
		LinkCel(gCubeSCcb, gCubeRCcb);
		LinkCel(gCubeRCcb, gCubeCcb);
		LinkCel(gCubeCcb, gGlobeSCcb);
		
		LinkCel(gGlobeSCcb, gGlobeRCcb);
		LinkCel(gGlobeRCcb, gGlobeCcb);
		LAST_CEL(gGlobeCcb);
		}
	else
		{
		LinkCel( gBallCcb, gBallSCcb );
		LinkCel( gBallSCcb, gTvCcb );

		LinkCel( gTvCcb, gTvSCcb );
		LinkCel( gTvSCcb, gCubeCcb );

		LinkCel( gCubeCcb, gCubeSCcb );
		LinkCel( gCubeSCcb, gGlobeCcb );

		LinkCel( gGlobeCcb, gGlobeSCcb );
		LAST_CEL(gGlobeSCcb);
		}
	}

void LinkNoBounce()
	/* Link the cels for the non-bouncing display */
	{
	if( gLightOn )
		{
		LinkCel(gBallSCcb, gBallRCcb);
		LinkCel(gBallRCcb, gTvSCcb);
		
		LinkCel(gTvSCcb, gTvRCcb);
		LinkCel(gTvRCcb, gCubeSCcb);
		
		LinkCel(gCubeSCcb, gCubeRCcb);
		LinkCel(gCubeRCcb, gBallCcb);
		
		LinkCel(gBallCcb, gTvCcb);
		LinkCel(gTvCcb, gCubeCcb);
		LAST_CEL(gCubeCcb);

		LinkCel(gGlobeSCcb, gGlobeRCcb);
		LinkCel(gGlobeRCcb, gGlobeCcb);
		LAST_CEL(gGlobeCcb);
		}
	else
		{
		LinkCel(gBallCcb, gBallSCcb);
		LinkCel(gBallSCcb, gTvCcb);
		
		LinkCel(gTvCcb, gTvSCcb);
		LinkCel(gTvSCcb, gCubeCcb);
		
		LinkCel(gCubeCcb, gCubeSCcb);
		LAST_CEL(gCubeSCcb);
		
		LinkCel(gGlobeCcb, gGlobeSCcb);
		LAST_CEL(gGlobeSCcb);
		}
	}

void DrawGlobe( Item currentBitmapItem )
	/* Draw the globe according to the light switch */ 
	{
	if( gLightOn )
		{
		DrawCels(currentBitmapItem, gGlobeSCcb);
		}
	else
		{
		DrawCels(currentBitmapItem, gGlobeCcb);
		}
	}

int32 CalcShadowSpeed( int32 xPos )
	/*
		Calculate the speed of any shadow as a function
		of an object's x-position.
	*/
	{
	int32 speed;
	
	if( xPos > MAX_RIGHT )				speed = 12;
	else if	( xPos > 280 )				speed = 10;
	else if	( xPos > 260 )				speed = 8;
	else if	( xPos > 240 )				speed = 6;
	else if	( xPos > 220 )				speed = 4;
	else if	( xPos > 200 )				speed = 3;
	else if	( xPos > 180 )				speed = 2;
	else if	( xPos > 160 )				speed = 1;
	else if	( xPos > 140 )				speed = -1;
	else if	( xPos > 120 )				speed = -2;
	else if	( xPos > 100 )				speed = -3;
	else if	( xPos >  80 )				speed = -4;
	else if	( xPos >  60 )				speed = -6;
	else if	( xPos >  40 )				speed = -8;
	else if	( xPos >  MIN_LEFT )		speed = -10;
	else /* if ( xPos <= MIN_LEFT ) */	speed = -12;
	
	return speed;
	}

void SetQuadFromPosAndSize(Point *aQuad, int32 xPos, int32 yPos, int32 width, int32 height)
	{
	aQuad[kQuadBL].pt_X = aQuad[kQuadTL].pt_X = xPos;
	aQuad[kQuadTR].pt_Y = aQuad[kQuadTL].pt_Y = yPos;
	aQuad[kQuadBR].pt_X = aQuad[kQuadTR].pt_X = xPos + width;
	aQuad[kQuadBL].pt_Y = aQuad[kQuadBR].pt_Y = yPos + height;
	}

void SetShadowQuad( Point *aQuad, int32 xPos, int32 ySpeed )
	/*
		Set the quad for an object's shadow.
		On entry, the quad is expected to be set
		to the object's display bounds.
	*/
	{
	int32 shadowSpeed = CalcShadowSpeed(xPos);
	int32 yIncrement = ySpeed + ySpeed + 8;
	
	aQuad[kQuadTL].pt_X += shadowSpeed;
	aQuad[kQuadTL].pt_Y += yIncrement;
	aQuad[kQuadTR].pt_X += shadowSpeed;
	aQuad[kQuadTR].pt_Y += yIncrement;
	aQuad[kQuadBR].pt_X += shadowSpeed;
	aQuad[kQuadBR].pt_Y += yIncrement;
	aQuad[kQuadBL].pt_X += shadowSpeed;
	aQuad[kQuadBL].pt_Y += yIncrement;
	}

void SetReflectionQuad( Point *aQuad, int32 speed )
	/*
		Set the quad for an object's reflection.
		On entry, the quad is expected to be set to the
		object's shadow bounds.
	*/
	{
	aQuad[kQuadTL].pt_X -= speed;
	aQuad[kQuadTR].pt_Y  = aQuad[kQuadTL].pt_Y  = REFLECTION_TOP + speed;
	aQuad[kQuadTR].pt_X += speed;
	aQuad[kQuadBR].pt_X += speed;
	aQuad[kQuadBL].pt_Y  = aQuad[kQuadBR].pt_Y  = REFLECTION_BOTTOM + speed;
	aQuad[kQuadBL].pt_X -= speed;
	}

void ConstrainQuad( Point *aQuad, int32 minX, int32 maxX, int32 minY, int32 maxY )
	/*
		Limit the quad's members to the specified min and max values.
	*/
	{
	if( aQuad[kQuadTL].pt_X <= minX )    aQuad[kQuadTL].pt_X = minX;
	if( aQuad[kQuadTR].pt_X <= minX )    aQuad[kQuadTR].pt_X = minX;
	if( aQuad[kQuadBR].pt_Y <= minY )    aQuad[kQuadBR].pt_Y = minY;
	if( aQuad[kQuadBL].pt_Y <= minY )    aQuad[kQuadBL].pt_Y = minY;
	if( aQuad[kQuadTL].pt_X >= maxX )    aQuad[kQuadTL].pt_X = maxX;
	if( aQuad[kQuadTR].pt_X >= maxX )    aQuad[kQuadTR].pt_X = maxX;
	if( aQuad[kQuadBR].pt_Y >= maxY )    aQuad[kQuadBR].pt_Y = maxY;
	if( aQuad[kQuadBL].pt_Y >= maxY )    aQuad[kQuadBL].pt_Y = maxY;
	}

void SpecialConstrainQuad(Point *aQuad, int32 maxX)
	/*
		Limit the quad's horizontal members to the standard minimum and
		specified maximum; limit the vertical to the standard maximum.
	*/
	{
	if( aQuad[kQuadTL].pt_X <= MIN_LEFT )
		aQuad[kQuadBL].pt_X  = aQuad[kQuadTL].pt_X  = MIN_LEFT;

	if( aQuad[kQuadTR].pt_X >= maxX )
		aQuad[kQuadBR].pt_X  = aQuad[kQuadTR].pt_X  = maxX;
	
	if( aQuad[kQuadBR].pt_Y >= MAX_BOTTOM )
		aQuad[kQuadBL].pt_Y  = aQuad[kQuadBR].pt_Y  = MAX_BOTTOM;
	}

/* Main functions */


int32 Initialize( void )
	/*
		Allocate global resources:

		- A single screen context for handling 2 screens.
		- Opens the graphics display for the video type in use.
		- Initialize all graphic objects.
		- Initialize the control pad utility.
		- Open the audio folio.
		- Initialize the mixer and sounds.

		Returns 0 if all operations are successful, otherwise -1.
	*/
    {
 	int32 retValue = -1;
	int32 errorCode;

	gVRAMIOReq = CreateVRAMIOReq();		/* Obtain an IOReq for all SPORT operations */
	if ( gVRAMIOReq < 0 )
		{
		PRT( ("Can't create VRAM IOReq\n") );
		goto DONE;
		}

	gScreenContext = (ScreenContext *) AllocMem( sizeof(ScreenContext), MEMTYPE_ANY );
    if (gScreenContext == NULL)
        {
        PRT( ("Can't allocate memory for ScreenContext\n") );
        goto DONE;
        }
	
	/* Open the graphics folio so we can get the displayType */
	errorCode = OpenGraphicsFolio();
	if (errorCode < 0)
		{
		DIAGNOSE_SYSERR( errorCode, ("Can't open the graphics folio\n") );
		goto DONE;
		}
				
	gDisplayType = GetDisplayType();
	if ( gDisplayType < 0 )
		{
		goto DONE;
		}
	
	if ((gDisplayType == DI_TYPE_PAL1) || (gDisplayType == DI_TYPE_PAL2))
		{
		gDisplayType = DI_TYPE_PAL2;
		}
	else
		{
		gDisplayType = DI_TYPE_NTSC;
		}

	if ( (gDisplayItem = CreateBasicDisplay(gScreenContext, gDisplayType, 2)) < 0 )
        {
        PRT( ("Can't initialize display\n") );
		goto DONE;
        }

    if ( InitGraphicObjects() )
		goto DONE;

	LinkNoBounce();

	if ( InitControlPad( 2 ) <  0 )
		{
		PRT( ("Can't initialize the control pad\n") );
		goto DONE;
		}

#if ENABLE_SOUND
    if ( OpenAudioFolio() )
        {
        PRT( ("Can't initialize audio\n") );
		goto DONE;
        }

    if ( InitBounceSound() < 0 )
		{
        PRT( ("InitBounceSound failed\n") );
        goto DONE;
    	}
#endif

	retValue = 0;
	
DONE:
	return retValue;
    }


void Cleanup( void )
	/*
		Dispose all global resources used by the program.  This mirrors
		the Initialize function:
	
		- Dispose the mixer and sound effects.
		- Close the audio folio.
		- Kill the control pad utility.
		- Dispose all graphic objects.
		- Deletes the graphics display.
		- Close the graphics folio.
		- Delete the VRAM IOReq used for SPORT calls.
		
		We use FreeMem rather than UnloadImage because we pre-allocated the
		memory and passed the pointer to LoadImage.
	*/
    {
#if ENABLE_SOUND
	KillBounceSound();

	CloseAudioFolio();
#endif

	KillControlPad();

	{
	int32 imagesize = gScreenContext->sc_nFrameByteCount;
	
	FreeMem( gWallOffPict, imagesize );
	FreeMem( gWallOnPict, imagesize );
	}

	UnloadAnim(gGlobeAnim);
	UnloadAnim(gGlobeSAnim);
	UnloadAnim(gGlobeRAnim);

	UnloadAnim(gTvAnim);
	UnloadAnim(gTvSAnim);
	UnloadAnim(gTvRAnim);

	UnloadAnim(gCubeAnim);
	UnloadAnim(gCubeSAnim);
	UnloadAnim(gCubeRAnim);

	UnloadAnim(gBallAnim);
	UnloadAnim(gBallSAnim);
	UnloadAnim(gBallRAnim);		

	if ( gDisplayItem >= 0 )
		DeleteBasicDisplay( gScreenContext );
		
	/* Close the graphics folio because we opened it
		to get the display type */
	CloseGraphicsFolio();

	if ( gScreenContext )
		FreeMem( gScreenContext, sizeof(ScreenContext) );
		
	DeleteVRAMIOReq( gVRAMIOReq );
    }


CCB *UnifyAnimation(ANIM* pAnim)
	/*
		Ensure that all frames of the animation use the same CCB,
		to facilitate making uniform changes to the position
		and scale.
	*/
    {
    int32 frameIndex;
    CCB* theCcb;
    
    theCcb = pAnim->pentries[0].af_CCB;
    for (frameIndex = 0; frameIndex < pAnim->num_Frames; frameIndex++)
		{
		pAnim->pentries[frameIndex].af_CCB = theCcb;
		}
		
	return theCcb;
    }
    
int main(int argc, char** argv)
{
	int32   screenSelect = 0;
	Item	currentBitmapItem;
	
	/*
		variables which specify the direction in which the corresponding object is traveling
	*/
	bool	ballUpDir       = FALSE;
    bool	ballLftDir      = TRUE;
    bool	globeUpDir      = FALSE;
    bool	globeLftDir     = FALSE;
    bool	tvUpDir         = FALSE;
    bool	tvLftDir        = TRUE;
    bool	cubeUpDir       = FALSE;
    bool	cubeLftDir      = FALSE;

    int32	xPos;
	int32	yPos;
	int32	ySpeed;

    Point   aQuad[4];

    gTvFrameIncr     = 0;
    gBallFrameIncr   = 0;
    gCubeFrameIncr   = 0;
    gGlobeFrameIncr  = Convert32_F16(1);
    gGlobeFrameIncr -= 0x00009000;	/* tweak the frame increment to obtain a satisfactory animation rate  */

#if ENABLE_SOUND
	printf( "\nbounce (with sound)\n\n" );
    printf( "Sounds are triggered when objects bounce on the floor and\n" );
    printf( "  when they collide in the air.  The pitch of an object\n" );
    printf( "  collision sound is determined by the height of the\n" );
    printf( "  collision, and panning is based on the horizontal\n" );
    printf( "  screen position.\n" );
#else
	printf( "\nbounce\n\n" );
#endif
    printf( "   A Key  - to start bouncing.\n" );
    printf( "          - to toggle light while bouncing.\n" );
    printf( "   B Key  - to reset, and turn collision on.\n" );
    printf( "   C Key  - to freeze, and turn collision off.\n" );
    printf( "   Arrows - to rearrange objects while\n" );
    printf( "            bouncing or frozen.\n" );
    printf( "\nLoading.........\n" );

	if ( Initialize() < 0 )
		goto DONE;

    PRT( ("Press A to start bouncing. Collision is off.\n") );
	
	/*
		Main animation loop.
	*/
	while (TRUE)
		{
		/* Check for user input */
		if ( HandleControlPad() < 0 )
			goto DONE;

        /*
			Handle the ball motion and collision
		*/

		xPos = gBallXPos;
		yPos = gBallYPos;
		
		/* Calculate the ball's vertical speed */
		if		( yPos > 180 ) 			ySpeed = 18;
        else if ( yPos > 160 )			ySpeed = 16;
        else if ( yPos > 140 )			ySpeed = 14;
        else if ( yPos > 120 )			ySpeed = 12;
        else if ( yPos > 100 )			ySpeed = 10;
        else if ( yPos >  80 )			ySpeed = 8;
        else if ( yPos >  60 )			ySpeed = 7;
        else if ( yPos >  40 )			ySpeed = 6;
        else if	( yPos >  20 )			ySpeed = 5;
        else if	( yPos >  10 )			ySpeed = 4;
        else if	( yPos >  0 )			ySpeed = 3;
        else if	( yPos >  -40 )			ySpeed = 2;
		else /* if ( yPos <= -40 ) */	ySpeed = 1;
		
        if( gBouncing )
			{
            if ( ballUpDir )
				{
                if ( (yPos -= ySpeed) <= BALL_BOUNCE_YMIN)
                    ballUpDir = FALSE;
                if ( gCollision )
					{
                    if ( UP_COLLISION( gBallCcb, xPos, yPos, gTvCcb, gTvXPos, gTvYPos ) )
						{
						ballUpDir = FALSE;
						tvUpDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_TV_COLL | BELOW);
#endif
						}
					}
                }
			else
				{
				if ( (yPos += ySpeed) >= BALL_BOUNCE_YMAX )
					{
					ballUpDir = TRUE;
#if ENABLE_SOUND
					DoRoomCollisionSound(BALL | FLOOR);
#endif
					}
				if ( gCollision )
					{
					if ( DOWN_COLLISION( gBallCcb, xPos, yPos, gTvCcb, gTvXPos, gTvYPos ) )
						{
						ballUpDir = TRUE;
						tvUpDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_TV_COLL | ABOVE);
#endif
						}
					}
				}

            if( ballLftDir )
				{
                if ( (xPos -= BALL_XSPEED) <= BALL_BOUNCE_XMIN )
                    ballLftDir = FALSE;
                if ( gCollision )
					{
                    if ( LEFT_COLLISION( gBallCcb, xPos, yPos, gTvCcb, gTvXPos, gTvYPos ) )
						{
						ballLftDir = FALSE;
						tvLftDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_TV_COLL | LEFT);
#endif
						}
					}
                }
			else
				{
				if ( (xPos += BALL_XSPEED) >= BALL_BOUNCE_XMAX )
					ballLftDir = TRUE;
				if ( gCollision )
					{
					if ( RIGHT_COLLISION( gBallCcb, xPos, yPos, gTvCcb, gTvXPos, gTvYPos ) )
						{
						ballLftDir = TRUE;
						tvLftDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_TV_COLL | RIGHT);
#endif
						}
					}
				}

            }


        /* Prepare the next frame of the object animation */
		gBallCcb = GetAnimCel(gBallAnim, gBallFrameIncr);
		SetQuadFromPosAndSize(aQuad, xPos, yPos, gBallCcb->ccb_Width, gBallCcb->ccb_Height);
        SpecialConstrainQuad(aQuad, MAX_RIGHT);
        MapCel(gBallCcb, aQuad);

        /* Prepare the next frame of the shadow animation */
        gBallSCcb = GetAnimCel(gBallSAnim, gBallFrameIncr);
        if ( gLightOn )
			{
            SetShadowQuad( aQuad, xPos, ySpeed );
			ConstrainQuad( aQuad, BALLSHADOW_XMIN, BALLSHADOW_XMAX, BALLSHADOW_YMIN, BALLSHADOW_YMAX );
            }
		MapCel(gBallSCcb, aQuad);
 
        /* Prepare the next frame of the reflection animation */
        gBallRCcb = GetAnimCel(gBallRAnim, gBallFrameIncr);
		SetReflectionQuad( aQuad, ySpeed );
        MapCel(gBallRCcb, aQuad);

		gBallYPos = yPos;
		gBallXPos = xPos;

        /*
			Handle the globe motion and collision
		*/

		/* Calculate the globe's vertical speed */
		if		( gGlobeYPos > 180 ) 		ySpeed = 13;
        else if ( gGlobeYPos > 160 )		ySpeed = 11;
        else if ( gGlobeYPos > 140 )		ySpeed = 9;
        else if ( gGlobeYPos > 120 )		ySpeed = 7;
        else if ( gGlobeYPos > 100 )		ySpeed = 6;
        else if ( gGlobeYPos >  80 )		ySpeed = 5;
        else if ( gGlobeYPos >  60 )		ySpeed = 4;
        else if ( gGlobeYPos >  35 )		ySpeed = 3;
        else if	( gGlobeYPos >  10 )		ySpeed = 2;
		else /* if ( gGlobeYPos <= 10 ) */	ySpeed = 1;
		
        if ( gBouncing )
			{
            if ( globeUpDir )
				{
                if ( (gGlobeYPos -= ySpeed) <= GLOBE_BOUNCE_YMIN )
                    globeUpDir = FALSE;
                if ( gCollision )
					{
					if ( UP_COLLISION(gGlobeCcb, gGlobeXPos, gGlobeYPos, gBallCcb, gBallXPos, gBallYPos) )
						{
						globeUpDir = FALSE;
						ballUpDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_GLOBE_COLL | ABOVE);
#endif
						}
					}
                }
			else
				{
				if ( (gGlobeYPos += ySpeed) >= GLOBE_BOUNCE_YMAX )
					{
					globeUpDir = TRUE;
#if ENABLE_SOUND
					DoRoomCollisionSound(GLOBE | FLOOR);
#endif
					}
				if ( gCollision )
					{
					if ( DOWN_COLLISION(gGlobeCcb, gGlobeXPos, gGlobeYPos, gBallCcb, gBallXPos, gBallYPos) )
						{
						globeUpDir = TRUE;
						ballUpDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_GLOBE_COLL | BELOW);
#endif
						}
					}
				}

            if ( globeLftDir )
				{
                if ( (gGlobeXPos -= GLOBE_XSPEED) <= GLOBE_BOUNCE_XMIN )
                    globeLftDir = FALSE;
                if ( gCollision )
					{
					if ( LEFT_COLLISION(gGlobeCcb, gGlobeXPos, gGlobeYPos, gBallCcb, gBallXPos, gBallYPos) )
						{
						globeLftDir = FALSE;
						ballLftDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_GLOBE_COLL | LEFT);
#endif
						}
					}
                }
			else
				{
				if ( (gGlobeXPos += GLOBE_XSPEED) >= GLOBE_BOUNCE_XMAX )
					globeLftDir = TRUE;
                if ( gCollision )
					{
					if ( RIGHT_COLLISION( gGlobeCcb, gGlobeXPos, gGlobeYPos, gBallCcb, gBallXPos, gBallYPos) )
						{
						globeLftDir = TRUE;
						ballLftDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(BALL_GLOBE_COLL | RIGHT);
#endif
						}
					}
				}


            }
		else
			/* When the other objects aren't bouncing, the globe still is, but within special limits */
			{
			if ( (globeLftDir) && ( (gGlobeXPos -= GLOBE_XSPEED) <= GLOBE_NOBOUNCE_XMIN ) )
				globeLftDir = FALSE;
			if ( (!globeLftDir) && ( (gGlobeXPos += GLOBE_XSPEED) >= GLOBE_NOBOUNCE_XMAX ) )
				globeLftDir = TRUE;
			}


        /* Prepare the next frame of the object animation */
        gGlobeCcb = GetAnimCel(gGlobeAnim,gGlobeFrameIncr);
		SetQuadFromPosAndSize(aQuad, gGlobeXPos, gGlobeYPos, gGlobeCcb->ccb_Width, gGlobeCcb->ccb_Height);
        SpecialConstrainQuad(aQuad, MAX_RIGHT);
        MapCel(gGlobeCcb, aQuad);

        /* Prepare the next frame of the shadow animation */
        gGlobeSCcb = GetAnimCel(gGlobeSAnim,gGlobeFrameIncr);
        if ( gLightOn )
			{
            SetShadowQuad( aQuad, gGlobeXPos, ySpeed );
			ConstrainQuad( aQuad, GLOBESHADOW_XMIN, GLOBESHADOW_XMAX, GLOBESHADOW_YMIN, GLOBESHADOW_YMAX );
            }
		MapCel(gGlobeSCcb, aQuad);
 
        /* Prepare the next frame of the reflection animation */
        gGlobeRCcb = GetAnimCel(gGlobeRAnim,gGlobeFrameIncr);
		SetReflectionQuad( aQuad, ySpeed );
        MapCel(gGlobeRCcb, aQuad);

        /*
			Handle the tv motion and collision
		*/

		/* Calculate the TV's vertical speed */
		if		( gTvYPos > 180 ) 		ySpeed = 13;
        else if ( gTvYPos > 165 )		ySpeed = 11;
        else if ( gTvYPos > 150 )		ySpeed = 9;
        else if ( gTvYPos > 135 )		ySpeed = 7;
        else if ( gTvYPos > 120 )		ySpeed = 6;
        else if ( gTvYPos > 105 )		ySpeed = 5;
        else if ( gTvYPos >  90 )		ySpeed = 4;
        else if ( gTvYPos >  75 )		ySpeed = 3;
        else if	( gTvYPos >  60 )		ySpeed = 2;
		else /* if ( gTvYPos <= 60 ) */	ySpeed = 1;
		
        if ( gBouncing ) 
			{
            if ( tvUpDir )
				{
                if ( (gTvYPos -= ySpeed) <= TV_BOUNCE_YMIN )
                    tvUpDir = FALSE;
                if ( gCollision )
					{
					if ( UP_COLLISION( gTvCcb, gTvXPos, gTvYPos, gCubeCcb, gCubeXPos, gCubeYPos ) )
						{
						tvUpDir = FALSE;
						cubeUpDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(TV_CUBE_COLL | BELOW);
#endif
						}
                    }
                }
			else
				{
				if ( (gTvYPos += ySpeed) >= TV_BOUNCE_YMAX )
					{
					tvUpDir = TRUE;
#if ENABLE_SOUND
					DoRoomCollisionSound(TV | FLOOR);
#endif
					}
				if ( gCollision )
					{
					if ( DOWN_COLLISION( gTvCcb, gTvXPos, gTvYPos, gCubeCcb, gCubeXPos, gCubeYPos ) )
						{
						tvUpDir = TRUE;
						cubeUpDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(TV_CUBE_COLL | ABOVE);
#endif
						}
					}
				}

            if ( tvLftDir )
				{
                if ( (gTvXPos -= TV_XSPEED) <= TV_BOUNCE_XMIN )
                    tvLftDir = FALSE;
                if ( gCollision )
					{
					if ( LEFT_COLLISION( gTvCcb, gTvXPos, gTvYPos, gCubeCcb, gCubeXPos, gCubeYPos ) )
						{
						tvLftDir = FALSE;
						cubeLftDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(TV_CUBE_COLL | RIGHT);
#endif
						}
				   }
                }
			else
				{
				if ( (gTvXPos += TV_XSPEED) >= TV_BOUNCE_XMAX )
					tvLftDir = TRUE;
				if ( gCollision )
					{
					if ( RIGHT_COLLISION( gTvCcb, gTvXPos, gTvYPos, gCubeCcb, gCubeXPos, gCubeYPos ) )
						{
						tvLftDir = TRUE;
						cubeLftDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(TV_CUBE_COLL | LEFT);
#endif
						}
					}
				}
            }

        /* Prepare the next frame of the object animation */
        gTvCcb = GetAnimCel(gTvAnim,gTvFrameIncr);
		SetQuadFromPosAndSize(aQuad, gTvXPos, gTvYPos, gTvCcb->ccb_Width, gTvCcb->ccb_Height);
        SpecialConstrainQuad(aQuad, MAX_RIGHT);
        MapCel(gTvCcb, aQuad);

        /* Prepare the next frame of the shadow animation */
        gTvSCcb = GetAnimCel(gTvSAnim,gTvFrameIncr);
        if ( gLightOn )
			{
            SetShadowQuad( aQuad, gTvXPos, ySpeed );
			ConstrainQuad( aQuad, TVSHADOW_XMIN, TVSHADOW_XMAX, TVSHADOW_YMIN, TVSHADOW_YMAX );
            }
		MapCel(gTvSCcb, aQuad);

        /* Prepare the next frame of the reflection animation */
        gTvRCcb = GetAnimCel(gTvRAnim, gTvFrameIncr);
		SetReflectionQuad( aQuad, ySpeed );
        MapCel(gTvRCcb, aQuad);

        /*
			Handle the cube motion and collision
		*/
		/* Calculate the cube's vertical speed */
		if		( gCubeYPos > 180 ) 	ySpeed = 18;
        else if ( gCubeYPos > 160 )		ySpeed = 16;
        else if ( gCubeYPos > 140 )		ySpeed = 14;
        else if ( gCubeYPos > 120 )		ySpeed = 12;
        else if ( gCubeYPos > 100 )		ySpeed = 10;
        else if ( gCubeYPos >  80 )		ySpeed = 8;
        else if ( gCubeYPos >  60 )		ySpeed = 6;
        else if ( gCubeYPos >  35 )		ySpeed = 4;
        else if	( gCubeYPos >  10 )		ySpeed = 2;
		else /* ( gCubeYPos <= 10 ) */	ySpeed = 1;
 

        if ( gBouncing )
			{
			if ( cubeUpDir )
				{
                if ( (gCubeYPos -= ySpeed) <= CUBE_BOUNCE_YMIN )
                    cubeUpDir = FALSE;
                if ( gCollision )
					{
					if ( UP_COLLISION( gCubeCcb, gCubeXPos, gCubeYPos, gGlobeCcb, gGlobeXPos, gGlobeYPos ) )
						{
						cubeUpDir = FALSE;
						globeUpDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(CUBE_GLOBE_COLL | BELOW);
#endif
						}
					}
                }
			else
				{
				if ( (gCubeYPos += ySpeed) >= CUBE_BOUNCE_YMAX )
					{
					cubeUpDir = TRUE;
#if ENABLE_SOUND
					DoRoomCollisionSound(CUBE | FLOOR);
#endif
					}
				if ( gCollision )
					{
					if ( DOWN_COLLISION( gCubeCcb, gCubeXPos, gCubeYPos, gGlobeCcb, gGlobeXPos, gGlobeYPos ) )
						{
						cubeUpDir = TRUE;
						globeUpDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(CUBE_GLOBE_COLL | ABOVE);
#endif
						}
					}
				}

            if ( cubeLftDir )
				{
                if ( (gCubeXPos -= CUBE_XSPEED) <= CUBE_BOUNCE_XMIN )
                    cubeLftDir = FALSE;
                if ( gCollision )
					{
					if ( LEFT_COLLISION( gCubeCcb, gCubeXPos, gCubeYPos, gGlobeCcb, gGlobeXPos, gGlobeYPos ) )
						{
						cubeLftDir = FALSE;
						globeLftDir = TRUE;
#if ENABLE_SOUND
						DoObjectCollisionSound(CUBE_GLOBE_COLL | RIGHT);
#endif
						}
					}
                }
			else
				{
				if ( (gCubeXPos += CUBE_XSPEED) >= CUBE_BOUNCE_XMAX )
					cubeLftDir = TRUE;
				if ( gCollision )
					{
					if ( RIGHT_COLLISION( gCubeCcb, gCubeXPos, gCubeYPos, gGlobeCcb, gGlobeXPos, gGlobeYPos ) )
						{
						cubeLftDir = TRUE;
						globeLftDir = FALSE;
#if ENABLE_SOUND
						DoObjectCollisionSound(CUBE_GLOBE_COLL | LEFT);
#endif
						}
					}
				}
			}

        /* Prepare the next frame of the object animation */
        gCubeCcb = GetAnimCel(gCubeAnim,gCubeFrameIncr);
		SetQuadFromPosAndSize(aQuad, gCubeXPos, gCubeYPos, gCubeCcb->ccb_Width, gCubeCcb->ccb_Height);
        SpecialConstrainQuad(aQuad, 310);
        MapCel(gCubeCcb, aQuad);

        /* Prepare the next frame of the shadow animation */
        gCubeSCcb = GetAnimCel(gCubeSAnim,gCubeFrameIncr);
        if ( gLightOn )
			{
			SetShadowQuad( aQuad, gCubeXPos, ySpeed );
			ConstrainQuad( aQuad, CUBESHADOW_XMIN, CUBESHADOW_XMAX, CUBESHADOW_YMIN, CUBESHADOW_YMAX );
            }
        MapCel(gCubeSCcb, aQuad);

        /* Prepare the next frame of the reflection animation */
        gCubeRCcb = GetAnimCel(gCubeRAnim,gCubeFrameIncr);
		SetReflectionQuad( aQuad, ySpeed );
        MapCel(gCubeRCcb, aQuad);

        /*
			Draw the appropriate background image
		*/
		{
		ubyte *whichPict = gLightOn ? gWallOnPict : gWallOffPict;
		CopyVRAMPages( gVRAMIOReq, gScreenContext->sc_Bitmaps[screenSelect]->bm_Buffer, whichPict, gScreenContext->sc_nFrameBufferPages, -1 );
		}
		
        /* Draw the cels in the correct order */
		currentBitmapItem = gScreenContext->sc_BitmapItems[ screenSelect ];
		
		if ( gBouncing )
			{
            if ( gLightOn )
				{				
				DrawCels(currentBitmapItem, gBallSCcb);
                }
			else /* No light */
				{
				DrawCels(currentBitmapItem, gBallCcb);
               }

            }
		else /* Not bouncing */
			{

			/*
				If globe is traveling to the right, it's behind
				everything else.
			*/
			if ( !globeLftDir )
				DrawGlobe( currentBitmapItem );
				
			if ( gLightOn )
				{
				DrawCels(currentBitmapItem, gBallSCcb);
				}
			else
				{
				DrawCels(currentBitmapItem, gBallCcb);
				}
				
			/*
				If globe is traveling to the left, it's in front
				of everything else.
			*/
			if ( globeLftDir )
				DrawGlobe( currentBitmapItem );
			}

		DisplayScreen( gScreenContext->sc_Screens[ screenSelect ], 0 );
		screenSelect = 1 - screenSelect;

        }

DONE:
    if ( gScreenContext )
		FadeToBlack( gScreenContext, 60 );
		
	Cleanup();
	
#if ENABLE_SOUND
    printf( "end of bounce (with sound)\n" );
#else
    printf( "end of bounce\n" );
#endif
	
    return 0;
}


/*  Miscellaneous Utility Routines */

int32 HandleControlPad( void )
	/*
		Respond to the user's control pad input.
	
		- Start button means quit the program
		- A button means start bouncing if not already bouncing, otherwise toggle the room light.
		- B button means enable collisions and if bouncing, stop bouncing.
		- C button means disable collisions and if bouncing, stop bouncing.
		- Arrows mean move objects apart in appropriate direction
	
		Returns -1 if user pressed the Start button to quit, otherwise 0.
	*/
	{
	uint32	button;
	int32	status;
	int32	retValue = 0;

 	status = DoControlPad(1, &button, CONTROL_CONTINUOUS );
    if ( button & ControlStart )
		{
		retValue = -1;
        goto DONE;
		}

    /* On ControlA ... */
    if ( button & ControlA )
		{
        if ( gBouncing )
			/*
				Toggle the light if in bouncing mode.
			*/
			{
			gLightOn = !gLightOn;
			LinkBounce();
			}
		else
			/*
				Enter bouncing mode if not already bouncing.
			*/
			{
            gBouncing = TRUE;
			LinkBounce();
            gTvFrameIncr  = Convert32_F16(1);
            gTvFrameIncr -= 0x00006000;	/* tweak the frame increment to obtain a satisfactory animation rate */
            }
		goto DONE;
    	}

    /* On ControlB ... */
    if ( button & ControlB )
		/*
			Enable collisions and stop bouncing
		*/
		{
		gCollision = TRUE;
		PRT( ("Collision on - A to start bouncing, Arrows to move objects.\n") );
		if ( gBouncing )
			{
			gBouncing = FALSE;
			LinkNoBounce();
			gGlobeXPos = GLOBE_INIT_XPOS;
			gGlobeYPos = GLOBE_INIT_YPOS;
			gCubeXPos = CUBE_INIT_XPOS;
			gCubeYPos = CUBE_INIT_YPOS;
			gTvXPos = TV_INIT_XPOS;
			gTvYPos = TV_INIT_YPOS;
			gBallXPos = BALL_INIT_XPOS;
			gBallYPos = BALL_INIT_YPOS;
			}
		goto DONE;
		}

    /* On ControlC ... */
    if ( button & ControlC )
        {
		/*
			Disable collisions.
		*/
		gCollision = FALSE;
		PRT( ("Collision off - A to start bouncing, Arrows to move objects.\n") );
		if ( gBouncing )
			{
			/*
				If bouncing, stop bouncing.
			*/
			gBouncing = FALSE;
			LinkNoBounce();
			gTvFrameIncr = 0x00000000;
			}
		goto DONE;
        }


    if ( button & CONTROL_ARROWS )
		/*
			Move the objects in the appropriate direction
		*/
        {
		if ( button & ControlUp )
            {
            if (--gGlobeYPos < MIN_YPOS) gGlobeYPos = MIN_YPOS;
            if (++gTvYPos > MAX_YPOS) gTvYPos = MAX_YPOS;
            if (--gCubeYPos < MIN_YPOS) gCubeYPos = MIN_YPOS;
            }
        else if ( button & ControlDown )
            {
            if (++gGlobeYPos > MAX_YPOS) gGlobeYPos = MAX_YPOS;
            if (--gTvYPos < MIN_YPOS) gTvYPos = MIN_YPOS;
            if (++gCubeYPos > MAX_YPOS) gCubeYPos = MAX_YPOS;
            }

        if ( button & ControlLeft )
            {
            if (--gGlobeXPos < MIN_XPOS) gGlobeXPos = MIN_XPOS;
            if (--gBallXPos < MIN_XPOS) gBallXPos = MIN_XPOS;
            if (++gTvXPos > MAX_XPOS) gTvXPos = MAX_XPOS;
            if (++gCubeXPos > MAX_XPOS) gCubeXPos = MAX_XPOS;
            }
        else if ( button & ControlRight )
            {
            if (++gGlobeXPos > MAX_XPOS) gGlobeXPos = MAX_XPOS;
            if (++gBallXPos > MAX_XPOS) gBallXPos = MAX_XPOS;
            if (--gTvXPos < MIN_XPOS) gTvXPos = MIN_XPOS;
            if (--gCubeXPos < MIN_XPOS) gCubeXPos = MIN_XPOS;
            }
        }
	
DONE:
    return retValue;
	}

void GetArtFolder( char *folderPath )
/* Get the path to the artwork data folder */
{
	if ( PAL_DISPLAY( gDisplayType ) )
		strcpy( folderPath, PAL_FOLDER );
	else
		strcpy( folderPath, NTSC_FOLDER );
	strcat( folderPath, ART_FOLDER );
}

void GetArtFilename( char *filename, char *fullPathname)
/* Get the full pathname of an artwork file */
{
	GetArtFolder( fullPathname );
	strcat( fullPathname, filename );
}

int32 InitGraphicObjects( void )
/*
	Allocate memory for and load all images and animations.
	For each animation, make all frames reference the same
	CCB, to facilitate uniform setting of position and scale.

	Returns 0 if all operations are successful,
	otherwise -1.
*/
{
	char filename[128];
	
	gWallOffPict = (ubyte *) AllocMem( (int) (gScreenContext->sc_nFrameByteCount),
            MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

    if ( !gWallOffPict )
		{
        PRT( ("Can't allocate wall, light off image\n") );
        goto DONE;
        }

    SetVRAMPages( gVRAMIOReq, gWallOffPict, 0, gScreenContext->sc_nFrameBufferPages, -1 );

	GetArtFilename( WALL_OFF_IMAGE, filename );
    if ( LoadImage( filename,  gWallOffPict, (VdlChunk **)NULL, gScreenContext ) == 0)
		{
        PRT( ("Can't load wall, light off image\n") );
        goto DONE;
        }

    gWallOnPict = (ubyte *)AllocMem( (int)(gScreenContext->sc_nFrameByteCount),
            MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

    if ( !gWallOnPict )
		{
        PRT( ("Can't allocate wall, light on image\n") );
        goto DONE;
        }

    SetVRAMPages( gVRAMIOReq, gWallOnPict, 0, gScreenContext->sc_nFrameBufferPages, -1 );

	GetArtFilename( WALL_ON_IMAGE, filename );
    if ( LoadImage( filename,  gWallOnPict, (VdlChunk **)NULL , gScreenContext) == 0)
		{
        PRT( ("Can't load wall, light on image\n") );
        goto DONE;
        }

    GetArtFilename( GLOBE_ANIM, filename );
	if ( ( gGlobeAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load globe animation\n") );
        goto DONE;
        }
	gGlobeCcb = UnifyAnimation( gGlobeAnim );
		
    GetArtFilename( GLOBESHADOW_ANIM, filename );
    if ( ( gGlobeSAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load globe shadow animation\n") );
        goto DONE;
        }
	gGlobeSCcb = UnifyAnimation( gGlobeSAnim );
		
    GetArtFilename( GLOBESHADOW_ANIM, filename );
    if ( ( gGlobeRAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load globe reflection animation\n") );
        goto DONE;
        }
	gGlobeRCcb = UnifyAnimation( gGlobeRAnim );

    GetArtFilename( TV_ANIM, filename );
    if ( ( gTvAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load TV animation\n") );
        goto DONE;
        }
	gTvCcb = UnifyAnimation( gTvAnim );
		
    GetArtFilename( TVSHADOW_ANIM, filename );
    if ( ( gTvSAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load TV shadow animation\n") );
        goto DONE;
        }
	gTvSCcb = UnifyAnimation( gTvSAnim );
		
    GetArtFilename( TVSHADOW_ANIM, filename );
    if ( ( gTvRAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load TV reflection animation\n") );
        goto DONE;
        }
	gTvRCcb = UnifyAnimation( gTvRAnim );

    GetArtFilename( CUBE_ANIM, filename );
    if ( ( gCubeAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load cube animation\n") );
        goto DONE;
        }
	gCubeCcb = UnifyAnimation( gCubeAnim );
		
    GetArtFilename( CUBESHADOW_ANIM, filename );
    if ( ( gCubeSAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load cube shadow animation\n") );
        goto DONE;
        }
	gCubeSCcb = UnifyAnimation( gCubeSAnim );
		
    GetArtFilename( CUBESHADOW_ANIM, filename );
    if ( ( gCubeRAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load cube reflection animation\n") );
        goto DONE;
        }
	gCubeRCcb = UnifyAnimation( gCubeRAnim );

    GetArtFilename( BALL_ANIM, filename );
    if ( ( gBallAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load ball animation\n") );
        goto DONE;
        }
	gBallCcb = UnifyAnimation( gBallAnim );
		
    GetArtFilename( BALLSHADOW_ANIM, filename );
    if ( ( gBallSAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load ball shadow animation\n") );
        goto DONE;
        }
	gBallSCcb = UnifyAnimation( gBallSAnim );
		
    GetArtFilename( BALLSHADOW_ANIM, filename );
    if ( ( gBallRAnim = LoadAnim (filename, MEMTYPE_CEL ) ) == 0 )
		{
        PRT( ("Can't load ball reflection animation\n") );
        goto DONE;
        }
	gBallRCcb = UnifyAnimation( gBallRAnim );
		
	return 0;
		
DONE:
	return -1;

}


