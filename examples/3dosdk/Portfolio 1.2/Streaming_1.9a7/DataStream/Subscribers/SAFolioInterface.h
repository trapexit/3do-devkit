/*******************************************************************************************
 *	File:			SAFolioInterface.h
 *
 *	Contains:		High level interface to AudioFolio for dynamic
 *buffer linking and playback
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	10/14/93	rdg		Version 2.8 -- New Today
 *						Split source into separate
 *files.
 */

#ifndef __SAFOLIOINTERFACE_H__
#define __SAFOLIOINTERFACE_H__

#include "SAChannel.h"
#include "SAudioSubscriber.h"
#include "SubscriberUtils.h"
#include "types.h"

/* For loading instrument templates */
#define SA_OUTPUT_INSTRUMENT_NAME "mixer2x2.dsp"
#define SA_ENVELOPE_INSTRUMENT_NAME "envelope.dsp"

/*****************************************************************
 * Items necessary to play a data chunk via the Audio Folio, one
 * set per channel, per stream.
 *****************************************************************/

typedef struct SAudioBuffer
{
  Item sample;     /* sample to use for playing a data chunk */
  Item attachment; /* attachment for connecting data to instrument */
  Item cue;        /* cue so we know when sample has finished */
  ulong signal;    /* signal so we can figure out which sample it was */
  SubscriberMsgPtr
      pendingMsg; /* ptr to msg so we can reply when this chunk is done */
  void *link;     /* for linking into lists */
} SAudioBuffer, *SAudioBufferPtr;

/**********************************************************/
/* Block to pass from InitChannel() to InitAudioBuffer()  */
/**********************************************************/
typedef struct SABufferInitBlock
{
  SAudioChannelPtr chanPtr;
  SAudioHeaderChunkPtr headerPtr;
} SABufferInitBlock, *SABufferInitBlockPtr;

/* Function Prototypes */
#ifdef __cplusplus
extern "C"
{
#endif

  Boolean InitAudioBuffer (void *initBlock, void *poolEntry);
  Boolean FreeAudioBuffer (void *initBlockPtr, void *poolEntry);
  bool AddBufferToTail (SAudioChannelPtr chanPtr, SAudioBufferPtr buffer);
  SAudioBufferPtr GetNextBuffer (SAudioChannelPtr chanPtr);
  void FindBuffer (SAudioContextPtr ctx, ulong signalBits,
                   SAudioChannelPtr *chanPtr, SAudioBufferPtr *bufferPtr);
  void MoveWaitingMsgsToBufferQueue (SAudioContextPtr ctx, long channelNumber);
  void QueueNewAudioBuffer (SAudioContextPtr ctx, SubscriberMsgPtr newSubMsg);
  void FreeBufferFromAudioFolio (SAudioBufferPtr bufferPtr);
  void HandleCompletedBuffers (SAudioContextPtr ctx, ulong signalBits);
  void OrphanPlayingBuffers (SAudioContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif /* __SAFOLIOINTERFACE_H__ */
