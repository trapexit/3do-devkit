#pragma once

#include "extern_c.h"

#include "graphics.h"
#include "subscriberutils.h"
#include "subscriberconstants.h"
#include "codec.h"
#include "datastreamlib.h"

#define	FILM_CHUNK_TYPE	CHAR4LITERAL('F','I','L','M') /* chunk data type for this subscriber */

/* FHDR_CHUNK_TYPE, FRME_CHUNK_TYPE, CPAK_MAX_SUBSCRIPTIONS,
 * CPAK_MAX_CHANNELS, CPAK_MAX_CHUNKS, BYTES_PER_PIXEL,
 * SCANLINES_PER_ROW and NO_FRAME_ITEM are shared with ezqsubscriber.h
 * and are defined in subscriberconstants.h. */


typedef struct ImageDesc
{
  s32	baseAddr;
  s32	rowBytes;
  s32	baseAddrFrame;
  s32	width;
  s32	height;
  s32	xPos;
  s32	yPos;
} ImageDesc;

typedef ImageDesc *ImageDescPtr;

typedef struct CPakChannel CPakChannel;
typedef CPakChannel* CPakChannelPtr;
struct CPakChannel
{
  unsigned long	status;		/* state bits (see below) */
  SubsQueue	dataQueue;	/* queue of waiting data chunks */
  Item		dataQueueSem;	/* semaphore to manage access to data list */
  boolean	fFlushOnSync;	/* flag: if true, flush all chunks from channel on sync */
  ImageDesc	imageData;    	/* this structure contains the LR form buffer and
                                   dimension fields for the unpacked Cinepak data */
};

typedef struct CPakContext CPakContext;
typedef CPakContext* CPakContextPtr;
struct CPakContext
{
  Item	 creatorTask;		/* who to signal when we're done initializing */
  u32 creatorSignal;		/* signal to send for synchronous completion */
  s32	 creatorStatus;		/* result code for creator */

  Item	threadItem;		/* subscriber thread item */
  void*	threadStackBlock;	/* pointer to thread's stack memory block */

  Item	 requestPort;		/* message port item for subscriber requests */
  u32 requestPortSignal;	/* signal to detect request port messages */

  Item   replyPort;             /* message port item for subscriber requests */
  u32 replyPortSignal;	/* signal to detect request port messages */

  Item	  cueItem;              /* audio cue item for scheduling output */
  u32  cueSignal;            /* signal associated with cueItem */
  s32	  localTimeOrigin;      /* local version of the time */
  boolean fTimerRunning;        /* flag: timer currently running */

  codecHandler codecHndlr;	/* Should be an item - I don't really know what this is */
  codec	       filmCodec;       /* a reference to the Cinepak decompressor code */

  boolean   freeListNotEmpty;	/* true if any entries in the freeList */
  SubsQueue freeList;		/* queue of freed data chunks */
  Item	    freeQueueSem;	/* signal to send to have subscriber free chunks */
  Item	    freeQueueSignal;    /* signal to send to have subscriber free chunks */
  Item	    cpakTask;		/* who to signal when we want to free chunks */

  boolean fTimeChanged;         /* flag: subscriber got sync msg */

  s32       numChannels;
  CPakChannel channel[CPAK_MAX_CHANNELS]; /* an array of channels */
};

typedef struct CPakChunkMsg CPakChunkMsg;
typedef CPakChunkMsg* CPakChunkMsgPtr;
struct CPakChunkMsg
{
  DS_MSG_HEADER;
  void*	buffer;			/* ptr to the data */
};

typedef	struct CinePakHeader
{
  SUBS_CHUNK_COMMON;
  s32	version;		/*	0 for this version			*/
  s32	cType;			/*	video compression type		*/
  s32	height;			/*	Height of each frame		*/
  s32	width;			/*	Width of each frame			*/
  s32	scale;			/*	Timescale of Film			*/
  s32	count;			/*	Number of frames			*/
} CinePakHeader, *CinePakHeaderPtr;


typedef	struct	CinePakFrame
{
  SUBS_CHUNK_COMMON;
  s32	duration;		/*	Duration of this sample		*/
  s32	frameSize;		/*	Number of bytes in frame	*/
  char	frameData[4];           /*	compressed frame data...	*/
} CinePakFrame, *CinePakFramePtr;


typedef struct CPakRec
{
  DSStreamCBPtr	       streamCBPtr;
  struct CinePakHeader cpHeader; /* Copy of the Header chunk for this cinepak film */
  CCB		       cpCCB;	/* The LRForm CCB chunk for this streamed anim */
  CinePakFramePtr      curFramePtr; /* the frame currently being displayed */
  s32		       channel;	/* The streamed anim channel to use with this record */
  SubscriberMsgPtr     curSubMsg; /* The msg containing the currently displayed frame */
  s32		       lastCurTime;	/* Remember the previous Stream clock time to check for loop */
} CPakRec, *CPakRecPtr;

EXTERN_C_BEGIN

s32 InitCPakSubscriber(void);
s32 CloseCPakSubscriber(void);

s32 NewCPakSubscriber(CPakContextPtr *pCtx, s32 numChannels, s32 priority);
s32 DisposeCPakSubscriber(CPakContextPtr ctx);

s32 CPakDuration(CPakRecPtr cpRecPtr);
s32 CPakCurrTime(CPakRecPtr cpRecPtr);

s32 InitCPakCel(DSStreamCBPtr   streamCBPtr,
                  CPakContextPtr  ctx,
                  CPakRecPtr	 *pCPRecPtr,
                  s32           channel,
                  boolean         flushOnSync);
s32 DestroyCPakCel(CPakContextPtr ctx, CPakRecPtr cpRecPtr, s32 channel);
CCB*  GetCPakCel(CPakContextPtr ctx, CPakRecPtr cpRecPtr);
void  DrawCPakToBuffer(CPakContextPtr ctx, CPakRecPtr cpRecPtr, Bitmap *bitmap);
void  FlushCPakChannel(CPakContextPtr ctx, CPakRecPtr cpRecPtr, s32 channel);
s32 SendFreeCPakSignal(CPakContextPtr ctx);

void CPakSubscriberThread(s32 notUsed, CPakContextPtr ctx);
void FreeMovieBuff(ImageDesc *imagePtr);

EXTERN_C_END
