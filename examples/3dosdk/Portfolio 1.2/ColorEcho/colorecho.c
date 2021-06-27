/*
	File:		colorecho.c

	Contains:	"Color Echo" - graphics feedback using LRForm cels

	Written by:	Phil Burk

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 7/28/93	JAY		remove ResetSystemGraphics for Dragon Tail release
		 <1>	 6/25/93	JAY		first checked in

	To Do:
*/

/****************************************************************
**
** "Color Echo" - graphics feedback using LRForm cels
**
** The graphic patterns are created by drawing a zoomed and rotated image of 
** the screen back into the screen. This is similar to the effect achieved by 
** pointing a television camera at a monitor showing the image from the 
** camera. You see an image of the monitor on the monitor. The monitor image 
** contains the same image and so on. This recursive feedback can generate 
** fractal patterns in real time.
**
**
*****************************************************************
** V0.6 930512 PLB   Fixed clipping of YOffset.
** V0.8 930528 PLB   Clean up and factored CE routines
** V0.9 930528 PLB   Auto Mode kicks in after a few seconds.
** V1.0 930528 PLB   -g -m command line options, better Auto.
** V1.1 930528 PLB   Reset IfSport TRUE ech time.
** V1.2 930528 PLB   Use FrameCount to seed random generator.
** V1.3 930620 PLB   Reorganized for color organ.
** V1.4 930623 PLB   Allocate DMA memory for CCB, general cleanup.
** V1.5 930623 PLB   Added Line seed.
****************************************************************/

#include "types.h"
#include "kernel.h"
#include "nodes.h"
#include "mem.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "event.h"
#include "graphics.h"
#include "operamath.h"
#include "Init3DO.h"

#include "colorecho.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define VERSION "1.5"

/* Alias old cardinal2b cel members. */
#ifdef cardinal2b
#define ccb_PIXC ccb_PPMPC
#define ccb_SourcePtr ccb_CelData
#endif


/* *************************************************************************
 * ***  Function Prototypes  ***********************************************
 * *************************************************************************
 */

int32  ce_DrawAboutScreen( ScreenContext *sc );
int32  ce_ProcessUserInput ( ScreenContext *sc, ColorEcho *ce, uint32 Joy );
int32  ce_FakeUserInput( ScreenContext *sc, ColorEcho *ce );
int32  ce_InteractiveLoop( ScreenContext *sc, ColorEcho *ce );



/************************************************************************/
int32 ce_DrawAboutScreen( ScreenContext *sc )
{
	int32 y;
	
	GrafCon GCon;
	
#define LINEHEIGHT (18)
#define LEFTMARGIN (30)
#define NEXTLINE(msg) \
	MoveTo ( &GCon, LEFTMARGIN,y ); \
	y += LINEHEIGHT; \
	DrawText8( &GCon, sc->sc_BitmapItems[0], msg );
	
	y = 28;
	NEXTLINE("Color Echo  ");
	DrawText8( &GCon, sc->sc_BitmapItems[0], VERSION );
	NEXTLINE("(c) 3DO, May 1993");
	NEXTLINE("by Phil Burk");
	NEXTLINE("  Left,Right   = Rotate");
	NEXTLINE("  Up,Down      = Zoom");
	NEXTLINE("  A            = Sprinkle");
	NEXTLINE("  B            = Lines");
	NEXTLINE("  C            = Center");
	NEXTLINE("  LeftShift    = PixelMode");
	NEXTLINE("  RightShift   = X,Y");
	NEXTLINE("Hit any key to continue.");
	
	return 0;
}

/*********************************************************************/

/* Roll binary die based on Prob/1024 */
#define Maybe(Prob) (( ((uint32)rand()) & 1023 ) < Prob)

/**********************************************************************
** Automatic demo mode.
** Randomly change parameters to simulate user unput.
**********************************************************************/
int32 ce_FakeUserInput( ScreenContext *sc, ColorEcho *ce )
{
	int32  Result=0;
	int32 iv;

	ce->ce_PIXC = PPMP_BOTH_AVERAGE;
		
/* Rotate -----------------------------------------*/
	
/* Maybe set random zoom velocity. */
	if(Maybe(10))
	{
		if( ce->ce_Zoom > ZOOMONE )
		{
			ce->ce_ZoomVelocity = Random(8) - 4;
		}
		else
		{
			ce->ce_ZoomVelocity = Random(8) - 3; 
		}
	}
	
/* Maybe set random anglular velocity. */
	if(Maybe(10))
	{
		iv = Random(17) - 8; /* -8 -> 8 */
		ce->ce_AngleVelocity = (ANGLEDELTA*iv)>>2;
	}
		
	
/* Maybe Seed display. */
	if( Maybe(30) )
	{
		ce_Seed( sc, ce );
		if(Maybe(500) )
		{
			ce_Freeze( ce );
		}
	}
	
/* Occasionally ReCenter display. */
	if(Maybe(1))
	{
		ce->ce_Zoom = ZOOMONE;
		ce->ce_Angle = 0;
		ce->ce_XOffset = 0;
		ce->ce_YOffset = 0;
		ce_Freeze( ce );
	}
	
/* Modify X,Y velocity. Only use if zoomed out past 1.0 */
	if(ce->ce_Zoom < ZOOMONE)
	{
		if((Maybe(10)) &&
			(ce->ce_XOffset == 0) &&
			(ce->ce_YOffset == 0))
		{
			ce->ce_XVelocity = Random(3) - 1;
			ce->ce_YVelocity = Random(3) - 1;
		}
		else if (Maybe(40))
		{
			ce->ce_XVelocity = 0;
			ce->ce_YVelocity = 0;
		}
	}
	else
	{
		ce->ce_XOffset = 0;
		ce->ce_YOffset = 0;
		ce->ce_XVelocity = 0;
		ce->ce_YVelocity = 0;
	}
	
	return Result;
}


/************************************************************************/
int32 ce_ProcessUserXY ( ColorEcho *ce, uint32 Joy )
{
	if (Joy & ControlRight)
	{
		ce->ce_XVelocity = 1;
	}
	else if (Joy & ControlLeft)
	{
		ce->ce_XVelocity = -1;
	}
	else
	{
		ce->ce_XVelocity = 0;
	}
	
	if (Joy & ControlDown)
	{
		ce->ce_YVelocity = 1;
	}
	else if (Joy & ControlUp)
	{
		ce->ce_YVelocity = -1;
	}
	else
	{
		ce->ce_YVelocity = 0;
	}
	
	return 0;
}

/************************************************************************/
int32 ce_ProcessUserZoom ( ColorEcho *ce, uint32 Joy )
{
	if (Joy & ControlUp)
	{
		ce->ce_ZoomVelocity = 2;
	}
	else if (Joy & ControlDown)
	{
		ce->ce_ZoomVelocity = -2;
	}
	else
	{
		ce->ce_ZoomVelocity = 0;
	}
	
	if (Joy & ControlLeft)
	{
		ce->ce_AngleVelocity = -ANGLEDELTA;
	}
	else if (Joy & ControlRight)
	{
		ce->ce_AngleVelocity = ANGLEDELTA;
	}
	else
	{
		ce->ce_AngleVelocity = 0;
	}
	
	return 0;
}

/************************************************************************/
int32 ce_ProcessUserInput (  ScreenContext *sc, ColorEcho *ce, uint32 Joy )
{

	int32 Result;
	Result = 0;
	
	ce->ce_IfSport = TRUE;
	
/* Turn Off Doloresizing with Left Shift */
	if (Joy & ControlLeftShift)     /* If ControlC held down, move in X,Y */
	{
		ce->ce_PIXC = PPMP_BOTH_NORMAL;
	}
	else
	{
		ce->ce_PIXC = PPMP_BOTH_AVERAGE;
	}

/* Use JoyPad to spin and zoom cel. */
	if (Joy & ControlRightShift)     /* If RightShift held down, move in X,Y */
	{
		ce_ProcessUserXY( ce, Joy );
	}
	else
	{
		ce_ProcessUserZoom( ce, Joy );
	}

/* Sprinkle rectangles or dots. */
	if (Joy & ControlA)
	{
		ce_Seed( sc, ce );		
	}

/* Display line pattern over image. */
	if (Joy & ControlB)
	{
		if(!(ce->ce_Flags & CE_PATTERN_ON))  /* Edge detect button press. */
		{
			ce->ce_Flags |= CE_PATTERN_ON;
			ce->ce_PatternSeed = ReadHardwareRandomNumber();
		}
		ce_SeedPattern( sc, ce );		
	}
	else
	{
		ce->ce_Flags &= ~CE_PATTERN_ON;  /* Turn off flag. */
	}
	
/* Recenter and freeze motion */
	if (Joy & ControlC)
	{	
		ce_Center( ce );
		ce_Freeze( ce );
	}
	
	return Result;
}

/*********************************************************************/
int32 ce_InteractiveLoop( ScreenContext *sc, ColorEcho *ce )
{
	int32 Result;
	uint32 Joy;
	int32 HandsOffCount;
	ControlPadEventData cped;
	HandsOffCount = 0;
	
/* Loop until the user presses ControlStart */
	while(1)
	{
/* Get User input. */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0) {
			ERR(("Error in GetControlPad\n"));
			PrintfSysErr(Result);
		}
		Joy = cped.cped_ButtonBits;
		if(Joy & ControlStart) break;
		
/* If no events, check to see if it is time for AutoMode */
		if( Joy == 0)
		{
			if(( HandsOffCount > MAXHANDSOFF ) && (ce->ce_Flags & CE_ENABLE_AUTO))
			{
				ce_FakeUserInput( sc, ce );
			}
			else
			{
				HandsOffCount++;
				ce_Freeze( ce );
				ce->ce_PIXC = PPMP_BOTH_AVERAGE;
				ce->ce_Flags &= ~CE_PATTERN_ON;  /* Turn off Line Pattern flag. */
			}
		}
		else
		{
			HandsOffCount = 0;
			Result = ce_ProcessUserInput( sc, ce, Joy );
			if (Result < 0)
			{
				ERR(("main: error in ce_ProcessUserInput\n"));
				PrintfSysErr(Result);
				goto DONE;
			}
		}

/* Generate the next video image. */
		Result = ce_DrawNextScreen( sc, ce );
		if (Result < 0)
		{
			ERR(("main: error in ce_DrawNextScreen\n"));
			PrintfSysErr(Result);
			goto DONE;
		}
		
/* Switch double buffered screens. */
		Result = DisplayScreen( sc->sc_Screens[ (sc->sc_curScreen) ], 0 );
		if (Result < 0)
		{
			PrintfSysErr(Result);
			return Result;
		}
		sc->sc_curScreen = 1 - sc->sc_curScreen;
	}
DONE:
	return Result;
}

/************************************************************************/
/************************************************************************/
/********* Color Echo Application ***************************************/
/************************************************************************/
/************************************************************************/

int main( int argc, char *argv[] )
{
	char *progname;
	int32 Result;
	ColorEcho MyCE;
	char *ptr, c;
	uint8  EnableAuto, WaitToStart;
	ScreenContext ScreenCon;
	ControlPadEventData cped;
	
	EnableAuto = 1;
	WaitToStart = 1;
	
/* Get arguments from command line. */
    progname = *argv++;
    
	printf( "\n%s %s by Phil Burk\n", progname, VERSION );
	printf( "Copyright 1993 3DO\n" );
	printf( "Options:\n" );
	printf( "   -g = Go, don't wait for button press to start.\n" );
	printf( "   -m = Manual, never enter Auto mode.\n" );

    for ( ; argc > 1; argc-- )
	{
        ptr = *argv++;
		c = *ptr++;
		if (c == '-')
        {
        	switch ( c = *ptr++ )
            {
				case 'm':
				case 'M':
					EnableAuto = 0;
					break;
              
                case 'g':
                case 'G':
                	WaitToStart = 0;
                	break;
    
            	default:
					ERR(( "ERROR:  unknown command arg %c\n", c ));
                	break;
			}
		}
	}

/* OpenMathFolio for access to transcendental hardware math. */
#ifdef cardinal2b
	Result = FindMathFolio();
#else
	Result = OpenMathFolio();
#endif
	if (Result < 0)
	{
		printf("OpenMathFolio() failed! Did you run operamath?\n");
		PrintfSysErr(Result);
		goto DONE;
	}
	
/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, LC_FocusListener);
	if (Result < 0)
	{
		ERR(("main: error in InitEventUtility\n"));
		PrintfSysErr(Result);
		goto DONE;
	}
	
/* Set up double buffered display. */
	Result = OpenGraphics( &ScreenCon, 2 );
	if(Result < 0)
	{
		ERR(("OpenGraphics failed 0x%x\n"));
		return (int) Result;
	}

/* Initialize ColorEcho structure. */
	if ( (Result = ce_Init(  &MyCE )) != 0 ) goto DONE;
	
	if(EnableAuto) MyCE.ce_Flags |= CE_ENABLE_AUTO;
	
/* Seed random number generator with hardware random . */
	srand(ReadHardwareRandomNumber());
	
/* Make the screen we have created visible. */
	ScreenCon.sc_curScreen = 1;
	Result = DisplayScreen( ScreenCon.sc_Screens[0], 0 );
	if ( Result < 0 )
	{
		printf("DisplayScreen() failed, error=0x%x\n", Result );
		PrintfSysErr( Result );
		goto DONE;
	}

/* Seed graphics */
	RandomBoxes( ScreenCon.sc_BitmapItems[ 0 ], 99 );
	RandomBoxes( ScreenCon.sc_BitmapItems[ 1 ], 99 );
	
/* Draw the help screen or just start. */
	if (WaitToStart)
	{
		ce_DrawAboutScreen( &ScreenCon );
		GetControlPad (1, TRUE, &cped);   /* Wait for button press. */
	}
	
	ce_InteractiveLoop( &ScreenCon, &MyCE );

/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		ERR(("main: error in KillEventUtility\n"));
		PrintfSysErr(Result);
	}
	
DONE:
	
	printf( "\n%s finished!\n", progname );
	return( (int) Result );
}

