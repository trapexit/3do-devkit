/*******************************************************************************************
 *	File:			PlaySAnim.c
 *
 *	Contains:		Multi-channel streamed animation file player
 *
 *	Usage:			PlaySAnim <SAnimStreamFile> 
 *
 *					"A" button:		Toggles stream stop/start
 *
 *					"B" button:		Toggles printf display of the stream clock
 *
 *					"C" button:		Switches to playing the next stream
 *									in the command line list. Rewinds
 *									the stream in between switching streams.
 *
 *					"Start" button:	exits the program
 *
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	05/24/94	DLD		Added feature to use a default stream header if there
 *						doesn't seem to be one in the input stream.
 *	12/2/93		jb		Add addition new arg to NewDataStream()
 *	9/20/93		jb		Version 1.1
 *						Added multiple channel rendering. Added conditional compilation
 *						for FORCE_CELS_ONTO_SCREEN. 
 *	9/20/93		jb		Version 1.0
 *						Stripped stuff out of TestDS to make this tool which is 
 *						oriented more towards playing animation files created with
 *						the Director Extractor tool.
 *	=======		==		===========
 *	9/15/93		jb		Version 2.3
 *						Add "gSaChannel" global to allow changing of animation
 *						channel number with debugger when playing test streams.
 *	8/25/93		jb		Fix test of uninitialized 'status' after call to InitDataStreaming()
 *						by actually assigning the result of the call to the 'status' variable.
 *	8/11/93		jb		Version 2.2
 *						Add CLOSE_AUDIO_ON_SWITCH code
 *	8/5/93		jb		Version 2.1
 *						Switch to using new interfaces that require the async flag.
 *	8/2/93		jb		Version 2.0
 *						Changes to support changes in Dragontail graphics folio.
 *						Fix bug that happened when Cinepak movie was first file in
 *						command line. Forgot to set audio subscriber clock channel
 *						to match the audio in the Cinepak example data (channel 1).
 *						Use 'curChannelNumber' variable to track the current audio
 *						clock channel, and then let the pan and volume controls
 *						operate on all audio by referring to this channel to make
 *						changes.
 *	6/29/93		ec		Version 1.8.1
 *						Show clock time, frames/sec, and calls/sec on 3DO screen.
 *	6/29/93		jb		Version 1.8
 *						Allow use of channel #1 for Cinepak audio
 *	6/25/93		jb		Version 1.7
 *						Make control subscriber one notch higer than all other subscribers
 *						so that sync/flush will never flush good data.
 *	6/24/93		rdg		Version 1.6
 *	6/23/93		rdg		In main(), send control message to audio subscriber to preload 
 *						instrument templates for 44K_16Bit_Stereo and 44K_16Bit_Mono so
 *						the subscriber won't have to seek for them when the stream starts.
 *						Alter main loop to control volume and panning via control messages
 *						when the arrow buttons are pressed.
 *						Initalize volume and panning using kSAudioCtlOpGetVol and ...Pan.
 *	6/23/93		jb		Version 1.5
 *						Change FindMathFolio() to OpenMathFolio() for Dragon O/S release
 *	6/22/93		jb		Version 1.4
 *						Add flushing option to start/stop stream calls. Flush when
 *						we're stopping and intending to start at a different part
 *						of the stream.
 *	6/19/93		jb		Version 1.3
 *						Display the stream clock with printf as the stream plays
 *	6/1/93		jb		Version 1.2.7
 *						Remove all variable initializers in local variable declarations
 *						to avoid register bashing by the ARM C compiler.
 *						Dispose of data acquisition threads at exit time.
 *	5/30/93		jb		Version 0.2
 *	5/30/93		jb		Change 'debugNext' field in DSDataBuf struct to 'permanentNext'
 *	5/20/93		jb		Integrate Eric's changes and switch to using DSConnect()
 *						to attach data acquisition to a stream.
 *	5/18/93		jb		Remove all references to DSHRunStream() and all the funky
 *						timing stuff that went with it.
 *	4/19/93		jb		Add DSHCloseStream() call at end, change to using updated interface
 *						to NewDataAcq() and NewTestSubscriber().
 *	4/17/93		jb		New today
 *
 *******************************************************************************************/

#include "PlaySAnim.h"
#include "PrepareStream.h"
#include "SAnimSubscriber.h"


/*********************************************************************************************
 * Routine to create a buffer pool to be used by the streamer. All buffers are made to
 * be the same size, bufferSize, and are returned as a linked list.
 *********************************************************************************************/
static DSDataBufPtr	CreateBufferList( long numBuffers, long bufferSize, SAnimPlayerPtr ctx  )
	{
#	define			ROUND_TO_LONG(x)	((x + 3) & ~3)
	DSDataBufPtr	bp;
	DSDataBufPtr	lastBp;
	long			totalBufferSpace;
	long			actualEntrySize;
	
	actualEntrySize = sizeof(DSDataBuf) + ROUND_TO_LONG( bufferSize );
	totalBufferSpace = numBuffers * actualEntrySize;

	bp = (DSDataBufPtr) malloc( totalBufferSpace );
	if ( bp == NULL )
		{
		printf( "malloc() of buffer list space failed!\n" );
		exit(0);
		}

	/* save the raw buffer ptr so we can free it later */
	ctx->rawBufferList = (long *)bp;
	/* Make a linked list of entries */
	lastBp = NULL;
	while ( numBuffers-- > 0 )
		{
		bp->permanentNext	= lastBp;
		bp->next			= lastBp;
		lastBp				= bp;
		
		bp = (DSDataBufPtr) ( ((char *) bp) + actualEntrySize );
		}

	/* Return a pointer to the first buffer in the list 
	 */
	return lastBp;
	}


/*********************************************************************************************
 * Routine to perform high level streaming initialization.
 *********************************************************************************************/
static int32	InitSAnimPlayerFromStreamHeader( SAnimPlayerPtr ctx, char* streamFileName )
	{
	int32			status;
	long			channelNum;
	long			subscriberIndex;
	SAudioCtlBlock	ctlBlock;
	DSHeaderSubsPtr	subsPtr;
	Boolean			fStreamHasAudio;
	Boolean			fStreamHasSANM = false;

	/* Assume no audio */
	fStreamHasAudio = false;

	/* Initialize fields to zero so that cleanup can
	 * tell what has been allocated.
	 */
	ctx->bufferList					= 0;
	ctx->streamCBPtr				= 0;
	ctx->acqContext					= 0;
	ctx->messageItem				= 0;
	//ctx->endOfStreamMessageItem 	= 0;
	ctx->messagePort				= 0;
	ctx->controlContextPtr			= 0;
	ctx->audioContextPtr			= 0;
	ctx->rawBufferList				= 0;
	
	for ( channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++)
		ctx->sanimChannelPtr[channelNum]	= 0;
		
	ctx->sanimContextPtr				= 0;

	if ( (ctx->VBLIOReq = GetVBLIOReq()) < 0 )
		{
		status = ctx->VBLIOReq;
		return status;
		}

	/* Get the stream header loaded */
	status = FindAndLoadStreamHeader( &ctx->hdr, streamFileName );
	
	/* If there was no stream header then use a default */
	if ( status == kPSHeaderNotFound )
		status = UseDefaultStreamHeader ( &ctx->hdr );
		
	/* By now, we must have a stream header */
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
	ctx->bufferList = CreateBufferList( ctx->hdr.streamBuffers, ctx->hdr.streamBlockSize , ctx);
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

	/* Initialize data acquisition for use with 1 file */
	status = InitDataAcq( NUMBER_OF_STREAMS );
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
	CHECK_SA_RESULT( "DSConnect()", status );
	if ( status != 0 )
		goto CLEANUP;


	/* Loop through the subscriber descriptor table and initialize all
	 * subscribers specified in the table.
	 */
	for ( subscriberIndex = 0; ctx->hdr.subscriberList[ subscriberIndex ].subscriberType != 0;
			subscriberIndex++ )
		{
		subsPtr = &ctx->hdr.subscriberList[ subscriberIndex ];
		switch ( subsPtr->subscriberType )
			{
			case SANM_CHUNK_TYPE:
				status = SetupSAnimSubscriber( ctx, subsPtr);
				if ( status != 0 )
					goto CLEANUP;
				fStreamHasSANM = true;
				break;
				
			case SNDS_CHUNK_TYPE:
				status = SetupAudioSubscriber( ctx, subsPtr );
				if ( status != 0 )
					goto CLEANUP;				
				fStreamHasAudio = true;
				break;

			case CTRL_CHUNK_TYPE:
				status = SetupControlSubscriber( ctx, subsPtr );
				if ( status != 0 )
					goto CLEANUP;
				break;

			default:
				printf( "InitSAnimPlayerFromStreamHeader() - unknown subscriber in stream header: '%.4s'\n",
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
		if ( ctx->hdr.preloadInstList[0] != 0 )
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
				CHECK_SA_RESULT( "DSSetChannel", status );
				if ( status != 0 )
					goto CLEANUP;
				}
			}
		}
	else 
		{
		DSHeaderSubs	soundSubscriber = {SNDS_CHUNK_TYPE, kSoundsPriority};

		/* we have to have an audio subscriber! */
		printf( "Warning in Streamed Anim: %s: \n", streamFileName,  status );
		printf( "     No Audio subscriber specified in stream header! \n" );
		status = SetupAudioSubscriber( ctx, &soundSubscriber );				
		}
	
	/* Set the audio clock to use the selected channel */	
	ctlBlock.clock.channelNumber = ctx->hdr.audioClockChan;
	status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
							 kSAudioCtlOpSetClockChan, &ctlBlock );
	CHECK_SA_RESULT( "DSControl - setting audio clock chan", status );

	/* We pretty much need a SANM subscriber too, so let's make
	 *	sure we got one. 
	 */
	if ( ! fStreamHasSANM ) {
		DSHeaderSubs	SANMSubscriber = {SANM_CHUNK_TYPE, kSAnimPriority};

		printf( "Warning in Streamed Anim: %s: \n", streamFileName,  status );
		printf( "     No SANM subscriber specified in stream header! \n" );
		status = SetupSAnimSubscriber( ctx, &SANMSubscriber );
	}
	
	if ( status == 0 )
		return 0;

CLEANUP:
	DismantlePlayer( ctx );
	return status;
	}


static int32
SetupSAnimSubscriber( SAnimPlayerPtr ctx, DSHeaderSubsPtr subsPtr )
	{
	int32 status;
	int32 channelNum;
	
	status = InitSAnimSubscriber();
	CHECK_SA_RESULT( "InitSAnimSubscriber", status );
	if ( status != 0 )
		goto CLEANUP;

	status = NewSAnimSubscriber( &ctx->sanimContextPtr, NUMBER_OF_STREAMS, subsPtr->deltaPriority );
	CHECK_SA_RESULT( "NewSAnimSubscriber", status );
	if ( status != 0 )
		goto CLEANUP;

	status = DSSubscribe( ctx->messageItem,		// control msg item
				NULL,							// synchronous call
				ctx->streamCBPtr, 				// stream context block
				(DSDataType) SANM_CHUNK_TYPE, 	// subscriber data type
				ctx->sanimContextPtr->requestPort );	// subscriber message port
	CHECK_SA_RESULT( "DSSubscribe for sanims", status );
	if ( status != 0 )
		goto CLEANUP;
	
	for ( channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++ )
		{
		status = InitSAnimCel( ctx->streamCBPtr, ctx->sanimContextPtr,	 &ctx->sanimChannelPtr[channelNum], channelNum, true );	
		if ( status != 0 )
			goto CLEANUP;
		}
		
CLEANUP:
	return status;
	}

static int32
SetupAudioSubscriber( SAnimPlayerPtr ctx, DSHeaderSubsPtr subsPtr )
	{
	int32 status;
	
	status = InitSAudioSubscriber();
	CHECK_SA_RESULT( "InitCtrlSubscriber", status );
	if ( status != 0 )
		goto CLEANUP;

	status = NewSAudioSubscriber( &ctx->audioContextPtr, 
						ctx->streamCBPtr, subsPtr->deltaPriority );
	CHECK_SA_RESULT( "NewSAudioSubscriber", status );
	if ( status != 0 )
		goto CLEANUP;

	status = DSSubscribe( ctx->messageItem,				// control msg item
				NULL,									// synchronous call
				ctx->streamCBPtr, 						// stream context block
				(DSDataType) SNDS_CHUNK_TYPE, 			// subscriber data type
				ctx->audioContextPtr->requestPort ); 	// subscriber message port
	CHECK_SA_RESULT( "DSSubscribe for audio subscriber", status );

CLEANUP:
	return status;
	}


static int32
SetupControlSubscriber( SAnimPlayerPtr ctx, DSHeaderSubsPtr subsPtr )
	{
	int32 status;

	status = InitCtrlSubscriber();
	CHECK_SA_RESULT( "InitCtrlSubscriber", status );
	if ( status != 0 )
		goto CLEANUP;

	status = NewCtrlSubscriber( &ctx->controlContextPtr, 
						ctx->streamCBPtr, subsPtr->deltaPriority );
	CHECK_SA_RESULT( "NewCtrlSubscriber", status );
	if ( status != 0 )
		goto CLEANUP;

	status = DSSubscribe( ctx->messageItem,				// control msg item
				NULL,									// synchronous call
				ctx->streamCBPtr, 						// stream context block
				(DSDataType) CTRL_CHUNK_TYPE, 			// subscriber data type
				ctx->controlContextPtr->requestPort ); 	// subscriber message port
	CHECK_SA_RESULT( "DSSubscribe for control subscriber", status );
	
CLEANUP:
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
void		DismantlePlayer( SAnimPlayerPtr ctx )
	{
	long	channelNum;
	
	if ( ctx->streamCBPtr )
		{
		DisposeDataStream( ctx->messageItem, ctx->streamCBPtr );
		CloseDataStreaming();
		}

	if ( ctx->acqContext )
		{
		DisposeDataAcq( ctx->acqContext );
		CloseDataAcq();
		}

	if ( ctx->controlContextPtr )
		{
		DisposeCtrlSubscriber( ctx->controlContextPtr );
		CloseCtrlSubscriber();
		}

	if ( ctx->sanimChannelPtr )
		{
		for (channelNum=0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++)
			{
			DestroySAnimCel( ctx->sanimContextPtr, ctx->sanimChannelPtr[channelNum],  ctx->sanimChannelPtr[channelNum]->channel );
			}
		}

	if ( ctx->sanimContextPtr )
		{
		DisposeSAnimSubscriber( ctx->sanimContextPtr );
		CloseSAnimSubscriber();
		}

	if ( ctx->audioContextPtr )
		{
		DisposeSAudioSubscriber( ctx->audioContextPtr );
		CloseSAudioSubscriber();
		}

	if ( ctx->rawBufferList ) 				free( ctx->rawBufferList );
	if ( ctx->messageItem )				RemoveMsgItem( ctx->messageItem );
	//if ( ctx->endOfStreamMessageItem )	RemoveMsgItem( ctx->endOfStreamMessageItem );
	if ( ctx->messagePort )				RemoveMsgPort( ctx->messagePort );
	if ( ctx->VBLIOReq )				DeleteItem( ctx->VBLIOReq );
	}

/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void Usage( char* programName )
	{
	printf ("%s version %s\n", programName, PROGRAM_VERSION_STRING);
	printf ("usage: %s <streamFile> \n", programName);
	printf ("\t\"A\" button:	Toggles stream stop/start\n");
	printf ("\t\"B\" button:	Toggles stream timing\n");
	printf ("\t\"C\" button:	rewind to start of stream\n");
	}


/*********************************************************************************************
 * Show stream frame timing
 *********************************************************************************************/
static	Item	gTimerRequest;
static	IOInfo	gTimerInfo;

/*
 * start up a timer for ourselves
 */
void	InitInstrumentation(void) 
	{
	Item	timerDevice; 
	IOReq	*timerReq;
	TagArg	ioTags[2];

	ioTags[0].ta_Tag	= CREATEIOREQ_TAG_DEVICE;
	ioTags[0].ta_Arg	= 0;
	ioTags[1].ta_Tag	= TAG_END;
	ioTags[1].ta_Arg	= 0;

	/* Timer initialization */
	timerDevice = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE), "timer"),0);
	if ( timerDevice < 0 ) 
		{
		PrintfSysErr(timerDevice);
		return;
		}

	ioTags[0].ta_Arg = (void *)timerDevice;
	gTimerRequest = CreateItem(MKNODEID(KERNELNODE,IOREQNODE),ioTags);
	
	if ( gTimerRequest < 0 )  
		{
		PrintfSysErr(gTimerRequest);
		exit (0);
		}
	timerReq = (IOReq *)LookupItem(gTimerRequest);
	gTimerInfo.ioi_Unit = 1;
	}


/*
 * Get the current second count from our private timer
 */
uint32	CurrentSeconds(void) 
	{
	static	int32	gTimerHasStarted = 0;
	struct	timeval	tmVal;
	
	/* has the timer been initialized??? */
	if ( gTimerHasStarted == 0 )
		{
		InitInstrumentation();
		gTimerHasStarted = 1;
		}
	
	/* Read elapsed time */
	gTimerInfo.ioi_Command = CMD_READ;
	gTimerInfo.ioi_Recv.iob_Buffer = &tmVal;
	gTimerInfo.ioi_Recv.iob_Len = sizeof(tmVal);
	DoIO(gTimerRequest, &gTimerInfo);
	
	/* we want seconds... */
	return tmVal.tv_sec;
	}
	
/*
 * draw the current timer & stream clock to the screen
 */
void	PutNumXY(int32 callsPerSec, int32 secCount, uint32 streamClock, int32 x, int32 y)
	{
	static 	char	gTimeStr[20];
	static 	char	gClockStr[24];
	static 	char	gCallsPerSecStr[20];
	static	int32	gLastSecCount = -1;
	static	int32	gLastStreamClock = -1;
	static	int32	gLastCallsPerSec = -1;
			GrafCon	localGrafCon;

	/* Don't reset the strings unless we have a new time/clock, just draw the last
	 *  string we made
	 */
	if ( secCount != gLastSecCount )
		{
		sprintf(gTimeStr, "frame/sec: %2lu", secCount);
		gLastSecCount = secCount;
		}

	if ( streamClock != gLastStreamClock )
		{
		if ( streamClock != kInvalidClock )
			sprintf(gClockStr, "stream clock: %2lu", streamClock);
		else
			sprintf(gClockStr, "stream clock: *INVALID*");
		gLastStreamClock = streamClock;
		}

	if ( callsPerSec != gLastCallsPerSec )
		{
		sprintf(gCallsPerSecStr, "calls/sec: %2lu", callsPerSec);
		gLastCallsPerSec = callsPerSec;
		}

#if 0
	localGrafCon.gc_PenX = x;
	localGrafCon.gc_PenY = y;
	DrawText8(&localGrafCon, gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], 
					gTimeStr);
	localGrafCon.gc_PenX = x;
	localGrafCon.gc_PenY = y + 12;
	DrawText8(&localGrafCon, gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], 
					gCallsPerSecStr);
#endif

	localGrafCon.gc_PenX = x;
	localGrafCon.gc_PenY = y + 24;
	DrawText8(&localGrafCon, gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], 
					gClockStr);
	}



/*
 * Calculate and show frame timing 
 */
void	ShowStreamTiming( SAnimPlayerPtr ctx )
	{
	static	void	*gLastFrame = NULL;
	static	int32	gCallsPerSec;
	static	int32	gCallCount = 0;
	static	int32	gPrevTime = -1;
	static	int32	gFramesPerSec;
	static	int32	gFrameCount = 0;
	static	uint32	gStreamClock;
			int32	currTime;
			void	*tempFrame;

	tempFrame = (void*)(ctx->sanimChannelPtr[0]->curFramePtr);

	currTime = CurrentSeconds();
	++gCallCount;
	
	// do we have a different frame from last time? if so updat the global
	//  frame/sec counter
	if ( gLastFrame != tempFrame )
		++gFrameCount;
	
	// has a second passed? if so, update the number of times
	//  that we've been through this proc in the last second 
	if ( currTime != gPrevTime ) 
		{
		// update the fps counter
		gFramesPerSec = gFrameCount;
		gFrameCount = 0;

		// update the stream time
		if ( DSGetClock(ctx->streamCBPtr, &gStreamClock) != kDSNoErr )
			gStreamClock = kInvalidClock;
		
		gPrevTime = currTime;
		gCallsPerSec = gCallCount;
		gCallCount = 0;
		}
	
	// remember this frame for next time through
	gLastFrame = tempFrame;
	
	// draw the last second's average count again...
	PutNumXY(gCallsPerSec, gFramesPerSec, gStreamClock, 20, 195);
	}



/*===========================================================================================
 ============================================================================================
									Main Program Logic
 ============================================================================================
 ===========================================================================================*/


/*********************************************************************************************
 * Main program
 *********************************************************************************************/

int32	PlaySAnimStream( ScreenContext *screenContextPtr, char* streamFileName)
{
	int32			status;
	int32			playerResult;
	//DSRequestMsg	EOSMessage;
	Bitmap 			*bitmap;
 	long			channelNum;
	CCB*			ccbListHead;
	CCB*			ccbListTail;
	CCB*			ccbPtr;
	SAnimPlayerPtr		ctx;
	SAnimPlayer			sanimPlayerContext;
	long			joybits;
	Boolean			fRunStream;
	Boolean			fDisplayStreamClock;
	SAudioCtlBlock	ctlBlock;

	fRunStream			= true;
	fDisplayStreamClock = false;

	status = InitSAnimPlayerFromStreamHeader( &sanimPlayerContext, streamFileName );
	if ( status != 0 )
		return status;
	
	ctx = &sanimPlayerContext;

	/* Remember the screen context pointer that we will use
	 * for drawing purposes.
	 */
	ctx->screenContextPtr = screenContextPtr;

	/* PreRoll the stream, to get those buffers filled. */
	status = DSPreRollStream( ctx->messageItem, NULL, ctx->streamCBPtr );
	CHECK_SA_RESULT( "DSPreRollStream", status );
	
	/* Start the stream running */
	status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
	CHECK_SA_RESULT( "DSStartStream", status );
	if ( status != 0 )
		return status;

	/* The SANM Stream has no SYNC CTRL message at its front so we
	 * have to jump-start this stream. If This stream did have a SYNC
	 * message, the GOTO CTRL message at the end of the stream would
	 * cause the Control Subscriber to process the SYNC immediatly upon
	 * receipt of the SYNC chunk. This would almost certainly be BEFORE
	 * the SAnim subscriber has completed processing the last frame Cel.
	 * Thus, the last frames in the looping SAnim would never be seen.
	 */
	status = DSSetClock( ctx->streamCBPtr, 0 );
	CHECKRESULT( "DSSetClock()", status );
	if ( status != 0 )
		return status;

	/* The audio clock has already been set to the appropriate channel */	

	/* Display the screen we will be drawing directly into.
	 * Do this only once to keep the overhead (and aditional implicit
	 * graphics folio WaitVBL() from happening).
	 */
	DisplayScreen( ctx->screenContextPtr->sc_Screens[ctx->screenContextPtr->sc_curScreen], 0 );
	
		/* Get a pointer to the bitmap we will draw into */
	bitmap = ctx->screenContextPtr->sc_Bitmaps[ ctx->screenContextPtr->sc_curScreen ];

	playerResult = 0;

	/******************/
	/* Run the stream */
	/******************/
	while ( true )
		{
		joybits = ReadControlPad( 0 );
		if ( joybits & JOYSTART )
			break;

		/* The "A" button toggles running the stream
		 */
		if ( joybits & JOYFIREA )
			{
			/* Flip the logical value of the "stream running" flag
			 */
			fRunStream = !fRunStream;

			/* If we're transitioning from stopped to running, start the
			 * stream. Otherwise, stop the stream.
			 */
			if ( fRunStream )
				{
				status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
				CHECK_SA_RESULT( "DSStartStream", status );
				printf("Stream started.\n");
				}
			else
				{
				/* Note: we don't use the SOPT_FLUSH option when we stop
				 * here so that the "A" button acts like a pause/resume
				 * key. Using the SOPT_FLUSH option will cause buffered
				 * data to be flushed, so that once resumed, any queued
				 * video will drop frames to "catch up". This is not what
				 * is usually desired for pause/resume.
				 */
				status = DSStopStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
				CHECK_SA_RESULT( "DSStopStream", status );
				printf("Stream stopped.\n");
				}

			printf(" stream flags = %lx\n", ctx->streamCBPtr->streamFlags );
			}

		/* The "B" button toggles displaying the stream clock
		 */
		if ( joybits & JOYFIREB )
		{
			fDisplayStreamClock = !fDisplayStreamClock;
			if ( fDisplayStreamClock )
				printf("Stream timing on.\n");
			else
				printf("Stream timing off.\n");
		}
			

		if ( joybits & JOYFIREC )
			{
			// stop the current stream and flush the subscriber to make sure
			//  it doesn't hang onto any buffers.
			if ( fRunStream )
				{
				status = DSStopStream( ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH );
				CHECK_SA_RESULT( "DSStopStream", status );
				}

			for ( channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++ )
				FlushSAnimChannel( ctx->sanimContextPtr, ctx->sanimChannelPtr[ channelNum ], 0 );
		
			// tell the newly connected stream to reset itself to 0 so that we get the 
			//  ccb from the file directly (the subscriber can't respond to our polling until
			//  it gets the CCB, which is only found at the begining of the file)
			status = DSGoMarker( ctx->messageItem, NULL, ctx->streamCBPtr, (unsigned long)0,
									GOMARKER_ABSOLUTE );
			CHECK_SA_RESULT( "DSGoMarker", status );

			if ( fRunStream )
				{
				status = DSPreRollStream( ctx->messageItem, NULL, ctx->streamCBPtr );
				CHECK_SA_RESULT( "DSPreRollStream", status );
				
				status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
				CHECK_SA_RESULT( "DSStartStream", status );

				/* Start clock for animation subscriber in case there's
				 * no audio data in the stream to do this for us.
				 */
				status = DSSetClock( ctx->streamCBPtr, 0 );
				CHECKRESULT( "DSSetClock()", status );
				}
			}
		
		
		if ( joybits & JOYUP )
			{
			long	volume=0;

			ctlBlock.amplitude.channelNumber = ctx->gAudioChannel;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
									 kSAudioCtlOpGetVol, &ctlBlock );
			if (status < 0) 
				{
				printf("\n error sending control message!\n");
				printf(" status = %ld.\n", status);
				}
			volume = ctlBlock.amplitude.value;

			if (volume < 0x7A00)
				{
				volume += 0x500;

				ctlBlock.amplitude.channelNumber = ctx->gAudioChannel;
				ctlBlock.amplitude.value = volume;
	
				status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
										 kSAudioCtlOpSetVol, &ctlBlock );
				if (status < 0) 
					{
					printf("\n error sending control message!\n");
					printf(" status = %ld.\n", status);
					}
				}
			}
		
		if ( joybits & JOYDOWN )
			{
			long	volume;

			ctlBlock.amplitude.channelNumber = ctx->gAudioChannel;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
									 kSAudioCtlOpGetVol, &ctlBlock );
			if (status < 0) 
				{
				printf("\n error sending control message!\n");
				printf(" status = %ld.\n", status);
				}
			volume = ctlBlock.amplitude.value;

			if (volume > 0x600)
				{
				volume -= 0x500;

				ctlBlock.amplitude.channelNumber = ctx->gAudioChannel;
				ctlBlock.amplitude.value = volume;
	
				status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
										 kSAudioCtlOpSetVol, &ctlBlock );
				if (status < 0) 
					{
					printf("\n error sending control message!\n");
					printf(" status = %ld.\n", status);
					}
				}
			}

		if ( joybits & JOYLEFT )
			{
			long	pan=0;

			ctlBlock.pan.channelNumber = ctx->gAudioChannel;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
									 kSAudioCtlOpGetPan, &ctlBlock );
			if (status < 0) 
				{
				printf("\n error sending control message!\n");
				printf(" status = %ld.\n", status);
				}
			pan = ctlBlock.pan.value;
	
			if (pan > 0x600)
				{
				pan -= 0x500;

				ctlBlock.pan.channelNumber = ctx->gAudioChannel;
				ctlBlock.pan.value = pan;
	
				status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
										 kSAudioCtlOpSetPan, &ctlBlock );
				if (status < 0) 
					{
					printf("\n error sending control message!\n");
					printf(" status = %ld.\n", status);
					}
				}
			}

		if ( joybits & JOYRIGHT )
			{
			long	pan;

			ctlBlock.pan.channelNumber = ctx->gAudioChannel;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
									 kSAudioCtlOpGetPan, &ctlBlock );
			if (status < 0) 
				{
				printf("\n error sending control message!\n");
				printf(" status = %ld.\n", status);
				}
			pan = ctlBlock.pan.value;
	
			if (pan < 0x7A00)
				{
				pan += 0x500;

				ctlBlock.pan.channelNumber = ctx->gAudioChannel;
				ctlBlock.pan.value = pan;
	
				status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
										 kSAudioCtlOpSetPan, &ctlBlock );
				if (status < 0) 
					{
					printf("\n error sending control message!\n");
					printf(" status = %ld.\n", status);
					}
				}
			}

		/****************************************************************/
		/* Get the next animation frames from the subscriber and draw 	*/
		/* them on the screen.											*/
		/****************************************************************/
		ccbListHead = NULL;
		ccbListTail = NULL;

		for ( channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++ )
			{
			/* Get the next frame */
			ccbPtr = GetSAnimCel( ctx->sanimContextPtr, ctx->sanimChannelPtr[ channelNum ] );
			if ( ccbPtr != NULL )
				{
				/* Always mark the current cel as the "end of list" */
				LAST_CEL( ccbPtr );

				/* If there's already one in the list, add the new one
				 * onto the end of the list, else set the head pointer to
				 * point to the one we just got (make tail point there too).
				 */
				if ( ccbListTail != NULL )
					{
					ccbListTail->ccb_Flags &= ~CCB_LAST;	/* clear "last ccb" bit */
					ccbListTail->ccb_Flags |= CCB_NPABS;	/* set "next ptr absolute" bit */
					ccbListTail->ccb_NextPtr = ccbPtr;		/* point to new addition */
					}
				else
					/* Add first and only entry to list */
					ccbListHead = ccbPtr;

				/* Always remember the last one in the list */
				ccbListTail = ccbPtr;

#if FORCE_CELS_ONTO_SCREEN
				
				if ( (ccbPtr->ccb_XPos > MAX_SAFE_X_POS)  
					|| (ccbPtr->ccb_YPos > MAX_SAFE_Y_POS) 
					|| (ccbPtr->ccb_XPos < MIN_SAFE_X_POS) 
					|| (ccbPtr->ccb_YPos <  MIN_SAFE_Y_POS) )
				
				  {
				  CenterCelOnScreen( ccbPtr );
				  }
#endif
				}
			}
	
		/* Draw the ccb list if it is non-empty */
		if ( ccbListHead != NULL )
			{
			gScreenContext.sc_curScreen = 1 - gScreenContext.sc_curScreen;

			WaitVBL( ctx->VBLIOReq, 1 );
			EraseScreen(&gScreenContext, gScreenContext.sc_curScreen);
			/* printf("Draw %x\n", ccbListHead->ccb_SourcePtr); */
			status = DrawScreenCels(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 
										ccbListHead );
			if ( status != 0 ) 
				printf("DrawCels() error...\n");

			/* Tell the SAnimSubscriber to reply all freed chunks back to the streamer */
			SendFreeSAnimSignal (ctx->sanimContextPtr);
			}
		
		/* Display the stream clock if we're supposed to do so, show the new image
		 */
		if ( fDisplayStreamClock )
			ShowStreamTiming( ctx);

		DisplayScreen(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 0);
		}
	/* Close the stream */
	printf("Stream stopping\n");
	status = DSStopStream( ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH );
	CHECK_SA_RESULT( "DSStopStream", status );

	/* flush anything held by the anim subscriber */
	for ( channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++ )
		FlushSAnimChannel( ctx->sanimContextPtr, ctx->sanimChannelPtr[ channelNum ], 0 );

	printf("Disposing the stream\n");
	/* Get rid of stuff we allocated */
	DismantlePlayer( ctx );
	
	return playerResult;

}

int		main (int argc, char **argv)
	{
	long			status;

	/* Initialize the library code */
	if ( ! StartUp() )
		{
		printf( "StartUp() initialization failed!\n" );
		exit(0);
		}

	// sanity check, need at least the name of a file
	if ( argc < 2 )
		{
		printf("### %s - invalid parameter list, need at least a stream file name.\n", argv[0] );
		Usage(argv[0]);
		exit(0);
		} 	
	/* Clean the screen */
	EraseScreen(&gScreenContext, gScreenContext.sc_curScreen); 
	
	status = PlaySAnimStream( &gScreenContext, argv[1] );
	CHECK_SA_RESULT( "PlaySAnimStream", status );

	if ( status == 0 )
		printf( "End of stream reached\n" );

	else if ( status == 1 )
		printf( "Stream stopped by control pad input\n" );

	exit(0);
	}

