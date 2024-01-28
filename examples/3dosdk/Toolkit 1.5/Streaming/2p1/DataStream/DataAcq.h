/*******************************************************************************************
 *	File:			DataAcq.h
 *
 *	Contains:		definitions for DataAcq.c
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	11/23/93	jb		Added CloseDataAcq() to allow deallocation
 *of context block pool for cleanup processing. 11/8/93		jb
 *Added TIME_BASED_BRANCHING
 *	8/19/93		jb		Took FAKE_ABORT_IO out. Kept the
 *internal abortQueue structure in the context block, and added two new compile
 *						switches to control whether I/O is
 *actually cancelled and whether or not the thread must wait for I/O
 *completions before exiting. See the code aroung REALLY_ABORT_IO_ON_FLUSH and
 *						WAIT_FOR_IO_COMPLETION_BEFORE_EXIT.
 *	5/20/93		jb		Added queuing mechanism to permit a
 *fixed number of IOReq items to be used no matter how many data buffers may
 *exist. See changes demarked by ALLOW_REQUEST_OVERLOADING conditional. 5/20/93
 *jb		Get rid of 'bufferList' as an input parameter. Data acquisition
 *						threads now internally own and track
 *their own resources instead of storing any part of them (permanently) in a
 *data request message. 5/19/93		jb		Add FAKE_ABORT_IO hack.
 *	5/10/93		jb		Add 'fEOFWasSent' flag to AcqContext
 *record. This gets set the first time we detect a request to read past EOF. We
 *won't send another EOF until we see a "go marker" request. Instead, we reply
 *with "was flushed" for further attempts to read past the EOF. 5/9/93
 *jb		Add 'deltaPriority' param to NewDataAcq() to allow its caller
 *						to specify a delta priority for creating
 *the data acquisition thread. The value is added to the calling task's
 *priority, so a signed value can raise or lower the thread's relative
 *priority.
 *	5/9/93		jb		Removed 'ResetDataAcq()' routine. All
 *buffer flushing is now controlled only within the data acquisition thread
 *when positioning takes place. 4/19/93		jb		Add fields to
 *context to support synchronous communications with our creator at the time of
 *creation (so that a proper completion status can be given to the creator).
 *	4/17/93		jb		Substantial overhaul to accomodate
 *New()/Dispose() model of creating and destroying data acquisition contexts.
 *	4/5/93		jb		New today.
 *
 *******************************************************************************************/
#ifndef __DATAACQ_H__
#define __DATAACQ_H__

#ifndef __BLOCKFILE_H__
#include "BlockFile.h"
#endif

#ifndef __DATASTREAM_H__
#include "DataStream.h"
#endif

#include "ItemPool.h"

/* The following switch is used to enable the receipt and processing of
 * a time-to-position table embedded in stream data.
 */
#define TIME_BASED_BRANCHING 1

/* Setting the following switch will accept/queue data requests when all
 * IOReqItems are in use. As operations complete, pending operations are
 * started by removing them from this queue. If no IOReqItem is available
 * when a data request arrives, we just queue it instead of replying with
 * an error status.
 */
#define ALLOW_REQUEST_OVERLOADING 1

/* The following switch determines what is done when I/O operations are
 * flushed. If the switch is non-zero, in progress operations are actually
 * cancelled with AbortIO() and then placed into an "aborted operations"
 * queue. When the operation completes, it is returned with the result
 * code set to 'kDSWasFlushedErr'. Setting the flag to zero allows the
 * operation to actually complete normally (moves data into memory).
 */
#define REALLY_ABORT_IO_ON_FLUSH 1

/* The following switch causes the thread to not exit until all I/O operations
 * have been returned (completed or aborted).
 */
#define WAIT_FOR_IO_COMPLETION_BEFORE_EXIT 1

#define ACQ_MAX_STREAMS 4 /* max # of streams supported */
#define NUM_IOREQ_ITEMS                                                       \
  8 /* number of ioReq items allocated for each instantiation */

#if TIME_BASED_BRANCHING
#include "MarkerChunk.h"
#endif

/********************************************************/
/* Acquisition context, one per open acquisition stream */
/********************************************************/
typedef struct AcqContext
{
  Item creatorTask;     /* who to signal when we're done initializing */
  uint32 creatorSignal; /* signal to send for synchronous completion */
  int32 creatorStatus;  /* result code for creator */
  char *fileName;       /* pointer to file name string at init time */

  ItemPoolPtr
      ioReqItemPoolPtr; /* a pool of ioReqItems for the DataBufs to use */

  Item threadItem;        /* The thread Item for the server process */
  void *threadStackBlock; /* pointer to thread's stack memory block */

  Item requestPort;         /* message port for data acquisition requests */
  uint32 requestPortSignal; /* signal associated with requestPort */

  Item ioDoneReplyPort;         /* message port for I/O completion messages */
  uint32 ioDoneReplyPortSignal; /* signal associated with ioDoneReplyPort */

  DataAcqMsgPtr requestQueue; /* list of outstanding I/O request messages */

  DataAcqMsgPtr abortQueue; /* list of outstanding I/O request messages ... */
                            /* ... that are considered 'aborted' */

#if ALLOW_REQUEST_OVERLOADING
  DataAcqMsgPtr dataQueueHead; /* head of requests waiting for ioreqitems */
  DataAcqMsgPtr dataQueueTail; /* tail of requests waiting for ioreqitems */
#endif

  Boolean fEOFWasSent; /* true if we sent an EOF to the parser */
  long offset;         /* file position offset */
  BlockFile blockFile; /* block file associated with this context */

#if TIME_BASED_BRANCHING
  Item dsReqReplyPort;         /* reply port for requests to streamer */
  uint32 dsReqReplyPortSignal; /* signal for replies to streamer requests */

  uint32 subscriberPortSignal; /* signal for receipt of subscriber messages */
  Item subscriberPort;         /* message port for our data type */

  DSStreamCBPtr
      streamCBPtr; /* stream control block of stream we are connected to */

  MarkerChunkPtr
      markerChunk; /* pointer to copy of most recent tranlation table */
#endif

} AcqContext, *AcqContextPtr;

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus
extern "C"
{
#endif

  int32 InitDataAcq (int32 dataAcqCount);
  int32 CloseDataAcq (void);

  int32 NewDataAcq (AcqContextPtr *pCtx, char *fileName, long deltaPriority);
  void DisposeDataAcq (AcqContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif /* __DATAACQ_H__ */
