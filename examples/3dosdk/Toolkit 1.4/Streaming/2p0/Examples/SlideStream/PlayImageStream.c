/*******************************************************************************************
 *	File:			PlayImageStream.c
 *
 *	Contains:		high level image stream playback function
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/3/93			jb		Change test for stream header version to != from >
 *	12/2/93			jb		Add addition new arg to NewDataStream()
 *	12/1/93			jb		Switch to using memcpy() from CopyVRAMPages() since the
 *							latter requires page aligned VRAM sources *only*.
 *	11/30/93		jb		Added CloseXXXX() routines in DismantlePlayer() routine.
 *	11/29/93		jb		New today from PlayCPakStream.c
 *
 *******************************************************************************************/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Utils3DO.h"
#include "UMemory.h"

#include "DataStreamDebug.h"
#include "DataStreamLib.h"
#include "DataAcq.h"
#include "SAudioSubscriber.h"
#include "ControlSubscriber.h"
#include "JoinSubscriber.h"

#include "PlayImageStream.h"
#include "DSStreamHeader.h"

#include "Debug3DO.h"


/************************************/
/* Local utility routine prototypes */
/************************************/
static int32	InitImagePlayerFromStreamHeader( PlayerPtr ctx, char* streamFileName );
void			DismantlePlayer( PlayerPtr ctx );


/*******************************************************************************************
 * Routine to play an chunified IMAG stream file. Stream may or may not contain audio data.
 * Audio channel selection is based upon info found in the stream header.
 *******************************************************************************************/
int32	PlayImageStream( ScreenContext *screenContextPtr, char* streamFileName,
							PlayerUserFn userFn, void *userContext )
	{
	int32			status;
	int32			playerResult;
	PlayerPtr		ctx;
	Player			playerContext;
	DSRequestMsg	EOSMessage;
	Bitmap 			*bitmap;
	JoinElementMsgPtr	jp;

	/* Open the specified file, locate the stream header, and initialize
	 * all resources necessary for performing stream playback.
	 */
	status = InitImagePlayerFromStreamHeader( &playerContext, streamFileName );
	if ( status != 0 )
		return status;

	ctx = &playerContext;

	/* Remember the screen context pointer that we will use
	 * for drawing purposes.
	 */
	ctx->screenContextPtr = screenContextPtr;

	/* Remember the user function pointer, if any */
	ctx->userFn = userFn;
	ctx->userContext = userContext;

	/* Start the stream running */
	status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
	CHECK_DS_RESULT( "DSStartStream", status );
	if ( status != 0 )
		return status;

	/* Register for end of stream notification */
	status = DSWaitEndOfStream( ctx->endOfStreamMessageItem, &EOSMessage, ctx->streamCBPtr );
	CHECK_DS_RESULT( "DSWaitEndOfStream", status );
	if ( status != 0 )
		return status;

	/* Display the screen we will be drawing directly into.
	 * Do this only once to keep the overhead (and aditional implicit
	 * graphics folio WaitVBL() from happening).
	 */
	DisplayScreen( ctx->screenContextPtr->sc_Screens[ctx->screenContextPtr->sc_curScreen], 0 );

	/* Get a pointer to the bitmap we will draw into */
	bitmap = ctx->screenContextPtr->sc_Bitmaps[ ctx->screenContextPtr->sc_curScreen ];

	playerResult = 0;

	while ( true )
		{
		/* If a user function is defined, then call it. If the user function
		 * returns a non-zero value, then we should exit.
		 */
		if ( ctx->userFn )
			{
			playerResult = (*ctx->userFn)( (void *) ctx, ctx->userContext );

			if ( playerResult )
				break;
			}

		/* Check for end of stream and exit the display loop if we're done...
		 */
		if ( PollForMsg( ctx->messagePort, NULL, NULL, NULL, &status ) )
			break;


		/* Poll for the arrival of a new completed join buffer. If we
		 * get one, free up the old one after displaying the newer one.
		 */
		if ( (jp = GetJoinElement( ctx->joinContextPtr, ctx->joinPort )) != NULL )
			{
#if 0
			/* SPORT the background background image into the current screen buffer */
			CopyVRAMPages( ctx->VRAMIOReq, ctx->screenContextPtr->sc_Bitmaps[ 
								ctx->screenContextPtr->sc_curScreen ]->bm_Buffer, 
								jp->dataPtr, ctx->screenContextPtr->sc_nFrameBufferPages, -1 );
#else
			memcpy( ctx->screenContextPtr->sc_Bitmaps[ 
						ctx->screenContextPtr->sc_curScreen ]->bm_Buffer, 
						jp->dataPtr, jp->dataSize );

			//printf( "memcpy( dest: %lX, src: %lX, count: %ld )\n",
			//	ctx->screenContextPtr->sc_Bitmaps[ 
			//		ctx->screenContextPtr->sc_curScreen ]->bm_Buffer,
			//		jp->dataPtr, jp->dataSize );
#endif
			/* Display the image we just copied into the offscreen buffer */
			playerResult = DisplayScreen( ctx->screenContextPtr->sc_Screens[ 
										ctx->screenContextPtr->sc_curScreen ], 0 );
			if ( playerResult < 0 )
				{
				CHECKRESULT( "DisplayScreen() failed, error=%d\n", playerResult );
				break;
				}

			/* Make the other screen buffer the offscreen buffer that we will copy
			 * the next image into before displaying it.
			 */
			ctx->screenContextPtr->sc_curScreen = 1 - ctx->screenContextPtr->sc_curScreen;

			ReleaseJoinElement( ctx->joinContextPtr, jp );
			}

		else
			/* Do something to waste some time to free the processor */
			WaitVBL( ctx->VBLIOReq, 1 );

		}

	/* Unconditionally stop the stream */
	status = DSStopStream( ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH );
	CHECK_DS_RESULT( "DSStopStream", status );

	/* Don't flush if the subscriber was not present */
	if ( ctx->joinContextPtr != 0 )
		{
		/* Flush anything held by the subscriber */
		FlushJoinSubscriber( ctx->joinContextPtr );
		}

	/* Get rid of stuff we allocated */
	DismantlePlayer( ctx );

	return playerResult;
	}


/*******************************************************************************************
 * Routine to get the stream header info, and perform all necessary allocations
 * and initializations necessary for stream playback.
 *******************************************************************************************/
static int32	InitImagePlayerFromStreamHeader( PlayerPtr ctx, char* streamFileName )
	{
	int32			status;
	long			subscriberIndex;
	long			channelNum;
	SAudioCtlBlock	ctlBlock;
	DSHeaderSubsPtr	subsPtr;
	Boolean			fStreamHasAudio;

	/* Assume no audio */
	fStreamHasAudio = false;

	/* Initialize fields to zero so that cleanup can
	 * tell what has been allocated.
	 */
	ctx->bufferList					= 0;
	ctx->streamCBPtr				= 0;
	ctx->acqContext					= 0;
	ctx->messageItem				= 0;
	ctx->endOfStreamMessageItem 	= 0;
	ctx->messagePort				= 0;
	ctx->controlContextPtr			= 0;
	ctx->audioContextPtr			= 0;
	ctx->joinContextPtr				= 0;
	
	
	if ( (ctx->VBLIOReq = GetVBLIOReq()) < 0 )
		{
		status = ctx->VBLIOReq;
		return status;
		}

	if ( (ctx->VRAMIOReq = GetVRAMIOReq()) < 0 )
		{
		status = ctx->VRAMIOReq;
		return status;
		}

	/* Get the stream header loaded */
	status = FindAndLoadStreamHeader( &ctx->hdr, streamFileName );
	if ( status != 0 )
		return status;


	/* Make sure this playback code is compatible with the version of the
	 * data in the stream file.
	 */
	if ( ctx->hdr.headerVersion != DS_STREAM_VERSION )
		return kPSVersionErr;


	/* Allocate the stream buffers and build the linked list of
	 * buffers that are input to the streamer.
	 */
	ctx->bufferList = CreateBufferList( ctx->hdr.streamBuffers, ctx->hdr.streamBlockSize );
	if ( ctx->bufferList == NULL )
		return kPSMemFullErr;


	/* We need to create a message port and one message item
	 * to communicate with the data streamer.
	 */
	ctx->messagePort = NewMsgPort( NULL );
	if ( ctx->messagePort < 0 )
		goto CLEANUP;


	ctx->messageItem = CreateMsgItem( ctx->messagePort );
	if ( ctx->messageItem < 0 )
		goto CLEANUP;


	ctx->endOfStreamMessageItem = CreateMsgItem( ctx->messagePort );
	if ( ctx->endOfStreamMessageItem < 0 )
		goto CLEANUP;


	/* Initialize data acquisition for use with 1 file */
	status = InitDataAcq( 1 );
	if ( status != 0 )
		goto CLEANUP;


	/* Initialize data acquisition for the specified file */
	status = NewDataAcq( &ctx->acqContext, streamFileName, ctx->hdr.dataAcqDeltaPri );
	if ( status != 0 )
		goto CLEANUP;

	/* Initialize for 1 streamer thread */
	status = InitDataStreaming( 1 );
	if ( status != 0 )
		goto CLEANUP;

	status = NewDataStream( &ctx->streamCBPtr, 		/* output: stream control block ptr */
					ctx->bufferList, 				/* pointer to buffer list */
					ctx->hdr.streamBlockSize, 		/* size of each buffer */
					ctx->hdr.streamerDeltaPri,		/* streamer thread relative priority */
					ctx->hdr.numSubsMsgs );			/* number of subscriber messages */
	if ( status != 0 )
		goto CLEANUP;

	/* Connect the stream to its data supplier */
	status = DSConnect( ctx->messageItem, NULL, ctx->streamCBPtr, 
							ctx->acqContext->requestPort );
	CHECK_DS_RESULT( "DSConnect()", status );
	if ( status != 0 )
		goto CLEANUP;

	/* Loop through the subscriber descriptor table and initialize all
	 * subscribers specified in the table.
	 */
	for ( subscriberIndex = 0; ctx->hdr.subscriberList[ subscriberIndex ].subscriberType != 0;
			subscriberIndex++ )
		{
		subsPtr = ctx->hdr.subscriberList + subscriberIndex;
		
		switch ( subsPtr->subscriberType )
			{
#if 0
			case FILM_CHUNK_TYPE:
				status = InitCPakSubscriber();
				CHECK_DS_RESULT( "InitCPakSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;

				status = NewCPakSubscriber( &ctx->cpakContextPtr, 1, subsPtr->deltaPriority );
				CHECK_DS_RESULT( "NewCPakSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;

				status = DSSubscribe( ctx->messageItem,		// control msg item
							NULL,							// synchronous call
							ctx->streamCBPtr, 				// stream context block
							(DSDataType) FILM_CHUNK_TYPE, 	// subscriber data type
							ctx->cpakContextPtr->requestPort );	// subscriber message port
				CHECK_DS_RESULT( "DSSubscribe for cinepak", status );
				if ( status != 0 )
					goto CLEANUP;

				status = InitCPakCel( ctx->streamCBPtr,		// Need this for DSGetClock in sub
							ctx->cpakContextPtr,			// The subscriber's context
							&ctx->cpakChannelPtr,			// The channel's context
							0,								// The channel number
							true );							// true = flush on synch msg from DS
				CHECK_DS_RESULT( "InitCPakCel", status );
				if ( status != 0 )
					goto CLEANUP;
				break;
#endif

			case SNDS_CHUNK_TYPE:
				status = InitSAudioSubscriber();
				CHECK_DS_RESULT( "InitCtrlSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;
			
				status = NewSAudioSubscriber( &ctx->audioContextPtr, 
									ctx->streamCBPtr, subsPtr->deltaPriority );
				CHECK_DS_RESULT( "NewSAudioSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;
			
				status = DSSubscribe( ctx->messageItem,				// control msg item
							NULL,									// synchronous call
							ctx->streamCBPtr, 						// stream context block
							(DSDataType) SNDS_CHUNK_TYPE, 			// subscriber data type
							ctx->audioContextPtr->requestPort ); 	// subscriber message port
				CHECK_DS_RESULT( "DSSubscribe for audio subscriber", status );
				if ( status != 0 )
					goto CLEANUP;
				
				fStreamHasAudio = true;
				break;

			case CTRL_CHUNK_TYPE:
				status = InitCtrlSubscriber();
				CHECK_DS_RESULT( "InitCtrlSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;
			
				status = NewCtrlSubscriber( &ctx->controlContextPtr, 
									ctx->streamCBPtr, subsPtr->deltaPriority );
				CHECK_DS_RESULT( "NewCtrlSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;
			
				status = DSSubscribe( ctx->messageItem,				// control msg item
							NULL,									// synchronous call
							ctx->streamCBPtr, 						// stream context block
							(DSDataType) CTRL_CHUNK_TYPE, 			// subscriber data type
							ctx->controlContextPtr->requestPort ); 	// subscriber message port
				CHECK_DS_RESULT( "DSSubscribe for control subscriber", status );
				if ( status != 0 )
					goto CLEANUP;
				break;

			case JOIN_CHUNK_TYPE:
				status = InitJoinSubscriber();
				CHECK_DS_RESULT( "InitJoinSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;
			
				status = NewJoinSubscriber( &ctx->joinContextPtr, subsPtr->deltaPriority );
				CHECK_DS_RESULT( "NewJoinSubscriber", status );
				if ( status != 0 )
					goto CLEANUP;

				status = DSSubscribe( ctx->messageItem,				// control msg item
							NULL,									// synchronous call
							ctx->streamCBPtr, 						// stream context block
							(DSDataType) JOIN_CHUNK_TYPE, 			// subscriber data type
							ctx->joinContextPtr->requestPort ); 	// subscriber message port
				CHECK_DS_RESULT( "DSSubscribe for join subscriber", status );
				if ( status != 0 )
					goto CLEANUP;

				/* Setting this to zero will make the subscriber util code create
				 * a port for us. Since we will be polling this port for the
				 * arrival of new data, we don't need to allocate a signal so
				 * we let the util code allocate a message port for us.
				 */
				ctx->joinPort = 0;
				status = InitJoinPort( ctx->joinContextPtr, &ctx->joinPort, IMAGE_DATA_TYPE );
				CHECK_DS_RESULT( "InitJoinPort", status );
				if ( status != 0 )
					goto CLEANUP;
				break;

			default:
				printf( "InitImagePlayerFromStreamHeader() - unknown subscriber in stream header: '%.4s'\n",
							(char*) &subsPtr->subscriberType );
				status = kPSUnknownSubscriber;
				goto CLEANUP;
			}
		}

	/* If the stream has audio, then do some additional initializations.
	 */
	if ( fStreamHasAudio )
		{
		/* Preload audio instrument templates, if any are specified
		 */
		if ( ctx->hdr.preloadInstList != 0 )
			{
			ctlBlock.loadTemplates.tagListPtr = ctx->hdr.preloadInstList;
		
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
								 kSAudioCtlOpLoadTemplates, &ctlBlock );
			if ( status != 0 )
				goto CLEANUP;
			}
	
		/* Enable any audio channels whose enable bit is set.
		 * NOTE: Channel zero is enabled by default, so we don't check it.
		 */
		for ( channelNum = 1; channelNum < 32; channelNum++ )
			{
			/* If the bit corresponding to the channel number is set,
			 * then tell the audio subscriber to enable that channel.
			 */
			if ( ctx->hdr.enableAudioChan & (1L << channelNum) )
				{
				status = DSSetChannel( ctx->messageItem, NULL, ctx->streamCBPtr, 
										SNDS_CHUNK_TYPE, channelNum, CHAN_ENABLED );
				CHECK_DS_RESULT( "DSSetChannel", status );
				if ( status != 0 )
					goto CLEANUP;
				}
			}
	
		/* Set the audio clock to use the selected channel */	
		ctlBlock.clock.channelNumber = ctx->hdr.audioClockChan;
		status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
								 kSAudioCtlOpSetClockChan, &ctlBlock );
		CHECK_DS_RESULT( "DSControl - setting audio clock chan", status );
		}

	if ( status == 0 )
		return 0;

CLEANUP:
	DismantlePlayer( ctx );
	return status;
	}


/*******************************************************************************************
 * Routine to free all resources associated with a Player structure. Assumes that all
 * relevant fields are set to ZERO when the struct is initialized.
 *
 *	NOTE:	THE ORDER OF THE FOLLOWING DISPOSALS IS IMPORTANT. DO NOT CHANGE UNLESS
 *			YOU KNOW WHAT YOU ARE DOING.
 *
 *******************************************************************************************/
void		DismantlePlayer( PlayerPtr ctx )
	{
	if ( ctx->streamCBPtr )
		DisposeDataStream( ctx->messageItem, ctx->streamCBPtr );

	if ( ctx->acqContext )
		DisposeDataAcq( ctx->acqContext );

	if ( ctx->controlContextPtr )
		{
		DisposeCtrlSubscriber( ctx->controlContextPtr );
		CloseCtrlSubscriber();
		}

	if ( ctx->joinContextPtr )
		{
		DestroyJoinPort( ctx->joinContextPtr, IMAGE_DATA_TYPE);
		DisposeJoinSubscriber( ctx->joinContextPtr );
		CloseJoinSubscriber();
		}

	if ( ctx->audioContextPtr )
		{
		DisposeSAudioSubscriber( ctx->audioContextPtr );
		CloseSAudioSubscriber();
		}

	if ( ctx->bufferList )
		FreePtr( ctx->bufferList );

	/* Delete any system resources */
	if ( ctx->messageItem )				RemoveMsgItem( ctx->messageItem );
	if ( ctx->endOfStreamMessageItem )	RemoveMsgItem( ctx->endOfStreamMessageItem );
	if ( ctx->messagePort )				RemoveMsgPort( ctx->messagePort );
	if ( ctx->VBLIOReq )				DeleteItem( ctx->VBLIOReq );
	}

