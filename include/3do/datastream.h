#pragma include_only_once

#include "extern_c.h"

#include "types.h"
#include "item.h"
#include "io.h"
#include "msgutils.h"
#include "mempool.h"
#include "subschunkcommon.h"
#include "haltchunk.h"
#include "operror.h"

/* The following definitions are included so this header will compile under OS Versions
 * prior to 1.4.x.  These provide support for the portfolio error definitions for the DS
 * library, which would normally live in operror.h...
 */

#ifndef ER_LINKLIB
#define ER_LINKLIB Make6Bit('L')
#endif

#ifndef ER_NoSignals
#define ER_NoSignals 24         /* No signals available */
#endif

#ifndef ER_DATASTREAM
#define ER_DATASTREAM MakeErrId('D','S')		/* DataStream library (includes ds.lib, dataacq.lib & subscriber.lib */
#endif

#ifndef MakeDSErr
#define MakeDSErr(svr,class,err) MakeErr (ER_LINKLIB,ER_DATASTREAM,svr,ER_E_SSTM,class,err)
#endif

#ifndef MAKEDSERR
#define MAKEDSERR(svr,class,err) MakeDSErr(svr,class,err)
#endif


/* The following constant is the maximum number of subscribers allowed for
 * a stream. The number should be kept small to minimize the search needed
 * on a per chunk basis.
 */
#define	DS_MAX_SUBSCRIBERS 16


#if RELATIVE_BRANCHING

#define	kKeyChunkFlag 1<<0	/* indicates a chunk that does not
                                 * depend on its sibling predecessors */
#endif  /*  RELATIVE_BRANCHING  */


/***************/
/* Basic types */
/***************/
typedef unsigned long DSDataType;
typedef unsigned long *DSChunkPtr;

/************************************************************************/
/* Error codes returned by DSH, subscribers, and data acquisition procs */
/************************************************************************/

/****************************************************************/
/*		Errors that map nicely to standard system errors		*/
/****************************************************************/
#define kDSNoErr	 0
#define kDSAbortErr	 MakeDSErr (ER_SEVERE,ER_C_STND,ER_Aborted)
#define kDSEndOfFileErr	 MakeDSErr (ER_SEVERE,ER_C_STND,ER_EndOfMedium) /* end of file reached */
#define kDSUnImplemented MakeDSErr (ER_SEVERE,ER_C_STND,ER_NotSupported) /* requested function not implemented */
#define kDSNoSignalErr	 MakeDSErr (ER_SEVERE,ER_C_STND,ER_NoSignals) /* couldn't allocate needed signal */
#define kDSNoMemErr	 MakeDSErr (ER_SEVERE,ER_C_STND,ER_NoMem) /* couldn't allocate needed memory */

/****************************************************************/
/*		Errors that don't map into standard system error space	*/
/****************************************************************/
#define kDSWasFlushedErr      MakeDSErr (ER_SEVERE,ER_C_NSTND,0) /* buffer was flushed */
#define kDSNotRunningErr      MakeDSErr (ER_SEVERE,ER_C_NSTND,1) /* stream not running */
#define kDSWasRunningErr      MakeDSErr (ER_SEVERE,ER_C_NSTND,2) /* stream already running */
#define kDSNoPortErr	      MakeDSErr (ER_SEVERE,ER_C_NSTND,3) /* couldn't allocate a message port for stream */
#define kDSNoMsgErr	      MakeDSErr (ER_SEVERE,ER_C_NSTND,4) /* couldn't allocate message item for a data buffer */
#define kDSNotOpenErr	      MakeDSErr (ER_SEVERE,ER_C_NSTND,5) /* stream not open */
#define kDSSignalErr	      MakeDSErr (ER_SEVERE,ER_C_NSTND,6) /* problem sending/receiving a signal */
#define kDSNoReplyPortErr     MakeDSErr (ER_SEVERE,ER_C_NSTND,7) /* message requires a reply port */
#define kDSBadConnectPortErr  MakeDSErr (ER_SEVERE,ER_C_NSTND,8) /* invalid port specified for data connection */
#define kDSSubDuplicateErr    MakeDSErr (ER_SEVERE,ER_C_NSTND,9) /* duplicate subscriber */
#define kDSSubMaxErr	      MakeDSErr (ER_SEVERE,ER_C_NSTND,10) /* subscriber table full, too many subscribers */
#define kDSSubNotFoundErr     MakeDSErr (ER_SEVERE,ER_C_NSTND,11) /* specified subscriber not found */
#define kDSInvalidTypeErr     MakeDSErr (ER_SEVERE,ER_C_NSTND,12) /* invalid subscriber data type specified */
#define kDSBadBufAlignErr     MakeDSErr (ER_SEVERE,ER_C_NSTND,13) /* buffer list passed to DSHOpenStream contains a non QUADBYTE aligned buffer */
#define kDSBadChunkSizeErr    MakeDSErr (ER_SEVERE,ER_C_NSTND,14) /* chunk size in stream is a non-QUADBYTE multiple */
#define kDSInitErr	      MakeDSErr (ER_SEVERE,ER_C_NSTND,15) /* some internal initialization failed */
#define kDSClockNotValidErr   MakeDSErr (ER_SEVERE,ER_C_NSTND,16) /* clock dependent call failed because clock not set */
#define kDSInvalidDSRequest   MakeDSErr (ER_SEVERE,ER_C_NSTND,17) /* unknown request message send to server thread */
#define kDSEOSRegistrationErr MakeDSErr (ER_SEVERE,ER_C_NSTND,18) /* EOS registrant replaced by new registrant */
#define kDSRangeErr	      MakeDSErr (ER_SEVERE,ER_C_NSTND,19) /* parameter out of range */
#define kDSBranchNotDefined   MakeDSErr (ER_SEVERE,ER_C_NSTND,20) /* branch destination not defined */

/*******************************************************************************************
 *		Data buffer structures
 *******************************************************************************************/

/* This is the basic data buffer structure managed by the stream parser. Each
 * buffer is obtained from the stream's data acquisition component and must PHYSICALLY
 * look exactly like the following. The first part of every physical data buffer
 * must contain the header information described by this struct. The actual data
 * component of a DSDataBuf can be a different length for each stream. However, buffer
 * sizes are fixed within a given stream.
 */
  struct DSStreamCB;

struct DSDataBuf
{
  struct DSDataBuf*  permanentNext; /* ptr for keeping permanent global buffer list ... */
  /* ... for debugging, etc. (not used by streamer) */
  struct DSDataBuf*  next;	/* ptr for linking buffers into lists (free, in use, etc.) */
  long		     useCount;	/* count of subscribers using this buffer */
  struct DSStreamCB* streamCBPtr; /* ptr to the context block of the owning stream */

  /* NOTE: The following two fields are filled in by	*/
  /* data acquisition at "opening" time. 				*/

  Item	 ioreqItem;		/* I/O request item for queuing reads for this buffer */
  IOReq* ioreqItemPtr;          /* pointer to the Item in system space */

  char streamData[4];	/* start of (variable length) stream data */
};
typedef struct DSDataBuf DSDataBuf, *DSDataBufPtr;


/* The following is a logical data chunk imbedded in the stream of blocks. The
 * 'streamDataType' is used to search the stream's subscriber list, and any chunks that
 * match existing subscribers are passed to the subscriber. Chunks that do not
 * match any existing subscriber are ignored.
 * NOTE: ALL CHUNKS MUST BE QUADBYTE ALIGNED!!!
 */
struct StreamChunk
{
  unsigned long	streamChunkType; /* 4 byte ASCII subscriber data type */
  unsigned long	streamChunkSize; /* size of chunk including this header */
  unsigned char	streamChunkData[4]; /* start of variable length data */
};
typedef struct StreamChunk StreamChunk, *StreamChunkPtr;

/* The following preamble is used for all types of messages sent
 * by DSH. This enables a couple of utility routines to perform general
 * operations on lists.
 */
#define	DS_MSG_HEADER                                                   \
  long	whatToDo;	/* opcode determining msg contents */           \
  Item	msgItem;	/* message item for sending this buffer */      \
  void*	privatePtr;	/* ptr to sender's private data */              \
  void*	link;		/* user defined -- for linking msg into lists */

typedef struct GenericMsg {
  DS_MSG_HEADER;
} GenericMsg, *GenericMsgPtr;

/*******************************************************************************************
 *		Subscriber Interface (DSH to subscriber)
 *******************************************************************************************/

/*****************************************/
/* Opcode values as passed in 'whatToDo' */
/*****************************************/
enum StreamOpcode
  {
   kStreamOpData = 0,		/* new data has arrived */
   kStreamOpGetChan,		/* get logical channel status */
   kStreamOpSetChan,		/* set logical channel status */
   kStreamOpControl,		/* perform subscriber defined function */
   kStreamOpSync,			/* clock stream resynched the clock */
   kStreamOpStart,			/* stream is being started */
   kStreamOpStop,			/* stream is being stopped */

   /* The following msgs have no extended message arguments
    * and only may use the whatToDo and context
    * fields in the following message record.
    */

   kStreamOpOpening,		/* one time initialization call from DSH */
   kStreamOpClosing,		/* stream is being closed */
   kStreamOpEOF,			/* physical EOF on data, no more to come */
   kStreamOpAbort			/* some subscriber gave up, streaming is aborted */
  };


/**************************************/
/* Messages sent by DS to subscribers */
/**************************************/
typedef struct SubscriberMsg
{
  DS_MSG_HEADER;

  union {
    struct {								/* kStreamOpData */
      void*			buffer;				/* ptr to the data */
    } data;

    struct {								/* kStreamOpGetChan, kStreamOpSetChan */
      long			number;				/* channel number to operate upon */
      long			status;				/* channel status (bits 31-16 subscriber defined) */
    } channel;

    struct {								/* kStreamOpControl */
      long			controlArg1;		/* subscriber defined */
      void*			controlArg2;		/* subscriber defined */
    } control;

    struct {								/* kStreamOpSync */
      unsigned long	clock;				/* current time */
    } sync;

    struct {								/* kStreamOpStart */
      unsigned long	options;			/* start options */
    } start;

    struct {								/* kStreamOpStop */
      unsigned long	options;			/* stop options */
    } stop;

  } msg;

} SubscriberMsg, *SubscriberMsgPtr;


/* bits in status of 'channel' message above */

#define	CHAN_ENABLED (1<<0)	/* R/W: '1 if channel enabled (allows data to flow) */
#define	CHAN_ACTIVE  (1<<1)	/* R/O: '1 if channel data currently flowing */
#define	CHAN_EOF     (1<<2)	/* R/O: '1 if channel finished */
#define	CHAN_ABORTED (1<<3)	/* R/O: '1 if channel aborted (error) */
#define	CHAN_SYSBITS (0x0000FFFE) /* Mask of reserved bits, rest subscriber defined */
/* NOTE: least significant bit is R/W !!! */

/* bits in options of start and stop messages above */

#define	SOPT_NOFLUSH (0)	/* for readability */
#define	SOPT_FLUSH   (1<<0)	/* '1 if flush on start or stop request */


/*******************************************************************************************
 * 		Data Acquisition Interface (DSH to application)
 *******************************************************************************************/

/*****************************************/
/* Opcode values as passed in 'whatToDo' */
/*****************************************/
enum DataAcqOpcode
  {
   kAcqOpGetData = 0,		/* fill buffer with data */
   kAcqOpGoMarker,			/* seek to a marker in the stream */
   kAcqOpConnect,			/* stream connecting, perform initializations */
   kAcqOpDisconnect,		/* stream disconnecting, no more data requests follow */
   kAcqOpExit				/* release resources and exit */
  };

/*******************************************/
/* Messages sent by DS to data acquisition */
/*******************************************/

typedef struct DataAcqMsg
{
  DS_MSG_HEADER;

  union {
    struct {								/* kAcqOpGetData */
      DSDataBufPtr	bufferPtr;			/* ptr to DatBuf struct */
      long			bufferSize;			/* size of buffer in BYTES */
    } data;

    struct {								/* kAcqOpGoMarker */
      unsigned long	value;				/* the marker value (interpreted by data proc) */
      unsigned long	options;			/* options for the branch */
    } marker;

    struct {								/* kAcqOpConnect */
      struct DSStreamCB*	streamCBPtr;	/* ptr to stream control block we are connecting to */
    } connect;

  } msg;
} DataAcqMsg, *DataAcqMsgPtr;

/*******************************************************/
/* Values for options in DataAcqMsg.msg.marker.options */
/*******************************************************/
enum DataAcqMarkerOptions
  {
   GOMARKER_ABSOLUTE  =	0,	/* value == absolute marker FILE POSITION */
   GOMARKER_FORWARD   =	1,	/* value == count of markers to skip forward */
   GOMARKER_BACKWARD  =	2,	/* value == count of markers to skip backward */
   GOMARKER_ABS_TIME  =	3,	/* value == absolute stream time of destination */
   GOMARKER_FORW_TIME =	4,	/* value == stream time by which to advance */
   GOMARKER_BACK_TIME =	5,	/* value == stream time by which to regress */
   GOMARKER_NAMED     =	6,	/* value == (char*) ptr to name of destination marker */

#if RELATIVE_BRANCHING
   GOMARKER_NUMBER	     = 7, /* go to given marker number: ccw mod */
   GOMARKER_FWD_POS_RELATIVE = 8, /* value == the number of blocks to skip forward relative to our current position */
   GOMARKER_BWD_POS_RELATIVE = 9 /* value == the number of blocks to skip backward relative to our current position */
#else
   GOMARKER_NUMBER	     = 7 /* go to given marker number: ccw mod */
#endif
  };

/*******************************************************************************************
 *		Stream Control Data Structures
 *******************************************************************************************/

/*****************************************/
/* Opcode values as passed in 'whatToDo' */
/*****************************************/
enum DSRequestOpcode
  {
   kDSOpPreRollStream = 0,			/* preload all possible buffers */
   kDSOpCloseStream,				/* close the stream */
   kDSOpWaitEndOfStream,			/* find end of stream */

   /* The following messages have
    * arguments that are passed in
    * the 'msg' extension of the
    * request message.
    */

   kDSOpStartStream,				/* start data flowing */
   kDSOpStopStream,				/* stop data flowing */
   kDSOpSubscribe,					/* add a subscriber */
   kDSOpGoMarker,					/* position stream to a marker */
   kDSOpGetChannel,				/* get channel status */
   kDSOpSetChannel,				/* set channel status */
   kDSOpControl,					/* perform subscriber dependent control */

#if RELATIVE_BRANCHING
   kDSOpConnect,					/* connect data client to stream */
   kDSOpSetSkipMode,				/* tell the streamer to go into STRM_KEYCHUNK_SEARCH mode */
   kDSOpLastOp
#else
   kDSOpConnect					/* connect data client to stream */
#endif
  };

/***********************/
/* Messages sent to DS */
/***********************/

typedef struct DSRequestMsg
{
  DS_MSG_HEADER;

  union {
    struct {								/* kDSOpSubscribe */
      DSDataType		dataType;			/* 4 byte ASCII data type identifier */
      Item			subscriberPort;		/* message port to send data to */
    } subscribe;

    struct {								/* kDSOpClockSync */
      DSDataType		exemptStream;		/* stream to *not* send sync message to */
      unsigned long 	nowTime;			/* time value to propagate to everyone */
    } clockSync;

    struct {								/* kDSOpGoMarker */
      unsigned long 	markerValue;		/* place to "branch" to in the stream */
      unsigned long	options;			/* options for the branch */
    } goMarker;

    struct {								/* kDSOpGetChannel */
      DSDataType		streamType;			/* which subscriber */
      long			channelNumber;		/* logical data channel */
      long*			channelStatusPtr;	/* place to return channel status */
    } getChannel;

    struct {								/* kDSOpSetChannel */
      DSDataType		streamType;			/* which subscriber */
      long			channelNumber;		/* logical data channel */
      long			channelStatus;		/* channel status bits to set */
    } setChannel;

    struct {								/* kDSOpControl */
      DSDataType		streamType;			/* subscriber to send control msg to */
      long			userDefinedOpcode;	/* subscriber defined action code */
      void*			userDefinedArgPtr;	/* subscriber defined argument */
    } control;

    struct {								/* kDSOpConnect */
      Item			acquirePort;		/* connect this data port to stream */
    } connect;

    struct {								/* kDSOpStartStream */
      unsigned long	options;			/* start options */
    } start;

    struct {								/* kDSOpStopStream */
      unsigned long	options;			/* stop options */
    } stop;

  } msg;

} DSRequestMsg, *DSRequestMsgPtr;


/* This data structure describes a subscriber. The 'dataType' field is used to match
 * logical data chunks in incoming data blocks in the stream. Each matching data
 * block is delivered to the registered subscriber for that type of data. When the
 * subscriber is finished with the data, it calls the 'buffer release' proc, which
 * eventually frees the whole data block when all subscribers are finished with
 * their individual logical data chunks.
 */
typedef struct DSSubscriber
{
  DSDataType dataType;		/* type of data owned by subscriber */
  int32	     subscriberPort;	/* pointer to data subscriber proc */
} DSSubscriber, *DSSubscriberPtr;


/* This data structure contains all the context necessary to drive a stream, including
 * a description of all subscribers and the info necessary to manage all memory buffers.
 * One of these exists for each open stream.
 */
typedef struct DSStreamCB
{
  /**************************************/
  /* Thread overhead and communications */
  /**************************************/

  Item	 creatorTask;		/* who to signal when we're done initializing */
  uint32 creatorSignal;		/* signal to send for synchronous completion */
  int32	 creatorStatus;		/* result code for creator */

  Item	threadItem;		/* The thread Item for the server process */
  void*	threadStackBlock;	/* pointer to thread's stack memory block */

  /**********************/
  /* Creation arguments */
  /**********************/

  Item	       acquirePort;	/* acquisition module message port */
  long	       bufDataSize;	/* size of data buffers in BYTES */
  DSDataBufPtr freeBufHead;	/* pointer to list of free data buffers */
  long	       numSubsMsgs;	/* number of subscriber messages to allocate */

  /*****************************/
  /* Dynamically created stuff */
  /*****************************/

  unsigned long	streamFlags;	/* stream state flags */
  uint32	clockOffset;	/* offset to get relative stream clock */
  uint32	lastValidClock;	/* set when stream stopped */
  uint32	branchDest;	/* marker we're branching to if STRM_GO_INPROGRESS true */

  Item	 requestPort;		/* work request message port */
  uint32 requestPortSignal;	/* signal for request port */

  Item	     acqReplyPort;	/* reply port for data acquisition communications */
  uint32     acqReplyPortSignal; /* signal for data acquisition reply port */
  MemPoolPtr dataMsgPool;	/* pool of data message blocks */

  Item	     subsReplyPort;	/* reply port for subscriber communication */
  uint32     subsReplyPortSignal; /* signal for subscriber reply port */
  MemPoolPtr subsMsgPool;	/* pool of subscriber message blocks */

  DSDataBufPtr filledBufHead;	/* pointer to list of filled data buffers */
  DSDataBufPtr filledBufTail;	/* pointer to end of list of filled data buffers */

  long		  totalBufferCount;			/* total number of buffers this stream owns */
  long		  currentFreeBufferCount;		/* number of buffers currently available for filling */
  DSRequestMsgPtr endOfStreamMsg;				/* reply to this request msg at end of stream */

  int32		  repliesPending;				/* # of replies needed before replying to request */
  DSRequestMsgPtr requestMsgHead;				/* pointer to first request msg in queue */
  DSRequestMsgPtr requestMsgTail;				/* pointer to last request msg in queue */

  boolean	   fDiscardRcvdData;			/* => discard buffers received from Data Acq
                                                         *    until kAcqOpGoMarker msg reply */
#if HALT_ENABLE									/* When in a halted state for syncronization */
  Item		   haltChunkReplyPort;			/* reply port for synchronizing the completion of
                                                         * of halt mode. */

  SubscriberMsgPtr halted_msg;	/* The HALT Message sent */
#endif /* HALT_ENABLE */

  /***********************************/
  /* array of subscriber descriptors */
  /***********************************/

  long	       numSubscribers;	/* number of subscribers in the following table */
  DSSubscriber subscriber[DS_MAX_SUBSCRIBERS];

} DSStreamCB, *DSStreamCBPtr;

/* bits in streamFlags of DSStreamCB */

#define	STRM_RUNNING	     (1<<0) /* '1 if start'ed (running) */
#define	STRM_EOF	     (1<<1) /* '1 if end of file from data supplier */
#define	STRM_CLOCK_VALID     (1<<2) /* '1 if stream clock has been set */
#define	STRM_GO_INPROGRESS   (1<<3) /* '1 if a branching operation is in progress */
#define	STRM_CLOCK_WAS_VALID (1<<4) /* '1 if stream clock was ever set */
#define STRM_GO_ABSOLUTE     (1<<5) /* '1 if branch is GOMARKER_ABSOLUTE */

#ifdef RELATIVE_BRANCHING

#define STRM_KEYCHUNK_SEARCH (1<<6) /* '1 if streamer is searching for new key chunk */
#endif

#if HALT_ENABLE

typedef struct StreamControlChunk
{
  SUBS_CHUNK_COMMON;		/* the stream control chunk	*/
  StreamChunk subscriberChunk;	/* embeded chunk for subscriber   */
} StreamControlChunk;

#endif /* HALT_ENABLE */

#define	STRM_STOPPED (~(STRM_RUNNING | STRM_CLOCK_VALID | STRM_GO_INPROGRESS))


/*******************************************************************************************
 *		High level procedural interface
 *******************************************************************************************/

EXTERN_C_BEGIN

int32 InitDataStreaming(long maxNumberOfStreams);
int32 CloseDataStreaming(void);

int32 NewDataStream(DSStreamCBPtr *pCtx,
                    void*          bufferListPtr,
                    long           bufferSize,
                    long           deltaPriority,
                    long           numSubsMsgs);
int32 DisposeDataStream(Item          msgItem,
                        DSStreamCBPtr streamCBPtr);

EXTERN_C_END
