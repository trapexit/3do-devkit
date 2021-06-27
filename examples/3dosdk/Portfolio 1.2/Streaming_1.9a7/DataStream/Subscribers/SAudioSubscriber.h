/*******************************************************************************************
 *	File:			SAudioSubscriber.h
 *
 *	Contains:		definitions for AudioSubscriber.c
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	1/20/94		rdg		make C++ compatible
 *	11/23/93	jb		Version 2.9
 *						Added CloseSAudioSubscriber()
 *	10/15/93	rdg		Version 2.8 
 *						Split source into separate files.
 *	10/13/93	rdg		Version 2.7
 *						Add support for envelopes again, hopefully the right way this time.
 *						Remove leftGain1 knob from output structure... not necessary.
 *						Add rate field to the pan part of the Control message data structure.
 *						Add leftEnvRateKnob and rightEnvRateKnob to output structure.
 *						Remove knobs from output struct, not needed with envelopes.
 *						Add error for SAudioErrGrabKnobFailed.
 *						Add savedAmpRate to output struct to support muting.
 *						Add leftEnvTargetKnob and rightEnvTargetKnob for ramping.
 *						Remove support for user controllable panning and amplitude rate.
 *						Remove SAudioErrBadChannel.  Use SAudioErrChanOutOfRange instead.
 *	9/28/93		rdg		Version 2.6.4
 *						Add kSAudioCtlOpMuteChannel, kSAudioCtlOpUnMuteChannel.
 *						Remove support for context envelope.
 *	9/27/93		rdg		Version 2.6.2
 *						Add Boolean attachmentsRunning flag to the Channel struct					
 *	9/24/93		rdg		Version 2.6.1
 *						Ramp volumes on start and stop with an envelope in the DSP.
 *						Added EnvIns, Envelope, EnvAttachment to context struct.
 *						Added SA_ENVELOPE_INSTRUMENT_NAME.
 *	9/20/93		rdg		Version 2.4
 *						Add savedAmp field to the SAudioOutput struct.  
 *	8/11/93		rdg		Version 2.2
 *						Removed attachRunning from channel structure.  No longer needed.
 *	8/9/93		rdg		Version 2.1
 *						Add kSAudioCtlOpFlushChannel which stops and flushes the currently queued data.
 *	8/6/93		rdg		Version 2.0
 *						Add kSAudioCtlOpCloseChannel so that you can unload that channel's instrument from
 *						the DSP when a logical channel's data is done to free up resources for
 *						newer stuff.
 *	6/25/93		rdg		Version 1.6
 *						Remove references to frequency.  Not useful in this context because
 *						doing sample rate interpolation to change frequency will screw up 
 *						sync with the stream clock.
 *	6/25/93		rdg		Version 1.5
 *						change SAudioOutput member bool stereo to int32 numChannels...
 *						New error codes...
 *						New control opcodes: kSAudioCtlOpGetVol, kSAudioCtlOpGetFreq, kSAudioCtlOpGetPan,	
 *						kSAudioCtlOpGetClockChan.	
 *						Add SA_22K_16B_M_SDX2.
 *						Add Boolean attachRunning flag to the Channel struct
 *	6/22/93		rdg		Version 1.1
 *						Modify for amplitude and pan control.
 *						Change output instrument to mixer2x2.dsp to allow for gain and pan
 *						control.
 *						Add SAudioOutput struct for deal with mixer knobs.  
 *						Remove initialAmplitude field from Channel struct.
 *						Replace channelOutput field with a SAudioOutput struct.
 *						Change SA_DIRECT_OUT_INSTRUMENT_NAME to SA_OUTPUT_INSTRUMENT_NAME
 *	6/19/93		rdg		Version 1.0
 *	6/19/93		rdg		Add clockChannel to SAudioContext.  Add clock struct to 
 *						SAudioControlBlock. clockChannel defaults to 0 and is 
 *						used to determine which logical channel to use for the
 *						"clock".  The time stamp data in all other logical channels is
 *						ignored.  clockChannel can be changed via an SAudioSubscriber
 *						control message.
 *						Added kSAudioCtlOpSetClockChan to the enum for SAudioControlOpcode.
 *						Added stream control block to the subscriber context struct.
 *	6/15/93		jb		Version 0.16
 *						Switch to using CHAR4LITERAL macro to avoid compiler warnings.
 *	5/27/93		rdg		Version 0.15
 *						Enabled #define tags for new instrument/data types:
 *						22K 8bit Stereo, 44K 8bit Stereo, 22K 16bit Stereo Compressed.
 *						Modified SAudioSampleDescriptor field sampleRate to be int32 instead
 *						of a fract16.
 *	5/27/93		rdg		Version 0.14
 *	5/26/93		rdg		Version 0.12
 *						Allow for preloading of instrument templates via control message.
 *						Preload output instrument template at thread instantiation time 
 *						and create output instrument when channel is opened. Make mechanism
 *						table/tag driven to allow for easy addition of new instrument types.
 *						Added 'outputTemplateItem' and 'templateArray' to subscriber context.
 *	5/26/93		rdg		Change to 'SAudioCtlBlock' to a union for different control
 *						request types. Add field for handling template stuff.
 *	5/25/93		rdg		Added TemplateRec to describe templates we know about.
 *						Added an opcode to SAudioCtl for loading templates.
 *	5/25/93		rdg		Version 0.11
 *						added acutalSampleSize to the SSMPHeaderChunk
 *  5/17/93		rdg		Version 0.10
 *						stripped the ctlBlock because the stream will now contain the initialization data
 *						and ctl messages will only be used to change the state of a playing stream.
 *						I also altered the SAudioHeaderChunk so I can pass it to OpenChannel() directly.
 *  5/17/93		rdg		Merged Neil's changes with my updates.  He created the chunk/subchunk structure
 *						which uses SNDS, SHDR, SSMP types.
 *  5/17/93		rdg		I removed the definition of SAUDIO_INITIAL_AMPLITUDE and added a field to
 *						the SAudioChannel and SAudioCtlBlock structures called initialAmplitude. Now this 
 *						value can be passed via a header chunk in the stream or from the main app at 
 *						OpenChannel() time.
 *  5/17/93		rdg		Changed the name of the "width" field in the SAudioSampleDescriptor
 *						to "numbits". 
 *  5/14/93		rdg		Changed the name of kSAudioCtlOpInitChan to kSAudioCtlOpOpenChan.
 *	5/11/93		rdg		Completed first pass at full implementation.
 *	5/8/93		rdg		begin conversion to AudioSubscriber
 *	4/19/93		jb		Add fields to context to support synchronous communications
 *						with a subscriber's creator at the time of creation (so that
 *						a proper completion status can be given to the creator).
 *	4/17/93		jb		Added TEST_CHUNK_TYPE
 *	4/16/93		jb		Switch to New/Dispose model; return context on New to allow for
 *						later cleanup of a subscriber.
 *	4/13/93		jb		Added TestChunkData.
 *	4/12/93		jb		New today.
 *
 *******************************************************************************************/
#ifndef __SAUDIOSUBSCRIBER_H__
#define __SAUDIOSUBSCRIBER_H__

#include "DataStream.h"
#include "SubscriberUtils.h"
#include "SAStreamChunks.h"
#include "SAChannel.h"
#include "SATemplates.h"
#include "SAControlMsgs.h"

/*************/
/* Constants */
/*************/

#define SAUDIO_STREAM_VERSION		0		/* stream version supported by subscriber */

#define	SNDS_CHUNK_TYPE		CHAR4LITERAL('S','N','D','S')	/* subscriber data type */
#define	SHDR_CHUNK_TYPE		CHAR4LITERAL('S','H','D','R')	/* header subchunk type */
#define	SSMP_CHUNK_TYPE		CHAR4LITERAL('S','S','M','P')	/* sample subchunk type */

#define	SA_SUBS_MAX_SUBSCRIPTIONS	4		/* max # of streams that can use this subscriber */

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct SAudioContext {
	Item			creatorTask;					/* who to signal when we're done initializing */
	ulong			creatorSignal;					/* signal to send for synchronous completion */
	long			creatorStatus;					/* result code for creator */

	Item			threadItem;						/* subscriber thread item */
	void*			threadStackBlock;				/* pointer to thread's stack memory block */

	Item			requestPort;					/* message port item for subscriber requests */
	ulong			requestPortSignal;				/* signal to detect request port messages */

	DSStreamCBPtr	streamCBPtr;					/* stream this subscriber belongs to */
	
	TemplateRecPtr	templateArray;					/* ptr to array of template records */

	ulong			allBufferSignals;				/* or's signals for all instruments on all channels */

	Item			outputTemplateItem;				/* template item for making channel's output instrument */

	Item			envelopeTemplateItem;			/* template item for making channel's envelope instrument(s) */

	long			clockChannel;					/* which logical channel to use for clock */
		 
	SAudioChannel	channel[SA_SUBS_MAX_CHANNELS];	/* an array of channels */

	} SAudioContext, *SAudioContextPtr;




/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

long	InitSAudioSubscriber( void );
long	CloseSAudioSubscriber( void );

long	NewSAudioSubscriber( SAudioContextPtr *pCtx, DSStreamCBPtr streamCBPtr, long deltaPriority );
long	DisposeSAudioSubscriber( SAudioContextPtr ctx );

#ifdef __cplusplus
}
#endif

#endif	/* __SAUDIOSUBSCRIBER_H__ */

