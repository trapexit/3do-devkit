
/*******************************************************************************************
 *	File:			SAChannel.c
 *
 *	Contains:		Mid-level routines for logical channel specific functions.
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/8/94		fyp		Version 2.9.2
 *						Yanked trace compile switches to the make file level.
 *	5/31/94		rdg		Version 2.9.2
 *						Make "channel" function names unique
 *	3/29/94		rdg		Version 2.9.1
 *						Add new trace code.
 *	10/26/93	rdg		Version 2.9 
 *						Distinguish between user and internal muting and unmuting
 *						Get rid of envelope rate knobs in channel context... tweak
 *						once at init time.
 *	10/15/93	rdg		Version 2.8
 *						Split source into separate files.
 */


/*****************************************************************************
 * Header file includes
 *****************************************************************************/

#include "types.h"
#include "audio.h"
#include "Debug3DO.h"
#include "SAErrors.h"
#include "SAChannel.h"
#include "SAudioSubscriber.h"
#include "SAStreamChunks.h"
#include "SAFolioInterface.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if TRACE_CHANNELS

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


/* External definition for number of audio instrument templates */
extern long kMaxTemplateCount;

/*******************************************************************************************
 * Routine to initalize an audio channel for a given context.  Allocates a pool of memory
 * for the AudioFolio buffers (attachments, cues, etc) and initalizes the Items.
 *******************************************************************************************/
long	InitSAudioChannel( SAudioContextPtr ctx, SAudioHeaderChunkPtr headerPtr )
	{
	long				status;
	SAudioChannelPtr	chanPtr;
	SABufferInitBlock	buffInitBlock;
	SAudioOutputPtr		chanOutputPtr;
	Item				templateItem;
	long				templateTag;
	Item				tempKnob;
			
	chanPtr = ctx->channel + headerPtr->channel;
	chanOutputPtr = &chanPtr->channelOutput;
	
	ADD_TRACE_L2( SATraceBufPtr, kTraceChannelInit, headerPtr->channel, 0, 0 );

	/* Make sure this version of .saudio files are compatible with tihs subscriber */
	if ( headerPtr->version > SAUDIO_STREAM_VERSION )
		return SAudioErrStreamVersionInvalid;

	/* Default channel 0 to enabled.  All other channels must be explicitly
	 * enabled with a SetChan message.
	 */
	if (headerPtr->channel == 0)
		chanPtr->status	|= CHAN_ENABLED;  /* MUST HAVE BOTH CHAN_ENABLED & CHAN_ACTIVE TO PLAY ANY DATA */

	/* Always make the channel active when we receive the header */
	chanPtr->status	|= CHAN_ACTIVE;

	/* If the channel does not already have an output instrument (mixer), make one
	 * from the template.
	 */
	if ( chanOutputPtr->instrument == 0 )
		{
		status = AllocInstrument( ctx->outputTemplateItem, 100 );
		if ( status < 0 ) 	return SAudioErrAllocInstFailed;

		chanOutputPtr->instrument = status;
		
		/* Start the mixer */
		StartInstrument( chanOutputPtr->instrument, NULL );

		/* Allocate envelope(s) for ramping */
		status = AllocInstrument( ctx->envelopeTemplateItem, 100 );
		if ( status < 0 ) 	return SAudioErrAllocInstFailed;

		chanOutputPtr->leftEnv = status;
		
		/* Connect Envelope to amplitude knob */
		status = ConnectInstruments (chanOutputPtr->leftEnv, "Output", 
										chanOutputPtr->instrument, "LeftGain0");
		if ( status < 0 ) 	return SAudioErrConnectInstFailed;

		/* Grab the envelope's rate knob so we can control the speed of ramping */
		status = GrabKnob( chanOutputPtr->leftEnv, "Env.incr" );
		if ( status < 0 ) 	return SAudioErrGrabKnobFailed;

		tempKnob = status;
		
		/* 
		 * 1 = 3/4 sec; 2 = twice at fast as 1;
		 */
		TweakRawKnob( tempKnob, 10 );
		
		ReleaseKnob( tempKnob );
		
		/* Grab the envelope's target knob so we can set the value to ramp to */
		status = GrabKnob( chanOutputPtr->leftEnv, "Env.request" );
		if ( status < 0 ) 	return SAudioErrGrabKnobFailed;

		chanOutputPtr->leftEnvTargetKnob = status;

		/* Start the left envelope */
		StartInstrument( chanOutputPtr->leftEnv, NULL );

		/* If this is a mono channel, we need two envelopes so we can pan properly */
		if ( headerPtr->sampleDesc.numChannels == 1 )
			{
			status = AllocInstrument( ctx->envelopeTemplateItem, 100 );
			if ( status < 0 ) 	return SAudioErrAllocInstFailed;
	
			chanOutputPtr->rightEnv = status;

			/* Connect Envelope to amplitude knobs for mono ramping and panning. */
			status = ConnectInstruments (chanOutputPtr->rightEnv, "Output", 
											chanOutputPtr->instrument, "RightGain0");
			if ( status < 0 ) 	return SAudioErrConnectInstFailed;

			/* Grab the envelope's rate knob so we can control the speed of ramping */
			status	= GrabKnob( chanOutputPtr->rightEnv, "Env.incr" );
			if ( status < 0 ) 	return SAudioErrGrabKnobFailed;
	
			tempKnob = status;
			
			/* 
			 * 1 = 3/4 sec; 2 = twice at fast as 1;
			 */
			TweakRawKnob( tempKnob, 10);
			
			ReleaseKnob( tempKnob );

			/* Grab the envelope's target knob so we can set the value to ramp to */
			status = GrabKnob( chanOutputPtr->rightEnv, "Env.request" );
			if ( status < 0 ) 	return SAudioErrGrabKnobFailed;

			chanOutputPtr->rightEnvTargetKnob = status;

			/* Start the left envelope */
			StartInstrument( chanOutputPtr->rightEnv, NULL );
			}
		else
			{
			/* Connect Envelopes to amplitude knobs for stereo ramping. */
			status = ConnectInstruments (chanOutputPtr->leftEnv, "Output", 
											chanOutputPtr->instrument, "RightGain1");
			if ( status < 0 ) 	return SAudioErrConnectInstFailed;
			}
		}

	/* If the channel's playback instrument has not been created - or was deleted with 
	 * CloseSAudioChannel() -, figure out which one to make based on the header data in the audio stream.
	 */
	if ( chanPtr->channelInstrument == 0 )
		{
		/* Remember if this is a stereo or mono stream */
		if (headerPtr->sampleDesc.numChannels == 1)
			chanOutputPtr->numChannels = 1;
		else  
			chanOutputPtr->numChannels = 2;
			
		/* Set the maximum buffers that can be queued to the AudioFolio at any one time */
		chanPtr->numBuffers	= headerPtr->numBuffers;
	
		/* Figure out which sample player instrument we should load for that data type */	
		status = GetTemplateTag( &headerPtr->sampleDesc ); 
		if ( status < 0 )	return SAudioErrUnsupSoundfileType;
		templateTag = status;
		
		/* Get the instrument template item we need for building an instrument */
		templateItem = GetTemplateItem( ctx->templateArray, templateTag, kMaxTemplateCount );
		if ( templateItem < 0 )  return SAudioErrTemplateNotFound;
	
		/* Make an instrument from the template */
		status = AllocInstrument( templateItem, 100 );
		if ( status < 0 ) 	return SAudioErrAllocInstFailed;
	
		chanPtr->channelInstrument = status;
	
		/* Connect Sampler to output- do different things depending on stereo or mono */
		if (chanOutputPtr->numChannels == 1)
			{
			status = ConnectInstruments (chanPtr->channelInstrument, "Output", chanOutputPtr->instrument, "Input0");
			if ( status < 0 ) 	return SAudioErrConnectInstFailed;
			}	
		else 
			{
			status = ConnectInstruments (chanPtr->channelInstrument, "LeftOutput", chanOutputPtr->instrument, "Input0");
			if ( status < 0 ) 	return SAudioErrConnectInstFailed;
			status = ConnectInstruments (chanPtr->channelInstrument, "RightOutput", chanOutputPtr->instrument, "Input1");
			if ( status < 0 ) 	return SAudioErrConnectInstFailed;
			}
	
		/* Since we're going to use Start/StopAttachment() to deal with playback, we
		 * can just start the instrument now and leave it running the whole time.
		 */
		StartInstrument(chanPtr->channelInstrument, NULL);
		chanPtr->instStarted = true;

		/* In order to eliminate pops once and for all, don't actually set the amplitude of a channel
		 * until after StartInstrument() has been called by BeginSAudioPlaybackIfAppropriate().  This is
		 * accomplished by calling MuteSAudioChannel() before SetSAudioChannelAmplitude().  This will cause
		 * UnMuteSAudioChannel() to restore the volume we set here when we want.
		 */
		MuteSAudioChannel(ctx, headerPtr->channel, INTERNAL_MUTE);
		
		/* CurrentAmp was initalized to -1 by the main thread so the user can set the amplitude 
		 * to zero with a control message before the stream started and not have it be overided 
		 * by the InitailAmplitude setting in the .saudio header. In other words, -1 means "never
		 * set" and 0 means "zero amplitude". 
		 * Since we've called MuteSAudioChannel(), the current amplitude setting has been saved in
		 * savedAmp.  We need to check that now to find out weather we've received an amplitude
		 * change control message or not.  
		 * This is admittedly a bit ugly, but it's the only place where we need to worry about 
		 * such stuff - so it gets left this way.
		 * 
		 * So, if we didn't get a control message, use the initialAmplitude field from the .saudio
		 * header - otherwise MuteSAudioChannel() captured whatever the user set with the control call
		 * and UnMuteSAudioChannel() will restore it when appropriate.  Clear as mud?
		 */
		if ( chanOutputPtr->savedAmp == -1 )
			status = SetSAudioChannelAmplitude( ctx, headerPtr->channel, headerPtr->initialAmplitude );

		if ( status < 0 ) 	return status;
		
		/* Only try to set channel pan if this is a mono stream; SetSAudioChannelPan() will
		 * return an error if called on stereo data.
		 */
		if (chanOutputPtr->numChannels == 1)
			{
			if ( chanOutputPtr->currentPan == 0)
				status = SetSAudioChannelPan( ctx, headerPtr->channel, headerPtr->initialPan );
			else
				status = SetSAudioChannelPan( ctx, headerPtr->channel, chanOutputPtr->currentPan );
			}
	
		if ( status < 0 ) 	return status;
		}
		
	/* Create a pool of buffers that can be allocated when new data arrives  
	 * and freed when buffers complete.
	 */
	if ( chanPtr->bufferPool == 0 )
		{
		chanPtr->bufferPool = CreateMemPool( headerPtr->numBuffers, sizeof(SAudioBuffer) );
		if ( chanPtr->bufferPool == NULL ) 	return SAudioErrCouldntAllocBufferPool;
		
		buffInitBlock.chanPtr = chanPtr;
		buffInitBlock.headerPtr = headerPtr;
		
		/* Initalize every pool member. */
		ForEachFreePoolMember( chanPtr->bufferPool, InitAudioBuffer, (void*) &buffInitBlock );	
		}
		
	return kDSNoErr;
	}

/*******************************************************************************************
 * Routine to begin data flow for the given channel.
 *******************************************************************************************/
void		StartSAudioChannel( SAudioContextPtr ctx, long channelNumber )
	{
	SAudioChannelPtr	chanPtr;
	
	chanPtr  =  ctx->channel + channelNumber;

	ADD_TRACE_L2( SATraceBufPtr, kTraceChannelStart, channelNumber, 0, 0 );

	if ( IsChanEnabled(chanPtr) && (!IsChanActive(chanPtr)) )
		{
		chanPtr->status |= CHAN_ACTIVE;

		/* If there are any msgs waiting in the dataQueue, use any free AudioFolio buffers
		 * and move them to the inUseQueue and try to start them playing.
		 */		
		MoveWaitingMsgsToBufferQueue( ctx, channelNumber );			

		BeginSAudioPlaybackIfAppropriate( ctx, channelNumber );
		}
	}

/*******************************************************************************************
 * Routine to halt data flow for the given channel.
 *******************************************************************************************/
long		StopSAudioChannel( SAudioContextPtr ctx, long channelNumber )
	{
	long				status;
	SAudioChannelPtr	chanPtr;
	SAudioBufferPtr 	bufferPtr;

	chanPtr  =  ctx->channel + channelNumber;

	ADD_TRACE_L2( SATraceBufPtr, kTraceChannelStop, channelNumber, 0, 0 );

	if ( IsChanEnabled(chanPtr) && IsChanActive(chanPtr) )
		{
		/* Since we know which channel we're dealing with, we can just use
		 * GetNextBuffer() because it removes the buffer at the head of the 
		 * inuseQueue for the specified channel which MUST be the currently 
		 * playing buffer.  If bufferPtr turns out to be NULL, then no 
		 * data was playing so we don't need to do anything.
		 */
		if ((bufferPtr = GetNextBuffer( chanPtr )) != NULL)		
			{
			/* Make sure mixer knobs are at zero */
			MuteSAudioChannel( ctx, channelNumber, INTERNAL_MUTE );		

			/* Stop the attachment */
			StopAttachment(bufferPtr->attachment, NULL);
			chanPtr->attachmentsRunning = false;	

			/* Leave a trace so we can tell which buffer was playing when stop was called */
			ADD_TRACE_L3( SATraceBufPtr, kCurrentBufferOnStop, channelNumber, 
							bufferPtr->signal, bufferPtr );

			/* Check to see if this buffer completed before we could get to StopInstrument() 
			 * and if it did, call WaitSignal() to clear the bit. 
			 */
			if ( (GetCurrentSignals() & bufferPtr->signal) )
				WaitSignal(bufferPtr->signal);
			
			/* Reply to the message */
			status = ReplyMsg(	bufferPtr->pendingMsg->msgItem, 
							kDSNoErr, 				
							bufferPtr->pendingMsg, 
							sizeof(SubscriberMsg)
					);
			CHECKRESULT( "StopSAudioChannel() - ReplyMsg() #1", status );

			/* Disassociate buffer from a subscriber msg */
			bufferPtr->pendingMsg = NULL;
						
			/* Disassociate buffer from any audio folio links */
			FreeBufferFromAudioFolio( bufferPtr );
			
			/* Remove this signal from the current buffer signals mask for this context*/	
			ctx->allBufferSignals &= ~bufferPtr->signal;
			 
			/* Free the buffer back into the buffer pool */	
			ReturnPoolMem( chanPtr->bufferPool, bufferPtr );
			}

		chanPtr->status &= ~CHAN_ACTIVE;
		}
	return status;
	}


/*******************************************************************************************
 * Routine to disable further data flow for the given channel, and to cause
 * any queued data to be flushed.
 *******************************************************************************************/
long		FlushSAudioChannel( SAudioContextPtr ctx, long channelNumber )
	{
	long				status;
	SAudioChannelPtr	chanPtr;
	SubscriberMsgPtr	msgPtr;
	SAudioBufferPtr		bufferPtr;

	chanPtr  =  ctx->channel + channelNumber;

	ADD_TRACE_L2( SATraceBufPtr, kTraceChannelFlush, channelNumber, 0, 0 );

	if (IsChanEnabled(chanPtr))	
		{
		/* Halt whatever activity is associated with the channel */
		status = StopSAudioChannel( ctx, channelNumber );
		CHECKRESULT( "FlushSAudioChannel() - StopSAudioChannel() failed", status );

		/* Give back all queued chunks for this channel to the
		 * stream parser. We do this by replying to all the
		 * "chunk arrived" messages that we have queued.
		 */
		while (	( msgPtr = GetNextDataMsg( &chanPtr->dataQueue ) ) != NULL ) 	
			{
			/* Reply to this chunk so that the stream parser
			 * can eventually reuse the buffer space.
			 */
			ADD_TRACE_L3( SATraceBufPtr, kFlushedDataMsg, channelNumber, 
							0, msgPtr );
			status = ReplyMsg( msgPtr->msgItem, kDSNoErr, msgPtr, sizeof(SubscriberMsg));
			CHECKRESULT( "FlushSAudioChannel() - ReplyMsg() #1", status );
			}

		/* Give back all inuse queued chunks for this channel to the
		 * stream parser. We do this by replying to all the
		 * "chunk arrived" messages that we have queued as inuse.
		 */
		while ((bufferPtr = GetNextBuffer( chanPtr )) != NULL)		
			{
			/* Reply to the message */
			ADD_TRACE_L3( SATraceBufPtr, kFlushedBuffer, channelNumber, 
							bufferPtr->signal, bufferPtr );

			status = ReplyMsg(	bufferPtr->pendingMsg->msgItem, 
							kDSNoErr, 				
							bufferPtr->pendingMsg, 
							sizeof(SubscriberMsg)
					);
			CHECKRESULT( "FlushSAudioChannel() - ReplyMsg() #2", status );

			/* Disassociate buffer from a subscriber msg */
			bufferPtr->pendingMsg = NULL;
									
			/* Disassociate buffer from any audio folio links */
			FreeBufferFromAudioFolio( bufferPtr );
			
			/* Remove this signal from the current buffer signals mask for this context*/	
			ctx->allBufferSignals &= ~bufferPtr->signal;
			 
			/* Free the buffer back into the buffer pool */	
			ReturnPoolMem( chanPtr->bufferPool, bufferPtr );
			}
		}
	return status;
	}

/*******************************************************************************************
 * Stop and Flush a channel; Then free up all it's resources.  Should leave a channel
 * in pre-initalized state.
 *******************************************************************************************/
long		CloseSAudioChannel( SAudioContextPtr ctx, long channelNumber )
	{
	long				status;
	SABufferInitBlock	buffInitBlock;
	SAudioChannelPtr	chanPtr;
	SAudioOutputPtr		chanOutputPtr;
	
	chanPtr	= ctx->channel + channelNumber;
	chanOutputPtr = &chanPtr->channelOutput;

	ADD_TRACE_L2( SATraceBufPtr, kTraceChannelClose, channelNumber, 0, 0 );

	/* Stop any activity and flush pending buffers if any */	
	FlushSAudioChannel( ctx, channelNumber );
		
	/* If the channel was initialized, free it up */
	if ( chanPtr->channelInstrument > 0 )
		{
		status = FreeInstrument(chanPtr->channelInstrument);
		if ( status < 0 )
			return SAudioErrUnableToFreeInstrument;
	
		chanPtr->channelInstrument = 0;
		}

	if ( chanOutputPtr->instrument > 0 )
		{
		/* The 2x2 mixer */
		StopInstrument(chanOutputPtr->instrument, NULL);
		
		status = FreeInstrument(chanOutputPtr->instrument);
		if ( status < 0 )
			return SAudioErrUnableToFreeInstrument;
	
		chanOutputPtr->instrument = 0;

		/* The left envelope, this always gets created */
		StopInstrument(chanOutputPtr->leftEnv, NULL);
		
		status = FreeInstrument(chanOutputPtr->leftEnv);
		if ( status < 0 )
			return SAudioErrUnableToFreeInstrument;
	
		chanOutputPtr->leftEnv = 0;
		
		/* If this was a mono channel and two envelopes exist */
		if ( chanOutputPtr->rightEnv > 0 )
			{
			StopInstrument(chanOutputPtr->rightEnv, NULL);
			
			status = FreeInstrument(chanOutputPtr->rightEnv);
			if ( status < 0 )
				return SAudioErrUnableToFreeInstrument;
		
			chanOutputPtr->rightEnv = 0;
			}
	}
		
	/* Clean up and free the pool of AudioFolio buffers that we created for this channel.
	 */
	if ( chanPtr->bufferPool != NULL )
		{
		buffInitBlock.chanPtr = chanPtr;
		
		/* Free every pool member. */
		ForEachFreePoolMember( chanPtr->bufferPool, FreeAudioBuffer, (void*) &buffInitBlock );	

		DeleteMemPool(chanPtr->bufferPool);
		}

	/* Reset all of the channel's variables */
	chanPtr->status								= 0;
	chanPtr->numBuffers							= 0;
	chanPtr->bufferPool							= 0;
	chanPtr->channelInstrument					= 0;
	chanPtr->instStarted						= false;
	chanPtr->attachmentsRunning					= false;
	chanPtr->channelOutput.instrument			= 0;
	chanPtr->channelOutput.numChannels			= 0;
	chanPtr->channelOutput.leftEnv				= 0;
	chanPtr->channelOutput.leftEnvTargetKnob	= 0;
	chanPtr->channelOutput.rightEnv				= 0;
	chanPtr->channelOutput.rightEnvTargetKnob	= 0;
	chanPtr->channelOutput.currentAmp			= -1;
	chanPtr->channelOutput.savedAmp				= 0;
	chanPtr->channelOutput.currentPan			= 0;
	chanPtr->channelOutput.muted				= false;
	chanPtr->channelOutput.externalMute			= false;
	chanPtr->signalMask							= 0;
	chanPtr->dataQueue.head						= NULL;
	chanPtr->dataQueue.tail						= NULL;
	chanPtr->inuseCount							= 0;
	chanPtr->inuseQueueHead						= NULL;
	chanPtr->inuseQueueTail						= NULL;

	return status;
	}

/*******************************************************************************************
 * Routine to set a channel's amplitude.
 *******************************************************************************************/
long		SetSAudioChannelAmplitude( SAudioContextPtr ctx, long channelNumber, long newAmp )
	{
	SAudioChannelPtr	chanPtr;
	SAudioOutputPtr		chanOutputPtr;
	long				leftAmp;
	long				rightAmp;
	
	chanPtr  		= ctx->channel + channelNumber;
	chanOutputPtr	= &chanPtr->channelOutput;

	ADD_TRACE_L3( SATraceBufPtr, kBeginSetChannelAmp, channelNumber, 
							chanOutputPtr->currentAmp, 0 );

	if ( (newAmp < 0x0) || (newAmp > 0x7FFF) )
		return SAudioErrVolOutOfRange;

	/* If instrument == 0 then the channel has not been initalized so 
	 * just store the value and don't try to change a knob that doesn't
	 * exist yet. If channel is muted, save value in savedAmp.
	 */
	if ( (chanOutputPtr->instrument > 0) && (!chanOutputPtr->muted) )
		{
		if ( chanOutputPtr->numChannels == 2 )
			{
			/* Set the ramp target */
			TweakRawKnob(chanOutputPtr->leftEnvTargetKnob, newAmp);
			}
		else
			{
			/* if this stream is mono, maintain proper panning */
			if ( chanOutputPtr->currentPan < 0x4000 )
				{
				leftAmp = newAmp;
				rightAmp = (chanOutputPtr->currentPan * newAmp) >> 14;
				}
			else
				{
				rightAmp = newAmp;
				leftAmp = ((0x8000 - chanOutputPtr->currentPan) * newAmp) >> 14;
				}

			/* Set the left ramp target */
			TweakRawKnob(chanOutputPtr->leftEnvTargetKnob, leftAmp);

			/* Set the right ramp target */
			TweakRawKnob(chanOutputPtr->rightEnvTargetKnob, rightAmp);
			}
		}

	/* If the channel is muted or the instrument was not created yet, real amplitude 
	 * values were not changed; but, we want GetSAudioChannelAmplitude() to return the 
	 * new setting AND we want UnMuteSAudioChannel() to restore to the proper value.
	 */
	chanOutputPtr->currentAmp = newAmp;

	if ( chanOutputPtr->muted )
		chanOutputPtr->savedAmp = newAmp;
	
	ADD_TRACE_L3( SATraceBufPtr, kEndSetChannelAmp, channelNumber, 
							chanOutputPtr->currentAmp, 0 );

	return kDSNoErr;
	}

/*******************************************************************************************
 * Routine to set a channel's pan.
 *******************************************************************************************/
long		SetSAudioChannelPan( SAudioContextPtr ctx, long channelNumber, long newPan )
	{
	SAudioChannelPtr	chanPtr;
	SAudioOutputPtr		chanOutputPtr;
	long				leftAmp;
	long				rightAmp;
	
	chanPtr  		= ctx->channel + channelNumber;
	chanOutputPtr	= &chanPtr->channelOutput;

	ADD_TRACE_L3( SATraceBufPtr, kBeginSetChannelPan, channelNumber, 
							chanOutputPtr->currentPan, 0 );

	if ( (newPan < 0x0) || (newPan > 0x7FFF) )
		return SAudioErrPanOutOfRange;

	/* If instrument == 0 then the channel has not been initalized so 
	 * just store the value and don't try to change a knob that doesn't
	 * exist yet.  If the channel is muted don't tweak knobs.
	 */
	if ( (chanOutputPtr->instrument > 0) && (!chanOutputPtr->muted) )
		{
		/* Ignore pan commands if this is a stereo stream */
		if ( chanOutputPtr->numChannels == 1 )
			{
			if (newPan < 0x4000)
				{
				leftAmp = chanOutputPtr->currentAmp;
				rightAmp = (newPan * chanOutputPtr->currentAmp) >> 14;
				}
			else
				{
				rightAmp = chanOutputPtr->currentAmp;
				leftAmp = ((0x8000 - newPan) * chanOutputPtr->currentAmp) >> 14;
				}

			/* Set the left ramp target */
			TweakRawKnob(chanOutputPtr->leftEnvTargetKnob, leftAmp);

			/* Set the right ramp target */
			TweakRawKnob(chanOutputPtr->rightEnvTargetKnob, rightAmp);
			}
		else
			{		
			/* if this is a stereo stream return an error and don't change any knobs */
			return SAudioErrNumAudioChannelsOutOfRange;
			}
		}
		
	chanOutputPtr->currentPan = newPan;

	ADD_TRACE_L3( SATraceBufPtr, kEndSetChannelPan, channelNumber, 
							chanOutputPtr->currentPan, 0 );

	return kDSNoErr;
	}
	
/*******************************************************************************************
 * Routine to get a channel's amplitude.  If the channel is currently muted, return
 * the amplitude that it will be restored to when unmute occurs.
 *******************************************************************************************/
long		GetSAudioChannelAmplitude( SAudioContextPtr ctx, long channelNumber, long* Amp )
	{
	SAudioChannelPtr	chanPtr;
	SAudioOutputPtr		chanOutputPtr;
	
	chanPtr  		= ctx->channel + channelNumber;
	chanOutputPtr	= &chanPtr->channelOutput;

	if ( chanOutputPtr->muted )
		*Amp = chanOutputPtr->savedAmp;
	else
		*Amp = chanOutputPtr->currentAmp;

	return kDSNoErr;
	}

/*******************************************************************************************
 * Routine to get a channel's pan.
 *******************************************************************************************/
long		GetSAudioChannelPan( SAudioContextPtr ctx, long channelNumber, long* Pan )
	{
	SAudioChannelPtr	chanPtr;
	SAudioOutputPtr		chanOutputPtr;
	
	chanPtr  		= ctx->channel + channelNumber;
	chanOutputPtr	= &chanPtr->channelOutput;

	*Pan = chanOutputPtr->currentPan;

	return kDSNoErr;
	}

/*******************************************************************************************
 * Routine to save current amplitude and then set it to 0.
 *******************************************************************************************/
long		MuteSAudioChannel( SAudioContextPtr ctx, long channelNumber, Boolean external )
	{
	long				status;
	SAudioChannelPtr	chanPtr;
	SAudioOutputPtr		chanOutputPtr;
	
	status			= 0;
	chanPtr  		= ctx->channel + channelNumber;
	chanOutputPtr	= &chanPtr->channelOutput;
				
	/* Remember if we were muted from a control message so we won't UnMute
	 * if the channel is stopped and restarted.
	 */
	if ( external ) 
		chanOutputPtr->externalMute = external;
	
	/* If the channel is not already muted, save current amplitude and turn volume down. */		
	if ( !chanOutputPtr->muted )
		{
		GetSAudioChannelAmplitude( ctx, channelNumber, &chanOutputPtr->savedAmp );		
		status = SetSAudioChannelAmplitude( ctx, channelNumber, 0 );
		chanOutputPtr->muted = true;
		
		ADD_TRACE_L3( SATraceBufPtr, kChannelMuted, channelNumber, 
							chanOutputPtr->savedAmp, 0 );
		}
		
	return status;
	}
	 
/*******************************************************************************************
 * Routine to restore amplitude from state saved by MuteSAudioChannel().
 *******************************************************************************************/
long		UnMuteSAudioChannel( SAudioContextPtr ctx, long channelNumber, Boolean external )
	{
	long				status;
	SAudioChannelPtr	chanPtr;
	SAudioOutputPtr		chanOutputPtr;
	
	status			= 0;
	chanPtr  		= ctx->channel + channelNumber;
	chanOutputPtr	= &chanPtr->channelOutput;
				
	/* Don't do anything if we're not muted. */		
	if ( chanOutputPtr->muted )
		{
		/* If we were Muted by the user with a control message and s/he is now
		 * trying to unmute us, do it.  If we are being unmuted by an internal
		 * call and we were last muted by the user, don't fuck with the outside 
		 * user's desires.
		 */
		if ( (external && chanOutputPtr->externalMute) || 
			 (!external && !chanOutputPtr->externalMute) )
			{
			chanOutputPtr->muted = false;
			status = SetSAudioChannelAmplitude( ctx, channelNumber, chanOutputPtr->savedAmp );		
			ADD_TRACE_L3( SATraceBufPtr, kChannelUnMuted, channelNumber, 
							chanOutputPtr->currentAmp, 0 );
			
			/* Always Clear flag */
			chanOutputPtr->externalMute = false;
			}
		}
		
	return status;
	}

/*******************************************************************************************
 * Routine to figure out if it's time to start playing. 
 *******************************************************************************************/
void		BeginSAudioPlaybackIfAppropriate( SAudioContextPtr ctx, long channelNumber )
	{
	SAudioBufferPtr		bufferPtr;
	SAudioDataChunkPtr	audioData;
	SAudioChannelPtr	chanPtr;
	
	chanPtr  = ctx->channel + channelNumber;

	/* If the AudioFolio has data queued to it, start things up.
	 */
	if 	( (IsChanEnabled(chanPtr)) && (chanPtr->inuseCount != 0) )
		{
		bufferPtr	= chanPtr->inuseQueueHead;
		audioData	= (SAudioDataChunkPtr) 	bufferPtr->pendingMsg->msg.data.buffer;

		/* Start the proper attachment if none are already running */
		if ( chanPtr->attachmentsRunning == false ) 
			{
			/* Update clock with current stream time from this chunk if this data
			 * belongs to the clock channel. 
			 */
			if (channelNumber == ctx->clockChannel)
				DSSetClock(ctx->streamCBPtr, audioData->time);

			/* Because the NOAUTOSTART flag has been set on all the attachments,
			 * StartInstrument() will not cause a buffer to begin playing.  We
			 * do that manually with StartAttachment().  This guarantees that the
			 * buffer we  wan't is the one that starts.
			 */
			StartAttachment(bufferPtr->attachment, NULL);
			chanPtr->attachmentsRunning = true;	

			/* Restore gain on the mixer after StartAttachment() has been called.
			 * This should guarantee no more pops.
			 */
			UnMuteSAudioChannel( ctx, channelNumber, INTERNAL_UNMUTE );		
			}
		}
	}


