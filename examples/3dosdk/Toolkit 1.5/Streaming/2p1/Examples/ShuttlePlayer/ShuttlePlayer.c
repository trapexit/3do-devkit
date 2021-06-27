/*******************************************************************************************
 *	File:			Cpak_Player.c
 *
 *	Contains:		high level Cinepak movie playback app
 *
 *	Usage:			Cpak_Player 
 *
 *	Written by:		Lynn Ackler
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  08/22/94		dtc		Version 1.1
 *							Replaced printf with printf.
 *	12/13/93		lla		Fix order of args to CHECK_DS_RESULT()
 *	12/01/93		lla		Made over into a player with menus etc.
 *	10/20/93		jb		New today
 *
 *******************************************************************************************/

#define	PROGRAM_VERSION_STRING		"1.1"
//#define MESG_POOL

#include "Portfolio.h"
#include "Init3DO.h"
#include "Utils3DO.h"

#include "CPakSubscriberS.h"
#include "PlayCPakStream.h"

#include "DataStreamDebug.h"
#include "ShuttlePlayer.h"

#include "joypad.h"

/******************************/
/* Library and system globals */
/******************************/
ScreenContext	gScreenContext;

/******************************/
/* Application globals */
/******************************/
		Boolean			fRunStream;
		Boolean			fCatchupStrategy;
static	char			SelectionName[255], *selectionName = SelectionName;	

/******************************/
/* Utility routine prototypes */
/******************************/
static	Boolean		StartUp( void );

/******************************/
/* Utility routine prototypes */
/******************************/
	   void		EraseScreen ( ScreenContext *sc, int32 screenNum );

extern long 	GetSelection ( ScreenContext *gScreenContextPtr, long LastSelection );
extern char		*movie_name ( long selection );
extern long		LoadScreens ( ScreenContext *gScreenContextPtr );
extern long 	ShowIntro (ScreenContext *gScreenContextPtr);
extern long		DisposeStream (void);
extern int32	initElKabong ( char	*filename );
extern	void	closeElKabong ( void );
#ifndef MESG_POOL
extern long		PlayerFunction( PlayerPtr ctx );
extern long		IntroFunction ( PlayerPtr ctx );
#else
extern long		PlayerFunction( PlayerFuntionContextPtr pfc );
extern long		IntroFunction ( PlayerFuntionContextPtr pfc );

extern long		InitPlayerFunction( PlayerFuntionContextPtr pfc, long numAsyncMsgs );
extern void 	ClosePlayerFunction( PlayerFuntionContextPtr pfc );
#endif
extern	void	SetCpakScreen( int32 horizontalOffset, int32 verticalOffset, Boolean centered );

/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
static Boolean		StartUp( void )
	{
	gScreenContext.sc_nScreens = 3;

	if ( ! OpenGraphics( &gScreenContext, 3 ) )
		return false;

	if ( ! OpenMacLink() );

	if ( ! OpenSPORT() )
		return false;

	if ( (OpenAudioFolio() != 0) )
		return false;

	if ( (OpenMathFolio() != 0) )
		return false;

	if ( (initElKabong ("$boot/Player_Screens/dummy") != 0) )
		return false;
		
	return true;
	}

/*********************************************************************************************
 * Routine to erase the current screen.
 *********************************************************************************************/
void	EraseScreen( ScreenContext *sc, int32 screenNum )
	{
	Item	VRAMIOReq;

	VRAMIOReq = GetVRAMIOReq();
	SetVRAMPages( VRAMIOReq, sc->sc_Bitmaps[screenNum]->bm_Buffer, 0, sc->sc_nFrameBufferPages, -1 );
	DeleteItem( VRAMIOReq );
	}

/*********************************************************************************************
 * Routine to wait in audio time.
 *********************************************************************************************/

void WaitAudioClicks ( AudioTime clicks )
	{
	AudioTime	endClicks;
	
	endClicks = GetAudioTime () + clicks;
	while ( GetAudioTime () < endClicks ) ;
	}

/*********************************************************************************************
 * Routine to return movies name.
 *********************************************************************************************/

char *movie_name ( long selection )
	{
/*	switch( selection )
		{
		case  0:	return "$boot/Movie_1.stream";
		case  1:	return "$boot/Movie_2.stream";
		case  2:	return "$boot/Movie_3.stream";
		case  3:	return "$boot/Movie_4.stream";
		case  4:	return "$boot/Movie_5.stream";
		case  5:	return "$boot/Movie_6.stream";
		default: return	"Bad File Name Index";

		}
*/
	if ( selection < 0 || selection >= N_TITLES )
		return	"Bad File Name Index";
		
	sprintf ( selectionName, "$boot/Player_Movies/Movie_%ld.stream", selection );
	return (selectionName);
	}
/*********************************************************************************************
 * Main program
 *********************************************************************************************/
int	 main( int argc, char **argv )
	{
	long					status = 0;
	long					streamFile;
	long					LastSelection;
	JoyPadState 			jpState;
#ifdef MESG_POOL
	PlayerFuntionContext 	PFC, *pfc = &PFC;
	long					numAsyncMsgs;
#endif


	/* Initialize the library code */
	if ( ! StartUp() )
		{
		printf( "StartUp() initialization failed!\n" );
		exit(0);
		}
		
#ifdef MESG_POOL
	numAsyncMsgs = 1000;
#endif
	
	SetJoyPadContinuous( PADARROWS | PADSHIFT | ControlC, 1 );
//	SetJoyPadContinuous( PADSHIFT | ControlC, 1 );
	

//------------------------------------------------------------------------------------------------
//
//			Show Intro screens
//			and load menu and help screens
//
//------------------------------------------------------------------------------------------------

	status = ShowIntro ( &gScreenContext );
	
	LastSelection = 0;
	status = 0;
#ifdef MESG_POOL
			status = InitPlayerFunction( pfc, numAsyncMsgs );
			CHECK_DS_RESULT( "InitPlayerFunction", status );
#endif
	while ( true )
		{
		/* Display menu and get movie */
		FadeToBlack ( &gScreenContext, 10);
		streamFile = GetSelection (&gScreenContext, LastSelection);
		LastSelection = streamFile;
		
		/* Play the movie */
		
			status = 0;
			fRunStream = true;
			fCatchupStrategy = true;

			gScreenContext.sc_curScreen = movieScreen;
			DisplayScreen(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 0);
			FadeFromBlack ( &gScreenContext, 10);

			while ( status == kMovieOK )
				{
				SetCpakScreen ( 0,0, true);
				status = PlayCPakStream( &gScreenContext, movie_name( streamFile ),
										(PlayCPakUserFn) PlayerFunction, &jpState, pfc );
				CHECK_DS_RESULT( "PlayCPakStream", status );
				}
				EraseScreen ( &gScreenContext, movieScreen ); 
		}
#ifdef MESG_POOL
			ClosePlayerFunction( pfc );
#endif
	closeElKabong ();
	exit(0);
	}


