/*******************************************************************************************
 *	File:			ControlSubscriber.h
 *
 *	Contains:		definitions for ControlSubscriber.c
 *
 *	Written by:		Neil Cormia (variations on a theme by Joe
 *Buczek)
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	11/23/93	jb		Added prototype for
 *CloseCtrlSubscriber() 8/16/93		jb		Change
 *CTRL_NUM_DS_REQS_MSGITEMS to CTRL_NUM_DS_REQS_MSGS because we need to
 *allocate msg buffer space as well as message items. Change dsReqMsgItemPool
 *to dsReqMsgPool.
 *	8/9/93		jb		Add dsReqMsgItemPool, a pool of message
 *items for communication of streamer requests (necessitated by async
 *operation). Add dsReqReplyPort, dsReqReplyPortSignal, to go with this. Add
 *CTRL_NUM_DS_REQS_MSGITEMS. 8/5/93		jb		Add
 *CTRL_MAX_SUBS_MESSAGES Added subsReplyPort, subsReplyPortSignal, and
 *subsMsgPool to the context block.
 *	6/28/93		jb		Add ALRM, PAUS, and STOP subchunk types
 *	6/15/93		jb		Change branch chunk type from 'MRKR' to
 *'GOTO' Remove include of "graphics.h", replace with "types.h" Switch to using
 *SubsChannel instead of custom channel struct that is identical with the
 *generic version. 5/10/93		njc		New today.
 *
 *******************************************************************************************/
#ifndef __CTRLSUBSCRIBER_H__
#define __CTRLSUBSCRIBER_H__

#include "ItemPool.h"
#include "SubscriberUtils.h"
#include "types.h"

#ifndef __DATASTREAM_H__
#include "DataStreamLib.h"
#endif

/**********************/
/* Internal constants */
/**********************/

#define CTRL_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('C', 'T', 'R', 'L') /* chunk type for this subscriber */

#define GOTO_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('G', 'O', 'T', 'O') /* GOTO chunk sub-type */
#define SYNC_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('S', 'Y', 'N', 'C') /* SYNC chunk sub-type */
#define ALRM_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('A', 'L', 'R', 'M') /* ALRM chunk sub-type */
#define PAUS_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('P', 'A', 'U', 'S') /* PAUS chunk sub-type */
#define STOP_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('S', 'T', 'O', 'P') /* STOP chunk sub-type */

#define CTRL_MAX_SUBSCRIPTIONS                                                \
  4                         /* max # of streams that can use this subscriber */
#define CTRL_MAX_CHANNELS 1 /* max # of logical channels per subscription */
#define CTRL_MAX_SUBS_MESSAGES                                                \
  16 /* max # of msgs to allocate for broadcasting */
#define CTRL_NUM_DS_REQS_MSGS                                                 \
  8 /* number of msgs for async streamer requests */

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct CtrlContext
{
  Item creatorTask;          /* who to signal when we're done initializing */
  uint32 creatorSignal;      /* signal to send for synchronous completion */
  int32 creatorStatus;       /* result code for creator */
  DSStreamCBPtr streamCBPtr; /* Ptr to the stream's context block */

  Item threadItem;        /* subscriber thread item */
  void *threadStackBlock; /* pointer to thread's stack memory block */

  Item requestPort;         /* message port item for subscriber requests */
  uint32 requestPortSignal; /* signal to detect request port messages */

  Item dsReqReplyPort;         /* reply port for requests to streamer */
  uint32 dsReqReplyPortSignal; /* signal for replies to streamer requests */
  MemPoolPtr
      dsReqMsgPool; /* pool of message blocks for requests to streamer */

  Item subsReplyPort;         /* reply port for subscriber broadcasts */
  uint32 subsReplyPortSignal; /* signal for subscriber reply port */
  MemPoolPtr subsMsgPool;     /* pool of subscriber message blocks */

  Item cueItem;          /* audio cue item for scheduling output */
  uint32 cueSignal;      /* signal associated with cueItem */
  Boolean fTimerRunning; /* flag: timer currently running */
  uint32 timerOwner;     /* subchunk processing that is using the timer */

  uint32 newClockTime; /* set stream clock to this when we wake from timer */

  int32 numChannels;
  SubsChannel channel[CTRL_MAX_CHANNELS]; /* an array of channels */

} CtrlContext, *CtrlContextPtr;

/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

typedef struct ControlChunk
{
  SUBS_CHUNK_COMMON /* from SubscriberUtils.h */
      union
  {
    struct
    { /* sub-type 'GOTO' */
      uint32 value;
    } marker;

    struct
    { /* sub-type 'SYNC' */
      uint32 value;
    } sync;

    struct
    { /* sub-type 'ALRM' */
      uint32 options;
      uint32 newTime;
    } alarm;

    struct
    { /* sub-type 'PAUS' */
      uint32 options;
      uint32 amount;
    } pause;

    struct
    { /* sub-type 'STOP' */
      uint32 options;
    } stop;
  } u;

} ControlChunk, *ControlChunkPtr;

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus
extern "C"
{
#endif

  int32 InitCtrlSubscriber (void);
  int32 CloseCtrlSubscriber (void);

  int32 NewCtrlSubscriber (CtrlContextPtr *pCtx, DSStreamCBPtr streamCBPtr,
                           int32 priority);
  int32 DisposeCtrlSubscriber (CtrlContextPtr ctx);

  void CtrlSubscriberThread (int32 notUsed, CtrlContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif /* __CTRLSUBSCRIBER_H__ */
