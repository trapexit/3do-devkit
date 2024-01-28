#pragma include_only_once

#include "graphics.h"
#include "subscriberutils.h"
#include "codec.h"
#include "datastreamlib.h"

/**********************/
/* Internal constants */
/**********************/

/* The following are definitions for constants that mark the beginning of
 * data chunka in a 3DO file.
 */
#define	FILM_CHUNK_TYPE	    CHAR4LITERAL('E','Z','Q','F') /* chunk data type for this subscriber */
#define	FHDR_CHUNK_TYPE	    CHAR4LITERAL('F','H','D','R') /* chunk data type for this subscriber */
#define	FRME_CHUNK_TYPE	    CHAR4LITERAL('F','R','M','E') /* chunk data type for this subscriber */
#define	KEY_FRME_CHUNK_TYPE CHAR4LITERAL('F','R','M','E') /* chunk data type for this subscriber */
#define	DIF_FRME_CHUNK_TYPE CHAR4LITERAL('D','F','R','M') /* chunk data type for this subscriber */

#define	CPAK_MAX_SUBSCRIPTIONS		  2 /* max # of streams that can use this subscriber */
#define	CPAK_MAX_CHANNELS			  8 /* max # of logical channels per subscription */
#define	CPAK_MAX_CHUNKS	128	/* max # of chunks pending per subscriber */

#define	BYTES_PER_PIXEL	  2	// this is all 16 bit uncoded data
#define	SCANLINES_PER_ROW 2	// 2 interlaced scanlines per frame buffer line

#define	NO_FRAME_ITEM -1	/* used to indicate no current frame msg item */

//------------------------------------------------------------------------------------------------//
//
//		Streaming ketchup strategy:
//
//			SKIP_DIFF_FRAMES			  0		//  skip difference frames to catch up
//			DECODE_DIFF_FRAMES			  1		//  decode as fast as you can
//			BEST_METHOD					 -1		//  something else
//
//		BEST_METHOD has not been determined yet.  However a candidate is to keep a running
//		average key frame rate.  If the keyframe rate is less than or equal to 2 seconds skip
//		 diff frames.  However if the keyframe rate is greater than 2 seconds decode difference frames.
//
//  lla 2/22/94
//
//------------------------------------------------------------------------------------------------//

#define	SKIP_DIFF_FRAMES   0	//  skip difference frames to catch up
#define DECODE_DIFF_FRAMES 1	//  decode as fast as you can
#define	BEST_METHOD	   -1	//  something else

/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

typedef struct ImageDesc_Tag
{
  int32	baseAddr;
  int32	rowBytes;
  int32	width;
  int32	height;
}ImageDesc;
typedef ImageDesc *ImageDescPtr;


typedef struct EZQChannel {
  unsigned long	status;		/* state bits (see below) */
  SubsQueue	dataQueue;	/* queue of waiting data chunks */
  Item		dataQueueSem;	/* semaphore to manage access to data list */
  boolean	fFlushOnSync;	/* flag: if true, flush all chunks from channel on sync */
  ImageDesc	imageData;      /* this structure contains the LR form buffer and
                                   dimension fields for the unpacked EZSqueeze data */
} EZQChannel, *EZQChannelPtr;


/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct EZQContext {
  Item	 creatorTask;		/* who to signal when we're done initializing */
  uint32 creatorSignal;		/* signal to send for synchronous completion */
  int32	 creatorStatus;		/* result code for creator */

  Item	threadItem;		/* subscriber thread item */
  void*	threadStackBlock;	/* pointer to thread's stack memory block */

  Item	 requestPort;		/* message port item for subscriber requests */
  uint32 requestPortSignal;	/* signal to detect request port messages */

  Item	 replyPort;             /* message port item for subscriber requests */
  uint32 replyPortSignal;	/* signal to detect request port messages */

  Item	  cueItem; /* audio cue item for scheduling output */
  uint32  cueSignal; /* signal associated with cueItem */
  int32	  localTimeOrigin; /* local version of the time */
  boolean fTimerRunning; /* flag: timer currently running */

  codecHandler codecHndlr;	/* Should be an item - I don't really know what this is */
  codec	       filmCodec; /* a reference to the EZSqueeze decompressor code */

  boolean   freeListNotEmpty;	/* true if any entries in the freeList */
  SubsQueue freeList;		/* queue of freed data chunks */
  Item	    freeQueueSem;	/* signal to send to have subscriber free chunks */
  Item	    freeQueueSignal;    /* signal to send to have subscriber free chunks */
  Item	    cpakTask;		/* who to signal when we want to free chunks */

  boolean fTimeChanged;         /* flag: subscriber got sync msg */
  boolean firstFrameFlag;       /* flag: Last chunk received was header frame  -- WSD */

  int32	     framesDropped;			/*	count of frames skipped because we are behind -- WSD */
  int32	     numChannels;
  EZQChannel channel[CPAK_MAX_CHANNELS]; /* an array of channels */

} EZQContext, *EZQContextPtr;

typedef struct EZQChunkMsg {
  DS_MSG_HEADER;
  void*			buffer;				/* ptr to the data */
} EZQChunkMsg, *EZQChunkMsgPtr;

/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

typedef	struct EZSqueezeHeader {
  SUBS_CHUNK_COMMON;
  int32	version;		/*	0 for this version			*/
  int32	cType;			/*	video compression type		*/
  int32	height;			/*	Height of each frame		*/
  int32	width;			/*	Width of each frame			*/
  int32	scale;			/*	Timescale of Film			*/
  int32	count;			/*	Number of frames			*/
} EZSqueezeHeader, *EZSqueezeHeaderPtr;


typedef	struct	EZSqueezeFrame {
  SUBS_CHUNK_COMMON;
  int32	duration;		/*	Duration of this sample		*/
  int32	frameSize;		/*	Number of bytes in frame	*/
  char	frameData[4];           /*	compressed frame data...	*/
} EZSqueezeFrame, *EZSqueezeFramePtr;


/**********************************/
/* EZSqueeze channel context record */
/**********************************/

typedef struct EZQRec
{
  DSStreamCBPtr		 streamCBPtr;
  struct EZSqueezeHeader cpHeader; /* Copy of the Header chunk for this cinepak film */
  CCB			 cpCCB;	/* The LRForm CCB chunk for this streamed anim */
  EZSqueezeFramePtr	 curFramePtr; /* the frame currently being displayed */
  int32			 channel; /* The streamed anim channel to use with this record */
  SubscriberMsgPtr	 curSubMsg; /* The msg containing the currently displayed frame */
  int32			 lastCurTime; /* Remember the previous Stream clock time to check for loop */
} EZQRec, *EZQRecPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/

int32 InitEZQSubscriber( void );
int32 CloseEZQSubscriber( void );

void SetEZQScreen( int32 horizontalOffset, int32 verticalOffset, boolean centered );

int32 NewEZQSubscriber( EZQContextPtr *pCtx, int32 numChannels, int32 priority );
int32 DisposeEZQSubscriber( EZQContextPtr ctx );

int32 EZQDuration (EZQRecPtr ezRecPtr);
int32 EZQCurrTime (EZQRecPtr ezRecPtr);

int32 InitEZQCel(DSStreamCBPtr  streamCBPtr,
                 EZQContextPtr  ctx,
                 EZQRecPtr     *pCPRecPtr,
                 int32	       channel,
                 boolean        flushOnSync );
int32 DestroyEZQCel(EZQContextPtr ctx, EZQRecPtr ezRecPtr, int32 channel);

CCB* GetEZQCel(EZQContextPtr ctx, EZQRecPtr ezRecPtr);
int32 DrawEZQToBuffer(EZQContextPtr ctx, EZQRecPtr ezRecPtr, Bitmap *bitmap,
                      long catsup_strategy);
CCB* GetStepEZQCel(EZQContextPtr ctx, EZQRecPtr ezRecPtr);
int32 DrawStepEZQToBuffer(EZQContextPtr ctx, EZQRecPtr ezRecPtr, Bitmap *bitmap);

void FlushEZQChannel(EZQContextPtr ctx, EZQRecPtr ezRecPtr, int32 channel);
int32 SendFreeEZQSignal(EZQContextPtr ctx);

void EZQSubscriberThread(int32 notUsed, EZQContextPtr ctx);
void FreeMovieBuff(ImageDesc *imagePtr);
