/*******************************************************************************************
 *	File:			ProtoSubscriber.c
 *
 *	Contains:		Prototype subscriber.
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/07/94		rdg		Modify to parse HALT chunks.
 *						Use unique #define names for trace switching
 *	6/10/94		rdg		Make the Macros for testing channel status have unique
 *						names.  
 *	5/31/94		rdg		Make "Channel" functions have unique names
 *	3/29/94		rdg		Allow for multiple levels of tracing.
 *	3/23/94		rdg		New today.
 *
 *******************************************************************************************/

/*****************************************************************************
 * Header file includes
 *****************************************************************************/
#include "DataStreamLib.h"
#include "MsgUtils.h"
#include "MemPool.h"
#include "ThreadHelper.h"
#include "Graphics.h"
#include "Audio.h"
#include "string.h"
#include "stdlib.h"				
#include "stdio.h"

#include "UMemory.h"

#include "Debug3DO.h"

#include "ProtoErrors.h"
#include "ProtoChannels.h"
#include "ProtoSubscriber.h"


/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if PROTO_TRACE_MAIN

#include "SubscriberTraceUtils.h"
#include "ProtoTraceCodes.h"

/* Declare the actual trace buffer.  Let it be global in case other 
 * modules in the subscriber want to write into it.
 */
TraceBuffer		ProtoTraceBuffer;

TraceBufferPtr	ProtoTraceBufPtr	= &ProtoTraceBuffer;

#define		ADD_PROTO_TRACE_L1( bufPtr, event, chan, value, ptr )	\
					AddTrace( bufPtr, event, chan, value, ptr )

#else

/* Trace is off */
#define		ADD_PROTO_TRACE_L1( bufPtr, event, chan, value, ptr )

#endif


/*****************************************************************************/
/* Pool from which to allocate subscriber contexts for multiple subscribers  */
/*****************************************************************************/
struct ProtoSubscriberGlobals	{
	MemPoolPtr		contextPool;		
	} ProtoGlobals;
	
/***********************************************************************/
/* Routines to handle incoming messages from the stream parser thread  */
/***********************************************************************/
static long		DoData( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoGetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoSetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoControl( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoOpening( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoClosing( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoStop( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoSync( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoEOF( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoAbort( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );

/********************************************************
 * Main Subscriber thread and its initalization routine 
 ********************************************************/
static long		InitializeThread( ProtoContextPtr ctx );
static void		ProtoSubscriberThread( long notUsed, ProtoContextPtr ctx );



/*==========================================================================================
  ==========================================================================================
							Subscriber procedural interfaces
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Routine to initialize the subscriber. Creates the a memory pool for allocating
 * subscriber contexts.  A context is allocated and a new thread started with
 * calls to NewProtoSubscriber().
 *******************************************************************************************/
long	InitProtoSubscriber( void )
	{
	ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceInitSubscriber, 0, 0, 0 );

	/* Create the memory pool for allocating subscriber
	 * contexts.
	 */
	ProtoGlobals.contextPool = CreateMemPool( PR_SUBS_MAX_SUBSCRIPTIONS, sizeof(ProtoContext) );
	if ( ProtoGlobals.contextPool == NULL )
		return kDSNoMemErr;

	/* Return success */
	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to deallocate things allocated by one-time initilaization in the
 * InitProtoSubscriber() routine.
 *******************************************************************************************/
long	CloseProtoSubscriber( void )
	{
	ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceCloseSubscriber, 0, 0, 0 );

	DeleteMemPool( ProtoGlobals.contextPool );

	ProtoGlobals.contextPool = NULL;
	
	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to instantiate a new subscriber. Creates the subscriber thread and passes the 
 * new context (allocated from the global pool) to it. Creates the message port through 
 * which all subsequent communications between the subscriber and the streamer take 
 * place, as well as any other necessary per-context resources. 
 *******************************************************************************************/
long	NewProtoSubscriber( ProtoContextPtr *pCtx, DSStreamCBPtr streamCBPtr, long deltaPriority )
	{
	long				status;
	ProtoContextPtr		ctx;
	ulong				signalBits;

	ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceNewSubscriber, 0, 0, 0 );

	/* Allocate a subscriber context */
	ctx = (ProtoContextPtr) AllocPoolMem( ProtoGlobals.contextPool );
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
				(void *) &ProtoSubscriberThread, 				/* thread entrypoint */
				4096, 											/* initial stack size */
				(long) CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
				NULL, 											/* name */
				&ctx->threadStackBlock, 						/* where to remember stack block addr */
				0, 												/* initial R0 */
				ctx );											/* initial R1 */

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

	ReturnPoolMem( ProtoGlobals.contextPool, ctx );

	return status;
	}


/*******************************************************************************************
 * Routine to get rid of all subscriber resources we created. This assumes that the
 * subscriber is in a clean state where it has returned all resources that were
 * passed to it, like messages. Deleting the subscriber thread should free up all
 * the rest of the stuff the thread allocated. What is left is the thread's stack
 * space, which we allocated, and the context block.
 *******************************************************************************************/
long	DisposeProtoSubscriber( ProtoContextPtr ctx )
	{
	if ( ctx != NULL )
		{
		ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceDisposeSubscriber, 0, 0, 0 );

		if ( ctx->threadItem > 0 )		DisposeThread( ctx->threadItem );
		if ( ctx->threadStackBlock )	FreePtr( ctx->threadStackBlock );

		RemoveMsgPort( ctx->requestPortSignal );
		
		ReturnPoolMem( ProtoGlobals.contextPool, ctx );
		}

	return kDSNoErr;
	}



/*==========================================================================================
  ==========================================================================================
					High level interfaces used by the main thread to process
									incoming messages. 
  ==========================================================================================
  ==========================================================================================*/
/*******************************************************************************************
 * Routine to process arriving data chunks. 
 *
 * NOTE:	Fields 'streamChunkPtr->streamChunkType' and 'streamChunkPtr->streamChunkSize'
 *			contain the 4 character stream data type and size, in BYTES, of the actual
 *			chunk data.
 *******************************************************************************************/
static long	DoData( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
{
	long					status;
	StreamChunkPtr			streamChunkPtr;
	ProtoHeaderChunkPtr 	protoHeader;
	ProtoDataChunkPtr		protoData;
	ProtoHaltChunkPtr		protoHalt;
	long					channelNumber;
	Item 					aTimerCue;
	
	streamChunkPtr	= (StreamChunkPtr) subMsg->msg.data.buffer;
	protoData 		= (ProtoDataChunkPtr) streamChunkPtr;
	protoHeader		= (ProtoHeaderChunkPtr) streamChunkPtr;
	protoHalt		= (ProtoHaltChunkPtr) streamChunkPtr;
	channelNumber	= protoData->channel;

	/* The incoming message could be data or a header chunk. Figure out
	 * which;  For headers, initalize the channel that the header arrived
	 * on.  For data, call the new data handling routine.
	 */
	switch (protoData->subChunkType)
		{
		case PDAT_CHUNK_TYPE:	/* Data msg arrived */

			status = ProcessNewProtoDataChunk( ctx, subMsg );
			break;
		
		case PHDR_CHUNK_TYPE:	/* Header msg arrived */

			status = InitProtoChannel( ctx, protoHeader );
			if (status < 0)  
				{
				printf( "Initializaion for proto channel %ld failed; error = %ld\n",
																		channelNumber, status );
				CloseProtoChannel( ctx, channelNumber );
				}

			/* Because the subscriber considers headers and data to both be "Data" 
			 * messages we have to reply to the header chunk message here.
			 * Data msgs will be replied to when the subscriber is done
			 * using the data.
			 */
			status = ReplyMsg( subMsg->msgItem, status, subMsg, sizeof(SubscriberMsg) ); 
			CHECKRESULT( "Reply to Proto header msg", status );
			break;

		case PHLT_CHUNK_TYPE:	/* HALT chunk arrived */
			ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kHaltChunkArrived, 
								protoHalt->channel, 
								protoHalt->haltDuration, 0 );

			/* Make an Audio Timer Cue */
			aTimerCue = CreateCue( NULL );

			/* Sleep for the duration specified in the HALT chunk */
			status = SleepUntilTime( aTimerCue, GetAudioTime() + protoHalt->haltDuration );

			DeleteCue( aTimerCue );

			/* As with header chunks, we have to reply to the header chunk message here.
			 */
			status = ReplyMsg( subMsg->msgItem, status, subMsg, sizeof(SubscriberMsg) ); 
			CHECKRESULT( "Reply to Proto header msg", status );

			ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kRepliedToHaltChunk, 
								protoHalt->channel, 
								protoHalt->haltDuration, 0 );
			break;

		default:
			/* For unrecognized or hosed chunk types */
			status = ReplyMsg( subMsg->msgItem, kDSNoErr, subMsg, sizeof(SubscriberMsg) ); 
			CHECKRESULT( "Reply to unrecognized proto chunk type", status );
			break;
		
		}	// switch
		
	return status;
	}

		
/*******************************************************************************************
 * Routine to set the status bits of a given channel.
 *******************************************************************************************/		
static long	DoSetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long				status;
	long				channelNumber;
	ProtoChannelPtr		channelPtr;
	long				wasEnabled;
	
	channelNumber	= subMsg->msg.channel.number;
	channelPtr		= ctx->channel + channelNumber;
	
	if ( channelNumber < PR_SUBS_MAX_CHANNELS )
		{
		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to 
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary.
		 */
		wasEnabled = IsProtoChanEnabled( channelPtr );

		/* Mask off bits reserved by the system or by the subscriber */
		channelPtr->status |= ( (~PROTO_CHAN_SUBSBITS | ~CHAN_SYSBITS) & 
									subMsg->msg.channel.status );

		/* If the channel is becoming disabled, flush data; if it is
		 * becoming enabled, start it up.
		 */
		if ( wasEnabled && (!IsProtoChanEnabled(channelPtr)) )
			status = FlushProtoChannel( ctx, channelNumber );
		else if ( !wasEnabled && (IsProtoChanEnabled(channelPtr)) )
			status = StartProtoChannel( ctx, channelNumber );
		}

	return status;
	}

		
/*******************************************************************************************
 * Routine to return the status bits of a given channel.
 *******************************************************************************************/
static long	DoGetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long		status;
	long		channelNumber;

	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < PR_SUBS_MAX_CHANNELS )
		status = ctx->channel[ channelNumber ].status;
	else
		status = 0;

	return status;
	}

		
/*******************************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 *******************************************************************************************/		
static long	DoControl( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long				status;
	long				userWhatToDo;
	ProtoCtlBlockPtr	ctlBlockPtr;

	userWhatToDo = subMsg->msg.control.controlArg1;
	ctlBlockPtr = (ProtoCtlBlockPtr) subMsg->msg.control.controlArg2;

	switch ( userWhatToDo )
		{
		case kProtoCtlOpTest:
			if ( ctlBlockPtr->test.channelNumber < PR_SUBS_MAX_CHANNELS )
				{
				// status = DoSomethingUseful( ctx, ctlBlockPtr->test.channelNumber,
				//								ctlBlockPtr->test.aValue );
				}
			else
				status = ProtoErrChanOutOfRange;
				break;

		default:
			status = 0;			/* ignore unknown control messages for now... */
			break;
		}
	
	return status;
	}

		
/*******************************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream, typically
 * just after the stream is opened.
 *******************************************************************************************/		
static long	DoOpening( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to close down an open subscription.
 *******************************************************************************************/		
static long	DoClosing( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS; channelNumber++ )
		CloseProtoChannel( ctx, channelNumber );

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to start all channels for this subscription.
 *******************************************************************************************/		
static long	DoStart( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Start all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		if ( subMsg->msg.start.options & SOPT_FLUSH )
			FlushProtoChannel( ctx, channelNumber );

		StartProtoChannel( ctx, channelNumber );
		}
		
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/		
static long	DoStop( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Stop all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		if ( subMsg->msg.stop.options & SOPT_FLUSH )
			FlushProtoChannel( ctx, channelNumber );		/* FlushProtoChannel() has an implicit Stop */
		else
			StopProtoChannel( ctx, channelNumber );		
		}	

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine flush all data waiting and/or queued under the assumption that we have 
 * just arrived at a branch point and should be ready to deal with all new data 
 * from an entirely different part of the stream.
 *******************************************************************************************/		
static long	DoSync( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Halt and flush all channels for this subscription, then restart.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS; channelNumber++ )
		FlushProtoChannel( ctx, channelNumber );
		
	DoStart( ctx, subMsg );
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to take action on end of file condition.
 *******************************************************************************************/		
static long	DoEOF( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 * Should return all channels to pre-initalized state.
 *******************************************************************************************/		
static long	DoAbort( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Halt, flush, and close all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS; channelNumber++ )
		CloseProtoChannel( ctx, channelNumber );

	return kDSNoErr;
	}

/*==========================================================================================
  ==========================================================================================
									The subscriber thread
  ==========================================================================================
  ==========================================================================================*/

static long	InitializeThread( ProtoContextPtr ctx )
	{
	long		status;
	long		k;

	/* Initialize fields in the context record */

	ctx->creatorStatus		= 0;			/* assume initialization will succeed */
	ctx->requestPort		= 0;

	/* Initialize once-only channel related things. 
	 */
	for ( k = 0; k < PR_SUBS_MAX_CHANNELS; k++ )
		{
		ctx->channel[k].status					= 0;
		ctx->channel[k].dataMsgQueue.head		= NULL;
		ctx->channel[k].dataMsgQueue.tail		= NULL;
		}

	/* Create the message port where the new subscriber will accept
	 * request messages.
	 */
	status = NewMsgPort( &ctx->requestPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->requestPort = status;

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio() ) < 0 )
		{
		ctx->creatorStatus = status;
		goto BAILOUT;
		}

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
 * This thread is started by a call to InitProtoSubscriber(). It reads the subscriber message
 * port for work requests and performs appropriate actions. The subscriber message
 * definitions are located in "DataStream.h".
 *******************************************************************************************/
static void		ProtoSubscriberThread( long notUsed, ProtoContextPtr ctx )
	{
	long				status;
	SubscriberMsgPtr	subMsg;
	ulong				signalBits;
	ulong				anySignal;
	long				channelNumber;
	Boolean				fKeepRunning;

	/* Call a utility routine to perform all startup initialization.
	 */
	if ( (status = InitializeThread( ctx )) != 0 )
		exit(0);

	/* All resources are now allocated and ready to use. Our creator has been informed
	 * that we are ready to accept requests for work. All that's left to do is
	 * wait for work request messages to arrive.
	 */
	fKeepRunning = true;
	while ( fKeepRunning )
		{
		anySignal = ctx->requestPortSignal;

		ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceWaitingOnSignal, 
					-1, ctx->requestPortSignal , 0 );

		signalBits = WaitSignal( anySignal );

		ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceGotSignal, -1, signalBits, 0 );

		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			/* Process any new requests for service as determined by the incoming
			 * message data.
			 */
			while( PollForMsg( ctx->requestPort, NULL, NULL, (void **) &subMsg, &status ) )
				{	
				switch ( subMsg->whatToDo )
					{
					case kStreamOpData:				/* new data has arrived */	
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, 
									kTraceDataMsg, -1, 0, subMsg );
						status = DoData( ctx, subMsg );		
						break;
		
		
					case kStreamOpGetChan:			/* get logical channel status */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, 
									kTraceGetChanMsg, subMsg->msg.channel.number, 
									0, subMsg );
						status = DoGetChan( ctx, subMsg );		
						break;
		
		
					case kStreamOpSetChan:			/* set logical channel status */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, 
									kTraceSetChanMsg, subMsg->msg.channel.number, 
									subMsg->msg.channel.status, subMsg );
						status = DoSetChan( ctx, subMsg );		
						break;
		
		
					case kStreamOpControl:			/* perform subscriber defined function */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceControlMsg, -1, 0, subMsg );
						status = DoControl( ctx, subMsg );		
						break;
		

					case kStreamOpSync:				/* clock stream resynched the clock */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceSyncMsg, -1, 0, subMsg );
						status = DoSync( ctx, subMsg );		
						break;
		
		
				/************************************************************************
				 * NOTE:	The following msgs have no extended message arguments
				 * 			and only may use the whatToDo and context
				 * 			fields in the message.
				 ************************************************************************/


					case kStreamOpOpening:			/* one time initialization call from DSH */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceOpeningMsg, -1, 0, subMsg );
						status = DoOpening( ctx, subMsg );		
						break;
		
		
					case kStreamOpClosing:			/* stream is being closed */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceClosingMsg, -1, 0, subMsg );
						status = DoClosing( ctx, subMsg );		
						fKeepRunning = false;

#if PROTO_DUMP_TRACE_ON_STREAM_CLOSE
						DumpRawTraceBuffer( ProtoTraceBufPtr, "ProtoTraceRawDump.txt" );
						DumpTraceCompletionStats( ProtoTraceBufPtr, kTraceDataSubmitted, 
											kTraceDataCompleted, "ProtoTraceStatsDump.txt" );
#endif
						break;
		
		
					case kStreamOpStop:				/* stream is being stopped */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceStopMsg, -1, 0, subMsg );
						status = DoStop( ctx, subMsg );
						break;
		
		
					case kStreamOpStart:			/* stream is being started */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceStartMsg, -1, 0, subMsg );
						status = DoStart( ctx, subMsg );
						break;
		
		
					case kStreamOpEOF:				/* physical EOF on data, no more to come */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceEOFMsg, -1, 0, subMsg );
						status = DoEOF( ctx, subMsg );		
						break;
		
		
					case kStreamOpAbort:			/* somebody gave up, stream is aborted */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceAbortMsg, -1, 0, subMsg );
						status = DoAbort( ctx, subMsg );		
						fKeepRunning = false;

#if PROTO_DUMP_TRACE_ON_STREAM_ABORT
						DumpRawTraceBuffer( ProtoTraceBufPtr, "ProtoTraceRawDump.txt" );
#endif
						break;
		
					default:
						;
					} /* switch whattodo */
		
				/* Reply to the request we just handled unless this is a "data arrived"
				 * message. These are replied to asynchronously as the data is actually
				 * consumed and must not be replied to here.
				 */
				if ( subMsg->whatToDo != kStreamOpData )
					{
					status = ReplyMsg( subMsg->msgItem, status, subMsg, sizeof(SubscriberMsg) );
					CHECKRESULT( "SAudioSubscriberThread() - ReplyMsg()", status );
					}

				} /* while PollForMsg */
				
			} /* if RequestPortSignal */

	} /* while KeepRunning */


	/* Halt, flush, and close all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS; channelNumber++ )
		CloseProtoChannel( ctx, channelNumber );

	/* Exiting should cause all resources we allocated to be
	 * returned to the system.
	 */
	ctx->threadItem = 0;
	exit(0);
	}


