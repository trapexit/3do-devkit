/*******************************************************************************************
 *	File:			ControlSubscriber.c
 *
 *	Contains:		Control subscriber
 *
 *	Written by:		Neil Cormia
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	11/23/93	jb		Version 1.5
 *						Added CloseCtrlSubscriber() to deallocate resources allocated
 *						by the InitCtrlSubscriber() routine.
 *	10/19/93	jb		Version 1.4
 *						Switch to using UMemory.c routines for memory allocation
 *	8/18/93		jb		Version 1.3
 *						Check the calculated delay time in DoAlarm() and if the time
 *						to perform the alarm action has already passed, trigger the
 *						action immediately.
 *	8/16/93		jb		Version 1.2
 *						Change API to match the actual async API. Allocate a memory pool
 *						of streamer request messages (msg items and buffers) for making
 *						async requests.
 *	8/10/93		jb		Version 1.1
 *						Clean up routine prototypes to make everything private except
 *						for things we want explicitly exported.
 *	8/9/93		jb		Add dsReqMsgItemPool, a pool of message items for communication
 *						of streamer requests (necessitated by async operation). Add
 *						dsReqReplyPort, dsReqReplyPortSignal, to go with this.
 *	8/6/93		jb		Version 1.0.1
 *						Add a subscriber message pool used for broadcast operations
 *						as required by DSClockSync(), which uses the _SubscriberBroadcast()
 *						routine internally. This may be handy for other subscriber
 *						broadcast purposes in the future.
 *	7/9/93		jb		Version 1.0
 *						Switch to using all async calls back into streamer.
 *	6/28/93		jb		Version 0.6
 *						Add hack for setting stream clock based on an ALRM subchunk
 *						to make the SAnim subscriber not be required to drive the
 *						stream clock itself.
 *	6/22/93		jb		Version 0.5
 *						Add argument to DSClockSync() call to avoid deadlock when
 *						the streamer synchronously calls *all* subscribers.
 *	6/15/93		jb		Version 0.4
 *						Create a reply port and message item for communicating with the
 *						streamer at thread init time instead of creating and deleting
 *						same every time a data message arrives.
 *						Add code in DoData() to implement the conditional call to 
 *						the DSClockSync() routine.
 *						Remove all extraneous references to timers and audio cue items
 *						and audio cue signals. 
 *						Remove all extraneous include file references.
 *						Set chanPtr->msgQueue.tail = NULL at end of flush loop to
 *						prevent spurious queuing later.
 *	6/2/93		jb		Version 0.3
 *						Switch to using Debug3DO.h for CHECKRESULT() macro
 *						Remove all variable initializers in local variable declarations
 *						to avoid register bashing by the ARM C compiler.
 *	5/30/93		jb		Version 0.2
 *	5/30/93		jb		Set 'threadItem' to zero so that dispose won't try to delete the
 *						thread if close has already been called.
 *	5/10/93		njc		New today.
 *
 *******************************************************************************************/

#include "Audio.h"
#include "UMemory.h"

#include "DataStreamLib.h"
#include "DataStreamDebug.h"
#include "MsgUtils.h"
#include "MemPool.h"
#include "ThreadHelper.h"
#include "Debug3DO.h"

#include "ControlSubscriber.h"

#include "stdlib.h"				/* for the exit() routine */


/**********************/
/* Subscriber globals */
/**********************/

struct CtrlSubscriberGlobals	{

	MemPoolPtr		CtrlContextPool;		/* The memory pool for subscriber contexts */

	} CSGlobals;


/****************************/
/* Local routine prototypes */
/****************************/

static int32	DoData( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoGetChan( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoSetChan( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoControl( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoOpening( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoClosing( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoStop( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoSync( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoEOF( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );
static int32	DoAbort( SubscriberMsgPtr subMsg, CtrlContextPtr ctx );

static int32	_InitializeCtrlThread( CtrlContextPtr ctx );


/**************************/
/* Local utility routines */
/**************************/

static void		_StopCtrlChannel( CtrlContextPtr ctx, long channelNumber );
static void		_FlushCtrlChannel( CtrlContextPtr ctx, long channelNumber );


/************************************/
/* Subchunk implementation routines */
/************************************/
static void		DoAlarm( CtrlContextPtr ctx, uint32 time, uint32 newTime, uint32 options );
static void		DoPause( CtrlContextPtr ctx, uint32 time, uint32 amount, uint32 options );
static void		DoTimerExpired( CtrlContextPtr ctx );


/*==========================================================================================
  ==========================================================================================
							Subscriber procedural interfaces
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * Routine to initialize the subscriber. Creates the a memory pool for allocating
 * subscriber contexts.
 *******************************************************************************************/
int32	InitCtrlSubscriber( void )
	{
	/* Create the memory pool for allocating data stream
	 * contexts.
	 */
	CSGlobals.CtrlContextPool = CreateMemPool( CTRL_MAX_SUBSCRIPTIONS, sizeof(CtrlContext) );
	if ( CSGlobals.CtrlContextPool == NULL )
		return -1;

	/* Return success */
	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to deallocate resources allocated by InitCtrlSubscriber()
 *******************************************************************************************/
int32	CloseCtrlSubscriber( void )
	{
	DeleteMemPool( CSGlobals.CtrlContextPool );
	
	CSGlobals.CtrlContextPool = NULL;
	
	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to instantiate a new subscriber. Creates the message port through which all
 * subsequent communications take place, as well as any other necessary per-context
 * resources. Creates the service thread and passes the new context to it.
 *******************************************************************************************/
int32	NewCtrlSubscriber( CtrlContextPtr *pCtx, DSStreamCBPtr	streamCBPtr, int32 priority )
	{
	int32			status;
	CtrlContextPtr	ctx;
	uint32			signalBits;

	/* Allocate a subscriber context */

	ctx = (CtrlContextPtr) AllocPoolMem( CSGlobals.CtrlContextPool );
	if ( ctx == NULL )
		return kDSNoMemErr;

	/* Allocate a signal to synchronize with the completion of the
	 * subscriber's initialization. It will signal us with this when
	 * it has finished, successfully or not, when it is done initializing
	 * itself.
	 */
	ctx->creatorTask		= CURRENTTASK->t.n_Item;	/* see "kernel.h" for this */
	ctx->streamCBPtr		= streamCBPtr;
	
	ctx->creatorSignal = AllocSignal( 0 );
	if ( ctx->creatorSignal == 0 )
		{
		status = kDSNoSignalErr;
		goto CLEANUP;
		}

	/* Create the thread that will handle all subscriber 
	 * responsibilities.
	 */
	status = NewThread( 
				(void *) &CtrlSubscriberThread, 			/* thread entrypoint */
				4096, 										/* initial stack size */
				(long) CURRENT_TASK_PRIORITY + priority,	/* priority */
				NULL, 										/* name */
				&ctx->threadStackBlock, 					/* where to remember stack block addr */
				0, 											/* initial R0 */
				ctx );										/* initial R1 */

	if ( status <= 0 )
		goto CLEANUP;
	else
		ctx->threadItem = status;

	/* Wait here while the subscriber initializes itself. When its done,
	 * look at the status returned to us in the context block to determine
	 * if it was happy.
	 */
	signalBits = WaitSignal( ctx->creatorSignal );
	if ( signalBits != ctx->creatorSignal )
		return kDSSignalErr;

	/* We're done with this signal, so give it back */
	FreeSignal( ctx->creatorSignal );

	/* Check the initialization status of the subscriber. If anything
	 * failed, the 'ctx->creatorStatus' field will be set to a system result
	 * code. If this is >= 0 then initialization was successful.
	 */
	status = ctx->creatorStatus;
	if ( status >= 0 )
		{
		*pCtx = ctx;		/* give the caller a copy of the context pointer */
		return status;		/* return 'success' */
		}

CLEANUP:
	/* Something went wrong in creating the new subscriber.
	 * Release anything we created and return the status indicating
	 * the cause of the failure.
	 */
	if ( ctx->threadStackBlock )	FreePtr( ctx->threadStackBlock );

	ReturnPoolMem( CSGlobals.CtrlContextPool, ctx );

	return status;
	}


/*******************************************************************************************
 * Routine to get rid of all subscriber resources we created. This assumes that the
 * subscriber is in a clean state where it has returned all resources that were
 * passed to it, like messages. Deleting the subscriber thread should free up all
 * the rest of the stuff the thread allocated. What is left is the thread's stack
 * space, which we allocated, and the context block.
 *******************************************************************************************/
int32	DisposeCtrlSubscriber( CtrlContextPtr ctx )
	{
	if ( ctx != NULL )
		{
		if ( ctx->threadItem > 0 )		DisposeThread( ctx->threadItem );
		if ( ctx->threadStackBlock )	FreePtr( ctx->threadStackBlock );

		ReturnPoolMem( CSGlobals.CtrlContextPool, ctx );
		}

	return kDSNoErr;
	}



/*==========================================================================================
  ==========================================================================================
						Routines to handle subchunk actions
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * This routine schedules an action that will wait for the stream clock to advance to
 * whenTime, and then set the stream clock to newTime. For now, options are ignored.
 *******************************************************************************************/
static void	DoAlarm( CtrlContextPtr ctx, uint32 whenTime, uint32 newTime, uint32 options )
	{
	int32		status;
	uint32		streamClock;
	int32		amountToDelay;
	AudioTime	whenToWakeUp;

	/* If we're already processing an alarm, throw this new one away.
	 * NOTE: For now, there isn't much choice here if we want to use the
	 *			Audio Folio for our timing. This is the best timing source
	 *			because it syncs directly with our other data, but there is
	 *			currently no way to cancel a cue timer once you start one.
	 */
	if ( ctx->fTimerRunning )
		return;

	/* Get the current stream time. If the clock isn't valid,
	 * just give up.
	 */
	status = DSGetClock( ctx->streamCBPtr, &streamClock );
	if ( status != kDSNoErr )
		return;

	/* Remember the value that will be used to set the stream time when
	 * the delay timer expires.
	 */
	ctx->timerOwner = ALRM_CHUNK_SUBTYPE;
	ctx->newClockTime = newTime;

	/* Calculate the amount of time we need to delay before performing
	 * the requested action.
	 */
	amountToDelay = whenTime - streamClock;

	if ( amountToDelay > 0 )
		{
		/* Calculate the absolute audio time when we wish to be
		 * signalled to perform the requested action.
		 */
		whenToWakeUp = GetAudioTime() + amountToDelay;

#if 0
	printf( "DoAlarm() - whenTime = %ld, streamClock = %ld, amountToDelay = %ld\n", 
					whenTime, streamClock, amountToDelay );
#endif

		status = SignalAtTime( ctx->cueItem, whenToWakeUp );
		if ( status < 0 )
			return;

		/* Assert that we are waiting for the timer */
		ctx->fTimerRunning = true;
		}
	else
		/* We're already too late. Do the deed immediately */
		DoTimerExpired( ctx );
	}


/*******************************************************************************************
 * Routine to pause the stream the specified amount, then restart it running. For now,
 * the options are ignored.
 *******************************************************************************************/
static void	DoPause( CtrlContextPtr ctx, uint32 time, uint32 amount, uint32 options )
	{
	int32			status;
	uint32			streamClock;
	AudioTime		whenToWakeUp;
	DSRequestMsgPtr reqMsg;

	/* If we're already processing an alarm, throw this new one away.
	 * NOTE: For now, there isn't much choice here if we want to use the
	 *			Audio Folio for our timing. This is the best timing source
	 *			because it syncs directly with our other data, but there is
	 *			currently no way to cancel a cue timer once you start one.
	 */
	if ( ctx->fTimerRunning )
		return;

	/* Get the current stream time. If the clock isn't valid,
	 * just give up.
	 */
	status = DSGetClock( ctx->streamCBPtr, &streamClock );
	if ( status != kDSNoErr )
		return;

	/* Calculate the audio time when we need to wake up in order to
	 * set the stream clock to the desired value, and start our
	 * timer running.
	 */
	whenToWakeUp = GetAudioTime() + (streamClock + amount);
	status = SignalAtTime( ctx->cueItem, whenToWakeUp );
	if ( status < 0 )
		return;

	/* Allocate a request message and send it */
	reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
	if ( reqMsg != NULL )
		{
		status = DSStopStream( reqMsg->msgItem, reqMsg, ctx->streamCBPtr, 0 );
		CHECK_DS_RESULT( "ControlSubscriber >> DoPause() >> DSStopStream() = ", status );

		/* Assert that we are waiting for the timer */
		ctx->timerOwner = PAUS_CHUNK_SUBTYPE;
		ctx->fTimerRunning = true;
		}
	}


/*******************************************************************************************
 * Routine to handle the timer going off. This may be set by either an ALRM subchunk's
 * processing or a PAUS subchunk's processing.
 *******************************************************************************************/
static void DoTimerExpired( CtrlContextPtr ctx )
	{
	DSRequestMsgPtr reqMsg;

	switch ( ctx->timerOwner )
		{
		case ALRM_CHUNK_SUBTYPE:
			/* Set the stream clock to the new time */
			DSSetClock( ctx->streamCBPtr, ctx->newClockTime );
			break;

		case PAUS_CHUNK_SUBTYPE:
			reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
			if ( reqMsg != NULL )
				{
				DSStartStream( reqMsg->msgItem, reqMsg, ctx->streamCBPtr, 0 );
				}
			break;

		default:
			;
		}

	/* Assert that the timer is free for its next use */
	ctx->fTimerRunning = false;
	}


/*==========================================================================================
  ==========================================================================================
						Routines to handle subscriber messages
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * Utility routine to disable further data flow for the given channel, and to halt
 * any associated physical processes associated with the channel (i.e., stop sound
 * playing, etc.)
 *******************************************************************************************/
static void		_StopCtrlChannel( CtrlContextPtr ctx, long channelNumber )
	{
	SubsChannelPtr		chanPtr;
	
	if ( channelNumber < CTRL_MAX_CHANNELS )
		{
		chanPtr = ctx->channel + channelNumber;

		/* Prevent further data queuing */
		chanPtr->status &= ~CHAN_ENABLED;

		/* Stop any physical processes associated with the channel */
		}
	}


/*******************************************************************************************
 * Utility routine to disable further data flow for the given channel, and to cause
 * any associated physical processes associated with the channel to stop.
 *******************************************************************************************/
static void		_FlushCtrlChannel( CtrlContextPtr ctx, long channelNumber )
	{
	SubsChannelPtr		chanPtr;
	SubscriberMsgPtr	msgPtr;
	SubscriberMsgPtr	next;
	
	if ( channelNumber < CTRL_MAX_CHANNELS )
		{
		chanPtr = ctx->channel + channelNumber;

		/* Halt whatever activity is associated with the channel */
		_StopCtrlChannel( ctx, channelNumber );

		/* Give back all queued chunks for this channel to the
		 * stream parser. We do this by replying to all the
		 * "chunk arrived" messages that we have queued.
		 */
		while ( (msgPtr = chanPtr->msgQueue.head) != NULL )
			{
			/* Get the pointer to the next message in the queue */
			next = (SubscriberMsgPtr) msgPtr->link;

			/* Reply to this chunk so that the stream parser
			 * can eventually reuse the buffer space.
			 */
			ReplyMsg( msgPtr->msgItem, kDSNoErr, msgPtr, sizeof(SubscriberMsgPtr));

			/* Continue with the next message in the queue */
			chanPtr->msgQueue.head = next;
			}

		chanPtr->msgQueue.tail = NULL;
		}
	}


/*******************************************************************************************
 * Routine to queue arriving data chunks. We determine the type of control message and call
 * back into the streamer to handle it. 
 *******************************************************************************************/
static int32	DoData( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	int32			status;
	ControlChunkPtr	ccp;
	Boolean			fAtMarkerPosition;
	DSRequestMsgPtr reqMsg;

	ccp = (ControlChunkPtr) subMsg->msg.data.buffer;

	/* Switch on the sub chunk type for each control chunk.
	 */
	switch ( ccp->subChunkType )
		{
		case GOTO_CHUNK_SUBTYPE:
			/* Implement an embedded branch to another place in the
			 * current stream.
			 */
			reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
			if ( reqMsg != NULL )
				{
				status = DSGoMarker( reqMsg->msgItem, reqMsg, ctx->streamCBPtr, 
									ccp->u.marker.value, GOMARKER_ABSOLUTE );
				}
			else
				status = kDSNoMsgErr;

			CHECK_DS_RESULT( "ControlSubscriber - DSGoMarker() = ", status );
			break;

		case SYNC_CHUNK_SUBTYPE:
			/* Implement a conditional call to DSClockSync() on whether we are arriving
			 * at a "marker" due to our playing through the marker, or due to
			 * having executed a branch to the marker. It is assumed that subscribers
			 * will flush any queued data when receiving one of these messages.
			 */
			status = DSIsMarker( ctx->streamCBPtr, ccp->u.sync.value, &fAtMarkerPosition );
			CHECK_DS_RESULT( "ControlSubscriber - DSIsMarker() = ", status );

			if ( (status == kDSNoErr) && fAtMarkerPosition )
				{
				/* Set the clock to the new time */
				status = DSSetClock( ctx->streamCBPtr, ccp->time );
				if ( status != kDSNoErr )
					return status;

				/* Tell the world that the clock just changed. 
				 */
				status = DSClockSync( ctx->streamCBPtr, ctx->subsMsgPool, ccp->time );
				CHECK_DS_RESULT( "ControlSubscriber - DSClockSync() = ", status );
				}
			break;

		case ALRM_CHUNK_SUBTYPE:
			/* Device to set the stream time to a new value when the stream
			 * clock reaches the time specified in the alarm chunk timestamp.
			 * Used in looping streams to allow the time to be set backwards
			 * without causing a flush as soon as the seek completes, as would
			 * happen as a result of a sync chunk.
			 */
			DoAlarm( ctx, ccp->time, ccp->u.alarm.newTime, ccp->u.alarm.options );
			status = kDSNoErr;
			break;

		case PAUS_CHUNK_SUBTYPE:
			/* Pause the stream for the specified amount of time, then resume
			 * playing again.
			 */
			DoPause( ctx, ccp->time, ccp->u.pause.amount, ccp->u.pause.options );
			status = kDSNoErr;
			break;

		case STOP_CHUNK_SUBTYPE:
			/* Stop the stream entirely when we receive one of these
			 */
			reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
			if ( reqMsg != NULL )
				{
				status = DSStopStream( reqMsg->msgItem, reqMsg, ctx->streamCBPtr, 
									ccp->u.stop.options );
				}
			else
				status = kDSNoMsgErr;

			CHECK_DS_RESULT( "ControlSubscriber - DSStopStream() = ", status );
			break;

		default:
			status = kDSNoErr;
			break;
		}

	return status;
	}

		
/*******************************************************************************************
 * Routine to return the status bits of a given channel.
 *******************************************************************************************/
static int32	DoGetChan( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	int32	status;
	long	channelNumber;

	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < CTRL_MAX_CHANNELS )
		status = ctx->channel[ channelNumber ].status;
	else
		status = 0;

	return status;
	}

		
/*******************************************************************************************
 * Routine to set the channel status bits of a given channel.
 *******************************************************************************************/		
static int32	DoSetChan( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	long	channelNumber;

	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < CTRL_MAX_CHANNELS )
		{
		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to 
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary.
		 */
		ctx->channel[ channelNumber ].status |= 
				(~CHAN_SYSBITS & subMsg->msg.channel.status);
		}

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 *******************************************************************************************/		
static int32	DoControl( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
#if 0
	int32			status;
	long			userDefinedArg1 = subMsg->msg.control.controlArg1;
	void*			userDefinedArg2 = subMsg->msg.control.controlArg2;
#endif

	/* This routine takes a long and a pointer as inputs. These are entirely
	 * user defined and can be used to do everything from adjust audio volume
	 * on a per channel basis, to ordering out for pizza.
	 */

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream, typically
 * just after the stream is opened.
 *******************************************************************************************/		
static int32	DoOpening( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to close down an open subscription.
 *******************************************************************************************/		
static int32	DoClosing( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/		
static int32	DoStop( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	long	channelNumber;

	/* Stop all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < CTRL_MAX_CHANNELS; channelNumber++ )
		_StopCtrlChannel( ctx, channelNumber );

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to (normally) flush any queued data we might have. Since we are (usually) the
 * originator of this message, we simply ignore it.
 *******************************************************************************************/		
static int32	DoSync( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to do whatever is necessary when EOF is asserted.
 *******************************************************************************************/		
static int32	DoEOF( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 *******************************************************************************************/		
static int32	DoAbort( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	long			channelNumber;

	/* Halt and flush all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < CTRL_MAX_CHANNELS; channelNumber++ )
		_FlushCtrlChannel( ctx, channelNumber );

	return kDSNoErr;
	}



/*==========================================================================================
  ==========================================================================================
									The subscriber thread
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * Routine to to perform one-time initialization for the thread prior to its execution.
 * Allocates all system resources, and communicates with the thread's creator if the
 * thread and all its resources cannot be created.
 *******************************************************************************************/		
static int32	_InitializeCtrlThread( CtrlContextPtr ctx )
	{
	int32			status;
	long			channelNumber;

	/* Initialize fields in the context record. Set the completion status
	 * field used by our creator to assert that initialization failed. We
	 * clear this later if everything goes well.
	 */
	ctx->creatorStatus			= -1;
	ctx->requestPort			= 0;
	ctx->cueItem				= 0;
	ctx->cueSignal				= 0;
	ctx->fTimerRunning			= false;

	/* Open the Audio Folio for this thread */
	
	if ( (status = OpenAudioFolio() ) < 0 )
		{
		ctx->creatorStatus = status;
		goto BAILOUT;
		}
	
	ctx->subsReplyPortSignal	= 0;
	ctx->subsReplyPort			= 0;
	ctx->subsMsgPool			= 0;

	ctx->dsReqReplyPort			= 0;
	ctx->dsReqReplyPortSignal	= 0;
	ctx->dsReqMsgPool			= 0;


	/* Initialize channel structures */
	for ( channelNumber = 0; channelNumber < CTRL_MAX_CHANNELS; channelNumber++ )
		{
		ctx->channel[channelNumber].status			= CHAN_ENABLED;
		ctx->channel[channelNumber].msgQueue.head	= NULL;
		ctx->channel[channelNumber].msgQueue.tail	= NULL;
		}

	/* Create the message port where the new subscriber will accept
	 * request messages.
	 */
	status = NewMsgPort( &ctx->requestPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->requestPort = status;


	/* Init the msg item pool and reply port used to communicate with 
	 * the streamer. We will need to send messages to the streamer when
	 * we process "go marker" and "sync" subchunks.
	 */
	status = NewMsgPort( &ctx->dsReqReplyPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->dsReqReplyPort = status;


	/* Allocate a pool of message blocks that we will later send to 
	 * the streamer when making asynchronous requests.
	 */
	ctx->creatorStatus = kDSNoMemErr;
	ctx->dsReqMsgPool = CreateMemPool( CTRL_NUM_DS_REQS_MSGS, sizeof(DSRequestMsg) );
	if ( ctx->dsReqMsgPool == NULL )
		goto BAILOUT;

	if ( ! FillPoolWithMsgItems( ctx->dsReqMsgPool, ctx->dsReqReplyPort ) )
		goto BAILOUT;


	/* Create a message pool for broadcast functions, such as is required
	 * when calling DSClockSync(), which broadcasts our timing message to all
	 * subscribers.
	 */
	status = NewMsgPort( &ctx->subsReplyPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->subsReplyPort = status;

	ctx->subsMsgPool = CreateMemPool( CTRL_MAX_SUBS_MESSAGES, sizeof(SubscriberMsg) );
	if ( ctx->subsMsgPool == NULL )
		{
		status = kDSNoMemErr;
		goto BAILOUT;
		}

	if ( ! FillPoolWithMsgItems( ctx->subsMsgPool, ctx->subsReplyPort ) )
		{
		status = kDSNoMsgErr;
		goto BAILOUT;
		}


	/* Create an Audio Cue Item for timing purposes 
	 */
	status = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->cueItem = status;

	/* Get the signal associated with the cue so we
	 * can wait on timed events in the thread's main loop.
	 */
	if ( (ctx->cueSignal = GetCueSignal( ctx->cueItem )) != 0 )
		ctx->creatorStatus = 0;

BAILOUT:
	/* Inform our creator that we've finished with initialization. We are either
	 * ready and able to accept requests for work, or we failed to initialize
	 * ourselves. If the latter, we simply exit after informing our creator.
	 * All resources we created are thankfully cleaned up for us by the system.
	 */
	status = SendSignal( ctx->creatorTask, ctx->creatorSignal );
	if ( (ctx->creatorStatus < 0) || (status < 0) )
		return -1;
	else
		return kDSNoErr;
	}


/*******************************************************************************************
 * This thread is started by a call to InitCtrlSubscriber(). It reads the subscriber message
 * port for work requests and performs appropriate actions. The subscriber message
 * definitions are located in "DataStreamLib.h".
 *******************************************************************************************/
void	CtrlSubscriberThread( int32 notUsed, CtrlContextPtr ctx )
	{
	int32				status;
	SubscriberMsgPtr	subMsg;
	long				channelNumber;
	Boolean				fKeepRunning;
	uint32				anySignal;
	uint32				signalBits;
	DSRequestMsgPtr 	reqMsg;

	/* Call a utility routine to perform all startup initialization.
	 */
	if ( (status = _InitializeCtrlThread( ctx )) != 0 )
		exit(0);

	/* All resources are now allocated and ready to use. Our creator has been informed
	 * that we are ready to accept requests for work. All that's left to do is
	 * wait for work request messages to arrive.
	 */
	anySignal = ctx->cueSignal
				| ctx->requestPortSignal 
				| ctx->subsReplyPortSignal
				| ctx->dsReqReplyPortSignal;

	fKeepRunning = true;
	while ( fKeepRunning )
		{
		signalBits = WaitSignal( anySignal );

		/***********************************************/
		/* Check for and process any timer expirations */
		/***********************************************/
		if ( signalBits & ctx->cueSignal )
			{
			DoTimerExpired( ctx );
			}


		/********************************************************************/
		/* Check for and handle replies to broadcasts that have been sent.	*/
		/* Currently, these are only as a result of calling DSClockSync() 	*/
		/********************************************************************/
		if ( signalBits & ctx->subsReplyPortSignal )
			{
			while( PollForMsg( ctx->subsReplyPort, NULL, NULL, (void **) &subMsg, &status ) )
				ReturnPoolMem( ctx->subsMsgPool, subMsg );
			}


		/********************************************************************/
		/* Check for and handle replies to asynchronous streamer requests.	*/
		/********************************************************************/
		if ( signalBits & ctx->dsReqReplyPortSignal )
			{
			while( PollForMsg( ctx->dsReqReplyPort, NULL, NULL, (void **) &reqMsg, &status ) )
				ReturnPoolMem( ctx->dsReqMsgPool, reqMsg );
			}


		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			while( PollForMsg( ctx->requestPort, NULL, NULL, (void **) &subMsg, &status ) )
				{	
				switch ( subMsg->whatToDo )
					{
					case kStreamOpData:				/* new data has arrived */	
						status = DoData( subMsg, ctx );		
						break;
		
		
					case kStreamOpGetChan:			/* get logical channel status */
						status = DoGetChan( subMsg, ctx );		
						break;
		
		
					case kStreamOpSetChan:			/* set logical channel status */
						status = DoSetChan( subMsg, ctx );		
						break;
		
		
					case kStreamOpControl:			/* perform subscriber defined function */
						status = DoControl( subMsg, ctx );		
						break;
		
	
					case kStreamOpSync:				/* clock stream resynched the clock */
						status = DoSync( subMsg, ctx );		
						break;
		
		
				/************************************************************************
				 * NOTE:	The following msgs have no extended message arguments
				 * 			and only may use the whatToDo field in the message.
				 ************************************************************************/
	
	
					case kStreamOpOpening:			/* one time initialization call */
						status = DoOpening( subMsg, ctx );		
						break;
		
		
					case kStreamOpClosing:			/* stream is being closed */
						status = DoClosing( subMsg, ctx );		
						fKeepRunning = false;
						break;
		
		
					case kStreamOpStop:				/* stream is being stopped */
						status = DoStop( subMsg, ctx );
						break;
		
		
					case kStreamOpEOF:				/* physical EOF on data, no more to come */
						status = DoEOF( subMsg, ctx );		
						break;
		
		
					case kStreamOpAbort:			/* somebody gave up, stream is aborted */
						status = DoAbort( subMsg, ctx );		
						break;
		
		
					default:
						;
					}
		
				/* Reply to the request we just handled.
				 */
				ReplyMsg( subMsg->msgItem, status, subMsg, sizeof(SubscriberMsgPtr));
				}
			}
		}

	/* Halt and flush all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < CTRL_MAX_CHANNELS; channelNumber++ )
		_FlushCtrlChannel( ctx, channelNumber );

	/* Exiting should cause all resources we allocated to be
	 * returned to the system.
	 */
	ctx->threadItem = 0;
	
	DeleteMemPool( ctx->dsReqMsgPool );
	DeleteMemPool( ctx->subsMsgPool );
	
	exit(0);
	}

