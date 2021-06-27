
/*******************************************************************************************
 *	File:			SAudioFolioInterface.c
 *
 *	Contains:		Code to deal with low level folio issues related to streaming.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/8/94		fyp		Version 2.9.2
 *						Yanked trace compile switches to the make file level.
 *	5/31/94		rdg		Version 2.9.2
 *						Make "channel" function names unique
 *	10/26/93	rdg		Version 2.9
 *						Fixed number of parameters to MuteSAudioChannel().
 *	10/14/93	rdg		Version 2.8 -- New Today
 *						Split source into separate files.
 *						Keep high level subscriber message parsing and logical channel related
 *						functionality in SAudioSubscriber.c.  Move AudioFolio stuff here.
 *						Move trace implementation to SAudioTrace.c
 */
 

/*****************************************************************************
 * Header file includes
 *****************************************************************************/

#include "audio.h"
#include "Debug3DO.h"
#include "SAErrors.h"
#include "SAFolioInterface.h"
#include "SAudioSubscriber.h"
 
/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if TRACE_BUFFERS

#define TRACE_LEVEL		3

#include "SubscriberTraceUtils.h"
#include "SAudioTraceCodes.h"

/* Find the actual trace buffer. It's declared in SAMain.c.
 */
extern	TraceBufferPtr	SATraceBufPtr;

/* Allow for multiple levels of tracing */
#if (TRACE_LEVEL >= 1)
	#define		ADD_TRACE_L1( bufPtr, event, chan, value, ptr )	\
					AddTrace( bufPtr, event, chan, value, ptr )
#else
	#define		ADD_TRACE_L1( bufPtr, event, chan, value, ptr )
#endif

#if (TRACE_LEVEL >= 2)
	#define		ADD_TRACE_L2( bufPtr, event, chan, value, ptr )	\
					AddTrace( bufPtr, event, chan, value, ptr )
#else
	#define		ADD_TRACE_L2( bufPtr, event, chan, value, ptr )
#endif

#if (TRACE_LEVEL >= 3)
	#define		ADD_TRACE_L3( bufPtr, event, chan, value, ptr )	\
					AddTrace( bufPtr, event, chan, value, ptr )
#else
	#define		ADD_TRACE_L3( bufPtr, event, chan, value, ptr )
#endif


#else

/* Trace is off */
#define		ADD_TRACE_L1( bufPtr, event, chan, value, ptr )
#define		ADD_TRACE_L2( bufPtr, event, chan, value, ptr )	
#define		ADD_TRACE_L3( bufPtr, event, chan, value, ptr )

#endif



/*******************************************************************************************
 * Routine to pass to ForEachFreePoolMember which initalizes and properly inter-relates 
 * the AudioFolio samples, attachments, cues and signals needed to do the streaming.
 *******************************************************************************************/
Boolean	InitAudioBuffer( void* initBlockPtr, void* poolEntry )
	{
	int32					status;
	uint32					attachmentFlags;
	TagArg					Tags[6];
	SAudioBufferPtr			bufferPtr;
	SABufferInitBlockPtr	myInitBlockPtr;
	SAudioChannelPtr	 	chanPtr;
	SAudioHeaderChunkPtr	headerPtr;
		
	bufferPtr 		= (SAudioBufferPtr) poolEntry;
	myInitBlockPtr	= (SABufferInitBlockPtr) initBlockPtr;
	chanPtr 		= (SAudioChannelPtr) myInitBlockPtr->chanPtr;
	headerPtr		= myInitBlockPtr->headerPtr;
		
	/* Tags to describe the format of the data for this channel */
	Tags[0].ta_Tag = AF_TAG_CHANNELS;
	Tags[0].ta_Arg = (int32 *) headerPtr->sampleDesc.numChannels;
	Tags[1].ta_Tag = AF_TAG_NUMBITS;
	Tags[1].ta_Arg = (int32 *) headerPtr->sampleDesc.sampleSize;
	Tags[2].ta_Tag = AF_TAG_SAMPLE_RATE;
	Tags[2].ta_Arg = (int32 *) headerPtr->sampleDesc.sampleRate;
	Tags[3].ta_Tag = AF_TAG_COMPRESSIONRATIO;
	Tags[3].ta_Arg = (int32 *) headerPtr->sampleDesc.compressionRatio;
	Tags[4].ta_Tag = AF_TAG_COMPRESSIONTYPE ;
	Tags[4].ta_Arg = (int32 *) headerPtr->sampleDesc.compressionType;
	Tags[5].ta_Tag = TAG_END;
     
	/* Create a sample from scratch */	
	bufferPtr->sample = MakeSample ( 0 , &Tags[0]);
	CHECKRESULT("MakeSample", bufferPtr->sample);	
	
	/* Attach the sample to the instrument. */
	bufferPtr->attachment = AttachSample(chanPtr->channelInstrument, bufferPtr->sample, 0);
	CHECKRESULT("AttachSample", bufferPtr->attachment);
	
	/* Make sure attachments don't start automatically when StartInstrument() is called */
	attachmentFlags = AF_ATTF_NOAUTOSTART;

	Tags[0].ta_Tag = AF_TAG_SET_FLAGS;
	Tags[0].ta_Arg = (void *) attachmentFlags;
	Tags[1].ta_Tag = TAG_END;
	
	SetAudioItemInfo( bufferPtr->attachment, Tags );

	/* Create a Cue for signalback when the attachment completes */	
	bufferPtr->cue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	CHECKRESULT("CreateItem Cue", bufferPtr->cue);
	
	/* Request a signal when the sample ends. */
	status = MonitorAttachment( bufferPtr->attachment, bufferPtr->cue, CUE_AT_END );
	CHECKRESULT("MonitorAttachment", status);

	/* Get the signal associated with this cue */	
	bufferPtr->signal = GetCueSignal( bufferPtr->cue );

	/* add this signal to the buffer signals mask for this channel */	
	chanPtr->signalMask |= bufferPtr->signal;

	/* checkresult wouldn't let us get this far if we had a failure */		
	return true;		/* MUST RETURN TRUE IF SUCCESSFUL OR ForEachFreePoolMember() WILL DIE */
	}
	

/*******************************************************************************************
 * Routine to pass to ForEachFreePoolMember which frees up the AudioFolio samples, 
 * attachments, cues and signals needed to do the streaming.
 *******************************************************************************************/
Boolean	FreeAudioBuffer( void* initBlockPtr, void* poolEntry )
	{
	int32					status;
	SAudioBufferPtr			bufferPtr;
	SABufferInitBlockPtr	myInitBlockPtr;
	SAudioChannelPtr	 	chanPtr;
		
	bufferPtr 		= (SAudioBufferPtr) poolEntry;
	myInitBlockPtr	= (SABufferInitBlockPtr) initBlockPtr;
	chanPtr 		= (SAudioChannelPtr) myInitBlockPtr->chanPtr;
		
	/* Unload the sample. */
	status = UnloadSample(bufferPtr->sample);
	CHECKRESULT("UnloadSample", status);
	
	/* Delete a Cue  */	
	status = DeleteItem(bufferPtr->cue);
	CHECKRESULT("DeleteItem Cue", status);
	
	chanPtr->signalMask &= ~bufferPtr->signal;

	/* checkresult wouldn't let us get this far if we had a failure */		
	return true;		/* MUST RETURN TRUE IF SUCCESSFUL OR ForEachFreePoolMember() WILL DIE */
	}

/*******************************************************************************************
 * Routine to add a buffer onto the tail of the in use queue of buffers sent to the
 * audio folio for the given channel.
 *******************************************************************************************/
bool		AddBufferToTail( SAudioChannelPtr chanPtr, SAudioBufferPtr buffer )
	{
	bool	fQueueWasEmpty;

	buffer->link = NULL;

	if ( chanPtr->inuseQueueHead != NULL )
		{
		/* Add the new buffer onto the end of the 
		 * existing list of queued messages.
		 */
		chanPtr->inuseQueueTail->link = (void *) buffer;
		chanPtr->inuseQueueTail = buffer;
		fQueueWasEmpty = false;
		}
	else
		{
		/* Add the message to a buffer queue  */
		chanPtr->inuseQueueHead = buffer;
		chanPtr->inuseQueueTail = buffer;
		fQueueWasEmpty = true;
		}
	
	chanPtr->inuseCount++;

	return fQueueWasEmpty;
	}


/*******************************************************************************************
 * Routine to remove a data buffer from the head of the queue of buffers sent to the
 * audio folio for the given channel.
 *******************************************************************************************/
SAudioBufferPtr		GetNextBuffer( SAudioChannelPtr chanPtr )
	{
	SAudioBufferPtr	buffer;

	if ( (buffer = chanPtr->inuseQueueHead) != NULL )
		{
		/* Advance the head pointer to point to the next entry
		 * in the list.
		 */
		chanPtr->inuseQueueHead = (SAudioBufferPtr) buffer->link;

		/* If we are removing the tail entry from the list, set the
		 * tail pointer to NULL.
		 */
		if ( chanPtr->inuseQueueTail == buffer )
			chanPtr->inuseQueueTail = NULL;

		chanPtr->inuseCount--;
		}
		
	return buffer;
	}

/*******************************************************************************************
 * Routine to help prevent concurrent logical channels from getting out of sync with each other.
 *******************************************************************************************/
void		OrphanPlayingBuffers( SAudioContextPtr ctx )
	{
	long				channelNumber;
	SAudioChannelPtr	chanPtr;
	SAudioBufferPtr 	bufferPtr;

	/* Check all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		chanPtr = ctx->channel + channelNumber;

		/* If a channel is Enabled, Active, and has a buffer currently playing,
		 * link the buffer to 0 so it can't cross a buffer boundary while we 
		 * are ramping amplitudes and whatnot. 
		 */
		if ( IsChanEnabled(chanPtr) && IsChanActive(chanPtr) )
			{
			bufferPtr = chanPtr->inuseQueueHead;

			if ( bufferPtr != NULL )
				{
				LinkAttachments(bufferPtr->attachment, 0);  
				ADD_TRACE_L3( SATraceBufPtr, kBufferOrphaned, channelNumber, 
								bufferPtr->signal, bufferPtr );
				}
			}
		}
	}

/*******************************************************************************************
 * As long as there are waiting messages and free AudioFolio buffers, move the msgs
 * to the buffer queue.
 *******************************************************************************************/
void		MoveWaitingMsgsToBufferQueue( SAudioContextPtr ctx, long channelNumber )
	{
	SubscriberMsgPtr 	subMsg;
	SAudioBufferPtr 	bufferPtr;	
	SAudioDataChunkPtr	audioData;
	SAudioChannelPtr	chanPtr;
	TagArg				Tags[3];		

	chanPtr	= ctx->channel + channelNumber;

	/* If channel is active, and as long as there are data messages and free buffers,
	 * submit those buffers to the AudioFolio.
	 */
	if ( IsChanActive(chanPtr) )
		{
		/* Sanity check... If no header has ever arrived for this 
		 * channel (i.e. output instrument = 0), don't 
		 * try to queue buffers.  Somebody out there fucked up.
		 */	
		if ( (!IsChanInitalized(chanPtr)) && (chanPtr->dataQueue.head != NULL) )	
			{
			printf("\n\n SERIOUS ERROR on channel %ld! -- Data buffers arrived on an uninitalized channel!\n\n", channelNumber);
			return;
			}
		
		while ( (chanPtr->dataQueue.head != NULL) 
				&& ((bufferPtr = (SAudioBufferPtr) AllocPoolMem( chanPtr->bufferPool)) != NULL) ) 
			{
			subMsg = GetNextDataMsg( &chanPtr->dataQueue );
	
			audioData = (SAudioDataChunkPtr) subMsg->msg.data.buffer;
	
			/* set the Sample Item to point to the sample data from this chunk */
			Tags[0].ta_Tag = AF_TAG_ADDRESS;
			Tags[0].ta_Arg = &audioData->samples[0];
			Tags[1].ta_Tag = AF_TAG_NUMBYTES;
			Tags[1].ta_Arg = (void *) audioData->actualSampleSize;
			Tags[2].ta_Tag = TAG_END;
		
			SetAudioItemInfo( bufferPtr->sample, Tags );
					
			/* save this message so we can reply to it when the sample finishes playing */
			bufferPtr->pendingMsg = subMsg;
	
			/* add this signal to the current buffer signals mask for this subscriber context*/	
			ctx->allBufferSignals |= bufferPtr->signal;
	
			/* If the inuseQueue is not empty, link the last buffer submitted to the current one, 
			 * Since a link to a buffer is made when a following buffer arrives, the final 
			 * buffer will not be linked and should stop gracefully;  the instrument will 
			 * begin playing silence until something else happens.
			 */
			if (chanPtr->inuseCount != 0 ) 
				{
				LinkAttachments(chanPtr->inuseQueueTail->attachment, bufferPtr->attachment);
				}
	
			/* add this buffer to the list of those "in use" by the audio folio */
			AddBufferToTail( chanPtr, bufferPtr);

			ADD_TRACE_L3( SATraceBufPtr, kMovedBuffer, channelNumber, 
								bufferPtr->signal, bufferPtr );
			}
		}
	}

/*******************************************************************************************
 * Routine to pass the sample data to the audio folio for playback. 
 *******************************************************************************************/
void		QueueNewAudioBuffer( SAudioContextPtr ctx, SubscriberMsgPtr newSubMsg )
	{
	int32				status;
	SAudioDataChunkPtr	audioData;
	int32				channelNumber;
	SAudioChannelPtr	chanPtr;

	audioData 		= (SAudioDataChunkPtr) newSubMsg->msg.data.buffer;
	channelNumber 	= audioData->channel;
	chanPtr			= ctx->channel + channelNumber;

	if (IsChanEnabled(chanPtr))
		{
		AddDataMsgToTail( &chanPtr->dataQueue, newSubMsg );		
		MoveWaitingMsgsToBufferQueue( ctx, channelNumber );		
		}
	 else
	 	{
		/* got a data chunk but never got a header chunk to init the channel */  
		status = ReplyMsg( newSubMsg->msgItem, kDSNoErr, newSubMsg, sizeof(SubscriberMsg) );
		CHECKRESULT( "QueueNewAudioBuffer() - ReplyMsg()", status );
		}
	}

/*******************************************************************************************
 * Routine to clean up buffer block data before returning block to the free pool.
 ******************************************************************************************/
void		FreeBufferFromAudioFolio( SAudioBufferPtr bufferPtr )
	{
	TagArg		Tags[3];		

	/* clear the address and numbytes for this buffer */
	Tags[0].ta_Tag = AF_TAG_ADDRESS;
	Tags[0].ta_Arg = NULL;
	Tags[1].ta_Tag = AF_TAG_NUMBYTES;
	Tags[1].ta_Arg = (void *) 0;
	Tags[2].ta_Tag = TAG_END;

	SetAudioItemInfo( bufferPtr->sample, Tags );

	/* remove link to the next buffer since it's already started playing */
	LinkAttachments(bufferPtr->attachment, 0);  

	ADD_TRACE_L3( SATraceBufPtr, kFreedBufferFromFolio, -1, 
								bufferPtr->signal, bufferPtr );
	}
 
 
/*******************************************************************************************
 * Routine to figure out which buffer completed based on the signalBits returned from
 * the AudioFolio. 
 *******************************************************************************************/		
void  FindBuffer( SAudioContextPtr ctx, uint32 signalBits, 	
							SAudioChannelPtr *channelPtr, SAudioBufferPtr *bufferPtr) 
	{
	int32				channelNumber;
	SAudioChannelPtr	localChanPtr;
	SAudioBufferPtr		localBuffPtr;
	uint32				mySigBits;
				
	for (channelNumber=0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++)
		{
		localChanPtr = ctx->channel + channelNumber;
		localBuffPtr = localChanPtr->inuseQueueHead;	/* point to least recent entry in the in use list */		

		/* Mask off all bits that are not possible buffer completion signals for this channel */
		mySigBits = (signalBits & localChanPtr->signalMask);		
		
		if ( mySigBits )	/* is the signal from this channel? */
			{
			/* Try to determine if a buffer has completed out of order by comparing the signal
			 * we got from the AudioFolio with the signal associated with the buffer we found
			 * at the head of the in use queue. 
			 */
			if ( mySigBits & localBuffPtr->signal )
				{
				/* One of the completion signal for this channel matches the signal for the
				 * buffer at the head of the inUseQueue so we are happy...
				 */
				localBuffPtr = GetNextBuffer( localChanPtr ); 
				ADD_TRACE_L3( SATraceBufPtr, kFoundBuffer, channelNumber, 
								localBuffPtr->signal, localBuffPtr );
				}
			else
				{
				/* The completion signal for this channel was not for the buffer at the
				 * head of the inUseQueue.  This means that the System or the Folio is
				 * sending spurious shit to us.
				 */
				ADD_TRACE_L3( SATraceBufPtr, kFoundWrongBuffer, channelNumber, 
								localBuffPtr->signal, localBuffPtr );
				localBuffPtr = NULL;

				printf("\n\n SERIOUS ERROR on channel %ld! -- A buffer completion signal was received out of order!\n\n", channelNumber);

#if DUMP_TRACE_ON_BUFFER_COMP_ERR	
				DumpRawTraceBuffer( SATraceBufPtr, "SAudioRawTrace.txt" );		
				DumpTraceCompletionStats( SATraceBufPtr, kMovedBuffer, 
									kBufferCompleted, "SAudioTraceStats.txt" );
#endif
				}

			break;
			}
		}
	
	*channelPtr = localChanPtr;
	*bufferPtr = localBuffPtr;			
	
	}

/*******************************************************************************************
 * Routine to handle completed chunks.  Figure out which chunk on which channel 
 * completed.  Get the subscriber message ptr and reply to the message.  Next, check to
 * see if there are any new messages in the queue, if there are, use the same sample
 * resources for the new message.    
 *******************************************************************************************/		
void		HandleCompletedBuffers( SAudioContextPtr ctx, uint32 signalBits )
	{
	int32				status;
	SubscriberMsgPtr 	subMsg;
	SAudioDataChunkPtr	audioData;
	SAudioChannelPtr	chanPtr;
	SAudioBufferPtr		bufferPtr;
	long				channelNumber;
	Boolean				clockChannelCompletion;
	
	/* Assume that this buffer completion was not on the clock channel */
	clockChannelCompletion = false;

	ADD_TRACE_L3( SATraceBufPtr, kBufferCompleted, -1, signalBits, 0 );

	/* Mask off anything that isn't a buffer completion signal */
	signalBits &= ctx->allBufferSignals;
	
	ADD_TRACE_L3( SATraceBufPtr, kMaskedSignalBits, -1, signalBits, 0 );

	/* Service all completed buffers */
	while ( signalBits )
		{
		/* Search all channels in acending order and return least recently submitted 
		 * buffer on first channel which gets a hit.
		 */
		FindBuffer(ctx, signalBits, &chanPtr, &bufferPtr);
		
		/* If FindBuffer() found the wrong buffer, or no buffer then do nothing else */
		if ( bufferPtr == NULL )
			goto error;
			
		/* Remove this buffer's signal from the current completed buffer signals */
		signalBits &= ~bufferPtr->signal;

		/* Remove this buffer's signal from the "currently queued" buffer signals mask 
		 * for this context
		 */	
		ctx->allBufferSignals &= ~bufferPtr->signal;

		/* Hack to figure out which channel this buffer is on before 
		 * the message is replied to.
		 */
		subMsg = bufferPtr->pendingMsg;
		audioData = (SAudioDataChunkPtr) subMsg->msg.data.buffer;	
		channelNumber = audioData->channel;

		/* If we had a buffer completion on the clock channel we need to know so we can
		 * set the stream time when we are done processing.  We can't set the clock now because we
		 * might have gotten more than one buffer completion on the clock channel.
		 */
		if ( channelNumber == ctx->clockChannel )
			clockChannelCompletion = true;
	
		/* Jam a magic number in the first longword of the sample data
		 * so the outside world can tell we've played and replied to
		 * this data message.
		 */
		*(long*) &audioData->samples = 0xdeadbeef;
		
		/* Reply to the message */
		status = ReplyMsg(	bufferPtr->pendingMsg->msgItem, 
						kDSNoErr, 				
						bufferPtr->pendingMsg, 
						sizeof(SubscriberMsg)
				);
		CHECKRESULT( "HandleCompletedBuffers() - ReplyMsg() #1", status );


		/* Disassociate buffer from a subscriber msg */
		bufferPtr->pendingMsg = NULL;
				
		/* Disassociate buffer from any audio folio links */
		FreeBufferFromAudioFolio( bufferPtr );

		/* Free the buffer back into the buffer pool */	
		ReturnPoolMem( chanPtr->bufferPool, bufferPtr );

		/* If there are any msgs waiting in the dataQueue, use up any free AudioFolio buffers
		 * and move them to the inUseQueue.
		 */		
		MoveWaitingMsgsToBufferQueue( ctx, channelNumber );			

		/* If there is now no data in the inuseQueue or the dataQueue, then this was the
		 * last buffer to complete and this channel is in idle.
		 */
		 if ( (chanPtr->inuseCount == 0) && (chanPtr->dataQueue.head == NULL) )
			{
			/* Because the last attachment (buffer) was linked to zero, the chain 
			 * will stop by itself when the end is reached.  No need to call 
			 * StopAttachment() explicitly.
			 */
			chanPtr->attachmentsRunning = false;

			/* Turn the mixer down now that sound has stopped */
			MuteSAudioChannel( ctx, channelNumber, INTERNAL_MUTE );
			}
		}

		/* We now need to update the stream time with the time stamp in the chunk
		 * which just _started_ playing. If we got buffer completion(s) on the clock
		 * channel, we know that the buffer which just started playing is now the at 
		 * the head of the inuseQueue for the clock channel.  So we get the time stamp 
		 * out of the message and set the clock accordingly.  
		 * If the inuseQueueHead is NULL then this was the last buffer to complete so we 
		 * don't need to do anything and the clock will free run.
		 */
		if ( clockChannelCompletion )
			{
			chanPtr	= ctx->channel + ctx->clockChannel;
			bufferPtr = chanPtr->inuseQueueHead;			
			if (bufferPtr != NULL)
				{
				subMsg = bufferPtr->pendingMsg;
				audioData = (SAudioDataChunkPtr) subMsg->msg.data.buffer;	
				if (audioData->channel == ctx->clockChannel)
					DSSetClock(ctx->streamCBPtr, audioData->time);
				}
			}
error:
	status = 1;
	}
		
		
