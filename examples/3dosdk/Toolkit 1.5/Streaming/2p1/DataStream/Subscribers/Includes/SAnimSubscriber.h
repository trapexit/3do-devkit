/*******************************************************************************************
 *	File:			SAnimSubscriber.h
 *
 *	Contains:		definitions for SAnimSubscriber.c
 *
 *	Written by:		Neil Cormia (variations on a theme by Joe
 *Buczek)
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/16/94		dld		Changed SANM_MAX_CHANNELS to 48 for
 *Streams created from Director 4.0. 4/27/94		fyp
 *Version 2.0 Changed the definitions of DS_MSG_HEADER and SUBS_CHUNK_COMMON to
 *require semicolon when used in context.  (For readibility and compilation
 *using ObjectMaster). 1/20/94		rdg		make C++ compatible
 *	11/23/93	jb		Add CloseSAnimSubscriber() routine
 *prototype 9/15/93		jb		Add 'APOS' and 'AMAP'
 *structures. 4/22/93		njc		New today.
 *
 *******************************************************************************************/
#ifndef __SANMSUBSCRIBER_H__
#define __SANMSUBSCRIBER_H__

#include "DataStreamLib.h"
#include "SubscriberUtils.h"
#include "audio.h"
#include "graphics.h"

/**********************/
/* Internal constants */
/**********************/

/* The following are definitions for constants that mark the beginning of
 * data chunka in a 3DO file.
 */
#define SANM_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('S', 'A', 'N', 'M') /* chunk data type for this subscriber */
#define AHDR_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('A', 'H', 'D', 'R') /* chunk data type for this subscriber */
#define ACCB_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('A', 'C', 'C', 'B') /* chunk data type for this subscriber */
#define FRME_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('F', 'R', 'M', 'E') /* chunk data type for this subscriber */

#define APOS_CHUNK_TYPE CHAR4LITERAL ('A', 'P', 'O', 'S') /* 'APOS' */
/* The above chunk subtype is used to change the X-Y position of
 * an animation cel without transmitting the entire CCB again.
 */

#define AMAP_CHUNK_TYPE CHAR4LITERAL ('A', 'M', 'A', 'P') /* 'AMAP' */
/* The above chunk subtype is used to change the mapping values
 * of an animation cel. The params are passed to MapCel() and
 * applied to the CCB.
 */

#define SANM_MAX_SUBSCRIPTIONS                                                \
  2 /* max # of streams that can use this subscriber */
#define SANM_MAX_CHANNELS 48 /* max # of logical channels per subscription */
#define SANM_MAX_CHUNKS 128  /* max # of chunks pending per subscriber */

#define SA_CTRL_GET_CHUNK 0
#define SA_CTRL_FREE_CHUNK 1

#define NO_FRAME_ITEM -1

/**************************************/
/* Messages sent by DS to subscribers */
/**************************************/
typedef struct SAnimMsg
{
  DS_MSG_HEADER;
  long channel;            /* retrieve a chunk from this channel	*/
  SubscriberMsgPtr subMsg; /* free this chunk back to the DSL		*/
} SAnimMsg, *SAnimMsgPtr;

/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

typedef struct SAnimChannel
{
  unsigned long status; /* state bits (see below) */
  SubsQueue dataQueue;  /* queue of waiting data chunks */
  Item dataQueueSem;    /* semaphore to manage access to data list */
  Boolean
      fFlushOnSync; /* flag: if true, flush all chunks from channel on sync */
} SAnimChannel, *SAnimChannelPtr;

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct SAnimContext
{
  Item creatorTask;     /* who to signal when we're done initializing */
  uint32 creatorSignal; /* signal to send for synchronous completion */
  int32 creatorStatus;  /* result code for creator */

  Item threadItem;        /* subscriber thread item */
  void *threadStackBlock; /* pointer to thread's stack memory block */

  Item requestPort;         /* message port item for subscriber requests */
  uint32 requestPortSignal; /* signal to detect request port messages */

  Item cueItem;          /* audio cue item for scheduling output */
  uint32 cueSignal;      /* signal associated with cueItem */
  Boolean fTimerRunning; /* flag: timer currently running */

  Boolean freeListNotEmpty; /* true if any entries in the freeList */
  SubsQueue freeList;       /* queue of freed data chunks */
  Item freeQueueSem;        /* semaphore to manage access to free list */
  Item freeQueueSignal;     /* signal to send to have subscriber free chunks */
  Item sanimTask;           /* who to signal when we want to free chunks */

  Boolean fTimeChanged; /* flag: subscriber got sync msg */

  int32 numChannels;
  SAnimChannel channel[SANM_MAX_CHANNELS]; /* an array of channels */

} SAnimContext, *SAnimContextPtr;

typedef struct SAnimChunkMsg
{
  DS_MSG_HEADER;
  void *buffer; /* ptr to the data */
} SAnimChunkMsg, *SAnimChunkMsgPtr;

/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

typedef struct StreamAnimHeader
{
  SUBS_CHUNK_COMMON; /*	subType = 'AHDR'			*/
  int32 version;     /*  current version = 0			*/
  int32 animType;    /* 	3 = streamed Anim			*/
  int32 numFrames;   /* 	number of frames in anim	*/
  int32 frameRate;   /* 	in frames per second		*/
} StreamAnimHeader, *StreamAnimHeaderPtr;

typedef struct StreamAnimCCB
{
  SUBS_CHUNK_COMMON; /*	subType = 'ACCB'			*/
  uint32 ccb_Flags;  /*	32 bits of CCB flags		*/
  struct CCB *ccb_NextPtr;
  CelData *ccb_CelData;
  void *ccb_PLUTPtr;
  Coord ccb_X;
  Coord ccb_Y;
  int32 ccb_hdx;
  int32 ccb_hdy;
  int32 ccb_vdx;
  int32 ccb_vdy;
  int32 ccb_ddx;
  int32 ccb_ddy;
  uint32 ccb_PPMPC;
  uint32 ccb_PRE0; /*	Sprite Preamble Word 0		*/
  uint32 ccb_PRE1; /*	Sprite Preamble Word 1		*/
  int32 ccb_Width;
  int32 ccb_Height;
} StreamAnimCCB, *StreamAnimCCBPtr;

typedef struct StreamAnimFrame
{
  SUBS_CHUNK_COMMON; /*	subType = 'FRME'			*/
  int32 duration;    /* 	in frames per second		*/
  /*	In here goes a PDAT (and maybe a PLUT chunk) */
} StreamAnimFrame, *StreamAnimFramePtr;

/****************************************************/
/* The APOS subtype is used to change the position	*/
/* of the current cel given new X-Y coordinates.	*/
/****************************************************/
typedef struct StreamAnimPos
{
  SUBS_CHUNK_COMMON; /* subType = 'APOS' */
  long duration;     /* in audio ticks */
  long newXValue;    /* new cel X position */
  long newYValue;    /* new cel Y position */
} StreamAnimPos, *StreamAnimPosPtr;

/****************************************************/
/* The AMAP subtype is used to issue MapCel() calls	*/
/* on the current cel. */
/****************************************************/
typedef struct StreamAnimMap
{
  SUBS_CHUNK_COMMON; /* subType = 'AMAP' */
  long duration;     /* in audio ticks */
  struct LongPoint
  {
    long x;
    long y;
  } quad[4]; /* 4 corner points for MapCel() call */

} StreamAnimMap, *StreamAnimMapPtr;

typedef struct SAnimRec
{
  DSStreamCBPtr streamCBPtr;
  struct StreamAnimCCB
      saCCB; /* Copy of the CCB chunk for this streamed anim */
  struct StreamAnimHeader
      saHeader; /* Copy of the Header chunk for this streamed anim */
  StreamAnimFramePtr curFramePtr; /* the frame currently being displayed */
  int32 channel; /* The streamed anim channel to use with this record */
  SubscriberMsgPtr
      curSubMsg;     /* The msg containing the currently displayed frame */
  int32 lastCurTime; /* Remember the previous Stream clock time to check for
                        loop */
} SAnimRec, *SAnimRecPtr;

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus
extern "C"
{
#endif

  int32 InitSAnimSubscriber (void);
  int32 CloseSAnimSubscriber (void);

  int32 NewSAnimSubscriber (SAnimContextPtr *pCtx, int32 numChannels,
                            int32 priority);
  int32 DisposeSAnimSubscriber (SAnimContextPtr ctx);

  int32 SendFreeSignal (SAnimContextPtr ctx);

  int32 InitSAnimCel (DSStreamCBPtr streamCBPtr, SAnimContextPtr ctx,
                      SAnimRecPtr *pSARecPtr, int32 channel,
                      Boolean flushOnSync);

  void FlushSAnimChannel (SAnimContextPtr ctx, SAnimRecPtr saRecPtr,
                          int32 channel);
  int32 DestroySAnimCel (SAnimContextPtr ctx, SAnimRecPtr saRecPtr,
                         int32 channel);
  CCB *GetSAnimCel (SAnimContextPtr ctx, SAnimRecPtr saRecPtr);
  int32 SendFreeSAnimSignal (SAnimContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif /* __SANMSUBSCRIBER_H__ */
