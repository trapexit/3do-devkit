#pragma include_only_once

#include "extern_c.h"

#include "types.h"
#include "mempool.h"
#include "subscriberutils.h"

#define	SA_SUBS_MAX_CHANNELS 8

#define	SA_SUBS_MAX_BUFFERS 8

#define INTERNAL_MUTE	false
#define	INTERNAL_UNMUTE	false
#define	USER_MUTE	true
#define	USER_UNMUTE	true

#define IsChanInitalized(x) (x->channelOutput.instrument > 0)
#define IsChanEnabled(x)    (x->status & CHAN_ENABLED)
#define IsChanActive(x)	    (x->status & CHAN_ACTIVE)


typedef struct SAudioOutput
{
  Item	  instrument;		/* instrument item */
  long	  numChannels;		/* is instrument mono or stereo or ? */
  Item	  leftEnv;		/* envelope instruments for channel amplitude ramping */
  Item	  leftEnvTargetKnob;	/* knob to set target envelope value */
  Item	  rightEnv;
  Item	  rightEnvTargetKnob;	/* knob to set target envelope value */
  long 	  currentAmp;
  long 	  savedAmp;		/* previous amplitude to restore, used for muting */
  boolean muted;		/* is channel in mute mode */
  boolean externalMute;		/* Should channel be unmuted by StartSAudioChannel()? */
  long 	  currentPan;
} SAudioOutput, *SAudioOutputPtr;


struct SAudioBufferPtr;   /* forward ref */

typedef struct SAudioChannel
{
  unsigned long	       status;	/* state bits (see below) */
  long		       numBuffers; /* how many buffers to use for this channel */
  MemPoolPtr 	       bufferPool; /* pool of pre initalized attachments, cues, signals, and samples */
  Item		       channelInstrument; /* DSP instrument to play channel's data chunks */
  boolean	       instStarted; /* flag to know if instrument is started */
  boolean	       instPaused; /* flag to know if instrument is paused */
  ulong		       pauseTime; /* stream clock value when pause was initiated */
  boolean	       attachmentsRunning; /* flag to know if attachments are running */
  ulong   	       signalMask; /* the ORd signals for all the current cues on this channel */
  SubsQueue	       dataQueue; /* waiting data chunks */
  long		       inuseCount; /* number of buffers currently in the in use queue */
  struct SAudioBuffer* inuseQueueHead; /* pointer to head of buffers queued to the audio folio */
  struct SAudioBuffer* inuseQueueTail; /* pointer to tail of buffers queued to the audio folio */
  SAudioOutput	       channelOutput; /* contains output instrument and control knobs */
} SAudioChannel, *SAudioChannelPtr;


struct SAudioContext;
struct SAudioHeaderChunk;

EXTERN_C_BEGIN

long InitSAudioChannel(struct SAudioContext* ctx, struct SAudioHeaderChunk* headerPtr);
void StartSAudioChannel(struct SAudioContext* ctx, long channelNumber, boolean doFlush);
long StopSAudioChannel(struct SAudioContext* ctx, long channelNumber, boolean doFlush);
long FlushSAudioChannel(struct SAudioContext* ctx, long channelNumber);
long CloseSAudioChannel(struct SAudioContext* ctx, long channelNumber);

long SetSAudioChannelAmplitude(struct SAudioContext* ctx, long channelNumber, long newAmp);
long SetSAudioChannelPan(struct SAudioContext* ctx, long channelNumber, long newPan);
long GetSAudioChannelAmplitude(struct SAudioContext* ctx, long channelNumber, long* Amp);
long GetSAudioChannelPan(struct SAudioContext* ctx, long channelNumber, long* Pan);
long MuteSAudioChannel(struct SAudioContext* ctx, long channelNumber, boolean callerFlag);
long UnMuteSAudioChannel(struct SAudioContext* ctx, long channelNumber, boolean callerFlag);

void BeginSAudioPlaybackIfAppropriate(struct SAudioContext* ctx, long channelNumber);

EXTERN_C_END
