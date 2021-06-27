
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: colorecho.c,v 1.24 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/colorecho
|||	colorecho - Shows interesting visual effects in tune with audio.
|||
|||	  Synopsis
|||
|||	    colorecho [-g] [-m]
|||
|||	  Description
|||
|||	    Displays graphic patterns which are created by drawing a zoomed and
|||	    rotated image of the screen back into the screen. This is similar to the
|||	    effect achieved by pointing a television camera at a monitor showing the
|||	    image from the camera: You see an image of the monitor on the monitor. The
|||	    monitor image contains the same image and so on. This recursive feedback
|||	    can generate fractal patterns in real time.
|||
|||	  Arguments
|||
|||	    -g                           Start without waiting for a button press.
|||
|||	    -m                           Never enter Auto mode.
|||
|||	  Associated Files
|||
|||	    colorecho_tools.c
|||
|||	  Location
|||
|||	    examples/Graphics/colorecho
|||
**/

#include "types.h"
#include "graphics.h"
#include "hardware.h"
#include "audio.h"
#include "event.h"
#include "stdio.h"
#include "displayutils.h"
#include "colorecho.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define VERSION "1.6"


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

/* Loop until the user presses ControlX */
	while(1)
	{
/* Get User input. */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0) {
			PrintError(0,"\\error in","GetControlPad",Result);
		}
		Joy = cped.cped_ButtonBits;
		if(Joy & ControlX) break;

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
				PrintError(0,"\\error in","ce_ProcessUserInput",Result);
				goto DONE;
			}
		}

/* Generate the next video image. */
		Result = ce_DrawNextScreen( sc, ce );
		if (Result < 0)
		{
			PrintError(0,"\\error in","ce_DrawNextScreen",Result);
			goto DONE;
		}

/* Switch double buffered screens. */
		Result = DisplayScreen( sc->sc_Screens[ (sc->sc_curScreen) ], 0 );
		if (Result < 0)
		{
			PrintError(0,"\\error in","DisplayScreen",Result);
			return Result;
		}
		sc->sc_curScreen = 1 - sc->sc_curScreen;

#ifdef USE_SOUND
		Result = cePictureToSound( sc, ce );
		if ( Result < 0 )
		{
			PrintError(0,"\\error in","cePictureToSound",Result);
			return  Result;
		}
#endif

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
	Result = OpenMathFolio();
	if (Result < 0)
	{
		PrintError(0,"open math folio",0,Result);
		printf("Did you run operamath?\n");
		goto DONE;
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, LC_ISFOCUSED);
	if (Result < 0)
	{
		PrintError(0,"init event utility",0,Result);
		goto DONE;
	}

/* Set up double buffered display. */
	Result = CreateBasicDisplay ( &ScreenCon, DI_TYPE_NTSC, 2 );
	if(Result < 0)
	{
		ERR(("CreateBasicDisplay failed 0x%x\n"));
		return (int) Result;
	}


#ifdef USE_SOUND
/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}
#endif

/* Initialize ColorEcho structure. */
	if ( (Result = ce_Init(  &MyCE )) != 0 ) goto DONE;

	if(EnableAuto) MyCE.ce_Flags |= CE_ENABLE_AUTO;

/* Seed random number generator with hardware random . */
	srand(ReadHardwareRandomNumber());


#ifdef USE_SOUND
/* Initialize Sound */
	Result = ceInitSound();
	if ( Result < 0 )
	{
		ERR(("Sound Initialization failed!\n"));
		return (int) Result;
	}
#endif

/* Make the screen we have created visible. */
	ScreenCon.sc_curScreen = 1;
	Result = DisplayScreen( ScreenCon.sc_Screens[0], 0 );
	if ( Result < 0 )
	{
		PrintError(0,"\\error in","DisplayScreen",Result);
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


#ifdef USE_SOUND
	Result = ceStartSound();
	if ( Result < 0 )
	{
		ERR(("Sound Initialization failed!\n"));
		return (int) Result;
	}
#endif

	ce_InteractiveLoop( &ScreenCon, &MyCE );

/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"\\error in","KillEventUtility",Result);
	}

DONE:

#ifdef USE_SOUND
	CloseAudioFolio();
#endif
	printf( "\n%s finished!\n", progname );
	return( (int) Result );
}

