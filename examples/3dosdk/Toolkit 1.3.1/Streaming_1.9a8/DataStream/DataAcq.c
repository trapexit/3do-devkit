/*******************************************************************************************
 *	File:			DataAcq.c
 *
 *	Contains:		DataStream data acquisition module for disk file
 *data
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright й 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/14/93	jb		Version 1.9.1
 *						Add a call to DSSetBranchDest() to tell
 *the streamer the translated stream time for the branch destination we
 *calculate. This enables the CTRL/SYNC mechanism to detect branch destinations
 *						correctly.
 *	11/23/93	jb		Version 1.9
 *						Added CloseDataAcq() routine to clean up
 *things allocated by the InitDataAcq() routine. 11/22/93	jb
 *Version 1.8 Add circular buffer trace of branch destinations. See code
 *						conditionalized by
 *TRACE_TIME_BASED_BRANCHING. 10/19/93	jb		Version 1.7 Switch to
 *using UMemory.c routines for memory allocation 10/5/93		jb
 *Version 1.6 Moved DeleteItemPool() to thread exit point to fix memory leak.
 *	9/13/93		rdg		Version 1.5
 *						Fixed memory leak.  _InitalizeThread()
 *was zeroing the ctx's stackBlock field so DisposeDataAcq() was never able to
 *free the stackBlock. 8/19/93		jb		Version 1.4 Took
 *FAKE_ABORT_IO out. Kept the internal abortQueue structure in the context
 *block, and added two new compile switches to control whether I/O is actually
 *cancelled and whether or not the thread must wait for I/O completions before
 *exiting. See the code aroung REALLY_ABORT_IO_ON_FLUSH and
 *						WAIT_FOR_IO_COMPLETION_BEFORE_EXIT.
 *	8/9/93		jb		Version 1.3
 *						Initialize ctx->abortQueue now that we
 *append request to onto this queue instead of just moving the request queue
 *onto it (and perhaps blowing a previous queue of aborting requests). 8/3/93
 *jb		Version 1.2.4 Fix bug in _FlushPendingIO() when called while
 *the abort queue was non-empty. Now, appends request queue onto the abort
 *queue instead of always replacing the abort queue with the request queue.
 *	6/1/93		jb		Version 1.2.3
 *						Remove all variable initializers in local
 *variable declarations to avoid register bashing by the ARM C compiler.
 *	5/31/93		jb		Version 0.3
 *						More kitchen sink code added to
 *FAKE_ABORT_IO hack. Now we prevent the thread exiting if any I/O's are
 *pending. We wait around until they complete so that the silly system doesn't
 *crash itself trying to abort the pending requests when we are requested to
 *exit via receipt of a 'kAcqOpExit' message.
 *	5/30/93		jb		Substantial change to DisposeDataAcq().
 *Now it sends an "exit" message to the thread to get it to clean up anything
 *it created. Add handling of 'kAcqOpExit' message to the thread. Set
 *'threadItem' to zero so that dispose won't try to delete the thread if the
 *"exit" message has already been sent. 5/20/93		jb
 *Added queuing mechanism to permit a fixed number of IOReq items to be used no
 *matter how many data buffers may exist. See changes demarked by
 *ALLOW_REQUEST_OVERLOADING conditional. 5/20/93		jb
 *Change to using connect/disconnect model of data acquisition. Requires
 *multiple instantiations of data acquisition thread, one per open file, but
 *prevents opening files "on the fly".
 *	5/19/93		jb		Add FAKE_ABORT_IO hack.
 *	5/16/93		ec		Add call to CloseBlockFile() to
 *DoClosing(), was leaving file open.
 *	5/15/93		ec		Split code to open block file and
 *initialize IORequest items out of _InitializeThread(), placed in new function
 *							OpenStreamFile().  Add
 *'kAcqOpChangeFile' case to DataAcqThread() Wrote new function
 *DoChangeStreamFile(). 5/12/93		jb		Change DoClosing() to
 *flush pending I/O, and get rid of args to DoOpening() since we don't really
 *use them right now.
 *	5/10/93		jb		Add 'fEOFWasSent' flag to AcqContext
 *record. This gets set the first time we detect a request to read past EOF. We
 *won't send another EOF until we see a "go marker" request. Instead, we reply
 *with "was flushed" for further attempts to read past the EOF. 5/9/93
 *jb		Added _MapMarkerToOffset() to break out the mapping of markers
 *to file position offsets. This is the place where anyone modifying this code
 *should consider adding a higher level mapping function for mapping markers to
 *file positions. For now, markers *are* file positions. At a minimum, these
 *should be times instead.
 *	5/9/93		jb		Change 'ResetDataAcq()' to
 *_FlushPendingIO(), & make routine static instead of external. All I/O
 *flushing is now controlled from within the thread by DoGoMarker(). 5/9/93
 *jb		Add 'deltaPriority' param to NewDataAcq() to allow its caller
 *						to specify a delta priority for creating
 *the data acquisition thread. The value is added to the calling task's
 *priority, so a signed value can raise or lower the thread's relative
 *priority.
 *	4/25/93		njc		DoGoMarker now removes buffers from the
 *pending queue and calls AbortIO. Changed call to NewThread to create the
 *thread 2 levels above the main level. 4/20/93		jb
 *If DoGetData() returns an error, reply to the work request immediately (do
 *not wait for an I/O completion message that will never come). 4/20/93
 *jb		Be sure to send the data message pointer back to the requestor
 *						in our call to ReplyMsg() when completing
 *an async I/O. 4/20/93		jb		Fixed linked list deletion in
 *_FindRequest()
 *	4/20/93		jb		Use ioreqItemPtr from the data message
 *instead of looking up the item on every I/O completion.
 *	4/19/93		jb		Context is no longer passed to us in
 *any messages, rather, it is implied by the message port that gets the
 *messages.
 *	4/19/93		jb		Move all initialization code to the
 *thread. Invent funky signal based mechanism to coordinate the procedure call
 *to create a new subscriber with the thread, which must do all the
 *intialization so that resource ownership works correctly.
 *	4/17/93		jb		Completely rework using New() &
 *Dispose() model like TestSubscriber.c Move all "open" initializations to the
 *New() routine, create the thread and pass the context pointer to the thread.
 *Get rid of all global data except the context allocation pool. Export the
 *context block definitions to the header file. 4/16/93		jb
 *Change to WaitSignal() from Wait() in 3B1
 *	4/13/93		jb		Switch to using NewMsgPort()
 *	4/5/93		jb		Masssive code injection to do message &
 *thread based implementation. 3/25/93		jb		New today.
 *
 *******************************************************************************************/

#include "DataAcq.h"
#include "ItemPool.h"
#include "MemPool.h"
#include "MsgUtils.h"
#include "ThreadHelper.h"
#include "UMemory.h"
#include "types.h"

#if TIME_BASED_BRANCHING
#include "MarkerChunk.h"
#endif

/* The following compile switch enables storing a circular buffer of
 * branch destinations. This should normally be set to zero.
 */
#define TRACE_TIME_BASED_BRANCHING 0

#if TRACE_TIME_BASED_BRANCHING
#define DA_BRANCH_TRACE_MAX 16
long gDABranchTableIndex = 0;
long gDABranchTable[DA_BRANCH_TRACE_MAX];
#endif

/****************************/
/* Data acquisition globals */
/****************************/

struct DataAcqGlobals
{

  MemPoolPtr acqContextPool; /* The memory pool for acquisition contexts */

} DAGlobals;

/****************************/
/* Local routine prototypes */
/****************************/

/* Request message handlers */

static int32 DoConnect (AcqContextPtr ctx, DataAcqMsgPtr dataMsg);
static int32 DoDisconnect (AcqContextPtr ctx);
static int32 DoGoMarker (AcqContextPtr ctx, DataAcqMsgPtr dataMsg);
static int32 DoGetData (AcqContextPtr ctx, DataAcqMsgPtr dataMsg);

/* Utility routines for managing queued I/O */

static void _FlushPendingIO (AcqContextPtr ctx);
static DataAcqMsgPtr _FindRequest (AcqContextPtr ctx, DataAcqMsgPtr *queueHead,
                                   Item ioReqItem);

/* Thread related routines */

static int32 _InitializeThread (AcqContextPtr ctx);
static void DataAcqThread (long notUsed, AcqContextPtr ctx);

/* Routine for mapping markers to file positions */

static int32 _MapMarkerToOffset (AcqContextPtr ctx, unsigned long markerValue,
                                 unsigned long markerOptions, long *pOffset);

/* Routine for opening a new stream file */
static int32 OpenStreamFile (AcqContextPtr ctx);

#if ALLOW_REQUEST_OVERLOADING
/* Routines for handling queuing of pending data requests */
static bool AddDataAcqMsgToTail (AcqContextPtr ctx, DataAcqMsgPtr dataMsg);
static DataAcqMsgPtr GetNextDataAcqMsg (AcqContextPtr ctx);
#endif

#if ALLOW_REQUEST_OVERLOADING
/*******************************************************************************************
 * Utility routine to add a data message onto the tail of the queue of data
 *messages waiting for available IOReq Items.
 *******************************************************************************************/
static bool
AddDataAcqMsgToTail (AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
  bool fQueueWasEmpty;

  dataMsg->link = NULL;

  if (ctx->dataQueueHead != NULL)
    {
      /* Add the new message onto the end of the
       * existing list of queued messages.
       */
      ctx->dataQueueTail->link = (void *)dataMsg;
      ctx->dataQueueTail = dataMsg;
      fQueueWasEmpty = false;
    }
  else
    {
      /* Add the message to an empty queue  */
      ctx->dataQueueHead = dataMsg;
      ctx->dataQueueTail = dataMsg;
      fQueueWasEmpty = true;
    }

  return fQueueWasEmpty;
}

/*******************************************************************************************
 * Utility routine to remove a data message from the head of the queue of data
 *messages waiting for available IOReq Items.
 *******************************************************************************************/
static DataAcqMsgPtr
GetNextDataAcqMsg (AcqContextPtr ctx)
{
  DataAcqMsgPtr dataMsg;

  if ((dataMsg = ctx->dataQueueHead) != NULL)
    {
      /* Advance the head pointer to point to the next entry
       * in the list.
       */
      ctx->dataQueueHead = (DataAcqMsgPtr)dataMsg->link;

      /* If we are removing the tail entry from the list, set the
       * tail pointer to NULL.
       */
      if (ctx->dataQueueTail == dataMsg)
        ctx->dataQueueTail = NULL;
    }

  return dataMsg;
}

#endif /* ALLOW_REQUEST_OVERLOADING */

/*******************************************************************************************
 * Utility routine to locate a data buffer given an I/O request Item and a
 *queue in which to search for the Item. This is called to find the data
 *message that matches a completing I/O request.
 *******************************************************************************************/
static DataAcqMsgPtr
_FindRequest (AcqContextPtr ctx, DataAcqMsgPtr *queueHead, Item ioReqItem)
{
  DataAcqMsgPtr acqMsg;
  DataAcqMsgPtr lastMsg;

  lastMsg = NULL;

  /* Search the in-progress buffer queue for the specified request
   * item pointer. Return a pointer to the buffer that the I/O
   * belongs to.
   */
  for (acqMsg = *queueHead; acqMsg != NULL;
       acqMsg = (DataAcqMsgPtr)acqMsg->link)
    {
      if (acqMsg->msg.data.bufferPtr->ioreqItem == ioReqItem)
        {
          /* Delete the found entry from the in-progress list
           */
          if (lastMsg == NULL)
            *queueHead = (DataAcqMsgPtr)acqMsg->link;
          else
            lastMsg->link = acqMsg->link;

          return acqMsg;
        }

      /* Remember the last entry we saw so we can do singly linked
       * list deletion.
       */
      lastMsg = acqMsg;
    }

  return NULL;
}

/*******************************************************************************************
 * Flush pending I/O requests. This is called when a positioning operation is
 *taking place to cancel any queued requests since there is no point in
 *continuing to read data from a place in the stream that is no longer of use.
 *******************************************************************************************/
static void
_FlushPendingIO (AcqContextPtr ctx)
{
  DataAcqMsgPtr acqMsg;
  DataAcqMsgPtr lastAcqMsg;

#if REALLY_ABORT_IO_ON_FLUSH
  /* Crawl the in-progress buffer queue and abort all pending reads.
   */
  for (acqMsg = ctx->requestQueue; acqMsg != NULL;
       acqMsg = (DataAcqMsgPtr)acqMsg->link)
    {
      AbortIO (acqMsg->msg.data.bufferPtr->ioreqItem);
    }
#endif

  /* Find the end of the current abort queue */
  for (acqMsg = ctx->abortQueue, lastAcqMsg = NULL; acqMsg != NULL;
       acqMsg = (DataAcqMsgPtr)acqMsg->link)
    lastAcqMsg = acqMsg;

  /* If the abort queue is empty, just move the entire request queue
   * onto the front of the abort queue. If it is not empty, then
   * append the request queue onto the end of the abort queue.
   */
  if (lastAcqMsg == NULL)
    ctx->abortQueue = ctx->requestQueue;
  else
    lastAcqMsg->link = ctx->requestQueue;

  /* Always leave the request queue empty after a flush */
  ctx->requestQueue = NULL;
}

/*******************************************************************************************
 * Routine to initialize all data acquisition activity. Called once at program
 *init time to allow allocation of context pool memory.
 *******************************************************************************************/
int32
InitDataAcq (int32 dataAcqCount)
{
  /* Create the memory pool for allocating data stream
   * contexts.
   */
  DAGlobals.acqContextPool = CreateMemPool (dataAcqCount, sizeof (AcqContext));
  if (DAGlobals.acqContextPool == NULL)
    return kDSNoMemErr;

  /* Return success */
  return kDSNoErr;
}

/*******************************************************************************************
 * Routine to clean up things allocated by InitDataAcq()
 *******************************************************************************************/
int32
CloseDataAcq (void)
{

  DeleteMemPool (DAGlobals.acqContextPool);
  DAGlobals.acqContextPool = NULL;

  /* Return success */
  return kDSNoErr;
}

/*******************************************************************************************
 * Routine to instantiate a new data acquirer. Creates all necessary resoures
 *and starts the data acquisition thread running. Called to create a stream
 *interface for a data streaming file.
 *******************************************************************************************/
int32
NewDataAcq (AcqContextPtr *pCtx, char *fileName, long deltaPriority)
{
  int32 status;
  AcqContextPtr ctx;
  uint32 signalBits;

  /* Allocate a data acquisition context */

  ctx = (AcqContextPtr)AllocPoolMem (DAGlobals.acqContextPool);
  if (ctx == NULL)
    return kDSNoMemErr;

  /* Pass creation arguments to the thread via the context block
   */
  ctx->fileName = fileName;

  /* Allocate a signal to synchronize with the completion of the
   * subscriber's initialization. It will signal us with this when
   * it has finished, successfully or not, when it is done initializing
   * itself.
   */
  ctx->creatorTask = CURRENTTASK->t.n_Item; /* see "kernel.h" for this */

  ctx->creatorSignal = AllocSignal (0);
  if (ctx->creatorSignal == 0)
    {
      status = kDSNoSignalErr;
      goto CLEANUP;
    }

  /* Create the data acquisition execution thread, passing it a pointer
   * to the context block we just initialized for it.
   */
  status = NewThread (
      (void *)&DataAcqThread,     /* thread entrypoint */
      4096,                       /* initial stack size */
      (long)CURRENT_TASK_PRIORITY /* priority, adjusted by */
          + deltaPriority,        /* ...the caller's value */
      NULL,                       /* name */
      &ctx->threadStackBlock,     /* where to remember stack block addr */
      0,                          /* initial R0 */
      ctx);                       /* initial R1 */

  if (status <= 0)
    goto CLEANUP;
  else
    ctx->threadItem = status;

  /* Wait here while the thread initializes itself. When its done,
   * look at the status returned to us in the context block to determine
   * if it was happy.
   */
  signalBits = WaitSignal (ctx->creatorSignal);
  if (signalBits != ctx->creatorSignal)
    return kDSSignalErr;

  /* We're done with this signal, so give it back */
  FreeSignal (ctx->creatorSignal);

  /* Check the initialization status of the thread. If anything
   * failed, the 'ctx->creatorStatus' field will be set to a system result
   * code. If this is >= 0 then initialization was successful.
   */
  status = ctx->creatorStatus;
  if (status >= 0)
    {
      *pCtx = ctx;   /* give the caller a copy of the context pointer */
      return status; /* return 'success' */
    }

CLEANUP:
  /* Some error occurred during initialization. Release all resources
   * we allocated and return the error status that was last captured.
   */
  if (ctx->threadStackBlock)
    FreePtr (ctx->threadStackBlock);

  ReturnPoolMem (DAGlobals.acqContextPool, ctx);

  return status;
}

/*******************************************************************************************
 * Routine to get rid of all data acquisition resources we created. We send an
 *"exit" message to the data acquisition thread. It cleans up all the resources
 *it created. Mainly, this is memory since thread exit should delete all system
 *resources other than memory that the thread created. All that is left should
 *be the thread's stack memory which we delete here. Note that this should be
 *called after any "disconnect" actions since the streamer will get errors if
 *it trys to communicate with the data acquisition thread if a disconnect does
 *not preceed this call.
 *******************************************************************************************/
void
DisposeDataAcq (AcqContextPtr ctx)
{
  int32 status;
  Message *msgPtr;
  DataAcqMsg acqMsg;
  Item dataReplyPort;

  if (ctx)
    {
      /* Create a temporary message item and a reply port so that
       * we can communicate with the data acquisition thread.
       */
      dataReplyPort = NewMsgPort (NULL);
      if (dataReplyPort < 0)
        {
          kprintf ("DisposeDataAcq - NewMsgPort() failed\n");
          return;
        }

      acqMsg.msgItem = CreateMsgItem (dataReplyPort);
      if (acqMsg.msgItem < 0)
        {
          kprintf ("DisposeDataAcq - CreateMsgItem() failed\n");
          return;
        }

      /* Tell data acquisition thread to exit */
      acqMsg.whatToDo = kAcqOpExit;

      /* Send the "exit" request and await a reply from the data
       * acquisition thread.
       */
      status = SendMsg (ctx->requestPort, acqMsg.msgItem, &acqMsg,
                        sizeof (DataAcqMsg));
      if (status >= 0)
        {
          status = WaitForMsg (dataReplyPort, NULL, &msgPtr, NULL,
                               acqMsg.msgItem);
        }

      /* Regardless of the reply message status, try to get rid of the
       * thread. Note that 'threadItem' should be set to zero if the
       * thread really exited before we return from WaitForMsg() above...
       */
      if (ctx->threadItem > 0)
        DisposeThread (ctx->threadItem);
      if (ctx->threadStackBlock)
        FreePtr (ctx->threadStackBlock);

      ReturnPoolMem (DAGlobals.acqContextPool, ctx);

      /* Get rid of the temporary message item and its reply port */
      RemoveMsgItem (acqMsg.msgItem);
      RemoveMsgPort (dataReplyPort);

#if TIME_BASED_BRANCHING
      /* Free any memory associated with branch tranlation */
      if (ctx->markerChunk)
        FreePtr (ctx->markerChunk);
#endif
    }
}

/*******************************************************************************************
 * Routine to perform any initialization at the time when a connection to a
 *stream happens. Gets called as a result of a call to DSConnect(). Note that
 *the message buffer and message item used to post the asynchronous
 *subscription request to the streamer are disposed of when the streamer
 *responds to the request.
 *******************************************************************************************/
static int32
DoConnect (AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
#if TIME_BASED_BRANCHING
  int32 status;
  Item messageItem;
  DSRequestMsgPtr dsReqMsgPtr;

  /* If this data acquirer currently owns a marker table, then
   * dispose of it now. Note that this should not happen because
   * the connection protocol should guarantee a disconnect message
   * if we get disconnected.
   */
  if (ctx->markerChunk)
    {
      FreePtr (ctx->markerChunk);
      ctx->markerChunk = NULL;
    }

  /* Copy the stream control block pointer into our
   * context block so that we can communicate with the
   * appropriate stream.
   */
  ctx->streamCBPtr = dataMsg->msg.connect.streamCBPtr;

  /* Create a message buffer for doing our subscription asynchronously */
  dsReqMsgPtr = (DSRequestMsgPtr)NewPtr (sizeof (DSRequestMsg), MEMTYPE_ANY);
  if (dsReqMsgPtr == NULL)
    return kDSNoMemErr;

  /* Create a message item to send the subscription request */
  messageItem = CreateMsgItem (ctx->dsReqReplyPort);
  if (messageItem < 0)
    {
      FreePtr (dsReqMsgPtr);
      return kDSNoMsgErr;
    }

  /* Subscribe to the time to position table data type. This will cause
   * translation table data to be delivered to us when it appears in the
   * stream.
   */
  status
      = DSSubscribe (messageItem,                    /* msg item */
                     dsReqMsgPtr,                    /* asynchronous call */
                     ctx->streamCBPtr,               /* stream context block */
                     (DSDataType)DATAACQ_CHUNK_TYPE, /* subscriber data type */
                     ctx->subscriberPort); /* subscriber message port */

  return status;

#else
  return kDSNoErr;
#endif
}

/*******************************************************************************************
 * Routine to handle disconnection to a stream. Flush any pending I/O
 *operations.
 *******************************************************************************************/
static int32
DoDisconnect (AcqContextPtr ctx)
{
#if TIME_BASED_BRANCHING
  int32 status;
  Item messageItem;
  DSRequestMsgPtr dsReqMsgPtr;

  dsReqMsgPtr = NULL;
  messageItem = 0;

  _FlushPendingIO (ctx);

  /* Create a message buffer for doing our subscription asynchronously */
  dsReqMsgPtr = (DSRequestMsgPtr)NewPtr (sizeof (DSRequestMsg), MEMTYPE_ANY);
  if (dsReqMsgPtr == NULL)
    {
      status = kDSNoMemErr;
      goto BAILOUT;
    }

  /* Create a message item to send the subscription request */
  messageItem = CreateMsgItem (ctx->dsReqReplyPort);
  if (messageItem < 0)
    {
      status = kDSNoMsgErr;
      goto BAILOUT;
    }

  /* Subscribe to the time to position table data type. This will cause
   * translation table data to be delivered to us when it appears in the
   * stream.
   */
  status
      = DSSubscribe (messageItem,                    /* msg item */
                     dsReqMsgPtr,                    /* asynchronous call */
                     ctx->streamCBPtr,               /* stream context block */
                     (DSDataType)DATAACQ_CHUNK_TYPE, /* subscriber data type */
                     0); /* message port == ZERO --> unsubscribe */

  dsReqMsgPtr = NULL;
  messageItem = 0;

BAILOUT:
  /* If this data acquirer currently owns a marker table, then
   * dispose of it now. Note that this should not happen because
   * the connection protocol should guarantee a disconnect message
   * if we get disconnected.
   */
  if (ctx->markerChunk)
    {
      FreePtr (ctx->markerChunk);
      ctx->markerChunk = NULL;
    }

  /* Disassociate this data acquirer from its stream */
  ctx->streamCBPtr = NULL;

  /* In case we are exiting because of an error, clean up any
   * resources that may have been left dangling.
   */
  if (dsReqMsgPtr)
    FreePtr (dsReqMsgPtr);
  if (messageItem)
    RemoveMsgItem (messageItem);

  return status;

#else

  _FlushPendingIO (ctx);
  return kDSNoErr;
#endif
}

/*******************************************************************************************
 * Routine to validate a marker value, and convert the marker to a file
 *position that can be used in subsequent read operations. This routine should
 *map the marker to a place in the stream where all logical streams within the
 *physical data stream have been temporally synchronized by the stream layout
 *tool to insure that, when data begins to flow again, all data belonging to
 *time T and beyond arrive *after* the physical location returned by this
 *routine.
 *******************************************************************************************/
static int32
_MapMarkerToOffset (AcqContextPtr ctx, unsigned long markerValue,
                    unsigned long markerOptions, long *pOffset)
{
#if TIME_BASED_BRANCHING
  int32 status;
  uint32 streamClockTarget;
  MarkerRecPtr marker;
  MarkerRecPtr markerTable;
  MarkerRecPtr endOfMarkerTable;

  /* Absolute file position branching is always allowed, even if we
   * are not currently connected to a stream, even if the stream clock
   * is not valid.
   */
  if (markerOptions == GOMARKER_ABSOLUTE)
    {
      if (markerValue >= GetBlockFileSize (&ctx->blockFile))
        return kDSEndOfFileErr;

      (*pOffset) = markerValue;
      return kDSNoErr;
    }

  /* Range check the branching options value */
  if (markerOptions > GOMARKER_BACK_TIME)
    return kDSRangeErr;

  /* All other types of branching depend on being able to interpret
   * the marker table, and/or being able to determine current stream
   * time. These require the presence of a marker table and a valid
   * stream clock.
   */
  if ((ctx->streamCBPtr == NULL) || (ctx->markerChunk == NULL))
    return kDSBranchNotDefined;

  /* Get the current stream time if the branch type is based on stream
   * time (all non-absolute branches are based on current time).
   */
  if (markerOptions != GOMARKER_ABS_TIME)
    {
      status = DSGetClock (ctx->streamCBPtr, &streamClockTarget);
      if (status != kDSNoErr)
        return status;

      /* For time-relative branching, use the current stream time
       * and adjust the search target by the amount specified, either
       * forward or backward.
       */
      if (markerOptions == GOMARKER_FORW_TIME)
        streamClockTarget += markerValue;

      else if (markerOptions == GOMARKER_BACK_TIME)
        streamClockTarget -= markerValue;
    }
  else
    {
      streamClockTarget = markerValue;
      status = kDSNoErr;
    }

  /* Find the most recent marker based on streamClockTarget. This may be set to
   * an absolute stream time if the markerOptions == GOMARKER_ABS_TIME.
   * In this case, result of the marker table search is the desired
   * position, and nothing further is required.
   */
  markerTable
      = (MarkerRecPtr)(((char *)ctx->markerChunk) + sizeof (MarkerChunk));

  endOfMarkerTable
      = (MarkerRecPtr)(((char *)ctx->markerChunk) + ctx->markerChunk->chunkSize
                       - sizeof (MarkerChunk));

  /* Search the marker table for the specified target time, as modified
   * by any of the caller specified branching options.
   *
   * NOTE: еее assumes there is at least one entry in the table еее
   */
  marker = markerTable;
  while ((marker < endOfMarkerTable)
         && (streamClockTarget > marker->markerTime))
    marker++;

  /* If the search went off the end of the table, then the requested
   * branch target is out of range.
   */
  if (marker > endOfMarkerTable)
    return kDSRangeErr;

  /* Adjust the pointer to the marker entry if the type of branching
   * requested is relative based on markers (instead of time).
   */
  if (markerOptions == GOMARKER_FORWARD)
    marker = &marker[markerValue];

  else if (markerOptions == GOMARKER_BACKWARD)
    marker = &marker[-markerValue];

  /* When all adjustments to the chosen marker have been made, make sure
   * that we are still operating within the defined marker table.
   */
  if ((marker < markerTable) || (marker > endOfMarkerTable))
    return kDSRangeErr;

  /* Lastly, check for an attempt to position past the beginning or end
   * of the file and return an error if so.
   */
  if (status == kDSNoErr)
    {
      if ((marker->markerOffset >= GetBlockFileSize (&ctx->blockFile))
          || (marker->markerOffset < 0))
        return kDSEndOfFileErr;

      /* Pass back the selected file position corresponding to the
       * marker time or position that was specified by the options
       * and the marker value.
       */
      (*pOffset) = marker->markerOffset;

      /* Tell the streamer the stream time for the branch
       * we are now about to perform.
       */
      DSSetBranchDest (ctx->streamCBPtr, marker->markerTime);
    }

  return status;

#else
  /* Check for an attempt to position past the end of the file
   * and return an error if so.
   */
  if (markerValue >= GetBlockFileSize (&ctx->blockFile))
    return kDSEndOfFileErr;

  /* Just return the input value. Later on, there will be some kind of
   * translation function that will map a marker number or time to a file
   * position. This may require some kind of lookup function to find a mapping
   * in a data structure obtained during stream initialization, for example.
   * Any mapping is possible, and for now we map 1:1 ==> marker:file position.
   */
  (*pOffset) = markerValue;
  return kDSNoErr;
#endif
}

/*******************************************************************************************
 * Routine to position subsequent data fetches to a different point in the data
 *stream. NOTE: This functionality is OPTIONAL for data acquisition. For some
 *implementations, such as cable TV streams, seeking may make no sense.
 *******************************************************************************************/
static int32
DoGoMarker (AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
  int32 status;
  long position;

#if TIME_BASED_BRANCHING
  /* Short-circuit name based branching requests here for now... */
  if (dataMsg->msg.marker.options == GOMARKER_NAMED)
    return kDSUnImplemented;
#endif

  /* Attempt to map the marker value to a file position. If we get
   * an error in the process, return it immediately.
   */
  status = _MapMarkerToOffset (ctx, dataMsg->msg.marker.value,
                               dataMsg->msg.marker.options, &position);
  if (status != 0)
    return status;

#if TRACE_TIME_BASED_BRANCHING
  /* Save the current branch destination in the circular buffer
   * for debugging purposes.
   */
  gDABranchTable[gDABranchTableIndex++] = position;

  if (gDABranchTableIndex >= DA_BRANCH_TRACE_MAX)
    gDABranchTableIndex = 0;
#endif

  /* Flush any pending operations to prevent unnecessary I/O and
   * subsequent parsing overhead.
   */
  _FlushPendingIO (ctx);

  /* Set the file offset to the value returned by the mapping function.
   */
  ctx->offset = position;

  /* Clear the "we sent an EOF" flag so that the next time an attempt is
   * made to read past the end, we will actually report an EOF error.
   */
  ctx->fEOFWasSent = false;

  return kDSNoErr;
}

/*******************************************************************************************
 * Routine perform an asynchronous read of the next data in the stream.
 *******************************************************************************************/
static int32
DoGetData (AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
  int32 status;
  DSDataBufPtr bp;

  bp = (DSDataBufPtr)dataMsg->msg.data.bufferPtr;

  /* Check for read past end of file and return the library error code
   * if we are attempting to do so. This gets propagated back to the
   * stream parser, which shuts down the streaming process by discontinuing
   * to issue new reads since there is obviously no more data.
   * Note that we will not send EOF more than once. This tremendously
   * simplifies the handling of EOF in the stream parser. Instead, when
   * an attempt to read past EOF is detected, we merely report that the
   * buffer was flushed. This has "safe" consequences back in the parser.
   * If needed, we may change this in the future to something like a
   * "secondary EOF", which the parser would ignore.
   */
  if (ctx->offset >= GetBlockFileSize (&ctx->blockFile))
    {
      if (ctx->fEOFWasSent)
        status = kDSWasFlushedErr;
      else
        status = kDSEndOfFileErr;

      ctx->fEOFWasSent = true;

      return status;
    }

  /* Allocate one ioReqItem for this dataBuffer from the pool */
  bp->ioreqItem = AllocPoolItem (ctx->ioReqItemPoolPtr);
  if (bp->ioreqItem == 0)
    {
#if ALLOW_REQUEST_OVERLOADING
      AddDataAcqMsgToTail (ctx, dataMsg);
      return kDSNoErr;
#else
      kprintf ("DoGetData - DataAcq.c - AllocPoolItem( ctx->ioReqItemPoolPtr "
               ") failed !!!");
      return kDSNoMemErr;
#endif
    }

  /* We need to cache the pointer to the item so that later, when
   * I/O completions occur, we can simply search the in-progress list
   * of buffers to determine which message we should reply to in order
   * to tell the requestor that the request for data has completed.
   */
  bp->ioreqItemPtr = (IOReq *)LookupItem (bp->ioreqItem);

  status = AsynchReadBlockFile (
      &ctx->blockFile,              /* stream's block file */
      bp->ioreqItem,                /* ioreq Item from DSDataBuf struct */
      &bp->streamData,              /* place to READ data into */
      dataMsg->msg.data.bufferSize, /* amount of data to READ */
      ctx->offset);                 /* offset of data in the file */

  /* If the read was successfully queued, add the buffer
   * onto the in-progress buffer queue. We need to do this so that later,
   * when the operation completes, we can find the message to reply to
   * to signal completion.
   */
  if (status >= 0)
    {
      /* Advance the file offset past the data we just read so that
       * subsequent reads get sequentially higher file offsets.
       */
      ctx->offset += dataMsg->msg.data.bufferSize;

      /* Add buffer to front of in-progress list.
       * еееееееееее NOTE: fix any I/O ordering problems here ееееееееееее
       *		It may be the case that completing I/O does NOT
       *complete in the order it was requested. This can be solved here by
       *		managing the order of the queue.
       */
      dataMsg->link = ctx->requestQueue;
      ctx->requestQueue = dataMsg;
    }

  return status;
}

/*==========================================================================================
  ==========================================================================================
                                                                Data
  Acquisition thread
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Utility routine to perform thread startup initialization. Handles all
 *resource setup and sets the context appropriately for communications with our
 *creator.
 *******************************************************************************************/
static int32
_InitializeThread (AcqContextPtr ctx)
{
  int32 status;

  /* Initialize context fields */

  ctx->creatorStatus = -1; /* assume initialization will fail */
  ctx->threadItem = 0;
  ctx->abortQueue = 0;
  ctx->requestPort = 0;
  ctx->ioDoneReplyPort = 0;
  ctx->requestQueue = 0;
  ctx->fEOFWasSent = false;
  ctx->offset = 0;
  ctx->ioReqItemPoolPtr = NULL;

#if ALLOW_REQUEST_OVERLOADING
  ctx->dataQueueHead = NULL;
  ctx->dataQueueTail = NULL;
#endif

#if TIME_BASED_BRANCHING
  ctx->subscriberPortSignal = 0;
  ctx->subscriberPort = 0;
  ctx->markerChunk = NULL;
  ctx->streamCBPtr = NULL;
#endif

#if TRACE_TIME_BASED_BRANCHING
  {
    long index;

    for (index = 0; index < DA_BRANCH_TRACE_MAX; index++)
      gDABranchTable[index] = -1;
  }
#endif

  /* Create the message port where we will accept data
   * request messages.
   */
  status = NewMsgPort (&ctx->requestPortSignal);
  if (status <= 0)
    goto BAILOUT;
  else
    ctx->requestPort = status;

  /* Create the message port where we will receive I/O completion
   * messages from the asynchronous I/O we will be performing.
   */
  status = NewMsgPort (&ctx->ioDoneReplyPortSignal);
  if (status <= 0)
    goto BAILOUT;
  else
    ctx->ioDoneReplyPort = status;

  /* Open the specified file for streaming data
   */
  status = OpenStreamFile (ctx);
  if (status != 0)
    goto BAILOUT;

#if TIME_BASED_BRANCHING
  /* Create a message port to receive subscriber messages. While this may
   * seem weird, its not exceedingly so. We subscribe to our own data type and
   * the streamer delivers this stuff to us (even though we are supplying
   * the raw data). We are not a usual subscriber, however, and don't have
   * any application oriented API to implement.
   */
  status = NewMsgPort (&ctx->subscriberPortSignal);
  if (status <= 0)
    goto BAILOUT;
  else
    ctx->subscriberPort = status;

  /* Create a message port and signal to receive replies to asynchronous
   * requests we send to the streamer.
   */
  status = NewMsgPort (&ctx->dsReqReplyPortSignal);
  if (status <= 0)
    goto BAILOUT;
  else
    ctx->dsReqReplyPort = status;
#endif

  /* Looks like everything initialized just fine.
   */
  ctx->creatorStatus = 0;

BAILOUT:
  /* Inform our creator that we've finished with initialization. We are either
   * ready and able to accept requests for work, or we failed to initialize
   * ourselves. If the latter, we simply exit after informing our creator.
   * All resources we created are thankfully cleaned up for us by the system.
   */
  status = SendSignal (ctx->creatorTask, ctx->creatorSignal);
  if ((ctx->creatorStatus < 0) || (status < 0))
    return -1;
  else
    return 0;
}

/*******************************************************************************************
 * This proc is called by CreateItemPool to create an ioReqItem for each pool
 *entry.
 *******************************************************************************************/
static Item
_createIOReqItem (AcqContextPtr ctx)
{
  return CreateBlockFileIOReq (ctx->blockFile.fDevice, ctx->ioDoneReplyPort);
}

/*******************************************************************************************
 * Utility routine to open a files and get it ready for for streaming.
 *******************************************************************************************/
static int32
OpenStreamFile (AcqContextPtr ctx)
{
  int32 status;

  /* what does THIS do */
  ctx->blockFile.fDevice = 0;

  /* Open the specified file for streaming data
   */
  status = OpenBlockFile (ctx->fileName, &ctx->blockFile);
  if (status != 0)
    goto BAILOUT;

  /* Create an I/O request Item for each of the buffers in the
   * buffer list. Later, we'll receive these buffers as messages to
   * do asynchronous I/O into and we'll need these Items at that time.
   */
  ctx->ioReqItemPoolPtr = CreateItemPool (
      NUM_IOREQ_ITEMS, (CreateProcPtr)_createIOReqItem, (void *)ctx);

  /* Assert 'success' */
  status = 0;

BAILOUT:
  return status;
}

/*******************************************************************************************
 *	Data Acquisition
 *	----------------
 *	This thread performs data acquisition services for the data stream
 *parser. The parser expects to be supplied with a stream of data buffers in
 *response to a set of messages to this code. This code is responsible for
 *asynchronously posting reads to obtain the data, and for reporting back to
 *the stream parser when each buffer is filled. The interface to this code from
 *the stream parser is opcode driven in that each message specifies what action
 *to take via the 'whatToDo' field of the 'DataAcqMsg' message struct defined
 *in DataStream.h.
 *
 *	Opcodes and what they do:
 *	-------------------------
 *	kAcqOpConnect	This call is make as a result of calling DSConnect().
 *The stream parser calls the data acquisition proc to inform it that the
 *stream is being connected so that it may perform any necessary
 *					initializations. The result code is reported
 *back to the caller of DSConnect(), so the data acquisition thread can prevent
 *the stream from connecting should sufficient resources not exist to actually
 *deliver data to the stream.
 *
 *	kAcqOpDisconnect This call is made as a result of calling
 *DSHStreamClose() and is the data acquisition proc's opportunity to release
 *any resources that may have been allocated by it, such as those allocated at
 *					the time the kAcqOpConnect call was
 *handled.
 *
 *	kAcqOpGetData	This is the data delivery call. The stream parser is
 *requesting that the specified data buffer be filled with data from the next
 *					available position (sequentially) in the
 *stream. THIS CALL SHOULD NOT BLOCK!!! The parser calls this whenever a buffer
 *becomes free in order to keep the data flowing. Later, when the data becomes
 *					available, the data acquisition proc is
 *reponsible for reporting the availability of the data to the parser.
 *
 *	kAcqOpGoMarker	No assumption is made by the parser about the
 *addressability of data in a stream (hence, there is no "seek()" interface to
 *this code). Instead, positions in streams are addressed with an abstract
 *					token called a "marker". The presence of
 *markers and their data dependent implementation is defined by the data
 *acquisition proc (i.e., markers may not be implemented in some applications,
 *such as streamed data from a cable TV network).
 *
 *	kAcqOpExit		Tells the thread to clean up and exit. Sent as a
 *result of a call to DisposeDataAcq().
 *
 *******************************************************************************************/
static void
DataAcqThread (long notUsed, AcqContextPtr ctx)
{
  int32 status;
  DataAcqMsgPtr dataMsg;
  Item ioReqItem;
  uint32 signalBits;
  uint32 anyPort;
  Boolean fKeepRunning;

#if TIME_BASED_BRANCHING
  MarkerChunkPtr markerChunkPtr;
  SubscriberMsgPtr subMsg;
  DSRequestMsgPtr dsReqMsgPtr;
  Item reqCompleteMsgItem;
#endif

#if WAIT_FOR_IO_COMPLETION_BEFORE_EXIT
  Boolean fWantToExitButCant;
  DataAcqMsgPtr exitRequestMsg;

  fWantToExitButCant = false;
  exitRequestMsg = NULL;
#endif

  /* Call a utility routine to perform all startup initialization.
   */
  if ((status = _InitializeThread (ctx)) != 0)
    exit (0);

    /* All resources are now allocated and ready to use. Our creator has been
     * informed that we are ready to accept requests for work. All that's left
     * to do is wait for work request messages to arrive.
     */
#if TIME_BASED_BRANCHING
  anyPort = ctx->requestPortSignal | ctx->ioDoneReplyPortSignal
            | ctx->subscriberPortSignal | ctx->dsReqReplyPortSignal;
#else
  anyPort = ctx->requestPortSignal | ctx->ioDoneReplyPortSignal;
#endif

  fKeepRunning = true;
  while (fKeepRunning)
    {
      /* Wait for a message on EITHER the request port or
       * the I/O completion port.
       */
      signalBits = WaitSignal (anyPort);

      /* Process I/O completions first as this may result in buffers
       * freeing up so that additional I/O can be queued (keep the data
       * flowing).
       */
      if (signalBits & ctx->ioDoneReplyPortSignal)
        {
          while (PollForMsg (ctx->ioDoneReplyPort, NULL, NULL,
                             (void **)&ioReqItem, &status))
            {
              /* Try to find the completing I/O in the pending request queue
               */
              dataMsg = _FindRequest (ctx, &ctx->requestQueue, ioReqItem);
              if (dataMsg != NULL)
                status = dataMsg->msg.data.bufferPtr->ioreqItemPtr->io_Error;

              else
                /* If we didn't find the I/O in the pending queue, check for
                 * it in the "aborted queue".
                 */
                {
                  dataMsg = _FindRequest (ctx, &ctx->abortQueue, ioReqItem);
                  if (dataMsg != NULL)
                    status = kDSWasFlushedErr;
                }

              if (dataMsg != NULL)
                {
                  ReplyMsg (dataMsg->msgItem, status, dataMsg,
                            sizeof (DataAcqMsg));

                  /* Free the ioReqItem for this dataBuffer back to the item
                   * pool */
                  ReturnPoolItem (ctx->ioReqItemPoolPtr, ioReqItem);

#if ALLOW_REQUEST_OVERLOADING
                  /* Since we know there's now a free IOReqItem available, we
                   * check to see if there's any pending data requests that we
                   * can start up at this point. If there is one, start one and
                   * just one, because that's all the free IOReq items that we
                   * know exist at this point.
                   */
                  if ((dataMsg = GetNextDataAcqMsg (ctx)) != NULL)
                    {
                      status = DoGetData (ctx, dataMsg);

                      /* If there was an error in queuing the I/O, then
                       * we will never see an I/O completion message.
                       * Therefore, we must reply to our requestor right now
                       * otherwise the request message will be lost forever.
                       */
                      if (status < 0)
                        ReplyMsg (dataMsg->msgItem, status, dataMsg,
                                  sizeof (DataAcqMsg));
                    }
#endif
                }
              else
                kprintf ("DataAcqThread got unknown i/o reply message!\n");
            }
        }

      /* Process any new requests for service as determined by the incoming
       * message data.
       */
      if (signalBits & ctx->requestPortSignal)
        {
          while (PollForMsg (ctx->requestPort, NULL, NULL, (void **)&dataMsg,
                             &status))
            {
              switch (dataMsg->whatToDo)
                {
                case kAcqOpConnect:
                  status = DoConnect (ctx, dataMsg);
                  break;

                case kAcqOpDisconnect:
                  status = DoDisconnect (ctx);
                  break;

                case kAcqOpGoMarker:
                  status = DoGoMarker (ctx, dataMsg);
                  break;

                case kAcqOpGetData:
                  status = DoGetData (ctx, dataMsg);

                  /* If there was an error in queuing the I/O, then
                   * we will never see an I/O completion message. Therefore,
                   * we must reply to our requestor right now otherwise the
                   * request message will be lost forever.
                   */
                  if (status < 0)
                    ReplyMsg (dataMsg->msgItem, status, dataMsg,
                              sizeof (DataAcqMsg));
                  break;

                case kAcqOpExit:
#if WAIT_FOR_IO_COMPLETION_BEFORE_EXIT
                  /* In the FAKE_ABORT_IO world, we can't really exit now
                   * unless all pending I/O is quiescent. Instead, we set a "we
                   * want to exit" flag and allow the I/O to finish up before
                   * actually exiting. Later, we check for empty in-progress
                   * and going-to-abort queues and exit the thread ONLY when
                   * these are both empty. We must wait until then to
                   * ReplyMsg() to the exit request.
                   */
                  fWantToExitButCant = true;

                  /* Make sure we don't get more than one exit request because
                   * our FAKE_ABORT_IO logic is not fancy enough to handle
                   * these. Only one to a customer...
                   */
                  if (exitRequestMsg == NULL)
                    exitRequestMsg = dataMsg;
                  else
                    kprintf (
                        "DataAcqThread got multiple kAcqOpExit messages!\n");
#else
                  fKeepRunning = false;
#endif
                  break;

                default:;
                }

                /* Reply to the request using the status result from
                 * whatever request we just processed, unless we are just
                 * done with a 'kAcqOpGetData' message. These are replied to
                 * separately, and asynchronously.
                 */
#if WAIT_FOR_IO_COMPLETION_BEFORE_EXIT
              /* In the FAKE_ABORT_IO world, exit is replied to async, too */
              if ((dataMsg->whatToDo != kAcqOpGetData)
                  && (dataMsg->whatToDo != kAcqOpExit))
#else
              if (dataMsg->whatToDo != kAcqOpGetData)
#endif
                ReplyMsg (dataMsg->msgItem, status, dataMsg,
                          sizeof (DataAcqMsg));
            }
        }

#if TIME_BASED_BRANCHING
      /********************************************************************/
      /* Check for and handle replies to asynchronous streamer requests.
       */
      /********************************************************************/
      if (signalBits & ctx->dsReqReplyPortSignal)
        {
          while (PollForMsg (ctx->dsReqReplyPort, &reqCompleteMsgItem, NULL,
                             (void **)&dsReqMsgPtr, &status))
            {
              /* We don't track or pool the buffers or message item resources
               * used to submit asynchronous streamer requests at this time.
               * Instead, because these happen very infrequently, we
               * dynamically allocate and free the message items and buffers
               * needed. If this proves to cause fragmentation or other
               * problems, we can always switch to using a pool approach...
               */
              RemoveMsgItem (reqCompleteMsgItem);
              FreePtr (dsReqMsgPtr);
            }
        }

      /********************************************************/
      /* Check for and process and incoming request messages. */
      /********************************************************/
      if (signalBits & ctx->subscriberPortSignal)
        {
          while (PollForMsg (ctx->subscriberPort, NULL, NULL, (void **)&subMsg,
                             &status))
            {
              if (subMsg->whatToDo == kStreamOpData)
                {
                  /* Get a pointer to our data */
                  markerChunkPtr = (MarkerChunkPtr)subMsg->msg.data.buffer;

                  /* For now, handle only the marker table data type */
                  if (markerChunkPtr->subChunkType == MARKER_TABLE_SUBTYPE)
                    {
                      /* If a marker table is currently defined, then
                       * dispose of it to make way for the new one.
                       */
                      if (ctx->markerChunk != NULL)
                        FreePtr (ctx->markerChunk);

                      /* Allocate a buffer to hold a copy of the new marker
                       * table */
                      ctx->markerChunk = (MarkerChunkPtr)NewPtr (
                          markerChunkPtr->chunkSize, MEMTYPE_ANY);

                      /* Copy the marker table data from the stream buffer so
                       * as not to constipate the stream buffering.
                       */
                      if (ctx->markerChunk != NULL)
                        memcpy (ctx->markerChunk, markerChunkPtr,
                                markerChunkPtr->chunkSize);
                    }
                }

              /* Reply to the request we just handled.
               */
              ReplyMsg (subMsg->msgItem, kDSNoErr, subMsg,
                        sizeof (SubscriberMsg));
            }
        }
#endif

#if WAIT_FOR_IO_COMPLETION_BEFORE_EXIT
      /* If we are having to implement the odious fake AbortIO() functionality,
       * we check for a pending exit request and the absence of either valid
       * pending I/O requests (found in ctx->requestQueue) or fake-aborted I/O
       * operations (found in ctx->abortQueue). If both queues are empty AND we
       * have a pending exit request, we can *really* exit. Otherwise, the
       * stupid system crashes because its _OWN_ attempt to abort pending
       * operations will cause a crash. At this point, we can actually reply to
       * the original exit request message...
       */
      if (fWantToExitButCant && (ctx->abortQueue == NULL)
          && (ctx->requestQueue == NULL))
        {
          ReplyMsg (exitRequestMsg->msgItem, kDSNoErr, exitRequestMsg,
                    sizeof (DataAcqMsg));
          fKeepRunning = false;
        }
#endif
    }

  /* Exiting should cause all non-memory resources we allocated to be
   * returned to the system.
   */
  ctx->threadItem = 0;

  /* Free the item pool we created */
  if (ctx->ioReqItemPoolPtr != NULL)
    DeleteItemPool (ctx->ioReqItemPoolPtr);

  exit (0);
}
