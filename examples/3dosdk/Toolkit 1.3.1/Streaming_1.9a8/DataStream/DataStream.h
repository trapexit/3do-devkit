/*******************************************************************************************
 *	File:			DataStream.h
 *
 *	Contains:		definitions for DataStream.c
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	12/2/93		jb		Version 1.7
 *						Added argument to NewDataStream() to specify the number of
 *						subscriber messages that should be allocated upon stream creation.
 *						Removed the definition of DS_MAX_SUBS_MESSAGES, which can now be
 *						specified dynamically because of the aforementioned modification.
 *						Add numSubsMsgs field to stream control block.
 *	11/23/93	jb		Version 1.6
 *						Added CloseDataStreaming() routine.
 *	11/9/93		jb		Version 1.5
 *						Added 'connect' union member to DataAcqMsg message to data acquirers.
 *						The connect message now carries a pointer to the stream control block
 *						with it so that data acquirers can communicate with the streamer.
 *						Expanded definition of DataAcqMarkerOptions enum for branching options.
 *	10/4/93		jb		Version 1.4
 *						Added End Of Stream notification.
 *	8/16/93		jb		Version 1.3.2
 *						Added kDSInvalidDSRequest
 *	8/5/93		jb		Version 1.3.1
 *						Get rid of semaphore since streamer messages can't be used by
 *						other threads anyway.
 *						Remove public access to _ForEachSubscriber() routine.
 *	7/9/93		jb		Version 1.3
 *						Convert all subscriber messaging to asynchronous operation. Keep
 *						the streamer single threaded by adding a request message queue
 *						that incoming streamer requests are queued upon. The message at the
 *						head of this queue is the "current" request that is being processed.
 *						It gets replied to when all subscribers/data acquisition threads
 *						have operated on any subsequent message sends that are used to 
 *						implement the request to the streamer.
 *						Added requestMsgHead and requestMsgTail to stream control block.
 *						Make utility routine _ForEachSubscriber() asynchronous, and hence,
 *						remove the "exempt" subscriber type parameter since it is not
 *						needed for deadlock prevention.
 *						Add 'subsMsgPoolSemaphore' to stream control block to allow
 *						library code to use the subscriber message pool to allocate
 *						messages to send to subscribers. For now, this is only used by
 *						DSClockSync(), which needs to send susbcribers the clock message
 *						immediately, and not through the normal streamer request mechanism.
 *						Increase DS_MAX_SUBS_MESSAGES to account for increased message
 *						utilization due to asynchronous operation.
 *	6/28/93		jb		Version 1.2.8
 *						Added lastValidClock field and STRM_CLOCK_WAS_VALID bit so that
 *						stream clock can be automatically restarted after a stop.
 *	6/25/93		jb		Version 1.2.7
 *						Removed kDSOpClockSync as DSClockSync() now handled as an
 *						immediate procedure call.
 *	6/24/93		jb		Version 1.2.6
 *						Add options field for kAcqOpGoMarker message to allow for
 *						relative branching in a stream.
 *	6/22/93		jb		Version 1.2.5
 *						Added options to start and stop subscriber messages to allow
 *						data flushing options to be communicated when a stream is
 *						started or stopped.
 *						Added 'exemptStreamType' to clocksync message to prevent deadlock
 *						when a subscriber calls DSClockSync(). Should we make this 
 *						asynchronous?
 *	6/18/93		jb		Added 'kStreamOpStart' subscriber message to, eventually, allow
 *						start/stop control that doesn't flush subscriber data.
 *	6/15/93		jb		Added STRM_CLOCK_VALID flag in streamFlags. This is set whenever
 *						a call to DSSetClock() is made, and cleared whever a successful
 *						DSGoMarker() operation is in progress.
 *						Added kDSClockNotValidErr error code.
 *						Added branchDest field and STRM_GO_INPROGRESS flag in streamFlags.
 *						Added STRM_STOPPED convenience constant for turning off all status
 *						bits that must be turned off when a stream is stopped.
 *	6/8/93		jb		Version 1.2.4
 *						Add 'clockOffset' field to stream control block.
 *	5/30/93		jb		Version 1.2.3
 *	5/30/93		jb		Added 'kAcqOpExit' data acquisition message opcode to tell
 *						data acquisition to shut down completely (as opposed to disconnect).
 *						This message is never sent by the streamer, but is sent by a call
 *						to DisposeDataAcq(). It defined here for standardization purposes
 *						only. Upon DSHCloseStream(), only a disconnect message is sent
 *						by the streamer.
 *	5/24/93		jb		Added 'permanentNext' to DSDataBuf struct to allow applications
 *						to have a permanent list of data buffers for debugging or other
 *						purposes. This field is not used by the streamer code.
 *	5/20/93		jb		Add 'kDSBadChunkSizeErr', which is returned during streaming when
 *						a chunk size is a non-quadbyte multiple.
 *	5/20/93		jb		Replace opening/closing data acquisition opcodes in 'DataAcqOpcode'
 *						with kAcqOpConnect & kAcqOpDisconnect. Remove the message extension
 *						fields from open/close since buffer list is no longer used to store
 *						any permanent data acquisition state (like IOReqItems, for example).
 *	5/20/93		jb		Remove 'acquirePort' from NewDataStream() interface; this is now
 *						handled by DSConnect().
 *	5/12/93		jb		Added DisposeDataStream()
 *	5/12/93		jb		Removed 'kAcqOpFlush'. Couldn't find a real use for it.
 *	5/11/93		jb		Major restructuring of stream control block, now patterned after
 *						the way subscribers and data acquisition because stream is now
 *						a free standing thread. Added 'DSRequestMsg' to pass
 *						work request messages to streams.
 *	5/11/93		jb		Get rid of 'subscriberContext' in subscriber messages since these 
 *						are now implied by the subscriber thread's context.
 *	5/10/93		jb		Removed 'DSHTime' data type
 *	5/10/93		jb		Remove DSHGetStreamTime(), remove 'streamTime' from stream 
 *						control block.
 *	5/9/93		jb		Added DSGoMarker()
 *	4/20/93		jb		Change type of 'ioreqItemPtr'to "IOReq*" from "void *"
 *	4/19/93		jb		Remove 'acquireContext' since it is implied by the acquisition 
 *						message port now.
 *	4/19/93		jb		Change dataPort to acqReplyPort, subsPort to subsReplyPort
 *	4/19/93		jb		Added signal related error codes.
 *	4/16/93		jb		Added 'sync' union for kStreamOpSync subscriber message data
 *	4/16/93		jb		Remove 'currTime' from message header.
 *	4/16/93		jb		Switch to using "Item.h" from "Items.h" in 3B1
 *	3/30/93		jb		Complete redesign of subscriber and data interfaces using messages
 *						instead of call/callback to allow decoupling of processing via
 *						threads.
 *	3/29/93		jb		Added DSHGetStreamTime()
 *	3/28/93		jb		Switch to using message port for iodone notifications instead
 *						of a straight callback mechanism.
 *	3/8/93		jb		New today.
 *
 *******************************************************************************************/
#ifndef __DATASTREAM_H__
#define __DATASTREAM_H__

#ifndef _TYPES_H
#include "Types.h"
#endif

#ifndef _ITEMS_H
#include "Item.h"
#endif

#ifndef _IO_H
#include "io.h"
#endif

#ifndef	__MSGHELPER_H__
#include "MsgUtils.h"
#endif

#ifndef	__MEMPOOL_H__
#include "MemPool.h"
#endif

/* The following constant is the maximum number of subscribers allowed for
 * a stream. The number should be kept small to minimize the search needed
 * on a per chunk basis.
 */
#define	DS_MAX_SUBSCRIBERS		16

/***************/
/* Basic types */
/***************/
typedef unsigned long DSDataType;
typedef unsigned long *DSChunkPtr;

/************************************************************************/
/* Error codes returned by DSH, subscribers, and data acquisition procs */
/************************************************************************/
typedef enum DSErrorCode {
	kDSNoErr				= 0,		/* no error, successful completion */
	kDSAbortErr				= -1000,	/* some error occurred */
	kDSEndOfFileErr			= -999,		/* end of file reached */
	kDSWasFlushedErr		= -998,		/* buffer was flushed */
	kDSNotRunningErr		= -997,		/* stream not running */
	kDSWasRunningErr		= -996,		/* stream already running */
	kDSNoPortErr			= -995,		/* couldn't allocate a message port for stream */
	kDSNoMsgErr				= -994,		/* couldn't allocate message item for a data buffer */
	kDSNotOpenErr			= -993,		/* stream not open */
	kDSNoMemErr				= -992,		/* couldn't allocate needed memory */
	kDSNoSignalErr			= -991,		/* couldn't allocate needed signal */
	kDSSignalErr			= -990,		/* problem sending/receiving a signal */
	kDSNoReplyPortErr		= -989,		/* message requires a reply port */
	kDSBadConnectPortErr	= -988,		/* invalid port specified for data connection */
	kDSSubDuplicateErr		= -987,		/* duplicate subscriber */
	kDSSubMaxErr			= -986,		/* subscriber table full, too many subscribers */
	kDSSubNotFoundErr		= -985,		/* specified subscriber not found */
	kDSInvalidTypeErr		= -984,		/* invalid subscriber data type specified */
	kDSBadBufAlignErr		= -983,		/* buffer list passed to DSHOpenStream contains */
										/* ... a non QUADBYTE aligned buffer */
	kDSBadChunkSizeErr		= -982,		/* chunk size in stream is a non-QUADBYTE multiple */
	kDSInitErr				= -981,		/* some internal initialization failed */
	kDSClockNotValidErr		= -980,		/* clock dependent call failed because clock not set */
	kDSInvalidDSRequest		= -979,		/* unknown request message send to server thread */
	kDSEOSRegistrationErr	= -978,		/* EOS registrant replaced by new registrant */
	kDSRangeErr				= -977,		/* parameter out of range */
	kDSUnImplemented		= -976,		/* requested function not implemented */
	kDSBranchNotDefined		= -975		/* branch destination not defined */
	};

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

struct DSDataBuf {
	struct DSDataBuf*	permanentNext;	/* ptr for keeping permanent global buffer list ... */
										/* ... for debugging, etc. (not used by streamer) */
	struct DSDataBuf*	next;			/* ptr for linking buffers into lists (free, in use, etc.) */
	long				useCount;		/* count of subscribers using this buffer */
	struct DSStreamCB*	streamCBPtr;	/* ptr to the context block of the owning stream */

	/* NOTE: The following two fields are filled in by	*/
	/* data acquisition at "opening" time. 				*/

	Item				ioreqItem;		/* I/O request item for queuing reads for this buffer */
	IOReq*				ioreqItemPtr;	/* pointer to the Item in system space */

	char				streamData[4];	/* start of (variable length) stream data */
	};
typedef struct DSDataBuf DSDataBuf, *DSDataBufPtr;


/* The following is a logical data chunk imbedded in the stream of blocks. The 
 * 'streamDataType' is used to search the stream's subscriber list, and any chunks that
 * match existing subscribers are passed to the subscriber. Chunks that do not
 * match any existing subscriber are ignored.
 * NOTE: ALL CHUNKS MUST BE QUADBYTE ALIGNED!!!
 */
struct StreamChunk {
	unsigned long		streamChunkType;	/* 4 byte ASCII subscriber data type */
	unsigned long		streamChunkSize;	/* size of chunk including this header */
	unsigned char		streamChunkData[4];	/* start of variable length data */
	};
typedef struct StreamChunk StreamChunk, *StreamChunkPtr;

/* The following preamble is used for all types of messages sent
 * by DSH. This enables a couple of utility routines to perform general 
 * operations on lists.
 */
#define	DS_MSG_HEADER	\
		long		whatToDo;	/* opcode determining msg contents */				\
		Item		msgItem;	/* message item for sending this buffer */			\
		void*		privatePtr;	/* ptr to sender's private data */					\
		void*		link;		/* user defined -- for linking msg into lists */

typedef struct GenericMsg {
		DS_MSG_HEADER
		} GenericMsg, *GenericMsgPtr;

/*******************************************************************************************
 *		Subscriber Interface (DSH to subscriber)
 *******************************************************************************************/

/*****************************************/
/* Opcode values as passed in 'whatToDo' */
/*****************************************/
typedef enum StreamOpcode {
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
typedef struct SubscriberMsg {
		DS_MSG_HEADER

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

#define	CHAN_ENABLED		(1<<0)				/* R/W: '1 if channel enabled (allows data to flow) */
#define	CHAN_ACTIVE			(1<<1)				/* R/O: '1 if channel data currently flowing */
#define	CHAN_EOF			(1<<2)				/* R/O: '1 if channel finished */
#define	CHAN_ABORTED		(1<<3)				/* R/O: '1 if channel aborted (error) */
#define	CHAN_SYSBITS		(0x0000FFFE)		/* Mask of reserved bits, rest subscriber defined */
												/* NOTE: least significant bit is R/W !!! */

/* bits in options of start and stop messages above */

#define	SOPT_NOFLUSH		(0)					/* for readability */
#define	SOPT_FLUSH			(1<<0)				/* '1 if flush on start or stop request */


/*******************************************************************************************
 * 		Data Acquisition Interface (DSH to application)
 *******************************************************************************************/

/*****************************************/
/* Opcode values as passed in 'whatToDo' */
/*****************************************/
typedef enum DataAcqOpcode {
			kAcqOpGetData = 0,		/* fill buffer with data */
			kAcqOpGoMarker,			/* seek to a marker in the stream */
			kAcqOpConnect,			/* stream connecting, perform initializations */
			kAcqOpDisconnect,		/* stream disconnecting, no more data requests follow */
			kAcqOpExit				/* release resources and exit */
			};

/*******************************************/
/* Messages sent by DS to data acquisition */
/*******************************************/

typedef struct DataAcqMsg {
		DS_MSG_HEADER

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
enum DataAcqMarkerOptions {
	GOMARKER_ABSOLUTE		=	0,		/* value == absolute marker FILE POSITION */
	GOMARKER_FORWARD		=	1,		/* value == count of markers to skip forward */
	GOMARKER_BACKWARD		=	2,		/* value == count of markers to skip backward */
	GOMARKER_ABS_TIME		=	3,		/* value == absolute stream time of destination */
	GOMARKER_FORW_TIME		=	4,		/* value == stream time by which to advance */
	GOMARKER_BACK_TIME		=	5,		/* value == stream time by which to regress */
	GOMARKER_NAMED			=	6		/* value == (char*) ptr to name of destination marker */
	};

/*******************************************************************************************
 *		Stream Control Data Structures
 *******************************************************************************************/

/*****************************************/
/* Opcode values as passed in 'whatToDo' */
/*****************************************/
typedef enum DSRequestOpcode {
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
			kDSOpConnect					/* connect data client to stream */
			};

/***********************/
/* Messages sent to DS */
/***********************/

typedef struct DSRequestMsg {
		DS_MSG_HEADER

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
typedef struct DSSubscriber {
	DSDataType		dataType;				/* type of data owned by subscriber */
	int32			subscriberPort;			/* pointer to data subscriber proc */
	} DSSubscriber, *DSSubscriberPtr;


/* This data structure contains all the context necessary to drive a stream, including
 * a description of all subscribers and the info necessary to manage all memory buffers.
 * One of these exists for each open stream.
 */
typedef struct DSStreamCB {

	/**************************************/
	/* Thread overhead and communications */
	/**************************************/

	Item			creatorTask;			/* who to signal when we're done initializing */
	uint32			creatorSignal;			/* signal to send for synchronous completion */
	int32			creatorStatus;			/* result code for creator */

	Item			threadItem;				/* The thread Item for the server process */
	void*			threadStackBlock;		/* pointer to thread's stack memory block */

	/**********************/
	/* Creation arguments */
	/**********************/

	Item			acquirePort;			/* acquisition module message port */
	long			bufDataSize;			/* size of data buffers in BYTES */
	DSDataBufPtr	freeBufHead;			/* pointer to list of free data buffers */
	long			numSubsMsgs;			/* number of subscriber messages to allocate */

	/*****************************/
	/* Dynamically created stuff */
	/*****************************/

	unsigned long	streamFlags;			/* stream state flags */
	uint32			clockOffset;			/* offset to get relative stream clock */
	uint32			lastValidClock;			/* set when stream stopped */
	uint32			branchDest;				/* marker we're branching to if STRM_GO_INPROGRESS true */

	Item			requestPort;			/* work request message port */
	uint32			requestPortSignal;		/* signal for request port */

	Item			acqReplyPort;			/* reply port for data acquisition communications */
	uint32			acqReplyPortSignal;		/* signal for data acquisition reply port */
	MemPoolPtr		dataMsgPool;			/* pool of data message blocks */

	Item			subsReplyPort;			/* reply port for subscriber communication */
	uint32			subsReplyPortSignal;	/* signal for subscriber reply port */
	MemPoolPtr		subsMsgPool;			/* pool of subscriber message blocks */

	DSDataBufPtr	filledBufHead;			/* pointer to list of filled data buffers */
	DSDataBufPtr	filledBufTail;			/* pointer to end of list of filled data buffers */

	long			totalBufferCount;		/* total number of buffers this stream owns */
	long			currentFreeBufferCount;	/* number of buffers currently available for filling */
	DSRequestMsgPtr	endOfStreamMsg;			/* reply to this request msg at end of stream */

	int32			repliesPending;			/* # of replies needed before replying to request */
	DSRequestMsgPtr	requestMsgHead;			/* pointer to first request msg in queue */
	DSRequestMsgPtr	requestMsgTail;			/* pointer to last request msg in queue */

	/***********************************/
	/* array of subscriber descriptors */
	/***********************************/

	long			numSubscribers;			/* number of subscribers in the following table */
	DSSubscriber	subscriber[DS_MAX_SUBSCRIBERS];

	} DSStreamCB, *DSStreamCBPtr;

/* bits in streamFlags of DSStreamCB */

#define	STRM_RUNNING			(1<<0)		/* '1 if start'ed (running) */
#define	STRM_EOF				(1<<1)		/* '1 if end of file from data supplier */
#define	STRM_CLOCK_VALID		(1<<2)		/* '1 if stream clock has been set */
#define	STRM_GO_INPROGRESS		(1<<3)		/* '1 if a branching operation is in progress */
#define	STRM_CLOCK_WAS_VALID	(1<<4)		/* '1 if stream clock was ever set */

#define	STRM_STOPPED		(~(STRM_RUNNING | STRM_CLOCK_VALID | STRM_GO_INPROGRESS))


/*******************************************************************************************
 *		High level procedural interface
 *******************************************************************************************/

#ifdef __cplusplus 
extern "C" {
#endif

int32	InitDataStreaming( long maxNumberOfStreams );
int32	CloseDataStreaming( void );

int32	NewDataStream( DSStreamCBPtr *pCtx, void* bufferListPtr, long bufferSize, 
							long deltaPriority, long numSubsMsgs );
int32	DisposeDataStream( Item msgItem, DSStreamCBPtr streamCBPtr );

#ifdef __cplusplus
}
#endif

#endif	/* __DATASTREAM_H__ */
