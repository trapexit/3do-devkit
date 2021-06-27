/*******************************************************************************************
 *	File:			UserControls.c
 *
 *	Contains:		routines for interpreting the keypad activities
 *
 *	Written by:		Lynn Ackler
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  08/22/94		dtc		Version 1.1
 *							Replaced printf with printf.
 *	3/7/94			lla		Added GOMARKER_FWD_POS_RELATIVE & GOMARKER_BWD_POS_RELATIVE for fast fwd & bwd
 *	3/2/94			lla		Added block skipping for fast fwd & bwd
 *	12/10/93		lla		Added fast forward and reverse
 *	12/01/93		lla		New today
 *
 *******************************************************************************************/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Utils3DO.h"

#include "DataStreamDebug.h"
#include "PlayCPakStream.h"

#include "ShuttlePlayer.h"
#include "joypad.h"

/******************************/
/* Library and system globals */
/******************************/
extern	ScreenContext	gScreenContext;

/******************************/
/* Application globals */
/******************************/
extern	Boolean			fRunStream;
extern	Boolean			fCatchupStrategy;

/******************************/
/* Utility routine prototypes */
/******************************/
extern	void			WaitABit(long b);
extern	void			EraseScreen( ScreenContext *sc, int32 screenNum );
extern	void 			WaitAudioClicks ( AudioTime clicks );
extern	int32 			performElKabong ( void *buf );
		void 			ClosePlayerFunction( PlayerFuntionContextPtr pfc );

//------------------------------------------------------------------------------------
//
//		These functions are called each time through the player's main loop and can be used to
//		implement any user interface actions necessary. Returning a zero causes playback to
//		continue. Any other value causes playback to cease and immediately return from
//		the PlayCPakStream() function.
//
//------------------------------------------------------------------------------------
//************************************************************************************
//
//			The "PlayerFunction" is the user controls for the main movie player
//			"X"			- Back to menu
//			"P"			- toggles pause and run
//			"C"			- Displays help menu
//			"Shift X"	- rewinds the movie
//
//			"Right ->"	- Fast Forward
//			"Left <-"	- Fast Reverse
//			"Shift & Right ->"	- Very Fast Forward
//			"Shift & left ->"	- Very Fast Reverse
//
//		Pause
//			"Right ->"	- Step Forward
//
//************************************************************************************

#define		SHIFT_KEY				(joypadStatePtr->rightShift || joypadStatePtr->leftShift)
#define		steppingClicks			60
#define		skippingClicks			60
#define		gFwdBlockSkip			10
#define		gBwdBlockSkip			20

long PlayerFunction ( PlayerFuntionContextPtr pfc )
	{
	long				status;
	JoyPadState 		joypadState, *joypadStatePtr = &joypadState;
	void				*nullBuffer;
	SAudioCtlBlock		CtlBlockPtr, *ctlBlockPtr = &CtlBlockPtr;
	long				skipBlocks;
	PlayerPtr 			ctx;
	DSRequestMsgPtr 	reqMsg;

	long				temp, poll_status;
	
	nullBuffer = NULL;

	ctx = pfc->playerCtx;
	status = 0;
	
	GetJoyPad( joypadStatePtr, 1 );

	if ( joypadStatePtr->xBtn )
		{
//------------------------------------------------------------------------------------
//			The "stop button" and "right shift" or "left shift" rewinds movie
//------------------------------------------------------------------------------------
		 if ( SHIFT_KEY )
			{
			if ( !fRunStream )
				{
//				status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
//				CHECK_DS_RESULT( "DSStartStream", status );
			ctlBlockPtr->mute.channelNumber = 0;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
												 kSAudioCtlOpUnMute, ctlBlockPtr );
				fRunStream = true;
				}
	
	
			/* Go backward in the stream */
			status = DSGoMarker( ctx->messageItem, 
								NULL, 
								ctx->streamCBPtr, 
								0,
								GOMARKER_ABSOLUTE );
	
			if ( status == kDSRangeErr || status == kDSBranchNotDefined  )
				{
				status = performElKabong ( nullBuffer );
				if ( !status ) status = kRewindMovie;
				}

			CHECK_DS_RESULT( "Rewind: DSGoMarker", status );
			status = kDSContinuousFrame;
			}

//------------------------------------------------------------------------------------
//			The "stop button" only returns to the menu
//------------------------------------------------------------------------------------
		else
			{
	
			status = DSStopStream( ctx->messageItem, false, ctx->streamCBPtr, SOPT_FLUSH );
			CHECK_DS_RESULT( "DSStopStream: PlayerFunction", status );
			fRunStream = false;

			status = performElKabong ( nullBuffer );
			if ( !status ) status = kStopMovie;
			}
		}
	
//------------------------------------------------------------------------------------
//			The "start Button" button toggles running the stream
//------------------------------------------------------------------------------------

	else if ( joypadStatePtr->startBtn )
		{
		/* If we're transitioning from stopped to running, start the
		 * stream. Otherwise, stop the stream.
		 */
		if ( fRunStream )
			{
			ctlBlockPtr->mute.channelNumber = 0;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
									 kSAudioCtlOpMute, ctlBlockPtr );
			status = kDSStepFrame;
			}
		else
			{
			ctlBlockPtr->mute.channelNumber = 0;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
												 kSAudioCtlOpUnMute, ctlBlockPtr );
			status = kDSContinuousFrame;
			}
		/* Flip the logical value of the "stream running" flag
		 */
		fRunStream = !fRunStream;
		}
	
//------------------------------------------------------------------------------------
//			The "C Button" displays help screen
//------------------------------------------------------------------------------------

	else if ( joypadStatePtr->cBtn )
		{
		if ( fRunStream )
			{
			ctlBlockPtr->mute.channelNumber = 0;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
									 kSAudioCtlOpMute, ctlBlockPtr );
			status = kDSStepFrame;
			}
		DisplayScreen(gScreenContext.sc_Screens[helpScreen], 0);

		while ( joypadStatePtr->cBtn )
			GetJoyPad( joypadStatePtr, 1 );
			
		DisplayScreen(gScreenContext.sc_Screens[movieScreen], 0);

		if ( fRunStream )
			{
			ctlBlockPtr->mute.channelNumber = 0;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
												 kSAudioCtlOpUnMute, ctlBlockPtr );
			status = kDSContinuousFrame;
			}
		}

	
	else if ( joypadStatePtr->bBtn )
		{
		fCatchupStrategy = fCatchupStrategy;
		}

//------------------------------------------------------------------------------------
//			We are in pause mode
//			therefore the pad is for step frame operation
//------------------------------------------------------------------------------------
	else if ( !fRunStream )
		{
		if ( joypadStatePtr->rightArrow )
			{
			WaitAudioClicks ( (AudioTime) steppingClicks );
			status = kDSDrawSingleFrame;
			}
		else if ( joypadStatePtr->leftArrow )
			{
			}
		}
//------------------------------------------------------------------------------------
//			We are in run mode
//			therefore the pad is for fast forward/reverse
//------------------------------------------------------------------------------------
	else
		{
//------------------------------------------------------------------------------------
//			The "Right arrow" fast forwards at least one second at a time
//------------------------------------------------------------------------------------

		if ( joypadStatePtr->rightArrow)
			{
			if ( SHIFT_KEY ) skipBlocks = 5*gFwdBlockSkip;
			else skipBlocks = gFwdBlockSkip;

			/* Go forward in the stream */

			reqMsg = (DSRequestMsgPtr) AllocPoolMem( pfc->dsMsgPool );
			if ( reqMsg )
				{
				status = DSSetSkipMode( reqMsg->msgItem, 
									 reqMsg,
									 ctx->streamCBPtr, 
									 skipBlocks,
									 GOMARKER_FWD_POS_RELATIVE );

				if ( status == kDSRangeErr || status == kDSBranchNotDefined  )
					{
					status = performElKabong ( nullBuffer );
					if ( !status ) status = kStopMovie;
					}
				else
					CHECK_DS_RESULT( "forward: DSGoMarker", status );
				}
			WaitAudioClicks ( (AudioTime) skippingClicks );	
			}
	
//------------------------------------------------------------------------------------
//			The "Down & Left arrow" fast reverses at least one second at a time
//------------------------------------------------------------------------------------

		else if ( joypadStatePtr->leftArrow )
			{
			if ( SHIFT_KEY ) skipBlocks = 5*gBwdBlockSkip;
			else skipBlocks = gBwdBlockSkip;
	
			/* Go backward in the stream */
			
			reqMsg = (DSRequestMsgPtr) AllocPoolMem( pfc->dsMsgPool );
			if ( reqMsg )
				{
				status = DSSetSkipMode( reqMsg->msgItem, 
									 reqMsg,
									 ctx->streamCBPtr, 
									 skipBlocks,
									 GOMARKER_BWD_POS_RELATIVE );

				if ( status == kDSRangeErr || status == kDSBranchNotDefined  )
					{
					status = performElKabong ( nullBuffer );
					if ( !status ) status = kRewindMovie;
					}
				else
					CHECK_DS_RESULT( "backward: DSGoMarker", status );
				}
			WaitAudioClicks ( (AudioTime) skippingClicks );
			}	
		}

//------------------------------------------------------------------------------------
//			flush joy pad
//------------------------------------------------------------------------------------

	GetJoyPad( joypadStatePtr, 1 );

//------------------------------------------------------------------------------------
//			Poll for any completion messages and return them to the pool
//------------------------------------------------------------------------------------

	while ( (temp = PollForMsg( pfc->replyPort, NULL, NULL, (void**) &reqMsg, &poll_status )) != false )
		{
//		printf ("PollForMsg:%d, %d <0x%x>\n", temp, poll_status, poll_status);
		ReturnPoolMem( pfc->dsMsgPool, reqMsg );
		}

//------------------------------------------------------------------------------------
//			Return the status if all else fails
//------------------------------------------------------------------------------------

	return (status);
	}

//************************************************************************************
//
//	InitPlayerFunction -- one time initialization for the player functions
//		Allocates a pool of streamer request message buffers, a reply port,
//		and message items to correspond with the request message buffers. 
//		These are dynamically allocated by the player function as needed
//		(when there is user interaction with the joypad), and are returned
//		to the pool by polling the reply port in the callback routine.
//		The corresponding cleanup routine, ClosePlayerFunction(), should
//		be called upon completion of use of the player function.
//
//************************************************************************************

long InitPlayerFunction( PlayerFuntionContextPtr pfc, long numAsyncMsgs )
	{
	long	status;

	status = 0;

	/* Init a couple of things in case of an error exit */
	pfc->dsMsgPool = NULL;
	pfc->replyPort = 0;

	/* Create a memory pool to hold streamer request messages
	 * that will be used for async communications with the streamer.
	 */
	pfc->dsMsgPool = CreateMemPool( numAsyncMsgs, sizeof(DSRequestMsg) );
	if ( pfc->dsMsgPool == NULL )
		{
		printf( "InitPlayerFunction: CreateMemPool() failed\n" );
		status = -1;
		goto BAILOUT;
		}

	/* Create a reply port for completed streamer requests to
	 * be posted to when operations complete.
	 */
	pfc->replyPort = NewMsgPort( NULL );
	if ( pfc->replyPort < 0 )
		{
		printf( "InitPlayerFunction: NewMsgPort() failed\n" );
		status = -2;
		goto BAILOUT;
		}

	/* Fill each message pool entry with a message item such
	 * that there is one message item per message buffer.
	 */
	if ( ! FillPoolWithMsgItems( pfc->dsMsgPool, pfc->replyPort ) )
		{
		printf( "InitPlayerFunction: FillPoolWithMsgItems() failed\n" );
		status = -3;
		goto BAILOUT;
		}

	return 0;

BAILOUT:
	ClosePlayerFunction( pfc );
	return status;
	}


//************************************************************************************
//
//	ClosePlayerFunction -- cleanup of one time initialization for the player functions
//
//************************************************************************************
	static Boolean	_FreeMsgItem( void* argumentNotUsed, void* poolEntry )
		{
		DeleteItem( ((DSRequestMsgPtr) poolEntry)->msgItem );
		return true;
		}

void ClosePlayerFunction( PlayerFuntionContextPtr pfc )
	{
	DSRequestMsgPtr reqMsg;


	/* Delete any message items associated with the pool,
	 * then delete the pool itself.
	 */
	if ( pfc->dsMsgPool )
		{
		/* Wait for any pending asynchronous replys and return
		 * each reply to the pool until all have returned.
		 */
		while ( pfc->dsMsgPool->numFreeInPool < pfc->dsMsgPool->numItemsInPool )
			{
			WaitForMsg( pfc->replyPort, NULL, NULL, (void**) &reqMsg, 0 );
			ReturnPoolMem( pfc->dsMsgPool, reqMsg );
			}

		/* Free up all message items and free the pool memory */
		ForEachFreePoolMember( pfc->dsMsgPool, _FreeMsgItem, 0 );
		DeleteMemPool( pfc->dsMsgPool );
		}

	/* Delete the reply port */
	if ( pfc->replyPort )
		RemoveMsgPort( pfc->replyPort );
	}
