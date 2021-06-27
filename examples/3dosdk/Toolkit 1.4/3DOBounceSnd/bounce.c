/*
	File:		bounce.c

	Contains:	3DO bounce demo

	Written by:	Greg Gorsiski and Darren Gibbs

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				 5/09/94	crm		removed explicit references to GrafBase struct
		 <7>	 8/15/93	JAY		change LoadAnim interface to new faster interface
		 <6>	 7/28/93	JAY		add IO Req parameter to SetVRAMPages() and CopyVRAMPages() for
									Dragon Tail release.
		 <5>	 6/24/93	JAY		change FindMathFolio to OpenMathFolio
		 <4>	  4/7/93	JAY		MEMTYPE_SPRITE is now MEMTYPE_CEL for Cardinal2
		 <3>	  4/5/93	JAY		add ChangeInitialDirectory()
		 <2>	  4/3/93	JAY		Removed custom joypad routine. Now call ReadControlPad() in
									Lib3DO.lib. This was done for Real Red.
		 <1>	  4/3/93	JAY		first checked in
			 	 3/31/93	RDG		added a (gasp) cleanup routine to unload the samplers
				 3/25/93	RDG		finished adding calls to sound module
	             2/10/93	gag		1.0 release

	To Do:
*/

#include "bounce.h"
#include "bounce_sound.h"

/* Function Prototypes */

void  InitPicts( void );
bool  DoJoystick( void );
ulong Random (ulong);


/* Data Declarations */

bool	gKeysDown 	= FALSE;
bool	gReverse 	= FALSE;
bool	gBouncing 	= FALSE;
bool	gLightON 	= TRUE;
bool	gCollision 	= FALSE;

long	gGlobeXPos 	= 50;
long	gGlobeYPos 	= 50;
long	gCubeXPos	= 150;
long	gCubeYPos 	= 15;
long	gTvXPos 	= 145;
long	gTvYPos 	= 60;
long	gBallXPos 	= 150;
long	gBallYPos 	= 100;

ANIM	*gBallAnimRec;
ANIM	*gBallSAnimRec;
ANIM	*gBallRAnimRec;
ANIM	*gGlobeAnimRec;
ANIM	*gGlobeSAnimRec;
ANIM	*gGlobeRAnimRec;
ANIM	*gTvAnimRec;
ANIM	*gTvSAnimRec;
ANIM	*gTvRAnimRec;
ANIM	*gCubeAnimRec;
ANIM	*gCubeSAnimRec;
ANIM	*gCubeRAnimRec;

frac16	gBallFrameIncr;
frac16	gGlobeFrameIncr;
frac16	gTvFrameIncr;
frac16	gCubeFrameIncr;

ubyte	*gWallOffPict = NULL;
ubyte	*gWallOnPict = NULL;

long	gScreenSelect = 0;

Item	VRAMIOReq;

#define GRAPHICSMASK 	1
#define AUDIOMASK 		2		
#define SPORTIOMASK 	4
#define MACLINKMASK 	8


long StartUp(ScreenContext *sc)
	{
	long retval = 0;
	if (!OpenGraphics(sc,2))
		{
		DIAGNOSTIC("Cannot initialize graphics");
		}
	else retval |= GRAPHICSMASK;
	
	if (!OpenSPORT())
		{
		DIAGNOSTIC("Cannot open DMA channel");
		}
	else retval |= SPORTIOMASK;

	if (!OpenMacLink())
		{
		DIAGNOSTIC("Cannot communicate with Mac");
		}
	else retval |= MACLINKMASK;

	if (OpenAudioFolio())
		{
		DIAGNOSTIC("Cannot initialize audio");
		}
	else retval |= AUDIOMASK;

	if (OpenMathFolio())
		{
		DIAGNOSTIC("Cannot initialize OperaMath");
		}

	return retval;	
	}

/* Main Routines */
static ScreenContext *sc;

int
main( )
{
	bool	ballUpDir 		= FALSE;
	bool	ballLftDir 		= TRUE;
	bool	globeUpDir 		= FALSE;
	bool	globeLftDir 	= FALSE;
	bool	tvUpDir 		= FALSE;
	bool	tvLftDir 		= TRUE;
	bool	cubeUpDir 		= FALSE;
	bool	cubeLftDir 		= FALSE;
	
	long	globeYSpeed;
	long	globeSwSpeed;
	long	cubeYSpeed;
	long	cubeSwSpeed;
	long	tvYSpeed;
	long	tvSwSpeed;
	long	ballYSpeed;
	long	ballSwSpeed;
	long	ballScale = 0;
	
	int32	retvalue;
	
	CCB*	ccbBall;
	CCB*	ccbBallS;
	CCB*	ccbBallR;
	CCB*	ccbGlobe;
	CCB*	ccbGlobeS;
	CCB*	ccbGlobeR;
	CCB*	ccbTv;
	CCB*	ccbTvS;
	CCB*	ccbTvR;
	CCB*	ccbCube;
	CCB*	ccbCubeS;
	CCB*	ccbCubeR;
	Point	aCorner[4];

	VRAMIOReq = GetVRAMIOReq();
	
	gTvFrameIncr     = Convert32_F16(0);
	gBallFrameIncr   = Convert32_F16(0);
	gCubeFrameIncr   = Convert32_F16(0);
	gGlobeFrameIncr  = Convert32_F16(1);
	gGlobeFrameIncr -= 0x00009000;

	kprintf( "\n3DO-Bounce w/sound\n" );
	kprintf( "Samples are triggered when objects bounce on the floor and\n" );
	kprintf( "  when they collide in the air.  The pitch of the object\n" );
	kprintf( "  collision sounds are determined by the height at which\n" );
	kprintf( "  they collide, and panning is changed based on left/right\n" );
	kprintf( "  screen position.\n" );
	kprintf( "\n   A Key - to start bounce mode." );
	kprintf( "\n         - to switch light during bounce mode." );
	kprintf( "\n   B Key - to reset, and turn collison on." );
	kprintf( "\n   C Key - to freeze, and turn collison off." );
	kprintf( "\n   Pad   - use pad to rearrange objects." );
	kprintf( "\n         - while in bounce or frozen mode.\n" );
	kprintf( "\nLoading.........\n" );
	
	sc = (ScreenContext *) ALLOCMEM(sizeof(ScreenContext),MEMTYPE_CEL );
	if (sc == NULL)
		{
		DIAGNOSTIC("Cannot Allocate memory for ScreenContext ");
		return FALSE;
		}
	
	if ( ChangeInitialDirectory( NULL, NULL, false ) < 0 )
		{
		DIAGNOSTIC("Cannot set initial working directory ");
		return FALSE;
		}
	
	if ( NOT StartUp(sc) ) goto DONE;

	InitPicts( );	
	if ( InitBounceSound() != 0 ) {
		kprintf( "\nInitBounceSound  Failed" );
		exit(0);
	}

	EnableVAVG( sc->sc_Screens[0] );		/*DisableHAVG*/
	EnableVAVG( sc->sc_Screens[1] );
	EnableHAVG( sc->sc_Screens[0] );			
	EnableHAVG( sc->sc_Screens[1] );
	kprintf( "Enable vert. averaging, enable horiz. averaging" );
	kprintf( "\nPress A to start bounce. Collision is off." );

	while (TRUE) {
		Random( 2 );
		if ( NOT DoJoystick() ) goto DONE;
		
		if( gBallYPos <= -40 )					  ballYSpeed = 1;
		if( gBallYPos > -40 && gBallYPos <= -20 ) ballYSpeed = 2;
		if( gBallYPos >   0 && gBallYPos <=  10 ) ballYSpeed = 3;
		if( gBallYPos >  10 && gBallYPos <=  20 ) ballYSpeed = 4;
		if( gBallYPos >  20 && gBallYPos <=  40 ) ballYSpeed = 5;
		if( gBallYPos >  40 && gBallYPos <=  60 ) ballYSpeed = 6;
		if( gBallYPos >  60 && gBallYPos <=  80 ) ballYSpeed = 7;
		if( gBallYPos >  80 && gBallYPos <= 100 ) ballYSpeed = 8;
		if( gBallYPos > 100 && gBallYPos <= 120 ) ballYSpeed = 10;
		if( gBallYPos > 120 && gBallYPos <= 140 ) ballYSpeed = 12;
		if( gBallYPos > 140 && gBallYPos <= 160 ) ballYSpeed = 14;
		if( gBallYPos > 160 && gBallYPos <= 180 ) ballYSpeed = 16;
		if( gBallYPos > 180 ) 					  ballYSpeed = 18;	

			
		if( gBouncing ) {
			if( ballUpDir ) {
				if( (gBallYPos -=ballYSpeed) <= -50) 
				{
					ballUpDir = FALSE;
				}
				
				if( gCollision ) {
					if( (gBallXPos+(ccbBall->ccb_Height-5) >= gTvXPos+15) && 
						(gBallXPos+5 <= gTvXPos+(ccbTv->ccb_Width-15)) &&
						(gBallYPos+5 >= gTvYPos+(ccbTv->ccb_Height-15)) && 
						(gBallYPos+5 <= gTvYPos+(ccbTv->ccb_Height-5)) ) {
								ballUpDir = FALSE;
								tvUpDir = TRUE;
								DoObjectCollisionSound(BALL_TV_COLL | BELOW);
								}
						}
				} else {
					if( (gBallYPos +=ballYSpeed) >= 190 ) 
					{
						DoRoomCollisionSound(BALL | FLOOR);
						ballUpDir = TRUE;	
					}
					if( gCollision ) {
						if( (gBallXPos+(ccbBall->ccb_Height-5) >= gTvXPos+15) && 
							(gBallXPos+5 <= gTvXPos+(ccbTv->ccb_Width-15)) &&
							(gBallYPos+(ccbTv->ccb_Height-5) >= gTvYPos+5) && 
							(gBallYPos+(ccbTv->ccb_Height-5) <= gTvYPos+15) ) {
									ballUpDir = TRUE;
									tvUpDir = FALSE;
									DoObjectCollisionSound(BALL_TV_COLL | ABOVE);
									}
							}
					}

			if( ballLftDir ) {
				if( (gBallXPos -=2) <= 0 )
					ballLftDir = FALSE;
				if( gCollision ) {
					if( (gBallXPos+5 >= gTvXPos+(ccbTv->ccb_Width-15)) && 
						(gBallXPos+5 <= gTvXPos+(ccbTv->ccb_Width-5)) &&
						(gBallYPos+(ccbBall->ccb_Height-15) >= gTvYPos+5) && 
						(gBallYPos+15 <= gTvYPos+(ccbTv->ccb_Height-5)) ) {
								ballLftDir = FALSE;
								tvLftDir = TRUE;
								DoObjectCollisionSound(BALL_TV_COLL | LEFT);
								}
						}
				} else {
					if( (gBallXPos +=2) >= 270 ) 
						ballLftDir = TRUE;	
					if( gCollision ) {
						if( (gBallXPos+(ccbBall->ccb_Width-5) <= gTvXPos+15) && 
							(gBallXPos+(ccbBall->ccb_Width-5) >= gTvXPos+5) &&
							(gBallYPos+(ccbBall->ccb_Height-15) >= gTvYPos+5) && 
							(gBallYPos+15 <= gTvYPos+(ccbTv->ccb_Height-5)) ) {
									ballLftDir = TRUE;
									tvLftDir = FALSE;
									DoObjectCollisionSound(BALL_TV_COLL | RIGHT);
									}
							}
						}
			}
			

/*		if( gBallXPos <= 20 )					  ballScale = -16;
		if( gBallXPos >  20 && gBallXPos <=  30 ) ballScale = -14;
		if( gBallXPos >  30 && gBallXPos <=  50 ) ballScale = -12;
		if( gBallXPos >  50 && gBallXPos <=  70 ) ballScale = -10;
		if( gBallXPos >  70 && gBallXPos <=  90 ) ballScale = -8;
		if( gBallXPos >  90 && gBallXPos <= 110 ) ballScale = -6;
		if( gBallXPos > 125 && gBallXPos <= 125 ) ballScale = -4;
		if( gBallXPos > 135 && gBallXPos <= 145 ) ballScale = -2;
		if( gBallXPos > 145 && gBallXPos <= 155 ) ballScale = 0;
		if( gBallXPos > 155 && gBallXPos <= 165 ) ballScale = -2;
		if( gBallXPos > 165 && gBallXPos <= 180 ) ballScale = -4;
		if( gBallXPos > 180 && gBallXPos <= 200 ) ballScale = -6;
		if( gBallXPos > 200 && gBallXPos <= 220 ) ballScale = -8;
		if( gBallXPos > 220 && gBallXPos <= 240 ) ballScale = -10;
		if( gBallXPos > 240 && gBallXPos <= 260 ) ballScale = -12;
		if( gBallXPos > 260 ) 					  ballScale = -14;	
*/
		ccbBall = GetAnimCel(gBallAnimRec,gBallFrameIncr);
		aCorner[0].pt_X = gBallXPos-ballScale;
		aCorner[0].pt_Y = gBallYPos-ballScale;
		aCorner[1].pt_X = (gBallXPos+ccbBall->ccb_Width)+ballScale;
		aCorner[1].pt_Y = gBallYPos-ballScale;
		aCorner[2].pt_X = (gBallXPos+ccbBall->ccb_Width)+ballScale;
		aCorner[2].pt_Y = (gBallYPos+ccbBall->ccb_Height)+ballScale;
		aCorner[3].pt_X = gBallXPos-ballScale;
		aCorner[3].pt_Y = (gBallYPos+ccbBall->ccb_Height)+ballScale;
	
		if( aCorner[2].pt_Y >= 215 ) {
			aCorner[2].pt_Y  = 215;
			aCorner[3].pt_Y  = 215;
			}
						
		if( gBouncing ) {	
			if( aCorner[1].pt_X >= 300 ) {
				aCorner[1].pt_X  = 300;
				aCorner[2].pt_X  = 300;
				}
			if( aCorner[0].pt_X <= 20 ) {
				aCorner[0].pt_X  = 20;
				aCorner[3].pt_X  = 20;
				}
			}
		MapCel(ccbBall, aCorner);
		
		ccbBallS = GetAnimCel(gBallSAnimRec,gBallFrameIncr);
		if( gLightON ) {
			if( gBallXPos <= 20 ) ballSwSpeed = -12;
			if( gBallXPos >  20 && gBallXPos <=  40 ) ballSwSpeed = -10;
			if( gBallXPos >  40 && gBallXPos <=  60 ) ballSwSpeed = -8;
			if( gBallXPos >  60 && gBallXPos <=  80 ) ballSwSpeed = -6;
			if( gBallXPos >  80 && gBallXPos <= 100 ) ballSwSpeed = -4;
			if( gBallXPos > 100 && gBallXPos <= 120 ) ballSwSpeed = -3;
			if( gBallXPos > 120 && gBallXPos <= 140 ) ballSwSpeed = -2;
			if( gBallXPos > 140 && gBallXPos <= 160 ) ballSwSpeed = -1;
			
			if( gBallXPos > 160 && gBallXPos <= 180 ) ballSwSpeed = 1;
			if( gBallXPos > 180 && gBallXPos <= 200 ) ballSwSpeed = 2;
			if( gBallXPos > 200 && gBallXPos <= 220 ) ballSwSpeed = 3;
			if( gBallXPos > 220 && gBallXPos <= 240 ) ballSwSpeed = 4;
			if( gBallXPos > 240 && gBallXPos <= 260 ) ballSwSpeed = 6;
			if( gBallXPos > 260 && gBallXPos <= 280 ) ballSwSpeed = 8;
			if( gBallXPos > 280 && gBallXPos <= 300 ) ballSwSpeed = 10;
			if( gBallXPos > 300 ) ballSwSpeed = 12;
	
			aCorner[0].pt_X += ballSwSpeed;
			aCorner[0].pt_Y += ballYSpeed+ballYSpeed+8;
			aCorner[1].pt_X += ballSwSpeed;
			aCorner[1].pt_Y += ballYSpeed+ballYSpeed+8;
			aCorner[2].pt_X += ballSwSpeed;
			aCorner[2].pt_Y += ballYSpeed+ballYSpeed+8;
			aCorner[3].pt_X += ballSwSpeed;
			aCorner[3].pt_Y += ballYSpeed+ballYSpeed+8;
	
			if( aCorner[2].pt_Y >= 170 )	aCorner[2].pt_Y = 180;
			if( aCorner[3].pt_Y >= 170 )	aCorner[3].pt_Y = 180;

			if( aCorner[0].pt_X <= 40 )		aCorner[0].pt_X = 40;
			if( aCorner[1].pt_X <= 40 )		aCorner[1].pt_X = 40;
			if( aCorner[2].pt_Y <= -40 )		aCorner[2].pt_X = -40;
			if( aCorner[3].pt_Y <= -40 )		aCorner[3].pt_X = -40;
			if( aCorner[0].pt_X >= 280 )	aCorner[0].pt_X = 280;
			if( aCorner[1].pt_X >= 280 )	aCorner[1].pt_X = 280;
			if( aCorner[2].pt_Y >= 280 )	aCorner[2].pt_X = 280;
			if( aCorner[3].pt_Y >= 280 )	aCorner[3].pt_X = 280;
			}
		MapCel(ccbBallS, aCorner);

		ccbBallR = GetAnimCel(gBallRAnimRec,gBallFrameIncr);
		aCorner[0].pt_X -= ballYSpeed;
		aCorner[0].pt_Y =  190 + ballYSpeed;
		aCorner[1].pt_X += ballYSpeed;
		aCorner[1].pt_Y =  190 + ballYSpeed;
		aCorner[2].pt_X += ballYSpeed;
		aCorner[2].pt_Y =  200 + ballYSpeed;
		aCorner[3].pt_X -= ballYSpeed;
		aCorner[3].pt_Y =  200 + ballYSpeed;
		MapCel(ccbBallR, aCorner);
		
		
		
		if( gGlobeYPos <= 10 ) 					    globeYSpeed = 1;
		if( gGlobeYPos >  10 && gGlobeYPos <=  35 ) globeYSpeed = 2;
		if( gGlobeYPos >  35 && gGlobeYPos <=  60 ) globeYSpeed = 3;
		if( gGlobeYPos >  60 && gGlobeYPos <=  80 ) globeYSpeed = 4;
		if( gGlobeYPos >  80 && gGlobeYPos <= 100 ) globeYSpeed = 5;
		if( gGlobeYPos > 100 && gGlobeYPos <= 120 ) globeYSpeed = 6;
		if( gGlobeYPos > 120 && gGlobeYPos <= 140 ) globeYSpeed = 7;
		if( gGlobeYPos > 140 && gGlobeYPos <= 160 ) globeYSpeed = 9;
		if( gGlobeYPos > 160 && gGlobeYPos <= 180 ) globeYSpeed = 11;
		if( gGlobeYPos > 180 ) 					    globeYSpeed = 13;
		

		if( gBouncing ) {
			if( globeUpDir ) {
				if( (gGlobeYPos -=globeYSpeed) <= 0 )
					globeUpDir = FALSE;
				if( gCollision ) {
					if( (gGlobeXPos+(ccbGlobe->ccb_Height-5) >= gBallXPos+15) && 
						(gGlobeXPos+5 <= gBallXPos+(ccbBall->ccb_Width-15)) &&
						(gGlobeYPos+5 >= gBallYPos+(ccbBall->ccb_Height-15)) && 
						(gGlobeYPos+5 <= gBallYPos+(ccbBall->ccb_Height-5)) ) {
								globeUpDir = FALSE;
								ballUpDir = TRUE;
								DoObjectCollisionSound(BALL_GLOBE_COLL | ABOVE);
								}
						}
				} else {
					if( (gGlobeYPos +=globeYSpeed) >= 190 )
					{
						globeUpDir = TRUE;
						DoRoomCollisionSound(GLOBE | FLOOR);
					}
					
					if( gCollision ) {
						if( (gGlobeXPos+(ccbGlobe->ccb_Height-5) >= gBallXPos+15) && 
							(gGlobeXPos+5 <= gBallXPos+(ccbBall->ccb_Width-15)) &&
							(gGlobeYPos+(ccbBall->ccb_Height-5) >= gBallYPos+5) && 
							(gGlobeYPos+(ccbBall->ccb_Height-5) <= gBallYPos+15) ) {
									globeUpDir = TRUE;
									ballUpDir = FALSE;
									DoObjectCollisionSound(BALL_GLOBE_COLL | BELOW);
									}
							}
					}

			if( globeLftDir ) {
				if( (gGlobeXPos -=3) <= 0 )
					globeLftDir = FALSE;
				
				if( gCollision ) {
					if( (gGlobeXPos+5 >= gBallXPos+(ccbBall->ccb_Width-15)) && 
						(gGlobeXPos+5 <= gBallXPos+(ccbBall->ccb_Width-5)) &&
						(gGlobeYPos+(ccbGlobe->ccb_Height-15) >= gBallYPos+5) && 
						(gGlobeYPos+15 <= gBallYPos+(ccbBall->ccb_Height-5)) ) {
								globeLftDir = FALSE;
								ballLftDir = TRUE;
								DoObjectCollisionSound(BALL_GLOBE_COLL | LEFT);
								}
						}
				} else {
					if( (gGlobeXPos +=3) >= 270 ) 
						globeLftDir = TRUE;	
				if( gCollision ) {
					if( (gGlobeXPos+(ccbGlobe->ccb_Width-5) <= gBallXPos+15) && 
						(gGlobeXPos+(ccbGlobe->ccb_Width-5) >= gBallXPos+5) &&
						(gGlobeYPos+(ccbGlobe->ccb_Height-15) >= gBallYPos+5) && 
						(gGlobeYPos+15 <= gBallYPos+(ccbBall->ccb_Height-5)) ) {
								globeLftDir = TRUE;
								ballLftDir = FALSE;
								DoObjectCollisionSound(BALL_GLOBE_COLL | RIGHT);
								}
						}
					}

					
			} else {
				if( globeLftDir )
					if( (gGlobeXPos -=3) <= 50 ) globeLftDir = FALSE;
				if( !globeLftDir )
					if( (gGlobeXPos +=3) >= 220 ) globeLftDir = TRUE;
				}


			
		ccbGlobe = GetAnimCel(gGlobeAnimRec,gGlobeFrameIncr);
		aCorner[0].pt_X = gGlobeXPos;
		aCorner[0].pt_Y = gGlobeYPos;
		aCorner[1].pt_X = gGlobeXPos+ccbGlobe->ccb_Width;
		aCorner[1].pt_Y = gGlobeYPos;
		aCorner[2].pt_X = gGlobeXPos+ccbGlobe->ccb_Width;
		aCorner[2].pt_Y = gGlobeYPos+ccbGlobe->ccb_Height;
		aCorner[3].pt_X = gGlobeXPos;
		aCorner[3].pt_Y = gGlobeYPos+ccbGlobe->ccb_Height;
	
		if( aCorner[2].pt_Y >= 215 ) {
			aCorner[2].pt_Y  = 215;
			aCorner[3].pt_Y  = 215;
			}
			
		if( gBouncing ) {	
			if( aCorner[1].pt_X >= 300 ) {
				aCorner[1].pt_X  = 300;
				aCorner[2].pt_X  = 300;
				}
			if( aCorner[0].pt_X <= 20 ) {
				aCorner[0].pt_X  = 20;
				aCorner[3].pt_X  = 20;
				}
			}
			
		MapCel(ccbGlobe, aCorner);
		
		ccbGlobeS = GetAnimCel(gGlobeSAnimRec,gGlobeFrameIncr);
		if( gLightON ) {
			if( gGlobeXPos <= 20 ) globeSwSpeed = -12;
			if( gGlobeXPos >  20 && gGlobeXPos <=  40 ) globeSwSpeed = -10;
			if( gGlobeXPos >  40 && gGlobeXPos <=  60 ) globeSwSpeed = -8;
			if( gGlobeXPos >  60 && gGlobeXPos <=  80 ) globeSwSpeed = -6;
			if( gGlobeXPos >  80 && gGlobeXPos <= 100 ) globeSwSpeed = -4;
			if( gGlobeXPos > 100 && gGlobeXPos <= 120 ) globeSwSpeed = -3;
			if( gGlobeXPos > 120 && gGlobeXPos <= 140 ) globeSwSpeed = -2;
			if( gGlobeXPos > 140 && gGlobeXPos <= 160 ) globeSwSpeed = -1;
			
			if( gGlobeXPos > 160 && gGlobeXPos <= 180 ) globeSwSpeed = 1;
			if( gGlobeXPos > 180 && gGlobeXPos <= 200 ) globeSwSpeed = 2;
			if( gGlobeXPos > 200 && gGlobeXPos <= 220 ) globeSwSpeed = 3;
			if( gGlobeXPos > 220 && gGlobeXPos <= 240 ) globeSwSpeed = 4;
			if( gGlobeXPos > 240 && gGlobeXPos <= 260 ) globeSwSpeed = 6;
			if( gGlobeXPos > 260 && gGlobeXPos <= 280 ) globeSwSpeed = 8;
			if( gGlobeXPos > 280 && gGlobeXPos <= 300 ) globeSwSpeed = 10;
			if( gGlobeXPos > 300 ) globeSwSpeed = 12;
	
			aCorner[0].pt_X += globeSwSpeed;
			aCorner[0].pt_Y += globeYSpeed+globeYSpeed+8;
			aCorner[1].pt_X += globeSwSpeed;
			aCorner[1].pt_Y += globeYSpeed+globeYSpeed+8;
			aCorner[2].pt_X += globeSwSpeed;
			aCorner[2].pt_Y += globeYSpeed+globeYSpeed+8;
			aCorner[3].pt_X += globeSwSpeed;
			aCorner[3].pt_Y += globeYSpeed+globeYSpeed+8;
	
			if( aCorner[2].pt_Y >= 170 )	aCorner[2].pt_Y = 180;
			if( aCorner[3].pt_Y >= 170 )	aCorner[3].pt_Y = 180;
	
			if( aCorner[0].pt_X <= 40 )		aCorner[0].pt_X = 40;
			if( aCorner[1].pt_X <= 40 )		aCorner[1].pt_X = 40;
			if( aCorner[2].pt_Y <= 40 )		aCorner[2].pt_X = 40;
			if( aCorner[3].pt_Y <= 40 )		aCorner[3].pt_X = 40;
			if( aCorner[0].pt_X >= 280 )	aCorner[0].pt_X = 280;
			if( aCorner[1].pt_X >= 280 )	aCorner[1].pt_X = 280;
			if( aCorner[2].pt_Y >= 280 )	aCorner[2].pt_X = 280;
			if( aCorner[3].pt_Y >= 280 )	aCorner[3].pt_X = 280;
			}
		MapCel(ccbGlobeS, aCorner);

		ccbGlobeR = GetAnimCel(gGlobeRAnimRec,gGlobeFrameIncr);
		aCorner[0].pt_X -= globeYSpeed;
		aCorner[0].pt_Y  = 190 + globeYSpeed;
		aCorner[1].pt_X += globeYSpeed;
		aCorner[1].pt_Y  = 190 + globeYSpeed;
		aCorner[2].pt_X += globeYSpeed;
		aCorner[2].pt_Y  = 200 + globeYSpeed;
		aCorner[3].pt_X -= globeYSpeed;
		aCorner[3].pt_Y  = 200 + globeYSpeed;
		MapCel(ccbGlobeR, aCorner);

//@@@@@@
		if( gTvYPos <= 60 ) 				  tvYSpeed = 1;
		if( gTvYPos >  60 && gTvYPos <=  75 ) tvYSpeed = 2;
		if( gTvYPos >  75 && gTvYPos <=  90 ) tvYSpeed = 3;
		if( gTvYPos >  90 && gTvYPos <= 105 ) tvYSpeed = 4;
		if( gTvYPos > 105 && gTvYPos <= 120 ) tvYSpeed = 5;
		if( gTvYPos > 120 && gTvYPos <= 135 ) tvYSpeed = 6;
		if( gTvYPos > 135 && gTvYPos <= 150 ) tvYSpeed = 7;
		if( gTvYPos > 150 && gTvYPos <= 165 ) tvYSpeed = 9;
		if( gTvYPos > 165 && gTvYPos <= 180 ) tvYSpeed = 11;
		if( gTvYPos > 180 ) 				  tvYSpeed = 13;
		
		
		if( gBouncing ) {
			if( tvUpDir ) {
				if( (gTvYPos -=tvYSpeed) <= 40 )
					tvUpDir = FALSE;
				if( gCollision ) {
					if( (gTvXPos+(ccbTv->ccb_Height-5) >= gCubeXPos+15) && 
						(gTvXPos+5 <= gCubeXPos+(ccbCube->ccb_Width-15)) &&
						(gTvYPos+5 >= gCubeYPos+(ccbCube->ccb_Height-15)) && 
						(gTvYPos+5 <= gCubeYPos+(ccbCube->ccb_Height-5)) ) {
								tvUpDir = FALSE;
								cubeUpDir = TRUE;
								DoObjectCollisionSound(TV_CUBE_COLL | BELOW);
								}
						}
				} else {
					if( (gTvYPos +=tvYSpeed) >= 190 )
					{
						DoRoomCollisionSound(TV | FLOOR);
						tvUpDir = TRUE;
					}
					
					if( gCollision ) {
						if( (gTvXPos+(ccbTv->ccb_Height-5) >= gCubeXPos+15) && 
							(gTvXPos+5 <= gCubeXPos+(ccbCube->ccb_Width-15)) &&
							(gTvYPos+(ccbCube->ccb_Height-5) >= gCubeYPos+5) && 
							(gTvYPos+(ccbCube->ccb_Height-5) <= gCubeYPos+15) ) {
									tvUpDir = TRUE;
									cubeUpDir = FALSE;
									DoObjectCollisionSound(TV_CUBE_COLL | ABOVE);
									}
							}
					}
	
			if( tvLftDir ) {
				if( (gTvXPos -=3) <= 0 )
					tvLftDir = FALSE;
				if( gCollision ) {
					if( (gTvXPos+5 >= gCubeXPos+(ccbCube->ccb_Width-15)) && 
						(gTvXPos+5 <= gCubeXPos+(ccbCube->ccb_Width-5)) &&
						(gTvYPos+(ccbTv->ccb_Height-15) >= gCubeYPos+5) && 
						(gTvYPos+15 <= gCubeYPos+(ccbCube->ccb_Height-5)) ) {
								tvLftDir = FALSE;
								cubeLftDir = TRUE;
								DoObjectCollisionSound(TV_CUBE_COLL | RIGHT);
								}
						}
				} else {
					if( (gTvXPos +=3) >= 270 )
						tvLftDir = TRUE;
					if( gCollision ) {
						if( (gTvXPos+(ccbTv->ccb_Width-5) <= gCubeXPos+15) && 
							(gTvXPos+(ccbTv->ccb_Width-5) >= gCubeXPos+5) &&
							(gTvYPos+(ccbTv->ccb_Height-15) >= gCubeYPos+5) && 
							(gTvYPos+15 <= gCubeYPos+(ccbCube->ccb_Height-5)) ) {
									tvLftDir = TRUE;
									cubeLftDir = FALSE;
									DoObjectCollisionSound(TV_CUBE_COLL | LEFT);
									}
							}
						}
			}
			
		ccbTv = GetAnimCel(gTvAnimRec,gTvFrameIncr);

		aCorner[0].pt_X = gTvXPos;
		aCorner[0].pt_Y = gTvYPos;
		aCorner[1].pt_X = gTvXPos+ccbTv->ccb_Width;
		aCorner[1].pt_Y = gTvYPos;
		aCorner[2].pt_X = gTvXPos+ccbTv->ccb_Width;
		aCorner[2].pt_Y = gTvYPos+ccbTv->ccb_Height;
		aCorner[3].pt_X = gTvXPos;
		aCorner[3].pt_Y = gTvYPos+ccbTv->ccb_Height;
	
		if( aCorner[2].pt_Y >= 215 ) {
			aCorner[2].pt_Y  = 215;
			aCorner[3].pt_Y  = 215;
			}
		if( aCorner[1].pt_X >= 300 ) {
			aCorner[1].pt_X  = 300;
			aCorner[2].pt_X  = 300;
			}
		if( aCorner[0].pt_X <= 20 ) {
			aCorner[0].pt_X  = 20;
			aCorner[3].pt_X  = 20;
			}
			
		MapCel(ccbTv, aCorner);
		
		ccbTvS = GetAnimCel(gTvSAnimRec,gTvFrameIncr);
		if( gLightON ) {
			if( gTvXPos <= 20 ) tvSwSpeed = -12;
			if( gTvXPos >  20 && gTvXPos <=  40 ) tvSwSpeed = -10;
			if( gTvXPos >  40 && gTvXPos <=  60 ) tvSwSpeed = -8;
			if( gTvXPos >  60 && gTvXPos <=  80 ) tvSwSpeed = -6;
			if( gTvXPos >  80 && gTvXPos <= 100 ) tvSwSpeed = -4;
			if( gTvXPos > 100 && gTvXPos <= 120 ) tvSwSpeed = -3;
			if( gTvXPos > 120 && gTvXPos <= 140 ) tvSwSpeed = -2;
			if( gTvXPos > 140 && gTvXPos <= 160 ) tvSwSpeed = -1;
			
			if( gTvXPos > 160 && gTvXPos <= 180 ) tvSwSpeed = 1;
			if( gTvXPos > 180 && gTvXPos <= 200 ) tvSwSpeed = 2;
			if( gTvXPos > 200 && gTvXPos <= 220 ) tvSwSpeed = 3;
			if( gTvXPos > 220 && gTvXPos <= 240 ) tvSwSpeed = 4;
			if( gTvXPos > 240 && gTvXPos <= 260 ) tvSwSpeed = 6;
			if( gTvXPos > 260 && gTvXPos <= 280 ) tvSwSpeed = 8;
			if( gTvXPos > 280 && gTvXPos <= 300 ) tvSwSpeed = 10;
			if( gTvXPos > 300 ) tvSwSpeed = 12;
	
			aCorner[0].pt_X += tvSwSpeed;
			aCorner[0].pt_Y += tvYSpeed+tvYSpeed+8;
			aCorner[1].pt_X += tvSwSpeed;
			aCorner[1].pt_Y += tvYSpeed+tvYSpeed+8;
			aCorner[2].pt_X += tvSwSpeed;
			aCorner[2].pt_Y += tvYSpeed+tvYSpeed+8;
			aCorner[3].pt_X += tvSwSpeed;
			aCorner[3].pt_Y += tvYSpeed+tvYSpeed+8;
	
			if( aCorner[2].pt_Y >= 190 )	aCorner[2].pt_Y = 190;
			if( aCorner[3].pt_Y >= 190 )	aCorner[3].pt_Y = 190;
	
			if( aCorner[0].pt_X <= 40 )		aCorner[0].pt_X = 40;
			if( aCorner[1].pt_X <= 40 )		aCorner[1].pt_X = 40;
			if( aCorner[2].pt_Y <= 40 )		aCorner[2].pt_X = 40;
			if( aCorner[3].pt_Y <= 40 )		aCorner[3].pt_X = 40;
			if( aCorner[0].pt_X >= 280 )	aCorner[0].pt_X = 280;
			if( aCorner[1].pt_X >= 280 )	aCorner[1].pt_X = 280;
			if( aCorner[2].pt_Y >= 280 )	aCorner[2].pt_X = 280;
			if( aCorner[3].pt_Y >= 280 )	aCorner[3].pt_X = 280;
			}
		MapCel(ccbTvS, aCorner);

		ccbTvR = GetAnimCel(gTvRAnimRec,gTvFrameIncr);
		aCorner[0].pt_X -= tvYSpeed;
		aCorner[0].pt_Y = 190+tvYSpeed;
		aCorner[1].pt_X +=tvYSpeed;
		aCorner[1].pt_Y = 190+tvYSpeed;
		aCorner[2].pt_X += tvYSpeed;
		aCorner[2].pt_Y = 200+tvYSpeed;
		aCorner[3].pt_X -= tvYSpeed;
		aCorner[3].pt_Y = 200+tvYSpeed;
		MapCel(ccbTvR, aCorner);
//@@@@@@

//@@@@@@
		if( gCubeYPos <= 10 ) 				      cubeYSpeed = 1;
		if( gCubeYPos >  10 && gCubeYPos <=  35 ) cubeYSpeed = 2;
		if( gCubeYPos >  35 && gCubeYPos <=  60 ) cubeYSpeed = 4;
		if( gCubeYPos >  60 && gCubeYPos <=  80 ) cubeYSpeed = 6;
		if( gCubeYPos >  80 && gCubeYPos <= 100 ) cubeYSpeed = 8;
		if( gCubeYPos > 100 && gCubeYPos <= 120 ) cubeYSpeed = 10;
		if( gCubeYPos > 120 && gCubeYPos <= 140 ) cubeYSpeed = 12;
		if( gCubeYPos > 140 && gCubeYPos <= 160 ) cubeYSpeed = 14;
		if( gCubeYPos > 160 && gCubeYPos <= 180 ) cubeYSpeed = 16;
		if( gCubeYPos > 180 ) 				      cubeYSpeed = 18;
		
		
		if( gBouncing ) {
			if( cubeUpDir ) {
				if( (gCubeYPos -=cubeYSpeed) <= 0 )
					cubeUpDir = FALSE;
				if( gCollision ) {
					if( (gCubeXPos+(ccbCube->ccb_Height-5) >= gGlobeXPos+15) && 
						(gCubeXPos+5 <= gGlobeXPos+(ccbGlobe->ccb_Width-15)) &&
						(gCubeYPos+5 >= gGlobeYPos+(ccbGlobe->ccb_Height-15)) && 
						(gCubeYPos+5 <= gGlobeYPos+(ccbGlobe->ccb_Height-5)) ) {
								cubeUpDir = FALSE;
								globeUpDir = TRUE;
								DoObjectCollisionSound(CUBE_GLOBE_COLL | BELOW);
								}
						}
				} else {
					if( gBouncing && (gCubeYPos +=cubeYSpeed) >= 190 )
					{
						DoRoomCollisionSound(CUBE | FLOOR);
						cubeUpDir = TRUE;
					}
					
					if( gCollision ) {
						if( (gCubeXPos+(ccbCube->ccb_Height-5) >= gGlobeXPos+15) && 
							(gCubeXPos+5 <= gGlobeXPos+(ccbGlobe->ccb_Width-15)) &&
							(gCubeYPos+(ccbGlobe->ccb_Height-5) >= gGlobeYPos+5) && 
							(gCubeYPos+(ccbGlobe->ccb_Height-5) <= gGlobeYPos+15) ) {
									cubeUpDir = TRUE;
									globeUpDir = FALSE;
									DoObjectCollisionSound(CUBE_GLOBE_COLL | ABOVE);
									}
							}
					}
				
			if( cubeLftDir ) {
				if( (gCubeXPos -=1) <= 10 )
					cubeLftDir = FALSE;
				if( gCollision ) {
					if( (gCubeXPos+5 >= gGlobeXPos+(ccbGlobe->ccb_Width-15)) && 
						(gCubeXPos+5 <= gGlobeXPos+(ccbGlobe->ccb_Width-5)) &&
						(gCubeYPos+(ccbCube->ccb_Height-15) >= gGlobeYPos+5) && 
						(gCubeYPos+15 <= gGlobeYPos+(ccbGlobe->ccb_Height-5)) ) {
								cubeLftDir = FALSE;
								globeLftDir = TRUE;
								DoObjectCollisionSound(CUBE_GLOBE_COLL | RIGHT);
								}
						}
				} else {
					if( gBouncing && (gCubeXPos +=1) >= 265 )
						cubeLftDir = TRUE;
					if( gCollision ) {
						if( (gCubeXPos+(ccbCube->ccb_Width-5) <= gGlobeXPos+15) && 
							(gCubeXPos+(ccbCube->ccb_Width-5) >= gGlobeXPos+5) &&
							(gCubeYPos+(ccbCube->ccb_Height-15) >= gGlobeYPos+5) && 
							(gCubeYPos+15 <= gGlobeYPos+(ccbGlobe->ccb_Height-5)) ) {
									cubeLftDir = TRUE;
									globeLftDir = FALSE;
									DoObjectCollisionSound(CUBE_GLOBE_COLL | LEFT);
									}
							}
						}
			}
			
		ccbCube = GetAnimCel(gCubeAnimRec,gCubeFrameIncr);
		aCorner[0].pt_X = gCubeXPos;
		aCorner[0].pt_Y = gCubeYPos;
		aCorner[1].pt_X = gCubeXPos+ccbCube->ccb_Width;
		aCorner[1].pt_Y = gCubeYPos;
		aCorner[2].pt_X = gCubeXPos+ccbCube->ccb_Width;
		aCorner[2].pt_Y = gCubeYPos+ccbCube->ccb_Height;
		aCorner[3].pt_X = gCubeXPos;
		aCorner[3].pt_Y = gCubeYPos+ccbCube->ccb_Height;
	
		if( aCorner[2].pt_Y >= 215 ) {
			aCorner[2].pt_Y  = 215;
			aCorner[3].pt_Y  = 215;
			}
		if( aCorner[1].pt_X >= 310 ) {
			aCorner[1].pt_X  = 310;
			aCorner[2].pt_X  = 310;
			}
		if( aCorner[0].pt_X <= 20 ) {
			aCorner[0].pt_X  = 20;
			aCorner[3].pt_X  = 20;
			}
			
		MapCel(ccbCube, aCorner);
		
		ccbCubeS = GetAnimCel(gCubeSAnimRec,gCubeFrameIncr);
		if( gLightON ) {
			if( gCubeXPos <= 20 ) cubeSwSpeed = -12;
			if( gCubeXPos >  20 && gCubeXPos <=  40 ) cubeSwSpeed = -10;
			if( gCubeXPos >  40 && gCubeXPos <=  60 ) cubeSwSpeed = -8;
			if( gCubeXPos >  60 && gCubeXPos <=  80 ) cubeSwSpeed = -6;
			if( gCubeXPos >  80 && gCubeXPos <= 100 ) cubeSwSpeed = -4;
			if( gCubeXPos > 100 && gCubeXPos <= 120 ) cubeSwSpeed = -3;
			if( gCubeXPos > 120 && gCubeXPos <= 140 ) cubeSwSpeed = -2;
			if( gCubeXPos > 140 && gCubeXPos <= 160 ) cubeSwSpeed = -1;
			
			if( gCubeXPos > 160 && gCubeXPos <= 180 ) cubeSwSpeed = 1;
			if( gCubeXPos > 180 && gCubeXPos <= 200 ) cubeSwSpeed = 2;
			if( gCubeXPos > 200 && gCubeXPos <= 220 ) cubeSwSpeed = 3;
			if( gCubeXPos > 220 && gCubeXPos <= 240 ) cubeSwSpeed = 4;
			if( gCubeXPos > 240 && gCubeXPos <= 260 ) cubeSwSpeed = 6;
			if( gCubeXPos > 260 && gCubeXPos <= 280 ) cubeSwSpeed = 8;
			if( gCubeXPos > 280 && gCubeXPos <= 300 ) cubeSwSpeed = 10;
			if( gCubeXPos > 300 ) cubeSwSpeed = 12;
	
			aCorner[0].pt_X += cubeSwSpeed;
			aCorner[0].pt_Y += cubeYSpeed+cubeYSpeed+8;
			aCorner[1].pt_X += cubeSwSpeed;
			aCorner[1].pt_Y += cubeYSpeed+cubeYSpeed+8;
			aCorner[2].pt_X += cubeSwSpeed;
			aCorner[2].pt_Y += cubeYSpeed+cubeYSpeed+8;
			aCorner[3].pt_X += cubeSwSpeed;
			aCorner[3].pt_Y += cubeYSpeed+cubeYSpeed+8;
	
			if( aCorner[2].pt_Y >= 170 )	aCorner[2].pt_Y = 180;
			if( aCorner[3].pt_Y >= 170 )	aCorner[3].pt_Y = 180;
	
			if( aCorner[0].pt_X <= 40 )		aCorner[0].pt_X = 40;
			if( aCorner[1].pt_X <= 40 )		aCorner[1].pt_X = 40;
			if( aCorner[2].pt_Y <= 40 )		aCorner[2].pt_X = 40;
			if( aCorner[3].pt_Y <= 40 )		aCorner[3].pt_X = 40;
			if( aCorner[0].pt_X >= 280 )	aCorner[0].pt_X = 280;
			if( aCorner[1].pt_X >= 280 )	aCorner[1].pt_X = 280;
			if( aCorner[2].pt_Y >= 280 )	aCorner[2].pt_X = 280;
			if( aCorner[3].pt_Y >= 280 )	aCorner[3].pt_X = 280;
			}
		MapCel(ccbCubeS, aCorner);

		ccbCubeR = GetAnimCel(gCubeRAnimRec,gCubeFrameIncr);
		aCorner[0].pt_X -= cubeYSpeed;
		aCorner[0].pt_Y = 190+cubeYSpeed;
		aCorner[1].pt_X +=cubeYSpeed;
		aCorner[1].pt_Y = 190+cubeYSpeed;
		aCorner[2].pt_X += cubeYSpeed;
		aCorner[2].pt_Y = 200+cubeYSpeed;
		aCorner[3].pt_X -= cubeYSpeed;
		aCorner[3].pt_Y = 200+cubeYSpeed;
		MapCel(ccbCubeR, aCorner);



//@@@@@@@@@@@@@@@@@@@@@
		if( gLightON )
			CopyVRAMPages( VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer, gWallOnPict, sc->sc_nFrameBufferPages, -1 );
			else
				CopyVRAMPages( VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer, gWallOffPict, sc->sc_nFrameBufferPages, -1 );

		if( gBouncing ) {
			if( gLightON ) {
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallS);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallR);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBall);
				} else {
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBall);			
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallS);
				}

			if( gLightON ) {
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvS);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvR);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTv);
				} else {
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTv);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvS);
					}		
					
			if( gLightON ) {
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeS);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeR);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCube);
				} else {
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCube);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeS);
					}
					
			if( gLightON ) {
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeR);
				retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobe);
				} else {
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobe);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
					}
					
			retvalue = DisplayScreen( sc->sc_Screens[ gScreenSelect ], 0 );
			gScreenSelect = 1 - gScreenSelect;
			} else {
				
				if( !globeLftDir ) {
					if( gLightON ) {
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeR);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobe);
						} else {
							retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobe);
							retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
							retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
							}
						}
				if( gLightON ) {
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallR);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvR);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeS);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeR);
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBall);	
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTv);	
					retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCube);	
					} else {						
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBall);	
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallS);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbBallS);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTv);	
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvS);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbTvS);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCube);	
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeS);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbCubeS);
						}
				if( globeLftDir ) {
					if( gLightON ) {
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeR);
						retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobe);						
						} else {
							retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobe);
							retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
							retvalue = DrawCels (sc->sc_BitmapItems[ gScreenSelect ], ccbGlobeS);
							}
						}
				retvalue = DisplayScreen( sc->sc_Screens[ gScreenSelect ], 0 );
				gScreenSelect = 1 - gScreenSelect;
		
				}
		
		}

DONE: 
	FadeToBlack( sc, FADE_FRAMECOUNT );
	TermBounceSound();
	kprintf( "\n\n3DO-Bounce w/sound has completed!" );
	return( (int)retvalue );
}


/*  Miscellaneous Utility Routines */

bool
DoJoystick( void )
{
	bool 	retvalue;
	long 	joybits;

	retvalue = TRUE;

	joybits = ReadControlPad( JOYCONTINUOUS );
	if ( joybits & JOYSTART ) {
		retvalue = FALSE;
		goto DONE;
		}
	
	/* With FIRE A adjust animation delay and CCB's P-Mode setting */
	if ( joybits & JOYFIREA ) {
		if( !gBouncing ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gBouncing = TRUE;
			gTvFrameIncr  = Convert32_F16(1);
			gTvFrameIncr -= 0x00006000;
			gKeysDown = TRUE;			
			goto DONE;
			} else {
				if ( gKeysDown == TRUE ) goto DONE;
				gLightON = !gLightON;
				gKeysDown = TRUE;			
				goto DONE;
				}
			
		if ( joybits & JOYLEFT ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;			
			goto DONE;
		}
		else if ( joybits & JOYRIGHT ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;
			goto DONE;
		}
		else if ( joybits & JOYUP ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;
			goto DONE;
		}
		else if ( joybits & JOYDOWN ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;
			goto DONE;
		}
	}

	/* With FIRE B ... */
	if ( joybits & JOYFIREB ) {
			gCollision = TRUE;
			if ( gKeysDown == TRUE ) goto DONE;
			kprintf( "\ncollison on - A to start bounce. PAD to move objects." );
			if( gBouncing ) {
				gBouncing = FALSE;
				gGlobeXPos = 50;
				gGlobeYPos = 50;
				gCubeXPos = 150;
				gCubeYPos = 15;
				gTvXPos = 145;
				gTvYPos = 60;
				gBallXPos = 150;
				gBallYPos = 100;
				gKeysDown = TRUE;
				goto DONE;
				}
				
		if ( joybits & JOYRIGHT ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;
			goto DONE;
		}
		else if ( joybits & JOYLEFT ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;
			goto DONE;
		}
		else if ( joybits & JOYUP ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;
			goto DONE;
		}
		else if ( joybits & JOYDOWN ) {
			if ( gKeysDown == TRUE ) goto DONE;
			gKeysDown = TRUE;
			goto DONE;
		}
	}

	/* If the select button (Button C) is pressed advance to the next select state */
	if ( joybits & JOYSELECT )
		{
			gCollision = FALSE;
			if( gBouncing )  {
				if ( gKeysDown == TRUE ) goto DONE;
				kprintf( "\ncollison off - A to start bounce. PAD to move objects." );
				gBouncing = FALSE;
				gTvFrameIncr    = Convert32_F16(1);
				gTvFrameIncr    = 0x00000000;
				gKeysDown = TRUE;			
				goto DONE;
				}
		}


	if ( joybits & (JOYUP|JOYDOWN|JOYLEFT|JOYRIGHT) )
		{
		if ( joybits & JOYUP )
			{
			if (--gGlobeYPos < 0) gGlobeYPos = 0;
			if (++gTvYPos > 200) gTvYPos = 200;
			if (--gCubeYPos < 0) gCubeYPos = 0;
			}
		else if ( joybits & JOYDOWN )
			{
			if (++gGlobeYPos > 200) gGlobeYPos = 200;
			if (--gTvYPos < 0) gTvYPos = 0;
			if (++gCubeYPos > 200) gCubeYPos = 200;
			}
		if ( joybits & JOYLEFT )
			{
			if (--gGlobeXPos < 0) gGlobeXPos = 0;
			if (--gBallXPos < 0) gBallXPos = 0;
			if (++gTvXPos > 270) gTvXPos = 270;
			if (++gCubeXPos > 270) gCubeXPos = 270;
			}
		else if ( joybits & JOYRIGHT )
			{
			if (++gGlobeXPos > 270) gGlobeXPos = 270;
			if (++gBallXPos > 270) gBallXPos = 270;
			if (--gTvXPos < 0) gTvXPos = 0;
			if (--gCubeXPos < 0) gCubeXPos = 0;
			}
		}


	gKeysDown = FALSE;
	
DONE:
	return( retvalue );
}


ulong
Random( ulong n )
{
	ulong i, j, k;

	i = (ulong)rand() << 1;
	j = i & 0x0000FFFF;
	k = i >> 16;
	return ( ( ((j*n) >> 16) + k*n ) >> 16 );
}


/* Rendering and SPORT Support Routines */

void
InitPicts( )
{

	gWallOffPict = (ubyte *)ALLOCMEM( (int)(sc->sc_nFrameByteCount),
			MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

	if ( NOT gWallOffPict) {
		kprintf ( "\nunable to allocate gWallOffPict" );
		exit(0);
		}

	SetVRAMPages( VRAMIOReq, gWallOffPict, 0, sc->sc_nFrameBufferPages, -1 );

	if ( LoadImage( wallOffImg,  gWallOffPict, (VdlChunk **)NULL, sc ) == 0) {
		kprintf ( "\nLoadImage 1 failed" );
		exit(0);
		}
		
	gWallOnPict = (ubyte *)ALLOCMEM( (int)(sc->sc_nFrameByteCount),
			MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

	if ( NOT gWallOnPict) {
		kprintf ( "\nunable to allocate gWallOffPict" );
		exit(0);
		}

	SetVRAMPages( VRAMIOReq, gWallOnPict, 0, sc->sc_nFrameBufferPages, -1 );
	if ( LoadImage( wallOnImg,  gWallOnPict, (VdlChunk **)NULL , sc) == 0) {
		kprintf ( "\nLoadImage  2 failed" );
		exit(0);
		}
		
	if ( ( gGlobeAnimRec = LoadAnim (globePict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nLoading...Globe Failed" );
		exit(0);
		}
	if ( ( gGlobeSAnimRec = LoadAnim (globeSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS1  Failed" );
		exit(0);
		}
	if ( ( gGlobeRAnimRec = LoadAnim (globeSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS2  Failed" );
		exit(0);
		}

	if ( ( gTvAnimRec = LoadAnim (tvPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nLoading...TV Failed" );
		exit(0);
		}
	if ( ( gTvSAnimRec = LoadAnim (tvSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS1  Failed" );
		exit(0);
		}
	if ( ( gTvRAnimRec = LoadAnim (tvSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS2  Failed" );
		exit(0);
		}

	if ( ( gCubeAnimRec = LoadAnim (cubePict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nLoading...Cube Failed" );
		exit(0);
		}
	if ( ( gCubeSAnimRec = LoadAnim (cubeSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS1  Failed" );
		exit(0);
		}
	if ( ( gCubeRAnimRec = LoadAnim (cubeSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS2  Failed" );
		exit(0);
		}

	if ( ( gBallAnimRec = LoadAnim (ballPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nLoading...Ball Failed" );
		exit(0);
		}
	if ( ( gBallSAnimRec = LoadAnim (ballSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS1  Failed" );
		exit(0);
		}
	if ( ( gBallRAnimRec = LoadAnim (ballSPict, MEMTYPE_CEL ) ) == 0 ) {
		kprintf( "\nS2  Failed" );
		exit(0);
		}
		

}


