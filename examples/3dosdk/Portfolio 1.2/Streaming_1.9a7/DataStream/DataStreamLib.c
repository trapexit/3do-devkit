/*******************************************************************************************
 *	File:			DataStreamLib.c
 *
 *	Contains:		Interface routines for data stream threads. Mostly, these routines
 *					build and send messages to a data stream thread which has been
 *					created by a call to NewDataStream(). Includes a couple of 
 *					convenience routines for managing pools of messages, as well as
 *					some functions that operate directly on stream control blocks.
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/14/93	jb		Added DSSetBranchDest() routine. Time based branching requires
 *						that data acquisition be able to set the value used in the comparison
 *						in the DSIsMarker() routine *after* translating the branch
 *						request into a time value. This is so that CTRL/SYNC messages
 *						that cause DSIsMarker() to be called will detect "hits" properly.
 *	10/4/93		jb		Add DSWaitEndOfStream()
 *	9/13/93		rdg		Fixed bug in DSGetChannel(): was setting the whatToDo field to SetChan!!!
 *	8/16/93		jb		Add code around VALIDATE_REQUEST_CODE to check the request
 *						code before sending it to the streamer thread.
 *						Change API to accept a DSRequestMsgPtr to indicate synchronous
 *						or asynchronous operation. If NULL is specified, the operation
 *						is performed syncrhonously, using a message buffer allocated
 *						on the caller's stack. Otherwise, the caller must not reuse 
 *						the buffer until the request is replied to by the streamer.
 *	8/6/93		jb		Add _SubscriberBroadcast() routine to replace previous use
 *						of _ForEachSubscriber(). Caller must supply a message pool
 *						since procedural callers cannot use the stream's messages.
 *						Interface to DSClockSync() now takes message pool pointer
 *						and gets rid of async flag (broadcast is always async).
 *	7/9/93		jb		Add 'async' flag to all streamer implemented functions to allow
 *						the caller to specify asynchronous operation.
 *						Make DSClockSync() use new asynchronous _ForEachSubscriber()
 *						routine.
 *	6/28/93		jb		Set the STRM_CLOCK_WAS_VALID whenever DSSetClock() is called.
 *						This is used by stop/start to implement automatic clock setting
 *						if the clock was ever valid.
 *	6/27/93		jb		Be sure to set subMsg->msgItem in DSClockSync()
 *						Copy code from _ForEachSubscriber() to implement the synchronous
 *						message send/response in DSClockSync() since we can't let it
 *						use the streamer's reply port...
 *	6/25/93		jb		Moved DSClockSync() here as an immediate procedure as opposed
 *						to a message which would cause an untimely delay in the delivery
 *						of the sync message to subscribers.
 *	6/24/93		jb		Add options argument to DSGoMarker()
 *	6/22/93		jb		Added options to start and stop subscriber messages to allow
 *						data flushing options to be communicated when a stream is
 *						started or stopped.
 *						Added 'exemptStreamType' to clocksync message to prevent deadlock
 *						when a subscriber calls DSClockSync(). Should we make this 
 *						asynchronous?
 *	6/19/93		jb		Only allow stream clock to be set if stream is running.
 *	6/15/93		jb		Add DSIsMarker()
 *	6/8/93		jb		Add DSSetClock() and DSGetClock()
 *	5/21/93		jb		Add check for valid message port on DSConnect() call.
 *						See code demarked by VALIDATE_CONNECT_PORT
 *	5/20/93		jb		Add DSConnect()
 *	5/12/93		jb		New today.
 *
 *******************************************************************************************/

#ifndef __audio_h
#include "audio.h"
#endif

#include "DataStreamLib.h"
#include "Debug3DO.h"

/* The following switch should be set to non-zero to check the formatting of
 * messages sent to the streamer *BEFORE* they are sent. Debugging aid.
 */
#define	VALIDATE_REQUEST_CODE	1

/* Set the following switch to non-zero to cause a connect request's specified message
 * port to be tested for existence at the time of the connect request. Makes for easier
 * debugging instead of letting it propagate all the way into the streamer thread before
 * we find out that the port was bad in the first place...
 */
#define	VALIDATE_CONNECT_PORT	0

#if VALIDATE_CONNECT_PORT
#include "debug.h"
#include "KernelNodes.h"
#endif


/*==========================================================================================
  ==========================================================================================
								Procedural Routines
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * Routine to set the stream clock. This clock is a clock based upon the Audio Folio's
 * audio time, but is RELATIVE to the stream. We determine the difference between
 * the supplied new relative time and the absolute time. Subsequent calls to get the
 * stream relative time are handled by subtracting this difference from the 
 * absolute audio clock.
 *******************************************************************************************/
int32	DSSetClock( DSStreamCBPtr streamCBPtr, uint32 newStreamClock )
	{
	/* Remember the difference between the supplied new (relative)
	 * clock value and the actual (absolute) audio time. Set the
	 * "clock is valid" bit in the stream control block.
	 */
	if ( streamCBPtr->streamFlags & STRM_RUNNING )
		{
		streamCBPtr->clockOffset = GetAudioTime() - newStreamClock;
		streamCBPtr->streamFlags |= (STRM_CLOCK_VALID | STRM_CLOCK_WAS_VALID);
		return kDSNoErr;
		}
	else
		return kDSNotRunningErr;
	}


/*******************************************************************************************
 * Return the current stream relative time. (See comments in DSSetClock() for an
 * explanation of the stream clock).
 *******************************************************************************************/
int32	DSGetClock( DSStreamCBPtr streamCBPtr, uint32* streamClock )
	{
	/* Return an error if the clock is known to not be valid. 
	 * This is the case when the stream starts before any subscriber
	 * sets the stream clock.
	 */
	if ( ! (streamCBPtr->streamFlags & STRM_CLOCK_VALID) )
		return kDSClockNotValidErr;

	/* The stream time is the current absolute audio clock minus
	 * the clock's offset, which is determined when the time gets
	 * set.
	 */
	(*streamClock) = GetAudioTime() - streamCBPtr->clockOffset;
	
	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to determine if the stream is positioned at the destination of a stream
 * branching operation due to its having arrived there as the result of a DSGoMarker()
 * call. This is used to implement conditional flushing of data based upon the knowlege
 * that the stream position is a result of "play through" or a "branch".
 *******************************************************************************************/
int32	DSIsMarker( DSStreamCBPtr streamCBPtr, uint32 markerValue, Boolean* pfIsMarker )
	{
	/* If a branch is in progress and the 'markerValue' matches
	 * the current branch destination, then clear the "branch in progress"
	 * flag and return TRUE.
	 */
	if ( (streamCBPtr->streamFlags & STRM_GO_INPROGRESS) 
		&& (markerValue == streamCBPtr->branchDest) )
		{
		streamCBPtr->streamFlags &= ~STRM_GO_INPROGRESS;
		(*pfIsMarker) = true;
		}
	else
		(*pfIsMarker) = false;

	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to set the branch destination value that is used by the DSIsMarker() routine
 * (see above). This is typically called only by data acquisition (or whatever code is
 * responisible for time to position translations). Note that the units value for the
 * 'branchTimeDest' arg is STREAM TIME.
 *******************************************************************************************/
int32	DSSetBranchDest( DSStreamCBPtr streamCBPtr, uint32 branchTimeDest )
	{
	/* To make this atomic, we clear the flag, set the destination value,
	 * then set the flag. 
	 */
	streamCBPtr->streamFlags &= ~STRM_GO_INPROGRESS;

	streamCBPtr->branchDest		= branchTimeDest;
	streamCBPtr->streamFlags	|= STRM_GO_INPROGRESS;

	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to allocate a message item for each entry in a message pool. Uses a utility
 * from MemPool.c that calls a callback routine for each element in the free list of
 * a pool. The callback function is called with a pointer to each entry and is expected
 * to fill in the entry. In our case, we create a message item with a specified
 * reply port. If any error occurs, the callback routine returns 'false' which will 
 * abort the iteration and return 'false' to the caller of ForEachFreePoolMember().
 *******************************************************************************************/

	/* The callback routine that fills in the pool entries
	 */
	static Boolean	_AllocMsgItemsFunc( void* replyPort, void* poolEntry )
		{
		Item			msgItem;
		GenericMsgPtr	genMsg;
	
		/* Create a message Item. Return FALSE if anything goes wrong
		 */
		msgItem = CreateMsgItem( (Item) replyPort );
		if ( msgItem <= 0 )
			return false;
	
		/* Store the message Item into the pool entry */
		genMsg = (GenericMsgPtr) poolEntry;
		genMsg->msgItem = msgItem;
	
		return true;
		}

Boolean	FillPoolWithMsgItems( MemPoolPtr memPool, Item replyPort )
	{
	return ForEachFreePoolMember( memPool, _AllocMsgItemsFunc, (void*) replyPort );
	}


/*==========================================================================================
  ==========================================================================================
								Message Based Routines
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * Utility to synchronously send a request message to a data stream thread. Takes a
 * caller supplied Message Item to send the request to the thread. If the caller
 * has created the message item with a reply port, then this routine will attempt
 * to await a reply from the data stream thread. In this case, it is the caller's
 * responsibility to detect the ReplyMsg() call out of the data stream thread in
 * order to know when the request completes and what the completion status is.
 *******************************************************************************************/
int32	_SendRequestToDSThread( Item msgItem, Boolean fAsync, DSStreamCBPtr streamCBPtr, 
						DSRequestMsgPtr reqMsg )
	{
	int32		status;
	Message*	msgPtr;
	Item		replyPortItem;

#if VALIDATE_REQUEST_CODE
	/* Do a range check on the whatToDo field to prevent queuing of
	 * messages that are bogus.
	 */
	if ( (reqMsg->whatToDo < 0) || (reqMsg->whatToDo > kDSOpConnect) )
		return kDSInvalidDSRequest;
#endif

	/* Get a pointer to the message structure. 
	 */
	reqMsg->msgItem = msgItem;
	msgPtr = (Message*) LookupItem( msgItem );
	if ( msgPtr == NULL )
		return -1;

	/* Get the reply port Item. 
	 */
	replyPortItem = msgPtr->msg_ReplyPort;
	if ( replyPortItem == 0)
		return kDSNoReplyPortErr;

	/* Send the request message to the stream thread
	 */
	status = SendMsg( streamCBPtr->requestPort, msgItem, reqMsg, sizeof(DSRequestMsg) );
	if ( status < 0 )
		return status;

	/* If the caller specifies synchronous operation (fAsync == FALSE), then
	 * wait right here for the result of the streamer request. 
	 * NOTE:	THERE IS A POTENTIAL FOR DEADLOCK HERE IF A SUBSCRIBER THREAD
	 *			CALLS ANY STREAMER FUNCTION SYNCHRONOUSLY!!!
	 */
	if ( fAsync == false )
		{
		/* Wait for a specific reply message to the one we just sent
		 */
		status = WaitForMsg( replyPortItem, NULL, &msgPtr, NULL, reqMsg->msgItem );
		if ( status < 0 )
			return status;

		status = msgPtr->msg_Result;
		}

	return status;
	}


/*******************************************************************************************
 * Routine to call each of a stream's subscribers with a message. If an error is
 * returned, some subscribers may not get called. Caller must supply a message
 * pool to allocate messages from (can be built with the FillPoolWithMsgItems() 
 * utility routine). There must be at least one message in the pool for each 
 * of the specified stream's subscribers.
 *******************************************************************************************/
int32	_SubscriberBroadcast( DSStreamCBPtr streamCBPtr, MemPoolPtr msgPoolPtr, 
								SubscriberMsgPtr subMsg )
	{
	int32				status;
	long				index;
	DSSubscriberPtr		sp;
	SubscriberMsgPtr	copySubMsg;

	/* If there are not a sufficient number of messages in the pool
	 * to send one to each subscriber, then we're doomed.
	 */
	if ( msgPoolPtr->numFreeInPool < streamCBPtr->numSubscribers )
		{
		ERR (("_SubscriberBroadcast() - insufficient number of subscriber messages\n"));
		return kDSNoMsgErr;
		}

	/* Send each subscriber a copy of the specified message. Bail out
	 * at the first sign of trouble.
	 */
	for ( index = 0; index < streamCBPtr->numSubscribers; index++ )
		{
		sp = streamCBPtr->subscriber + index;

		copySubMsg = (SubscriberMsgPtr) AllocPoolMem( msgPoolPtr );
		if ( copySubMsg != NULL )
			{
			/* Copy the input message to the one we just allocated so that
			 * a unique copy of the message can be sent to each subscriber.
			 * However, remember to preserve the actual 'msgItem' field in
			 * the message we just allocated!!!
			 */
			subMsg->msgItem = copySubMsg->msgItem;	/* move msgItem into the template */
			*copySubMsg = *subMsg;					/* ...now copy ALL fields from template */

			/* Send the message to the next subscriber */
			status = SendMsg( sp->subscriberPort, copySubMsg->msgItem, copySubMsg, 
								sizeof(SubscriberMsg) );
			if ( status < 0 )
				{
				CHECKRESULT( "_SubscriberBroadcast() -> SendMsg() ", status );
				break;
				}
			}
		else
			{
			ERR (("_SubscriberBroadcast() - ran out of subscriber messages\n"));
			status = kDSNoMsgErr;
			break;
			}
		}

	return status;
	}


/*******************************************************************************************
 * Routine to format a "subscribe" message and send it to the data stream thread
 *******************************************************************************************/
int32	DSSubscribe( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
							DSDataType dataType, Item subscriberPort )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo						= kDSOpSubscribe;
	reqMsg->msg.subscribe.dataType			= dataType;
	reqMsg->msg.subscribe.subscriberPort	= subscriberPort;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "preroll stream" message and send it to the data stream thread
 *******************************************************************************************/
int32	DSPreRollStream( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo = kDSOpPreRollStream;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "start stream" message and send it to the data stream thread
 *******************************************************************************************/
int32	DSStartStream( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
						unsigned long startOptions )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo = kDSOpStartStream;
	reqMsg->msg.start.options = startOptions;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "stop stream" message and send it to the data stream thread
 *******************************************************************************************/
int32	DSStopStream( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
						unsigned long stopOptions )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo = kDSOpStopStream;
	reqMsg->msg.stop.options = stopOptions;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to asynchronously broadcast a sync message to all subscribers
 *******************************************************************************************/
int32	DSClockSync( DSStreamCBPtr streamCBPtr, MemPoolPtr msgPoolPtr, 
						unsigned long nowTime )
	{
	int32			status;
	SubscriberMsg	subMsg;

	/* First, set the stream clock to the value specified. If the stream
	 * is not running, this will fail and we can return the appropriate
	 * result code.
	 */
	status = DSSetClock( streamCBPtr, nowTime );
	if ( status != kDSNoErr )
		return status;

	/* Fill in the prototype message for the broadcast function */
	subMsg.whatToDo			= kStreamOpSync;
	subMsg.privatePtr		= NULL;
	subMsg.msg.sync.clock	= nowTime;

	/* Send the message to all subscribers, including our caller if it
	 * is a subscriber of this stream.
	 */
	status = _SubscriberBroadcast( streamCBPtr, msgPoolPtr, &subMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "go marker" message and send it to the data stream thread
 *******************************************************************************************/
int32	DSGoMarker( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
						unsigned long markerValue, unsigned long options )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo						= kDSOpGoMarker;
	reqMsg->msg.goMarker.markerValue		= markerValue;
	reqMsg->msg.goMarker.options			= options;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "get channel status" message and send it to the data stream thread
 *******************************************************************************************/
int32	DSGetChannel( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
						DSDataType streamType, long channelNumber, long* channelStatusPtr )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo						= kDSOpGetChannel;
	reqMsg->msg.getChannel.streamType		= streamType;
	reqMsg->msg.getChannel.channelNumber	= channelNumber;
	reqMsg->msg.getChannel.channelStatusPtr	= channelStatusPtr;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "set channel status" message and send it to the data stream thread
 *******************************************************************************************/
int32	DSSetChannel( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
						DSDataType streamType, long channelNumber, long channelStatus )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo						= kDSOpSetChannel;
	reqMsg->msg.setChannel.streamType		= streamType;
	reqMsg->msg.setChannel.channelNumber	= channelNumber;
	reqMsg->msg.setChannel.channelStatus	= channelStatus;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "perform control" message and send it to the data stream thread.
 * These are used to communicate user determined requests to subscribers. All of these
 * are subscriber defined.
 *******************************************************************************************/
int32	DSControl( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
					DSDataType streamType, long userDefinedOpcode, void* userDefinedArgPtr )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo						= kDSOpControl;
	reqMsg->msg.control.streamType			= streamType;
	reqMsg->msg.control.userDefinedOpcode	= userDefinedOpcode;
	reqMsg->msg.control.userDefinedArgPtr	= userDefinedArgPtr;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "connect" message and send it to the data stream thread.
 * These are used to connect a data acquisition thread to the streamer, either as an
 * initialiization for the stream, or to change data sources during the streaming
 * process.
 *******************************************************************************************/
int32	DSConnect( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
						Item acquirePort )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

#if VALIDATE_CONNECT_PORT
	MsgPort*		msgPort;

	/* Validate that the port we are connecting to is either zero or
	 * is a valid port.
	 */
	if ( acquirePort != 0 )
		{
		msgPort = (MsgPort*) LookupItem( acquirePort );
		if ( ((long) msgPort) <= 0 )
			{
			kprintf( "DSConnect() - invalid message port specified: %lx\n", acquirePort );
			return kDSBadConnectPortErr;
			}
		else
			{
			if ( msgPort->mp.n_Type != MSGPORTNODE )
				{
				kprintf( "DSConnect() - message port specified not a port: %lx\n", acquirePort );
				return kDSBadConnectPortErr;
				}
			}
		}
#endif

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo				= kDSOpConnect;
	reqMsg->msg.connect.acquirePort	= acquirePort;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}


/*******************************************************************************************
 * Routine to format a "register for end of stream condition" message and send it to the 
 * data stream thread.
 *******************************************************************************************/
int32	DSWaitEndOfStream( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr )
	{
	int32			status;
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo	= kDSOpWaitEndOfStream;

	status = _SendRequestToDSThread( msgItem, fAsync, streamCBPtr, reqMsg );

	return status;
	}

