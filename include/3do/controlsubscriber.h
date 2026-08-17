#pragma once

#include "extern_c.h"

#include "types.h"
#include "subscriberutils.h"
#include "itempool.h"
#include "datastreamlib.h"

#define	CTRL_CHUNK_TYPE    CHAR4LITERAL('C','T','R','L') /* chunk type for this subscriber */
#define	GOTO_CHUNK_SUBTYPE CHAR4LITERAL('G','O','T','O') /* GOTO chunk sub-type */
#define	SYNC_CHUNK_SUBTYPE CHAR4LITERAL('S','Y','N','C') /* SYNC chunk sub-type */
#define	ALRM_CHUNK_SUBTYPE CHAR4LITERAL('A','L','R','M') /* ALRM chunk sub-type */
#define	PAUS_CHUNK_SUBTYPE CHAR4LITERAL('P','A','U','S') /* PAUS chunk sub-type */
#define	STOP_CHUNK_SUBTYPE CHAR4LITERAL('S','T','O','P') /* STOP chunk sub-type */

#define	CTRL_MAX_SUBSCRIPTIONS 4 /* max # of streams that can use this subscriber */
#define	CTRL_MAX_CHANNELS      1 /* max # of logical channels per subscription */
#define	CTRL_MAX_SUBS_MESSAGES 16 /* max # of msgs to allocate for broadcasting */
#define	CTRL_NUM_DS_REQS_MSGS  8 /* number of msgs for async streamer requests */


typedef struct CtrlContext
{
  Item		creatorTask;	/* who to signal when we're done initializing */
  u32	creatorSignal;	/* signal to send for synchronous completion */
  s32		creatorStatus;	/* result code for creator */
  DSStreamCBPtr	streamCBPtr;	/* Ptr to the stream's context block */

  Item	threadItem;		/* subscriber thread item */
  void*	threadStackBlock;	/* pointer to thread's stack memory block */

  Item	 requestPort;		/* message port item for subscriber requests */
  u32 requestPortSignal;	/* signal to detect request port messages */

  Item	     dsReqReplyPort;	/* reply port for requests to streamer */
  u32     dsReqReplyPortSignal; /* signal for replies to streamer requests */
  MemPoolPtr dsReqMsgPool;	/* pool of message blocks for requests to streamer */

  Item	     subsReplyPort;	/* reply port for subscriber broadcasts */
  u32     subsReplyPortSignal; /* signal for subscriber reply port */
  MemPoolPtr subsMsgPool;	/* pool of subscriber message blocks */

  Item	  cueItem;		/* audio cue item for scheduling output */
  u32  cueSignal;		/* signal associated with cueItem */
  boolean fTimerRunning;	/* flag: timer currently running */
  u32  timerOwner;		/* subchunk processing that is using the timer */

  u32 newClockTime;		/* set stream clock to this when we wake from timer */

  s32	      numChannels;
  SubsChannel channel[CTRL_MAX_CHANNELS]; /* an array of channels */
} CtrlContext, *CtrlContextPtr;

typedef struct ControlChunk
{
  SUBS_CHUNK_COMMON;			/* from SubscriberUtils.h */
  union {
    struct {				/* sub-type 'GOTO' */
      u32		value;
    } marker;

    struct {				/* sub-type 'SYNC' */
      u32		value;
    } sync;

    struct {				/* sub-type 'ALRM' */
      u32		options;
      u32		newTime;
    } alarm;

    struct {				/* sub-type 'PAUS' */
      u32		options;
      u32		amount;
    } pause;

    struct {				/* sub-type 'STOP' */
      u32		options;
    } stop;
  } u;

} ControlChunk, *ControlChunkPtr;

EXTERN_C_BEGIN

s32 InitCtrlSubscriber(void);
s32 CloseCtrlSubscriber(void);

s32 NewCtrlSubscriber(CtrlContextPtr *pCtx, DSStreamCBPtr	streamCBPtr, s32 priority);
s32 DisposeCtrlSubscriber(CtrlContextPtr ctx);

void CtrlSubscriberThread(s32 notUsed, CtrlContextPtr ctx);

EXTERN_C_END
