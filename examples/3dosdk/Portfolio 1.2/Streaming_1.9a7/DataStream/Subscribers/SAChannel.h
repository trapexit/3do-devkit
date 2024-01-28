/*********************************************************************************
 *	File:			SAChannel.h
 *
 *	Contains:		Definitions the logical channel data
 *structures.
 *
 *	Written by:		Darren Gibbs.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	10/26/93	rdg		Version 2.9
 *						Get rid of envelope rate knobs... set
 *once and for all at init time. Distinguish between user and internal muting
 *and unmuting 10/15/93	rdg		Version 2.8 -- New Today Split source
 *into separate files.
 */

#ifndef __SACHANNEL_H__
#define __SACHANNEL_H__

#include "MemPool.h"
#include "SubscriberUtils.h"
#include "types.h"

/* max # of logical channels per subscription */
#define SA_SUBS_MAX_CHANNELS 8

/* max # of sample buffers which can be queued
 * to the folio at on time; per channel
 */
#define SA_SUBS_MAX_BUFFERS 8

/* Passed to MuteChannel() so that we know if we were muted
 * by the user, or internally.
 */
#define INTERNAL_MUTE false
#define INTERNAL_UNMUTE false
#define USER_MUTE true
#define USER_UNMUTE true

/* Handy macros for testing channel status.
 */
#define IsChanInitalized(x) (x->channelOutput.instrument > 0)
#define IsChanEnabled(x) (x->status & CHAN_ENABLED)
#define IsChanActive(x) (x->status & CHAN_ACTIVE)

/*****************************************************************
 * The output instrument Item and fields necessary to control
 * volume and panning.
 *****************************************************************/

typedef struct SAudioOutput
{
  Item instrument;  /* instrument item */
  long numChannels; /* is instrument mono or stereo or ? */
  Item leftEnv;     /* envelope instruments for channel amplitude ramping */
  Item leftEnvTargetKnob; /* knob to set target envelope value */
  Item rightEnv;
  Item rightEnvTargetKnob; /* knob to set target envelope value */
  long currentAmp;
  long savedAmp;        /* previous amplitude to restore, used for muting */
  Boolean muted;        /* is channel in mute mode */
  Boolean externalMute; /* Should channel be unmuted by StartChannel()? */
  long currentPan;
} SAudioOutput, *SAudioOutputPtr;

/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

struct SAudioBufferPtr; /* forward ref */

typedef struct SAudioChannel
{
  unsigned long status;   /* state bits (see below) */
  long numBuffers;        /* how many buffers to use for this channel */
  MemPoolPtr bufferPool;  /* pool of pre initalized attachments, cues, signals,
                             and samples */
  Item channelInstrument; /* DSP instrument to play channel's data chunks */
  Boolean instStarted;    /* flag to know if instrument is started */
  Boolean attachmentsRunning; /* flag to know if attachments are running */
  ulong signalMask; /* the ORd signals for all the current cues on this channel
                     */
  SubsQueue dataQueue; /* waiting data chunks */
  long inuseCount;     /* number of buffers currently in the in use queue */
  struct SAudioBuffer *inuseQueueHead; /* pointer to head of buffers queued to
                                          the audio folio */
  struct SAudioBuffer *inuseQueueTail; /* pointer to tail of buffers queued to
                                          the audio folio */
  SAudioOutput
      channelOutput; /* contains output instrument and control knobs */
} SAudioChannel, *SAudioChannelPtr;

/* Forward references */
struct SAudioContext;
struct SAudioHeaderChunk;

#ifdef __cplusplus
extern "C"
{
#endif

  long InitChannel (struct SAudioContext *ctx,
                    struct SAudioHeaderChunk *headerPtr);
  void StartChannel (struct SAudioContext *ctx, long channelNumber);
  long StopChannel (struct SAudioContext *ctx, long channelNumber);
  long FlushChannel (struct SAudioContext *ctx, long channelNumber);
  long CloseChannel (struct SAudioContext *ctx, long channelNumber);

  long SetChannelAmplitude (struct SAudioContext *ctx, long channelNumber,
                            long newAmp);
  long SetChannelPan (struct SAudioContext *ctx, long channelNumber,
                      long newPan);
  long GetChannelAmplitude (struct SAudioContext *ctx, long channelNumber,
                            long *Amp);
  long GetChannelPan (struct SAudioContext *ctx, long channelNumber,
                      long *Pan);
  long MuteChannel (struct SAudioContext *ctx, long channelNumber,
                    Boolean callerFlag);
  long UnMuteChannel (struct SAudioContext *ctx, long channelNumber,
                      Boolean callerFlag);

  void BeginPlaybackIfAppropriate (struct SAudioContext *ctx,
                                   long channelNumber);

#ifdef __cplusplus
}
#endif

#endif /* __SACHANNEL_H__ */
