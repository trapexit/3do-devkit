/*******************************************************************************************
 *	File:			SAMain.c
 *
 *	Contains:		Streaming audio subscriber.
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/2/93		jb		Version 2.9.3
 *						Move ReplyMsg() call into while loop for processing request
 *						messages.
 *	11/23/93	jb		Version 2.9.2
 *						Added CloseSAudioSubscriber() routine to clean up after one-time
 *						initializations done by InitSAudioSubscriber().
 *	10/26/93	rdg		Version 2.9.1 
 *						Distinguish between user and internal muting and unmuting
 *						Get rid of envelope rate knobs in channel context... tweak
 *						once at init channel time.
 *	10/19/93	rdg		Versoin 2.9
 *						Switch to using UMemory.c routines for memory allocation
 *	10/18/93	rdg		Version 2.8
 *						Split source into separate files.
 *						Keep high level subscriber message parsing and logical channel related
 *						functionality here.  Move AudioFolio stuff to SAudioFolioInterface.c.
 *						Move trace implementation to SAudioTrace.c
 *						Replace int32 with long.
 *						If a channel fails to initialze properly, close it - but continue.
 *						Make DoAbort() call CloseChannel() for all channels instead 
 *						of FlushChannel().
 *						DoSync() now calls FlushChannel() directly.
 *	10/13/93	rdg		Version 2.7d5
 *						Fix bug in InitalizeThread which caused ctx->threadStackBlock to be set
 *						to zero.  This meant DisposeSAudioSubscriber could not free the stack
 *						block.
 *						Add RemoveMsgPort() call to DisposeSAudioSubscriber().
 *						Put in code to use envelopes to ramp amplitudes.
 *						Stereo streams now get 22 extra DSP ticks and mono streams
 *						44 extra ticks.  Mono needs two envelopes for proper panning.
 *						Make CloseChannel() reset all of the channel's variables.
 *						Can't use tags to set envelope target values, have to use knobs.
 *						The target knob is the value you want to get to at the end of 
 *						the time specified by rate.
 *						Remove support for user controllable panning and amplitude rate.
 *						Replace error code SAudioErrBadChannel with SAudioErrChanOutOfRange.
 *						Add check for stream version.  InitChannel() will now fail if the
 *						version field in the saudio header is greater than COMPATIBLE_AUDIO_STREAM_VERSION.
 *						Modify SetChannelAmplitude() to ramp quickly when going to 0 or only 
 *						altering amplitude by a small amount.  Otherwise, ramp slowly.
 *	9/28/93		rdg		Version 2.6.4
 *						Remove all of the ramping code.  Has to be done a different way.  
 *						Also remove the DUMP_SAMPLE code from FlushChannel().
 *						New routines MuteChannel() and UnMuteChannel() to encapsulate all amplitude
 *						change and testing functionality inside of these routines as well as 
 *						SetChannelAmplitude() and SetChannelPan().
 *						If you set amplitude or pan while a channel is muted, it will remember
 *						what you did and restore that when UnMuteChannel() is called.
 *						Define a IsChanInitalized() macro.
 *						Re-wrote GetTemplateTag() - it returns an error now if a proper template
 *						cannot be found.  So if you embed a data type into a stream that is not
 *						supported you will now get an error message and the channel will be closed.
 *	9/27/93		rdg		Version 2.6.3
 *						Fix redundancy bugs in CloseChannel()... was doing all sorts of things that 
 *						that had already been done by StopChannel().  
 *						CloseChannel() now disables the channel so it can't receive any more
 *						data messages until a header has arrived.  This means that you must
 *						explicitly re-enable the channel with a control message if you want
 *						to use it again.
 *						Fix bug in InitChannel()... was setting status EQUAL to CHAN_ENABLED instead
 *						of ORing the bits as I should have been.
 *						Put proper channel numbers into the trace events for GetChan and SetChan.
 *						Also put status bits into signal field of trace for SetChan.
 *	9/27/93		rdg		Version 2.6.2
 *						New compile switch USE_START_INSTRUMENT.  When on causes Start/StopInstrument()
 *						to be used in Start/StopChannel().  When off, Start/StopAttachment()
 *						is used.  Should generally be left OFF.  See definition of switches for details.
 *	9/24/93		rdg		Version 2.6.1
 *						Add code to ramp volumes on start and stop with an envelope in the DSP.
 *						There are now compile switches for RAMP_SET_CHAN_AMP, START_STOP_RAMP_WITH_ARM,
 *						START_STOP_RAMP_WITH_ENV; see description below for details.
 *						InitalizeThread(), InitChannel(), DoStart(), DoStop(), and DoAbort() were 
 *						slighlty affected by the changes.
 *						Added sanity check to MoveWaitingMsgsToBufferQueue() in case data messages
 *						start arriving on a channel when no header has ever arrived.  For now, data
 *						messages will just pile up until the stream screams to a halt.  Serves
 *						em right for jump randomly into an audio stream
 *	9/23/93		rdg		Version 2.5.5
 *						Create new routine: OrphanPlayingBuffers().  This call prevents all 
 *						currently playing buffers from crossing the link to the next queued buffer.
 * 						This should prevent syncronization problems created by delays between 
 * 						calls to StopInstrument() for each active channel.  This routine is
 *						now called by DoStop() and DoAbort() before attempting to halt playback.
 *						Also added a trace code kBufferOrphaned.
 *	9/23/93		rdg		Version 2.5.4
 *						Add code to StopChannel() and CloseChannel() so they now check to see if 
 *						the current buffer completed before we could get to StopInstrument(). If 
 *						it did, call WaitSignal() to clear the signal bit. 
 *						This should finally solve the problem of buffers seeming to complete 
 *						out of order.
 *	9/23/93		rdg		Version 2.5.3
 *						Added kWaitingOnSignal and kGotSignal trace codes.
 *						Move amplitude ramping before StopInstrument().
 *	9/23/93		rdg		Version 2.5.2
 *						Moved ramping code after StopInstrument() call in StopChannel() and CloseChannel() 
 *						to try to eliminate buffer completion errors.  Didn't work.
 *	9/22/93		rdg		Version 2.5.1
 *						Added more trace event codes; also added and address field to ADD_TRACE so you
 *						can track buffer and msg addresses.
 *						Modified FindBuffer() and HandleCompletedBuffers() so that no action is taken
 *						if the buffer completion signal sent by the system does not match the signal
 *						for the buffer least recently queued to the AudioFolio.  A trace event is
 *						recorded for the event so we can see what the system sent and what was really
 *						at the head of the inUse queue.
 *						Added DUMP_TRACE_ON_BUFFER_COMP_ERR. 
 *	9/21/93		rdg		Version 2.5
 *						Rewrite HandleCompletedBuffers() to process multiple completions on different
 *						channels as well as multiple completions on the same channel.  The loop
 *						was moved from the main thread into this routine.
 *						Removed DUMP_TRACE_ON_FIRST_WRONG_BUFFER.
 *						Renamed DUMP_TRACE_BUFFER DUMP_RAW_TRACE_BUFFER and made a macro.
 *						Added DumpBufferCompletionStats() routine which is also compile switched 
 *						with TRACE_ENABLE.
 *	9/20/93		rdg		Version 2.4
 *						Modify HandleCompletedBuffers() and the main thread to process multiple
 *						completion signals in one signal word.  HandleCompletedBuffers() is now 
 *						called in a while loop - each signal is subtracted as the associated 
 *						buffer is processed until all are done.
 *						Added #defines DUMP_TRACE_ON_FIRST_WRONG_BUFFER	and DUMP_TRACE_ON_STREAM_CLOSE
 *						Merged all the code in from the NoPop versions as conditional compiles
 *							9/8/93		rdg		NoPop.2
 *							Even more mods to try to get rid of pops and clicks.  These should be backoutable
 *							when Phil is able to fix the AudioFolio to deal with things internally.
 *							Modify InitChannel() BeginPlaybackIfAppropriate() and HandleCompletedBuffers() to
 *							deal with amplitude differently.  Now InitChannel() saves the proper amplitude in *						the outputPtr->savedAmp.  This is used in  BeginPlaybackIfAppropriate() to set the
 *							mixer amplitude AFTER StartAttachment() has been called.  HandleCompletedBuffers()
 *							now saves the current amplitude and set the mixer to zero before calling StopInstrument().
 *							8/26/93		rdg		NoPop Version
 *							Modified SetChannelAmplitude() and SetChannelPan() to ramp volume.  This seems
 *							be benign so far.  It did not eliminate pops however.  This seems to be due
 *							to calling StartInstrument() with the SET_NO_AUTO_START attachment.
 *							Modified CloseChannel() to Ramp down volume on mixer and do free instruments 
 *							in a different order to prevent popping.
 *	9/17/93		rdg		Version 2.3.1
 *						Added some new trace messages for debugging multiple concurrent audio
 *						stream playback.
 *	9/14/93		rdg		Version 2.3
 *						Initalize currentAmp to -1 so we can distinguish between 0 and "not yet set".
 *						Now if a control message comes in setting the amplitude to 0 before a header 
 *						arrives, InitChannel() will ignore the initialAmplitude value in the SSND header.
 *	8/11/93		rdg		Version 2.2
 *						Get rid of the notion of attachRunning in program logic.  Now instead
 *						of calling StopAttachment() we just use StopInstrument().  This is possible
 *						again because of the NOAUTOSTART attachment flag.  This flag is now set
 *						when attachments are created so that StartInstrument() will NOT start a 
 *						buffer playing.  We then use StartAttachment() to start the first buffer
 *						in the inUseQueue.  When the last buffer completes, StopInstrument() is
 *						called and things shut down.  
 *						These changes were made because in 1p01, calling StopAttachment() will not
 *						necessarilly mean that the instrument plays silence.  In fact, it will sometimes
 *						play random pitches.  Therefore, calling StopAttachment() without StopInstrument()
 *						is not, and never was, kosher.  No one knows why this has been working OK for
 *						several releases.
 *	8/10/93		rdg		Version 2.1
 *						Modified StopChannel() and FlushChannel() to return status.
 *						Add kSAudioCtlOpFlushChannel which stops and flushes the currently queued data.
 *	8/7/93		rdg		Version 2.0
 *						Add kSAudioCtlOpCloseChannel so that you can unload a channel's instrument from
 *						the DSP and free any other allocated resources when a logical channel's data is 
 *						done.  The two new routines CloseChannel() and FreeAudioBuffer() 
 *						stop and flush the specified channel, make it inactive, free it's DSP instruments, 
 *						and free it's pool of AudioFolio buffers.
 *						DoClosing() now calls CloseChannel() on all channels.
 *	7/15/93		rdg		Version 1.9
 *						Jam 0xdeadbeef into the first longword of the sample data when a 
 *						buffer completes so you can tell from the outside worlds when a
 *						buffer has been replied to by the subscriber.
 *	6/29/93		rdg		Version 1.8
 *						Always make a channel active when we receive its header.
 *	6/29/93		rdg		Version 1.7
 *	6/28/93		rdg		Fix bug in InitChannel(): SetChannelPan() was being called on
 *						initalization of a stereo stream. This causes an error in SetChannelPan().
 *	6/28/93		jb		Get rid of unreferenced vars caused by conditional compilation of
 *						diagnostic code.
 *						Add additional trace instrumentation.
 *						Add CHECKRESULT() for all ReplyMsg() calls.
 *	6/25/93		rdg		Version 1.6
 *						Remove routines that deal with frequency.  Not useful in this context because
 *						doing sample rate interpolation to change frequency will screw up 
 *						sync with the stream clock.
 *	6/25/93		rdg		Version 1.5
 *						!!!!!!! Replying to a message with non-zero status causes a deadlock!
 *						If an error occurs in InitChannel(), DoData() for now just disables the 
 *						channel and returns 0 anyway.
 *						Clean up error codes in InitChannel() to return meaningful codes.
 *						Fix bug with channelOutput(s) in all routines that references them.  
 *						Fix bug in InitChannel(): i was not setting chanOutputPtr->stereo based 
 *						on stream header numberChannels.
 *						Change chanOutputPtr->stereo flag to chanOutputPtr->numChannels.
 *						Modify DoControl() with opcodes for getting pan, volume, freq, and clockchan;
 *						make new routines to do these functions.
 *						Reworked the way new buffers are handled.  SubmitBufferToAudioFolio() changed 
 *						to QueueNewAudioBuffer() and BeginPlaybackIfAppropriate().  QueueNewAudioBuffer() 
 *						now makes all the descions about queueing and will move messages from the wait
 *						queue to the inuse queue if necessary (HandleCompletedBuffers() used to do this). 
 *						BeginPlaybackIfAppropriate() can be called to find out if conditions are right 
 *						for calling StartInstrument() or StartAttachment().
 *						Add SA_22K_16B_M_SDX2. Modify GetTemplateTag() to deal with new data type.
 *						Add new flag to channel struct called attachRunning.  This is initalized to false
 *						and can be set only by BeginPlaybackIfAppropriate(), HandleCompletedBuffers() and
 *						StopChannel().
 *						HandleCompletedBuffers() sets it to false when it leaves the inuseQueue and 
 *						the dataQueue empty.  BeginPlaybackIfAppropriate() sets it to true when it first
 *						starts the instrument, and when it restarts an attachment after things have been
 *						stopped or idle.  StopChannel() sets it to false when it calls StopAttachment().
 *						Create MoveWaitingMsgsToBufferQueue() which factors out the moving of buffers
 *						from the waiting msg queue to the buffer queue.  This routine is now called when
 *						new buffers arrive, when buffers complete from the AudioFolio and when StartChannel()
 *						is called.  It does nothing if the channel is non active.
 *	6/22/93		rdg		Version 1.4
 *						Modify for amplitude and pan control stubs..
 *						Change SA_DIRECT_OUT_INSTRUMENT_NAME to SA_OUTPUT_INSTRUMENT_NAME
 *						Modify InitalizeThread() to use initalize new channelOutput structure fields.
 *						Modify InitChannel() to use new channelOutput structure fields.
 *						Modify SubmitBufferToAudioFolio() to no longer use initialAmplitude tags for
 *						StartInstrument().
 *	6/21/93		rdg		Version 1.3
 *						Use IsChanEnabled macro instead of (chanPtr->status & CHAN_ENABLED).
 *						Use IsChanActive macro instead of (chanPtr->status & CHAN_ACTIVE).
 *						Initalize all of the fields in all of the channels at InitalizeThread() time.
 *						Make sure the CHAN_ACTIVE bit is set when attachments are non-stopped.
 *	6/21/93		rdg		Version 1.2
 *						Rename OpenChannel() to InitChannel().  InitChannel() now does
 *						NOT enable the channel, it only initalizes the channel.
 *						Modify InitalizeChannel() to default channel 0 to enabled and all 
 *						others to disabled.
 *						Fix bug in StopChannel(): it wasn't checking to make sure data
 *						was actually playing before trying to call StopAttachment().
 *						Modify DoSetChan() to stop and flush a channel if it was enabled
 *						and is being disabled.
 *						Changed all occurences of channelPtr to chanPtr for consistancy.
 *	6/19/93		rdg		Version 1.1
 *	6/19/93		rdg		Add code to deal properly with stopping and re-starting the stream
 *						as well as flushing due to a sync message.
 *						StopChannel() now has guts that do something.
 *						StartChannel() added from scratch.
 *	6/19/93		rdg		Version 1.0
 *	6/19/93		rdg		Modify InitalizeThread() to set default channel 0 to be the clock channel.
 *						Modify SubmitBufferToAudioFolio() and HandleCompletedBuffers()
 *						to use ctx.clockChannel to determine which time stamp data to use.
 *						Modify DoControl to handle kSAudioCtlOpSetClockChan.
 *	6/18/93		rdg		Start on code to deal with syncing the streams...
 *						Modify SubmitBufferToAudioFolio() to call DSSetClock() when
 *						the first ever data chunk arrives and StartInstrument() is called.
 *						Modify HandleCompletedBuffers() to update the stream time to the 
 *						time stamp in the chunk which just _started_ playing; i.e. the buffer
 *						which followed the one which just completed in the inuseQueue.
 *	6/2/93		jb		Version 0.18
 *						Switch to using Debug3DO.h for CHECKRESULT() macro.
 *						Initialize 'dataQueueHead' field to NULL for each channel at
 *						thread instantiation time.
 *						Remove all variable initializers in local variable declarations
 *						to avoid register bashing by the ARM C compiler.
 *						Did some source code re-formatting (sorry, couldn't resist).
 *	5/30/93		jb		Version 0.17
 *	5/30/93		jb		Initialize 'bufferPool' pointer to NULL in InitializeThread().
 *						Add cleanup code to thread exit to release any memory that we
 *						allocated in InitializeThread(). Free the 'templateArray' copy.
 *						Set 'threadItem' to zero upon exiting so that dispose will not
 *						have to try to delete the thread if close is called first.
 *	5/27/93		rdg		Version 0.16
 *						Get rid of the StopInstrument call in DoStop().  
 *	5/27/93		rdg		Version 0.15
 *						Modified GetTemplateTag() and the gInitialTemplates table to deal with 
 *						new instrument/data types:
 *						22K 8bit Stereo, 44K 8bit Stereo, 22K 16bit Stereo Compressed.
 *	5/27/93		rdg		Version 0.14
 *						Alter SubmitBufferToAudioFolio so that it will resume a stream by
 *						calling StartAttachment() instead of the previous method which involved
 *						stopping and starting the whole instrument.  Now, when we stop getting
 *						data chunks, the instrument will just play silence until we tell it to
 *						resume with StartAttachment().
 *	5/27/93		rdg		Take the StopInstrument() call from HandleCompleteBuffers.  Now, when
 *						the data runs out, just play silence.
 *	5/27/93		rdg		Take out the check for multiple queued buffers before calling StartInstrument.
 *	5/26/93		rdg		Version 0.13
 *						In SubmitBufferToAudioFolio, rip out the initialization of variables
 *						in the declarations.
 *	5/26/93		rdg		Version 0.12
 *						Fix bug in DoData() which header chunk data messages were not being
 *						replied to.
 *						Allow for preloading of instrument templates via control message.
 *						Preload output instrument template at thread instantiation time 
 *						and create output instrument when channel is opened. Make mechanism
 *						table/tag driven to allow for easy addition of new instrument types.
 *  5/25/93		rdg		set channelPtr->instStarted to 'false' when StopInstrument is called
 *  5/25/93		rdg		Version .11
 *  5/25/93		rdg		Make SubmitBufferToAudioFolio use actualSampleSize
 *	5/25/93		rdg		Make HandleCompletedBuffers call StopInstrument when we are out of data.	
 *	5/25/93		rdg		Make FindBuffer return true if there are more buffers in the in use queue.	
 *  5/17/93		rdg		Version .10
 *						Merged Neil's code: 
 *						   File now uses common subscriber code and structures. 
 *						   Uses full chunk and subchunk format to be consistant with other subscribers.
 *						   Pass context to all routines which used to get it from the SubscriberMsg.
 *						   Renamed SGlobals to SAudioGlobals and SAudioContextPool to contextPool
 *						Modified InitAudioBuffer and altered the bufferInitBlock struct to contain a pointer 
 *						to a sampleHeaderChunk struct instead of a ctlBlockPtr.  CtlBlocks are no longer
 *						used to open a channel, now the initialization data is in the stream itself.
 *  5/17/93		rdg		I removed the reference to SAUDIO_INITIAL_AMPLITUDE and added a field to
 *						the SAudioChannel and SAudioCtlBlock structures called initialAmplitude. Now 
 *						this value can be passed via a header chunk in the stream or from the main 
 *						app.  It is set in the Channel at OpenChannel() time and used with StartInstrument
 *						in SubmitBufferToAudioFolio.
 *  5/17/93		rdg		In InitAudioBuffer() and OpenChannel() I removed the WIDTH tag and left
 *						the numbits tag.  I changed the name of the field in the SAudioSampleDescriptor
 *						from width to numbits to accomodate the change. 
 *  5/17/93		rdg		In SubmitBufferToAudioFolio I now pass AF_TAG_AMPLITUDE to StartInstrument 
 *						with SAUDIO_INITIAL_AMPLITUDE (defined in SAudioSubscriber.h) as an arg 
 *						so you can set the initial volume of the sample.
 *  5/17/93		rdg		In InitAudioBuffer() and OpenChannel() I added the AF_TAG_NUMBITS tag
 *						because SelectSamplePlayer does not seem to look at the width field of 
 *						the sample descripter when it chooses an instrument name.
 *  5/14/93		rdg		Changed the name of HandleCompletedChunks, to HandleCompletedBuffers.
 *						Changed the name of InitChannel to OpenChannel.
 *	5/13/93		rdg		Fixed bug in SubmitBufferToAudioFolio: LinkAttachment was linking to
 *						the HEAD of the inuseQueue instead of the TAIL.  IT WORKS!!!!.
 *	5/11/93		rdg		Completed first pass at full implementation.
 *	5/8/93		rdg		begin conversion to audio subscriber
 *	4/21/93		njc		Clear the fTimerRunning flag when timer has expired in DoTimerExpired.
 *	4/21/93		njc		FlushChannel was caught in an infinite loop calling
 *						ReplyMsg to all pending msgs.
 *	4/20/93		jb		Be sure to pass back a pointer to the message we reply
 *						to in all calls to ReplyMsg().
 *	4/19/93		jb		Move all initialization code to the thread. Invent funky signal
 *						based mechanism to coordinate the procedure call to create a
 *						new subscriber with the thread, which must do all the intialization
 *						so that resource ownership works correctly.
 *	4/17/93		jb		Switch to New() and Dispose() for subscriber creation and
 *						destructors. Solves the "what to do with stack space" problem.
 *	4/16/93		jb		Change to WaitSignal() from Wait() in 3B1
 *	4/12/93		jb		New today.
 *
 *******************************************************************************************/

/*
 * Trace switches, to help with real time debugging.
 *
 * TRACE_MAIN causes the subscriber to leave timestamped trace data in a 
 * circular buffer which can be examined using the debugger, or dumped with
 * DUMP_TRACE_ON_STREAM_CLOSE - which dumps the buffer whenever a StreamClosing
 * message is received.
 *
 * Don't forget link with the trace code if you want to use it..
 *
 */
 
#define TRACE_MAIN 	1

/* DON'T TURN ON UNLESS TRACE_MAIN IS ON, OR THE COMPILER WILL BARF!!!!! */
#define DUMP_TRACE_ON_STREAM_CLOSE				0

#ifndef ABS	
	#define	ABS(num) (num > 0) ? (num) : (-num)
#endif

#include "DataStreamLib.h"
#include "MsgUtils.h"
#include "MemPool.h"
#include "ThreadHelper.h"
#include "Graphics.h"
#include "Audio.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"				
#include "stdio.h"

#include "UMemory.h"

#include "Debug3DO.h"

#include "SAErrors.h"
#include "SATemplates.h"
#include "SAControlMsgs.h"
#include "SAFolioInterface.h"
#include "SAudioSubscriber.h"

#if TRACE_MAIN
#include "SATrace.h"
extern long 		traceIndex;
extern long 		savedTraceIndex;
extern TraceEntry	trace[ MAX_TRACE ];
#else
#define ADD_TRACE( event, chan, value, ptr )
#endif

/****************************************************************************/
/* Pool from which to allocate subscriber context for multiple subscribers  */
/****************************************************************************/
struct SAudioSubscriberGlobals	{
	MemPoolPtr		contextPool;		
	} SAudioGlobals;
	
/* External definition for template cache */	
extern TemplateRec gInitialTemplates;

/* External definition for template count */
extern long kMaxTemplateCount;

/********************************************************
 * Main Subscriber thread and its initalization routine 
 ********************************************************/
static long		InitializeThread( SAudioContextPtr ctx );
static void		SAudioSubscriberThread( long notUsed, SAudioContextPtr ctx );

/***********************************************************************/
/* Routines to handle incoming messages from the stream parser thread  */
/***********************************************************************/
static long		DoData( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoGetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoSetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoControl( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoOpening( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoClosing( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoStop( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoSync( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoEOF( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
static long		DoAbort( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );

/*==========================================================================================
  ==========================================================================================
							Subscriber procedural interfaces
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Routine to initialize the subscriber. Creates the a memory pool for allocating
 * subscriber contexts.
 *******************************************************************************************/
long	InitSAudioSubscriber( void )
	{
	/* Create the memory pool for allocating data stream
	 * contexts.
	 */
	SAudioGlobals.contextPool = CreateMemPool( SA_SUBS_MAX_SUBSCRIPTIONS, sizeof(SAudioContext) );
	if ( SAudioGlobals.contextPool == NULL )
		return SAudioErrCouldntCreateContextPool;

	/* Return success */
	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to deallocate things allocated by one-time initilaization in the
 * InitSAudioSubscriber() routine.
 *******************************************************************************************/
long	CloseSAudioSubscriber( void )
	{
	DeleteMemPool (SAudioGlobals.contextPool);

	SAudioGlobals.contextPool = NULL;
	
	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to instantiate a new subscriber. Creates the message port through which all
 * subsequent communications take place, as well as any other necessary per-context
 * resources. Creates the service thread and passes the new context to it.
 *******************************************************************************************/
long	NewSAudioSubscriber( SAudioContextPtr *pCtx, DSStreamCBPtr streamCBPtr, long deltaPriority )
	{
	long				status;
	SAudioContextPtr	ctx;
	ulong				signalBits;

	/* Allocate a subscriber context */

	ctx = (SAudioContextPtr) AllocPoolMem( SAudioGlobals.contextPool );
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
				(void *) &SAudioSubscriberThread, 				/* thread entrypoint */
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

	ReturnPoolMem( SAudioGlobals.contextPool, ctx );

	return status;
	}


/*******************************************************************************************
 * Routine to get rid of all subscriber resources we created. This assumes that the
 * subscriber is in a clean state where it has returned all resources that were
 * passed to it, like messages. Deleting the subscriber thread should free up all
 * the rest of the stuff the thread allocated. What is left is the thread's stack
 * space, which we allocated, and the context block.
 *******************************************************************************************/
long	DisposeSAudioSubscriber( SAudioContextPtr ctx )
	{
	if ( ctx != NULL )
		{
		if ( ctx->threadItem > 0 )		DisposeThread( ctx->threadItem );
		if ( ctx->threadStackBlock )	FreePtr( ctx->threadStackBlock );

		RemoveMsgPort( ctx->requestPortSignal );
		
		ReturnPoolMem( SAudioGlobals.contextPool, ctx );
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
 * Routine to process arriving data chunks. If message points to sample data, then
 * queue it to the audio folio.  If message points to header, initialize the channel
 * that the header was received on.
 *
 * NOTE:	Fields 'streamChunkPtr->streamChunkType' and 'streamChunkPtr->streamChunkSize'
 *			contain the 4 character stream data type and size, in BYTES, of the actual
 *			chunk data.
 *******************************************************************************************/
static long	DoData( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
{
	long					status;
	StreamChunkPtr			streamChunkPtr;
	SAudioDataChunkPtr		audioData;
	SAudioHeaderChunkPtr 	audioHeader;	/* incoming message could be data or header chunk */
	long					channelNumber;

	streamChunkPtr	= (StreamChunkPtr) subMsg->msg.data.buffer;
	audioData		= (SAudioDataChunkPtr) streamChunkPtr;
	audioHeader		= (SAudioHeaderChunkPtr) streamChunkPtr;
	channelNumber	= audioData->channel;

	switch (audioData->subChunkType)
		{
		case SSMP_CHUNK_TYPE:	/* Sample data */

			ADD_TRACE( kNewDataMsgArrived, channelNumber, 0, (long)subMsg );
			QueueNewAudioBuffer( ctx, subMsg );		
			BeginPlaybackIfAppropriate( ctx, channelNumber );
			status = kDSNoErr;
			break;
		
		case SHDR_CHUNK_TYPE:	/* Sample header structure */

			ADD_TRACE( kNewHeaderMsgArrived, channelNumber, 0, (long)subMsg );
			status = InitChannel(ctx, audioHeader);
			if (status < 0) 
				{
				printf( "Initializaion for SAudio channel %ld failed; error = %ld\n",
																		channelNumber, status );
				printf( "Closing channel %ld, other channel(s) will still function\n", channelNumber);
			
				CloseChannel( ctx, channelNumber );
				
				/* Nothing all that terrible happened so Reset the status so we won't 
				 * get aborted by the streamer when it looks at the replyd message.
				 */
				status = 0;
				}

			/* Because the subscriber considers audio headers and audio data to both be "Data" 
			 * messages we have to reply to the header chunk message here if init was successful.
			 */
			status = ReplyMsg( subMsg->msgItem, status, subMsg, sizeof(SubscriberMsg) ); 
			CHECKRESULT( "QueueNewAudioBuffer() - ReplyMsg() #1", status );
			break;
		

		default:
			/* For unrecognized or hosed chunk types */
			status = ReplyMsg( subMsg->msgItem, kDSNoErr, subMsg, sizeof(SubscriberMsg) ); 
			CHECKRESULT( "QueueNewAudioBuffer() - ReplyMsg() #2", status );
			break;
		
		}	// switch
		
	return status;
	}

		
/*******************************************************************************************
 * Routine to set the status bits of a given channel.
 *******************************************************************************************/		
static long	DoSetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long				chanNumber;
	SAudioChannelPtr	chanPtr;
	long				wasEnabled;
	
	chanNumber	= subMsg->msg.channel.number;
	chanPtr		= ctx->channel + chanNumber;
	
	if ( chanNumber < SA_SUBS_MAX_CHANNELS )
		{
		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to 
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary.
		 */
		wasEnabled = IsChanEnabled(chanPtr);

		chanPtr->status |= (~CHAN_SYSBITS & subMsg->msg.channel.status);

		/* If the channel is becoming disabled, flush data; if it is
		 * becoming enabled, start it up.
		 */
		if (wasEnabled && (!IsChanEnabled(chanPtr)))
			FlushChannel(ctx, chanNumber);
		else if (!wasEnabled && (IsChanEnabled(chanPtr)))
			StartChannel(ctx, chanNumber);
		}

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to return the status bits of a given channel.
 *******************************************************************************************/
static long	DoGetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long		status;
	long		channelNumber;

	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < SA_SUBS_MAX_CHANNELS )
		status = ctx->channel[ channelNumber ].status;
	else
		status = 0;

	return status;
	}

		
/*******************************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 *******************************************************************************************/		
static long	DoControl( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long				status;
	long				userWhatToDo;
	SAudioCtlBlockPtr	ctlBlockPtr;

	userWhatToDo = subMsg->msg.control.controlArg1;
	ctlBlockPtr = (SAudioCtlBlockPtr) subMsg->msg.control.controlArg2;

	switch ( userWhatToDo )
		{
		case kSAudioCtlOpLoadTemplates:
			status = LoadTemplates( ctx->templateArray, ctlBlockPtr->loadTemplates.tagListPtr, kMaxTemplateCount );
			break;

		case kSAudioCtlOpSetVol:
			if ( ctlBlockPtr->amplitude.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = SetChannelAmplitude( ctx, ctlBlockPtr->amplitude.channelNumber, 
								ctlBlockPtr->amplitude.value );
				}
			else
				status = SAudioErrChanOutOfRange;
			break;
			
		case kSAudioCtlOpSetPan:
			if ( ctlBlockPtr->pan.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = SetChannelPan( ctx, ctlBlockPtr->pan.channelNumber, 
								ctlBlockPtr->pan.value );
				}
			else
				status = SAudioErrChanOutOfRange;
			break;

		case kSAudioCtlOpGetVol:
			if ( ctlBlockPtr->amplitude.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = GetChannelAmplitude( ctx, ctlBlockPtr->amplitude.channelNumber, 
								&ctlBlockPtr->amplitude.value);
				}
			else
				status = SAudioErrChanOutOfRange;
			break;
			
		case kSAudioCtlOpGetPan:
			if ( ctlBlockPtr->pan.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = GetChannelPan( ctx, ctlBlockPtr->pan.channelNumber, 
								&ctlBlockPtr->pan.value);
				}
			else
				status = SAudioErrChanOutOfRange;
			break;

		case kSAudioCtlOpMute:
			if ( ctlBlockPtr->mute.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = MuteChannel( ctx, ctlBlockPtr->mute.channelNumber, USER_MUTE );
				}
			else
				status = SAudioErrChanOutOfRange;
			break;

		case kSAudioCtlOpUnMute:
			if ( ctlBlockPtr->unMute.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = UnMuteChannel( ctx, ctlBlockPtr->unMute.channelNumber, USER_UNMUTE );
				}
			else
				status = SAudioErrChanOutOfRange;
			break;

		case kSAudioCtlOpSetClockChan:
			if ( ctlBlockPtr->clock.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				ctx->clockChannel = ctlBlockPtr->clock.channelNumber;
				status = 0;			/* these are no-op's for now... */
				}
			else
				status = SAudioErrChanOutOfRange;
				break;

		case kSAudioCtlOpGetClockChan:
			if ( ctlBlockPtr->clock.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				ctlBlockPtr->clock.channelNumber = ctx->clockChannel;
				status = kDSNoErr;			
				}
			else
				status = SAudioErrChanOutOfRange;
				break;

		case kSAudioCtlOpCloseChannel:
			status = CloseChannel( ctx, ctlBlockPtr->closeChannel.channelNumber );
			break;

		case kSAudioCtlOpFlushChannel:
			status = FlushChannel( ctx, ctlBlockPtr->flushChannel.channelNumber );
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
static long	DoOpening( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to close down an open subscription.
 *******************************************************************************************/		
static long	DoClosing( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++ )
		CloseChannel( ctx, channelNumber );

#if DUMP_TRACE_ON_STREAM_CLOSE
	DUMP_RAW_TRACE_BUFFER();
	DumpBufferCompletionStats();
#endif

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to start all channels for this subscription.
 *******************************************************************************************/		
static long	DoStart( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Start all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		if ( subMsg->msg.start.options & SOPT_FLUSH )
			FlushChannel( ctx, channelNumber );

		StartChannel( ctx, channelNumber );
		}
		
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/		
static long	DoStop( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Prevent all playing buffers from crossing the link to the next queued buffer.
	 * This should prevent syncronization problems created by delays between 
	 * StopInstrument() calls for each active channel.
	 */
	OrphanPlayingBuffers(ctx);	

	/* Stop all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		if ( subMsg->msg.stop.options & SOPT_FLUSH )
			FlushChannel( ctx, channelNumber );
		else
			StopChannel( ctx, channelNumber );
		}

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine flush everything waiting and queued to the AudioFolio under the
 * assumption that we have just arrived at a branch point and should be ready
 * to deal with all new data for this stream.
 *******************************************************************************************/		
static long	DoSync( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Prevent all playing buffers from crossing the link to the next queued buffer.
	 */
	OrphanPlayingBuffers(ctx);	

	/* Halt and flush all channels for this subscription, then restart.
	 */
	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++ )
		FlushChannel( ctx, channelNumber );
		
	DoStart( ctx, subMsg );
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to 
 *******************************************************************************************/		
static long	DoEOF( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 *******************************************************************************************/		
static long	DoAbort( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	long	channelNumber;

	/* Halt, flush, and close all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++ )
		CloseChannel( ctx, channelNumber );

	return kDSNoErr;
	}

/*==========================================================================================
  ==========================================================================================
									The subscriber thread
  ==========================================================================================
  ==========================================================================================*/

static long	InitializeThread( SAudioContextPtr ctx )
	{
	long		status;
	long		k;

	/* Initialize fields in the context record */

	ctx->creatorStatus		= 0;			/* assume initialization will succeed */
	ctx->requestPort		= 0;
	ctx->allBufferSignals	= 0;

	/* Open the Audio Folio for this thread */
	
	if ( (status = OpenAudioFolio() ) < 0 )
		{
		ctx->creatorStatus = status;
		goto BAILOUT;
		}
	
	/* Create a place to copy the initial template records for this
	 * instantiation of the audio subscriber.
	 */
	ctx->templateArray = (TemplateRecPtr) NewPtr( (sizeof( TemplateRec ) 
											* kMaxTemplateCount), MEMTYPE_ANY );
	if ( ctx->templateArray == 0 )
		{
		ctx->creatorStatus = kDSNoMemErr;
		goto BAILOUT;
		}

	/* Copy the (unloaded) template stuff to our own space. We modify
	 * this copy of the template records as we are told to load templates
	 * or as header data in a stream tells us that we need to load a new
	 * instrument.
	 */
	memcpy( ctx->templateArray, &gInitialTemplates, (sizeof( TemplateRec ) * kMaxTemplateCount) );

	/* Load the output instrument template. This gets used every time
	 * a channel is opened to create an output instrument for the channel
	 * if one does not already exist.
	 */
	ctx->outputTemplateItem = LoadInsTemplate( SA_OUTPUT_INSTRUMENT_NAME, 0 );
	if ( ctx->outputTemplateItem < 0 )
		{
		ctx->creatorStatus = (long) ctx->outputTemplateItem;
		goto BAILOUT;
		}

	/* Load the envelope instrument template. This gets used every time
	 * a channel is opened to create an envelope instrument for the channel
	 * so we can ramp amplitudes on start and stop.
	 */
	ctx->envelopeTemplateItem = LoadInsTemplate( SA_ENVELOPE_INSTRUMENT_NAME, 0 );
	if ( ctx->envelopeTemplateItem < 0 )
		{
		ctx->creatorStatus = (long) ctx->envelopeTemplateItem;
		goto BAILOUT;
		}

	/* Default clock channel to 0 */
	ctx->clockChannel	= 0;						

	/* Initialize once-only channel related things. 
	 *
	 * NOTE: CurrentAmp is initalized to -1 so you can set the amplitude
	 * to zero with a control message and not have it be
	 * overided by the initialAmplitude setting in the .saudio 
	 * header.  i.e. to distinguish between amp 0 and "not yet set".
	 */
	for ( k = 0; k < SA_SUBS_MAX_CHANNELS; k++ )
		{
		ctx->channel[k].status								= 0;
		ctx->channel[k].numBuffers							= 0;
		ctx->channel[k].bufferPool							= 0;
		ctx->channel[k].channelInstrument					= 0;
		ctx->channel[k].instStarted							= false;
		ctx->channel[k].attachmentsRunning					= false;
		ctx->channel[k].channelOutput.instrument			= 0;
		ctx->channel[k].channelOutput.numChannels			= 0;
		ctx->channel[k].channelOutput.leftEnv				= 0;
		ctx->channel[k].channelOutput.leftEnvTargetKnob		= 0;
		ctx->channel[k].channelOutput.rightEnv				= 0;
		ctx->channel[k].channelOutput.rightEnvTargetKnob	= 0;
		ctx->channel[k].channelOutput.currentAmp			= -1;
		ctx->channel[k].channelOutput.savedAmp				= 0;
		ctx->channel[k].channelOutput.currentPan			= 0;
		ctx->channel[k].channelOutput.muted					= false;
		ctx->channel[k].channelOutput.externalMute			= false;
		ctx->channel[k].signalMask							= 0;
		ctx->channel[k].dataQueue.head						= NULL;
		ctx->channel[k].dataQueue.tail						= NULL;
		ctx->channel[k].inuseCount							= 0;
		ctx->channel[k].inuseQueueHead						= NULL;
		ctx->channel[k].inuseQueueTail						= NULL;
		}

	/* Create the message port where the new subscriber will accept
	 * request messages.
	 */
	status = NewMsgPort( &ctx->requestPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->requestPort = status;

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
 * This thread is started by a call to InitSAudioSubscriber(). It reads the subscriber message
 * port for work requests and performs appropriate actions. The subscriber message
 * definitions are located in "DataStream.h".
 *******************************************************************************************/
static void		SAudioSubscriberThread( long notUsed, SAudioContextPtr ctx )
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
		anySignal = ctx->requestPortSignal | ctx->allBufferSignals;

		ADD_TRACE( kWaitingOnSignal, -1, (ctx->requestPortSignal | ctx->allBufferSignals), 0 );

		signalBits = WaitSignal( anySignal );

		ADD_TRACE( kGotSignal, -1, signalBits, 0 );

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
						ADD_TRACE( kStreamOpData, -1, 0, (long)subMsg );
						status = DoData( ctx, subMsg );		
						break;
		
		
					case kStreamOpGetChan:			/* get logical channel status */
						ADD_TRACE( kStreamOpGetChan, subMsg->msg.channel.number, 0, (long)subMsg );
						status = DoGetChan( ctx, subMsg );		
						break;
		
		
					case kStreamOpSetChan:			/* set logical channel status */
						ADD_TRACE( kStreamOpSetChan, subMsg->msg.channel.number, subMsg->msg.channel.status, (long)subMsg );
						status = DoSetChan( ctx, subMsg );		
						break;
		
		
					case kStreamOpControl:			/* perform subscriber defined function */
						ADD_TRACE( kStreamOpControl, -1, 0, (long)subMsg );
						status = DoControl( ctx, subMsg );		
						break;
		

					case kStreamOpSync:				/* clock stream resynched the clock */
						ADD_TRACE( kStreamOpSync, -1, 0, (long)subMsg );
						status = DoSync( ctx, subMsg );		
						break;
		
		
				/************************************************************************
				 * NOTE:	The following msgs have no extended message arguments
				 * 			and only may use the whatToDo and context
				 * 			fields in the message.
				 ************************************************************************/


					case kStreamOpOpening:			/* one time initialization call from DSH */
						ADD_TRACE( kStreamOpOpening, -1, 0, (long)subMsg );
						status = DoOpening( ctx, subMsg );		
						break;
		
		
					case kStreamOpClosing:			/* stream is being closed */
						ADD_TRACE( kStreamOpClosing, -1, 0, (long)subMsg );
						status = DoClosing( ctx, subMsg );		
						fKeepRunning = false;
						break;
		
		
					case kStreamOpStop:				/* stream is being stopped */
						ADD_TRACE( kStreamOpStop, -1, 0, (long)subMsg );
						status = DoStop( ctx, subMsg );
						break;
		
		
					case kStreamOpStart:			/* stream is being started */
						ADD_TRACE( kStreamOpStart, -1, 0, (long)subMsg );
						status = DoStart( ctx, subMsg );
						break;
		
		
					case kStreamOpEOF:				/* physical EOF on data, no more to come */
						ADD_TRACE( kStreamOpEOF, -1, 0, (long)subMsg );
						status = DoEOF( ctx, subMsg );		
						break;
		
		
					case kStreamOpAbort:			/* somebody gave up, stream is aborted */
						ADD_TRACE( kStreamOpAbort, -1, 0, (long)subMsg );
						status = DoAbort( ctx, subMsg );		
						fKeepRunning = false;

						DUMP_RAW_TRACE_BUFFER();

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

		/*******************************************************/
		/* Check for and process any sample buffer completions */
		/*******************************************************/
		if ( signalBits & ctx->allBufferSignals )
			HandleCompletedBuffers( ctx, signalBits );
	
	} /* while KeepRunning */


	/* Halt and flush all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		FlushChannel( ctx, channelNumber );

		if ( ctx->channel[ channelNumber ].bufferPool != NULL )
			DeleteMemPool( ctx->channel[ channelNumber ].bufferPool );
		}

	/* Get rid of any other memory we allocated */
	FreePtr( ctx->templateArray );

	/* Exiting should cause all resources we allocated to be
	 * returned to the system.
	 */
	ctx->threadItem = 0;
	exit(0);
	}


