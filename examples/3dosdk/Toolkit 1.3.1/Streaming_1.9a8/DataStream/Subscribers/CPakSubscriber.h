/*******************************************************************************************
 *	File:			CPakSubscriber.h
 *
 *	Contains:		definitions for CPakSubscriber.c
 *
 *	Written by:		Neil Cormia
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	11/23/93	jb		Added prototype for
 *CloseCPakSubscriber() 4/28/93		njc		New today.
 *
 *******************************************************************************************/
#ifndef __CPAKSUBSCRIBER_H__
#define __CPAKSUBSCRIBER_H__

#include "SubscriberUtils.h"
#include "codec.h"
#include "graphics.h"

#ifndef __DATASTREAM_H__
#include "DataStreamLib.h"
#endif

/**********************/
/* Internal constants */
/**********************/

/* The following are definitions for constants that mark the beginning of
 * data chunka in a 3DO file.
 */
#define FILM_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('F', 'I', 'L', 'M') /* chunk data type for this subscriber */
#define FHDR_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('F', 'H', 'D', 'R') /* chunk data type for this subscriber */
#define FRME_CHUNK_TYPE                                                       \
  CHAR4LITERAL ('F', 'R', 'M', 'E') /* chunk data type for this subscriber */

#define CPAK_MAX_SUBSCRIPTIONS                                                \
  2                         /* max # of streams that can use this subscriber */
#define CPAK_MAX_CHANNELS 8 /* max # of logical channels per subscription */
#define CPAK_MAX_CHUNKS 128 /* max # of chunks pending per subscriber */

#define BYTES_PER_PIXEL 2   // this is all 16 bit uncoded data
#define SCANLINES_PER_ROW 2 // 2 interlaced scanlines per frame buffer line

#define NO_FRAME_ITEM -1 /* used to indicate no current frame msg item */

/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

typedef struct ImageDesc_Tag
{
  int32 baseAddr;
  int32 rowBytes;
  int32 width;
  int32 height;
} ImageDesc;
typedef ImageDesc *ImageDescPtr;

typedef struct CPakChannel
{
  unsigned long status; /* state bits (see below) */
  SubsQueue dataQueue;  /* queue of waiting data chunks */
  Item dataQueueSem;    /* semaphore to manage access to data list */
  Boolean
      fFlushOnSync; /* flag: if true, flush all chunks from channel on sync */
  ImageDesc imageData; /* this structure contains the LR form buffer and
                                          dimension fields for the unpacked
                          Cinepak data */
} CPakChannel, *CPakChannelPtr;

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct CPakContext
{
  Item creatorTask;     /* who to signal when we're done initializing */
  uint32 creatorSignal; /* signal to send for synchronous completion */
  int32 creatorStatus;  /* result code for creator */

  Item threadItem;        /* subscriber thread item */
  void *threadStackBlock; /* pointer to thread's stack memory block */

  Item requestPort;         /* message port item for subscriber requests */
  uint32 requestPortSignal; /* signal to detect request port messages */

  Item replyPort;         /* message port item for subscriber requests */
  uint32 replyPortSignal; /* signal to detect request port messages */

  Item cueItem;          /* audio cue item for scheduling output */
  uint32 cueSignal;      /* signal associated with cueItem */
  int32 localTimeOrigin; /* local version of the time */
  Boolean fTimerRunning; /* flag: timer currently running */

  codecHandler
      codecHndlr;  /* Should be an item - I don't really know what this is */
  codec filmCodec; /* a reference to the Cinepak decompressor code */

  Boolean freeListNotEmpty; /* true if any entries in the freeList */
  SubsQueue freeList;       /* queue of freed data chunks */
  Item freeQueueSem;        /* signal to send to have subscriber free chunks */
  Item freeQueueSignal;     /* signal to send to have subscriber free chunks */
  Item cpakTask;            /* who to signal when we want to free chunks */

  Boolean fTimeChanged; /* flag: subscriber got sync msg */

  int32 numChannels;
  CPakChannel channel[CPAK_MAX_CHANNELS]; /* an array of channels */

} CPakContext, *CPakContextPtr;

typedef struct CPakChunkMsg
{
  DS_MSG_HEADER
  void *buffer; /* ptr to the data */
} CPakChunkMsg, *CPakChunkMsgPtr;

/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

typedef struct CinePakHeader
{
  SUBS_CHUNK_COMMON
  int32 version; /*	0 for this version			*/
  int32 cType;   /*	video compression type		*/
  int32 height;  /*	Height of each frame		*/
  int32 width;   /*	Width of each frame			*/
  int32 scale;   /*	Timescale of Film			*/
  int32 count;   /*	Number of frames			*/
} CinePakHeader, *CinePakHeaderPtr;

typedef struct CinePakFrame
{
  SUBS_CHUNK_COMMON
  int32 duration;    /*	Duration of this sample		*/
  int32 frameSize;   /*	Number of bytes in frame	*/
  char frameData[4]; /*	compressed frame data...	*/
} CinePakFrame, *CinePakFramePtr;

/**********************************/
/* Cinepak channel context record */
/**********************************/

typedef struct CPakRec
{
  DSStreamCBPtr streamCBPtr;
  struct CinePakHeader
      cpHeader; /* Copy of the Header chunk for this cinepak film */
  CCB cpCCB;    /* The LRForm CCB chunk for this streamed anim */
  CinePakFramePtr curFramePtr; /* the frame currently being displayed */
  int32 channel; /* The streamed anim channel to use with this record */
  SubscriberMsgPtr
      curSubMsg;     /* The msg containing the currently displayed frame */
  int32 lastCurTime; /* Remember the previous Stream clock time to check for
                        loop */
} CPakRec, *CPakRecPtr;

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus
extern "C"
{
#endif

  int32 InitCPakSubscriber (void);
  int32 CloseCPakSubscriber (void);

  int32 NewCPakSubscriber (CPakContextPtr *pCtx, int32 numChannels,
                           int32 priority);
  int32 DisposeCPakSubscriber (CPakContextPtr ctx);

  int32 CPakDuration (CPakRecPtr cpRecPtr);
  int32 CPakCurrTime (CPakRecPtr cpRecPtr);

  int32 InitCPakCel (DSStreamCBPtr streamCBPtr, CPakContextPtr ctx,
                     CPakRecPtr *pCPRecPtr, int32 channel,
                     Boolean flushOnSync);
  int32 DestroyCPakCel (CPakContextPtr ctx, CPakRecPtr cpRecPtr,
                        int32 channel);
  CCB *GetCPakCel (CPakContextPtr ctx, CPakRecPtr cpRecPtr);
  void DrawCPakToBuffer (CPakContextPtr ctx, CPakRecPtr cpRecPtr,
                         Bitmap *bitmap);
  void FlushCPakChannel (CPakContextPtr ctx, CPakRecPtr cpRecPtr,
                         int32 channel);
  int32 SendFreeCPakSignal (CPakContextPtr ctx);

  void CPakSubscriberThread (int32 notUsed, CPakContextPtr ctx);
  void FreeMovieBuff (ImageDesc *imagePtr);

#ifdef __cplusplus
}
#endif

#endif /* __CPAKSUBSCRIBER_H__ */
