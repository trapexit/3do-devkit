/*******************************************************************************************
 *	File:			DataStream.c
 *
 *	Contains:		stream parser and data distributer
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/14/93	jb		Version 1.9.1
 *						Moved setting of the "branch in progress"
 *flag and the translated branch destination value to an external routine
 *callable by data acquisition called DSSetBrancDest(). Code was moved from the
 *DoGoMarker() routine to set the flag and store the target value. 12/2/93
 *jb		Version 1.9 Added argument to NewDataStream() to specify the
 *number of subscriber messages that should be allocated upon stream creation.
 *	11/23/93	jb		Version 1.8
 *						Add the CloseDataStreaming() routine to
 *allow for deallocation of context block pool. Connect message to data
 *acquisition now includes a pointer to the stream control block of the
 *						connecting stream.
 *	11/22/93	jb		Version 1.7
 *						Add check for impossible chunk sizes,
 *zero or negative, in the DeliverData() routine. 10/21/93	rdg
 *Version 1.6 Modified DoStopStream() to always send a stop message even if the
 *stream is already stopped so if a flush option is present, it will always be
 *passed on to the subscribers. 10/19/93	jb		Version 1.5
 *						Switch to using UMemory.c routines for
 *memory allocation 10/4/93		jb		Version 1.4 Added End
 *Of Stream notification. 8/18/93		jb		Version 1.3.4
 *						Initialize repliesPending to zero in
 *HandleDSRequest() before dispatching to a request handler. Also, reply to
 *requests immediately in that routine if processing is completed and no
 *replies from dataacq or subscribers are required. 8/16/93		jb
 *Version 1.3.2 Add call to OpenAudioFolio() in _InitializeThread() if the
 *						compile switch STARTSTREAM_SETS_THE_CLOCK
 *is set. Add a range check on the whatToDo field HandleDSRequest() to prevent
 *queueing of totally invalid request messages. 8/5/93		jb
 *Version 1.3.1 Add EXIT_ONLY_AFTER_CLOSE_REPLIES related code to cause the
 *						streamer only to exit after all replies
 *to subscriber close messages have been received. Fix bug in DoSetChannel()
 *that would always return an error status.
 *	8/3/93		jb		Set privatePtr = NULL in DoConnect()
 *for the disconnect message sent to previous data acquisition thread. Get rid
 *of semaphore control of message pool since procedure level access to the
 *library routines can't use the messages due to resource ownership problems
 *anyway. 7/9/93		jb		Version 1.3 Convert all
 *subscriber messaging to asynchronous operation. Keep the streamer single
 *threaded by adding a request message queue that incoming streamer requests
 *are queued upon. The message at the head of this queue is the "current"
 *request that is being processed. It gets replied to when all subscribers/data
 *acquisition threads have operated on any subsequent message sends that are
 *used to implement the request to the streamer. Added HandleSubscriberReply()
 *routine. Added _AllocSubsMsg() routine to allocate subscriber messages under
 *						control of a semaphore.
 *						Get rid of _CallDataAcquisition() routine
 *as all communications with data acquisition are now asynchronous. 6/25/93
 *jb		Version 1.2.7 Change check in _ReleaseChunk() to == 0 from <= 0
 *so that if someone replies chaotically to messages we don't get confused
 *						about the buffer.
 *						Move processing of DSClockSync() to a
 *procedure in the DataStreamLib.c file so that sync can be sent immediately
 *						instead of suffering propagation delay by
 *handling it as a streamer request message. Export _ForEachSubscriber() for
 *use by DataStreamLib.c 6/25/93		jb		Substract some
 *from end of buffer test in the parser to take into account FILL chunks that
 *are smaller than the smallest legitimate size. 6/24/93		jb
 *Version 1.2.6 Add options to GoMarker messages to allow for relative
 *branching 6/22/93		jb		Version 1.2.5 Added options to
 *start and stop subscriber messages to allow data flushing options to be
 *communicated when a stream is started or stopped. Add an 'exemptStream' arg
 *to _ForEachSubscriber() to allow the prevention of deadlock when
 *syncrhonously calling subscribers. This is currently only used to handle
 *DoClockSync() where it is the normal case that a subscriber is the originator
 *of the DSClockSync() call.
 *	6/18/93		jb		Make DoStartStream() send a
 *'kStreamOpStart' message to all subscribers when a stream is started. This
 *will allow pause/resume to be implemented in subscribers by providing a way
 *to stop the data flow without flushing buffered data. 6/15/93 jb
 *Make DoGoMarker() set STRM_GO_INPROGRESS flag and remember branch destination
 *if data acquisition accepts the request. Make STRM_CLOCK_VALID transition
 *when stream is started, stopped or aborted. This gets set only in
 *DSSetClock(). 6/8/93		jb		Version 1.2.4 Add 'clockOffset'
 *field to stream control block. 6/1/93		jb		Version 1.2.3
 *						Remove all variable initializers in local
 *variable declarations to avoid register bashing by the ARM C compiler.
 *	5/20/93		jb		Add test for quadbyte alignment of
 *chunk sizes. Also, add the NOT_QUADBYTE_ALIGNED() macro to clean up same test
 *for buffer list.
 *	5/20/93		jb		Add DoConnect() and the general
 *connect/disconnect facility. Prevent queuing data unless a data port is
 *defined.
 *	5/19/93		jb		Add additional data request messages
 *when instantiating a new stream to cover extras needed if trying to send to
 *data acq when all buffers are in use. 5/12/93		jb
 *Get rid of STRM_OPEN now that we are thread based.
 *	5/12/93		jb		Move _FillPoolWithMsgItems() to library
 *interface module and rename as FillPoolWithMsgItems(). 5/11/93
 *jb		Major, significant overhaul. Entire stream parser is now a
 *thread. All "library" routines now are message handlers. An external
 *						interface library builds request messages
 *and sends them to the thread. This mess was necessitated because it is
 *otherwise impossible for subscribers to call any stream control routines,
 *						such as "go marker", or "stop". This is
 *because doing so requires sending messages, and the messages owned by a
 *stream were *really* owned by the stream's initializer.
 *	5/11/93		jb		Get rid of 'subscriberContext' in
 *subscriber messages since these are now implied by the subscriber thread's
 *context.
 *	5/10/93		jb		Clear the EOF flag in the stream
 *control block when a DSGoMarker() call is received.
 *	5/10/93		jb		Removed 'DSHTime' data type
 *	5/10/93		jb		Remove DSHGetStreamTime(), remove
 *'streamTime' from stream control block. 5/9/93		jb
 *Ensure that 'status' is set properly in DSHOpenStream() when branching to the
 *'ABORTOPEN' label.
 *	5/9/93		jb		Added DSGoMarker(), handle EOF replies
 *from data client.
 *	5/9/93		jb		Remove all "seeking" related code from
 *the parser. All seeking actions are handled by calls to DSGoMarker(), and
 *subsequently by the data acquisition client.
 *	5/9/93		jb		Switch to using new iterator function
 *in MemPool.c for initing message pools with message items in
 *_FillPoolWithMsgItems().
 *	4/25/93		njc		Added _TryToSeekStream to DSHRunStream.
 *Looks for a seek chunk and, if a seek chunk is found, flushes the filled
 *buffer queue and sends a DoGoMarker msg to the Data Acquisition module.
 *	4/22/93		njc		Test messagePtr->msg_Result in
 *DSHRunStream for Read errors. On error, stop stream and add buffer back to
 *free list.
 *	4/21/93		njc		Was not incrementing the useCount for
 *buffers in DSHRunStream.
 *	4/19/93		jb		Remove 'acquireContext' since it is
 *implied by the acquisition message port now.
 *	4/19/93		jb		Get our ports straight when
 *communicating with data acquisition. Change dataPort to acqReplyPort,
 *subsPort to subsReplyPort
 *	4/19/93		jb		Handle the case where no subscriber
 *claims any of the chunks in a filled data buffer by placing the buffer back
 *into the free pool.
 *	4/19/93		jb		Fix use of 'privatePtr' field in data
 *acq messages. 4/19/93		jb		Make sure free buffers get
 *filled in DSHRunStream()
 *	4/16/93		jb		Do not attempt to maintain stream time.
 *Timestamp info must be imbedded in time-critical data chunk streams so that
 *individual subscribers can output their data at rates appropriate to the
 *data. Get rid of 'currTime' field in message header, only allow calls to
 *DSClockSync() to change the stream time, and pass the new clock to all
 *subscribers as a result of that call. 4/16/93		jb
 *Switch to using SendMsg() from PutMsg() in 3B1 4/13/93		jb
 *Switch to using NewMsgPort()
 *	4/4/93		jb		Get rid of _GetTime() and use direct
 *reference to stream control block's time field. This assumes that a timer
 *thread has been created that will periodically update the time for the
 *stream, which is done when a stream is opened. 4/3/93		jb
 *Add checks for open/running stream at start of each routine
 *	4/1/93		jb		Mostly done with 1st cut of message
 *based version 3/30/93		jb		Complete redesign of subscriber
 *and data interfaces using messages instead of call/callback to allow
 *decoupling of processing via threads. 3/28/93		jb
 *Switch to using message port for iodone notifications instead of a straight
 *callback mechanism.
 *	3/26/93		jb		Add 'opening' call to data acquisition,
 *and pass stream context in args to open and closing so that data acquisition
 *can be written to support multiple streams.
 *	3/24/93		jb		Wrote rest of "run stream" and buffer
 *management.
 *	3/20/93		jb		Rethink the data acquisition interface
 *to include markers and non sequential buffer returns. 3/18/93 jb
 *Add convenience functions; start on buffer management 3/16/93 jb
 *New today.
 *
 *******************************************************************************************/

/* Settomg the following switch causes the streamer thread to defer exiting the
 * system until all subscribers have replied to their close messages.
 * Otherwise, the streamer will exit immediately upon receipt of having sent
 * the close messages.
 */
#define EXIT_ONLY_AFTER_CLOSE_REPLIES 1

/* Turn on the following when WaitPort() really works. This will make the
 * DSGoMarker() operate synchronously with respect to data acquisition,
 * allowing the setting of STRM_GO_INPROGRESS according to whether the branch
 * request is accepted or rejected by data acquisition.
 */
#define GOMARKER_SYNCHRONOUS 0

/* The following compile switch causes the stream clock to be paused and
 * resumed when stop stream and start stream are called. It depends on the
 * lastValidClock field and STRM_CLOCK_WAS_VALID bit to do its job. When stop
 * is called, the bit is tested to see if the clock was ever set. If so, the
 * current relative time is remembered so that start can just set the clock to
 * that value.
 */
#define STARTSTREAM_SETS_THE_CLOCK 1

#include "Audio.h"
#include "Debug3DO.h"
#include "UMemory.h"
#include "operror.h"
#include "stdlib.h"
#include "types.h"

#include "DataStream.h"
#include "DataStreamLib.h"
#include "SemHelper.h"
#include "ThreadHelper.h"

/***********************/
/* Data stream globals */
/***********************/

struct DataStreamGlobals
{

  MemPoolPtr streamContextPool; /* The memory pool for stream contexts */

} DSGlobals;

/*************************************************/
/* Macro for conditional output of system errors */
/* NOTE: DEBUG switch set in make file			 */
/*************************************************/
#if DEBUG
#define OUTPUTSYSERR(x) PrintfSysErr (x)
#else
#define OUTPUTSYSERR(x)
#endif

/********************************************************/
/* Macro for testing a value to be non-QUADBYTE aligned */
/********************************************************/
#define NOT_QUADBYTE_ALIGNED(x) (((unsigned long)x) & 0x3)

/************************************/
/* Local utility routine prototypes */
/************************************/
static int32 _GetSubscriberGivenType (DSStreamCBPtr streamCBPtr,
                                      DSDataType streamType,
                                      DSSubscriberPtr *pSubscriberPtr);
static void _DeleteSubscriber (DSStreamCBPtr streamCBPtr, DSSubscriberPtr sp);
static void _AbortStream (DSStreamCBPtr streamCBPtr);
static void _TearDownStreamCB (DSStreamCBPtr streamCBPtr);

/* DSDataBuf utility routines */

static void _FreeDataBuf (DSStreamCBPtr streamCBPtr, DSDataBufPtr bp);
static DSDataBufPtr _GetFreeDataBuf (DSStreamCBPtr streamCBPtr);
static DSDataBufPtr _GetFilledDataBuf (DSStreamCBPtr streamCBPtr);
static int32 _FillDataBuf (DSStreamCBPtr streamCBPtr, DSDataBufPtr bp);
static int32 _ReleaseChunk (DSStreamCBPtr streamCBPtr, int32 subscriberStatus,
                            DSDataBufPtr bp);

/************************************************/
/* The following implement the request messages	*/
/* in the thread portion of the streamer		*/
/************************************************/
static int32 DoStartStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoStopStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoPreRollStream (DSStreamCBPtr streamCBPtr,
                              DSRequestMsgPtr reqMsg);
static int32 DoCloseStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoSubscribe (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoGoMarker (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoGetChannel (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoSetChannel (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoControl (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoConnect (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static int32 DoRegisterEndOfStream (DSStreamCBPtr streamCBPtr,
                                    DSRequestMsgPtr reqMsg);

/* High-level utilities local to the data streaming thread */

static void HandleSubscriberReply (DSStreamCBPtr streamCBPtr,
                                   SubscriberMsgPtr subMsg,
                                   Message *messagePtr);
static void HandleDataAcqReply (DSStreamCBPtr streamCBPtr,
                                DataAcqMsgPtr acqMsg, Message *messagePtr);
static int32 DeliverData (DSStreamCBPtr streamCBPtr);
static int32 HandleDSRequest (DSStreamCBPtr streamCBPtr,
                              DSRequestMsgPtr reqMsg, Boolean *pFKeepRunning);
static int32 _InitializeThread (DSStreamCBPtr streamCBPtr);
static void DataStreamThread (long notUsed, DSStreamCBPtr streamCBPtr);

/*==========================================================================================
  ==========================================================================================
                                                        Miscellaneous Utility
  Routines
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Routine to call each of a stream's subscribers with a message. If an error
 *is returned, some subscribers may not get called.
 *******************************************************************************************/
static int32
_ForEachSubscriber (DSStreamCBPtr streamCBPtr, SubscriberMsgPtr subMsg)
{
  int32 status;
  long index;
  DSSubscriberPtr sp;
  SubscriberMsgPtr copySubMsg;

  /* If there are not a sufficient number of messages in the pool
   * to send one to each subscriber, then we're doomed.
   */
  if (streamCBPtr->subsMsgPool->numFreeInPool < streamCBPtr->numSubscribers)
    {
      ERR (("_ForEachSubscriber - insufficient number of subscriber "
            "messages\n"));
      return kDSNoMsgErr;
    }

  /* The number of replies we should expect is equal to the
   * number of subscribers to whom we broadcast the message.
   */
  streamCBPtr->repliesPending = 0;

  /* Send each subscriber a copy of the specified message. Bail out
   * at the first sign of trouble.
   */
  for (index = 0; index < streamCBPtr->numSubscribers; index++)
    {
      sp = streamCBPtr->subscriber + index;

      copySubMsg = (SubscriberMsgPtr)AllocPoolMem (streamCBPtr->subsMsgPool);
      if (copySubMsg != NULL)
        {
          /* Copy the input message to the one we just allocated so that
           * a unique copy of the message can be sent to each subscriber.
           * However, remember to preserve the actual 'msgItem' field in
           * the message we just allocated!!!
           */
          subMsg->msgItem
              = copySubMsg->msgItem; /* move msgItem into the template */
          *copySubMsg = *subMsg;     /* ...now copy ALL fields from template */

          /* Send the message to the next subscriber */
          status = SendMsg (sp->subscriberPort, copySubMsg->msgItem,
                            copySubMsg, sizeof (SubscriberMsg));
          if (status < 0)
            {
              CHECKRESULT (
                  "DataStreamThread - _ForEachSubscriber - SendMsg() ",
                  status);
              break;
            }
          else
            /* Increment the number of replies we are expecting */
            streamCBPtr->repliesPending++;
        }
      else
        {
          ERR (("_ForEachSubscriber - ran out of subscriber messages\n"));
          status = kDSNoMsgErr;
          break;
        }
    }

  return status;
}

/*******************************************************************************************
 * Routine to find the subscriber descriptor given the data type. Sets up
 *pSubscriberPtr with a pointer to the subscriber if found, else sets it to
 *NULL.
 *******************************************************************************************/
static int32
_GetSubscriberGivenType (DSStreamCBPtr streamCBPtr, DSDataType streamType,
                         DSSubscriberPtr *pSubscriberPtr)
{
  long index;
  DSSubscriberPtr sp;

  /* Search the stream's subscribers for the one desired.
   */
  for (index = 0; index < streamCBPtr->numSubscribers; index++)
    {
      sp = streamCBPtr->subscriber + index;

      if (sp->dataType == streamType)
        {
          *pSubscriberPtr = sp;
          return kDSNoErr;
        }
    }

  /* We exited the loop without finding the desired subscriber.
   * Force the returned subscriber pointer to NULL and return
   * an appropriate error status.
   */
  *pSubscriberPtr = NULL;
  return kDSSubNotFoundErr;
}

/*******************************************************************************************
 * Routine to delete the subscriber from the subscriber table.
 *******************************************************************************************/
static void
_DeleteSubscriber (DSStreamCBPtr streamCBPtr, DSSubscriberPtr sp)
{
  long numToMove;

  /* The number of entries to slide down is calculated as the
   * total number in the table less the index of the one we
   * are removing, less 1.
   */
  numToMove = streamCBPtr->numSubscribers - (sp - streamCBPtr->subscriber) - 1;

  while (numToMove-- > 0)
    {
      *(sp + 1) = *sp;
      sp++;
    }

  /* One less entry in the table */
  streamCBPtr->numSubscribers--;
}

/*******************************************************************************************
 * Utility routine to implement a stream abort.
 *******************************************************************************************/
static void
_AbortStream (DSStreamCBPtr streamCBPtr)
{
  SubscriberMsg subMsg;

  /* Prevent any additional data queuing */
  streamCBPtr->streamFlags &= STRM_STOPPED;

  subMsg.whatToDo = kStreamOpAbort;
  subMsg.privatePtr = NULL;

  /* Tell all subscribers to give up (abort) */
  _ForEachSubscriber (streamCBPtr, &subMsg);
}

/*******************************************************************************************
 * Utility routine to tear down a stream control block. Here, we only dispose
 *of the non-system resources that were created when the stream was created.
 *The system is supposed to be responsible for removing all system resources we
 *created, so we let it do its own cleanup.
 *******************************************************************************************/
static void
_TearDownStreamCB (DSStreamCBPtr streamCBPtr)
{
  DeleteMemPool (streamCBPtr->subsMsgPool);
  DeleteMemPool (streamCBPtr->dataMsgPool);
}

/*******************************************************************************************
 * Utility routine to place a buffer onto a stream's free buffer queue.
 *******************************************************************************************/
static void
_FreeDataBuf (DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
{
  /* Place free'd buffer onto front of singly linked
   * free buffer list.
   */
  bp->next = streamCBPtr->freeBufHead;
  streamCBPtr->freeBufHead = bp;

  /* Increment the number of free buffers we posess */
  streamCBPtr->currentFreeBufferCount++;

  /* If we are the end of file with data acquisition, and we have
   * all of our buffers back in our posession, then we have arrived
   * at the "end of stream" condition. Send a message to the registrant.
   */
  if ((streamCBPtr->endOfStreamMsg != NULL)
      && (streamCBPtr->streamFlags & STRM_EOF)
      && (streamCBPtr->currentFreeBufferCount
          == streamCBPtr->totalBufferCount))
    {
      /* Reply to registrant to notify of the end of stream condition */
      ReplyMsg (streamCBPtr->endOfStreamMsg->msgItem, kDSNoErr,
                streamCBPtr->endOfStreamMsg, sizeof (DSRequestMsg));

      /* Caller must re-register for end of stream each time since we
       * can only reply to the request one time.
       */
      streamCBPtr->endOfStreamMsg = NULL;
    }
}

/*******************************************************************************************
 * Routine to return a pointer to a free data buffer. Called by the buffer
 *filling code that will schedule disk reads into these buffers.
 *******************************************************************************************/
static DSDataBufPtr
_GetFreeDataBuf (DSStreamCBPtr streamCBPtr)
{
  DSDataBufPtr bp;

  /* Take the first free buffer in the queue */
  bp = streamCBPtr->freeBufHead;

  /* If one exists, update the list head pointer, set the use
   * count to zero, and fill in the stream pointer field.
   */
  if (bp != NULL)
    {
      streamCBPtr->freeBufHead = bp->next; /* forward link or NULL */
      bp->useCount = 0;
      bp->streamCBPtr = streamCBPtr;

      /* Decrement the number of free buffers we posess */
      streamCBPtr->currentFreeBufferCount--;
    }

  return bp;
}

/*******************************************************************************************
 * Routine to return a pointer to a filled data buffer. Called by the buffer
 *delivery code that will dispatch chunks from these buffers. Returns NULL if
 *there are no more filled data buffers in the list.
 *******************************************************************************************/
static DSDataBufPtr
_GetFilledDataBuf (DSStreamCBPtr streamCBPtr)
{
  DSDataBufPtr bp;

  if ((bp = streamCBPtr->filledBufHead) != NULL)
    {
      /* Update the head of the filled buffer list to point to the
       * next available buffer in the list, if any.
       */
      streamCBPtr->filledBufHead = bp->next;

      /* If we just removed the tail buffer from the list, make sure
       * to set the tail pointer to NULL so further queuing will do
       * the right thing.
       */
      if (streamCBPtr->filledBufTail == bp)
        streamCBPtr->filledBufTail = NULL;
    }

  return bp;
}

/*******************************************************************************************
 * Utility routine to fill an empty data buffer with data. Routine either sends
 *a request to the data acquisition client, or queues the free buffer onto the
 *stream's free buffer list. The latter happens when the stream is stopping, at
 *EOF, or has had some other problem.
 *******************************************************************************************/
static int32
_FillDataBuf (DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
{
  DataAcqMsgPtr acqMsg;
  int32 status;

  /* If we're not at end of file, then attempt to queue the buffer
   * back to the data acquisition client to be filled again. Otherwise
   * place the buffer back onto the free queue and return success.
   */
  if ((streamCBPtr->acquirePort != 0)
      && (streamCBPtr->streamFlags & STRM_RUNNING)
      && !(streamCBPtr->streamFlags & STRM_EOF))
    {
      acqMsg = (DataAcqMsgPtr)AllocPoolMem (streamCBPtr->dataMsgPool);

      if (acqMsg != NULL)
        {
          /* Inform the data acquisition module that a stream is opening. Send
           * a message to the acquisition port and await its reply.
           */
          acqMsg->whatToDo = kAcqOpGetData;
          acqMsg->privatePtr = NULL;
          acqMsg->msg.data.bufferPtr = bp;
          acqMsg->msg.data.bufferSize = streamCBPtr->bufDataSize;

          status = SendMsg (streamCBPtr->acquirePort, acqMsg->msgItem, acqMsg,
                            sizeof (DataAcqMsg));
          if (status < 0)
            return status;
        }
      else
        return kDSNoMsgErr;
    }
  else
    /* Add buffer to the free list */
    _FreeDataBuf (streamCBPtr, bp);

  return kDSNoErr;
}

/*******************************************************************************************
 * Utility routine to record the end of use of a chunk in a buffer. This
 *routine is called by DSHRunStream() when subscribers reply to their "data
 *arriving" messages. A count is kept for each physical I/O block of the number
 *of outstanding owners of chunks in that block. When the count is decremented
 *to zero, the block is sent to data acquisition to be refilled.
 *******************************************************************************************/
static int32
_ReleaseChunk (DSStreamCBPtr streamCBPtr, int32 subscriberStatus,
               DSDataBufPtr bp)
{
  int32 status;

  status = kDSNoErr;

  /* Decrement block's use count, and put it back into the
   * free list if there are no other users (use count == 0).
   */
  if (--bp->useCount == 0)
    {
      /* Get a pointer to the owning stream's control block */
      streamCBPtr = bp->streamCBPtr;

      /* If the subscriber is not aborting us and data delivery is
       * still enabled (stream is running), queue the buffer for another
       * read and keep the data coming...
       */
      if (subscriberStatus == kDSNoErr)
        status = _FillDataBuf (streamCBPtr, bp);
    }

  /* Make sure we haven't seen more replies for this buffer
   * than there were messages in it.
   */
  if (bp->useCount < 0)
    {
      ERR (("_ReleaseChunk() - use count negative!!!\n"));
      _AbortStream (streamCBPtr);
    }

  /* Since the subscriber may be informing us of an error condition, pass
   * the subscriber's input status to all other subscribers if something
   * other than 'success' is indicated.
   */
  if ((subscriberStatus != kDSNoErr) || (status != kDSNoErr))
    _AbortStream (streamCBPtr);

  return kDSNoErr;
}

/*******************************************************************************************
 * Utility routine to add a streamer request message onto the tail of the
 *streamer's request queue.
 *******************************************************************************************/
static void
_AddDSRequestToTail (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  reqMsg->link = NULL;

  if (streamCBPtr->requestMsgHead != NULL)
    {
      /* Add the new message onto the end of the
       * existing list of queued messages.
       */
      streamCBPtr->requestMsgTail->link = (void *)reqMsg;
      streamCBPtr->requestMsgTail = reqMsg;
    }
  else
    {
      /* Add the message to an empty queue  */
      streamCBPtr->requestMsgHead = reqMsg;
      streamCBPtr->requestMsgTail = reqMsg;
    }
}

/*******************************************************************************************
 * Utility routine to remove the first entry from the streamer request queue
 *and return a point to it. If the queue is empty, returns NULL.
 *******************************************************************************************/
static DSRequestMsgPtr
_GetNextDSRequest (DSStreamCBPtr streamCBPtr)
{
  DSRequestMsgPtr reqMsg;

  if ((reqMsg = streamCBPtr->requestMsgHead) != NULL)
    {
      /* Advance the head pointer to point to the next entry
       * in the list.
       */
      streamCBPtr->requestMsgHead = (DSRequestMsgPtr)reqMsg->link;

      /* If we are removing the tail entry from the list, set the
       * tail pointer to NULL.
       */
      if (streamCBPtr->requestMsgTail == reqMsg)
        streamCBPtr->requestMsgTail = NULL;
    }

  return reqMsg;
}

/*==========================================================================================
  ==========================================================================================
                                                Message handlers for the Data
  Stream Thread
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Routine to get channel status bits. Note that this call is routed through
 *DSH such that runtime binding is available for commonly used subscriber
 *functions. This will permit the creation of easily replacable subscriber
 *components. The subscriber proc gets called with whatToDo == kStreamOpGetChan
 *******************************************************************************************/
static int32
DoGetChannel (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DSSubscriberPtr sp;
  SubscriberMsgPtr subMsg;

  /* Find the subscriber descriptor given the data type
   */
  status = _GetSubscriberGivenType (streamCBPtr,
                                    reqMsg->msg.getChannel.streamType, &sp);
  if (status != kDSNoErr)
    return status;

  /* Allocate a message and send the request to the subscriber */
  if ((subMsg = ((SubscriberMsgPtr)AllocPoolMem (streamCBPtr->subsMsgPool)))
      != NULL)
    {
      subMsg->whatToDo = kStreamOpGetChan;
      subMsg->privatePtr = reqMsg;
      subMsg->msg.channel.number = reqMsg->msg.getChannel.channelNumber;
      subMsg->msg.channel.status = 0; /* not used for 'get' */

      /* Send the message to a subscriber */
      status = SendMsg (sp->subscriberPort, subMsg->msgItem, subMsg,
                        sizeof (SubscriberMsg));

      /* If we successfully sent the request, assert that we are awaiting
       * one reply before completing the current streamer request.
       */
      if (status >= 0)
        streamCBPtr->repliesPending = 1;
      else
        streamCBPtr->repliesPending = 0;
    }
  else
    status = kDSNoMsgErr;

  return status;
}

/*******************************************************************************************
 * Routine to ask subscriber for the status of a given logical channel.
 * The subscriber proc gets called with whatToDo == kStreamOpSetChan
 *******************************************************************************************/
static int32
DoSetChannel (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DSSubscriberPtr sp;
  SubscriberMsgPtr subMsg;

  /* Find the subscriber descriptor given the data type
   */
  status = _GetSubscriberGivenType (streamCBPtr,
                                    reqMsg->msg.setChannel.streamType, &sp);
  if (status != kDSNoErr)
    return status;

  /* Allocate a message and send the request to the subscriber */
  if ((subMsg = ((SubscriberMsgPtr)AllocPoolMem (streamCBPtr->subsMsgPool)))
      != NULL)
    {
      subMsg->whatToDo = kStreamOpSetChan;
      subMsg->privatePtr = reqMsg;
      subMsg->msg.channel.number = reqMsg->msg.setChannel.channelNumber;
      subMsg->msg.channel.status = reqMsg->msg.setChannel.channelStatus;

      /* Send the message to a subscriber */
      status = SendMsg (sp->subscriberPort, subMsg->msgItem, subMsg,
                        sizeof (SubscriberMsg));

      /* If we successfully sent the request, assert that we are awaiting
       * one reply before completing the current streamer request.
       */
      if (status >= 0)
        streamCBPtr->repliesPending = 1;
      else
        streamCBPtr->repliesPending = 0;
    }
  else
    status = kDSNoMsgErr;

  return status;
}

/*******************************************************************************************
 * Routine to ask subscriber to perform some subscriber defined function.
 * The subscriber proc gets called with whatToDo == kStreamOpControl
 *******************************************************************************************/
static int32
DoControl (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DSSubscriberPtr sp;
  SubscriberMsgPtr subMsg;

  /* Find the subscriber descriptor given the data type
   */
  status = _GetSubscriberGivenType (streamCBPtr,
                                    reqMsg->msg.control.streamType, &sp);
  if (status != kDSNoErr)
    return status;

  /* Allocate a message and send the request to the subscriber */
  if ((subMsg = ((SubscriberMsgPtr)AllocPoolMem (streamCBPtr->subsMsgPool)))
      != NULL)
    {
      subMsg->whatToDo = kStreamOpControl;
      subMsg->privatePtr = reqMsg;
      subMsg->msg.control.controlArg1 = reqMsg->msg.control.userDefinedOpcode;
      subMsg->msg.control.controlArg2 = reqMsg->msg.control.userDefinedArgPtr;

      /* Send the message to a subscriber */
      status = SendMsg (sp->subscriberPort, subMsg->msgItem, subMsg,
                        sizeof (SubscriberMsg));

      /* If we successfully sent the request, assert that we are awaiting
       * one reply before completing the current streamer request.
       */
      if (status >= 0)
        streamCBPtr->repliesPending = 1;
      else
        streamCBPtr->repliesPending = 0;
    }
  else
    status = kDSNoMsgErr;

  return status;
}

/*******************************************************************************************
 * Routine to discontinue the use of an open stream. Passes the "closing"
 *message to all subscribers.
 *******************************************************************************************/
static int32
DoCloseStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DataAcqMsgPtr acqMsg;
  SubscriberMsg subMsg;

  status = kDSNoErr;

  /* Inform the data acquisition module that a stream is closing.
   * This lets the data acquisition module do any post processing on
   * the buffer list (like free any allocated I/O request items).
   */
  acqMsg = (DataAcqMsgPtr)AllocPoolMem (streamCBPtr->dataMsgPool);
  if (acqMsg != NULL)
    {
      acqMsg->whatToDo = kAcqOpDisconnect;
      acqMsg->privatePtr = NULL;

      status = SendMsg (streamCBPtr->acquirePort, acqMsg->msgItem, acqMsg,
                        sizeof (DataAcqMsg));
    }
  else
    status = kDSNoMsgErr;

  /* Tell each subscriber that the stream is being closed
   */
  subMsg.whatToDo = kStreamOpClosing;
  subMsg.privatePtr = reqMsg;

  _ForEachSubscriber (streamCBPtr, &subMsg);

#if !EXIT_ONLY_AFTER_CLOSE_REPLIES
  _TearDownStreamCB (streamCBPtr);
#endif

  return status;
}

/*******************************************************************************************
 * Routine to add a data subscriber to the subscriber list for a given stream.
 *Afterwards, all data designated as belonging to this subscriber (by dataType)
 *will be delivered to the subscriber's message port.
 *******************************************************************************************/
static int32
DoSubscribe (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DSSubscriberPtr sp;
  SubscriberMsgPtr subMsg;

  /* First make sure the specified subscriber doesn't already
   * exist. If so, check to see if the subscriber is resigning and,
   * if so, delete the subscriber, else return an error.
   */
  status = _GetSubscriberGivenType (streamCBPtr,
                                    reqMsg->msg.subscribe.dataType, &sp);
  if (status != kDSSubNotFoundErr)
    {
      /* Subscriber was really there after all. See if it wants
       * to be removed, otherwise this is an error.
       */
      if (reqMsg->msg.subscribe.subscriberPort == 0)
        {
          _DeleteSubscriber (streamCBPtr, sp);
          return kDSNoErr;
        }
      else
        return kDSSubDuplicateErr;
    }

  /* See if there's room for another subscriber in the
   * subscriber table for this stream.
   */
  if (streamCBPtr->numSubscribers >= DS_MAX_SUBSCRIBERS)
    return kDSSubMaxErr;

  /* Get a pointer to the next free subscriber slot */
  sp = streamCBPtr->subscriber + streamCBPtr->numSubscribers;

  /* Increase the subscriber count */
  streamCBPtr->numSubscribers++;

  /* Fill in the subscriber structure */
  sp->dataType = reqMsg->msg.subscribe.dataType;
  sp->subscriberPort = reqMsg->msg.subscribe.subscriberPort;

  /* Give the subscriber an initialization call so it can
   * perform any initialization it requires.
   */
  if ((subMsg = ((SubscriberMsgPtr)AllocPoolMem (streamCBPtr->subsMsgPool)))
      != NULL)
    {
      subMsg->whatToDo = kStreamOpOpening;
      subMsg->privatePtr = reqMsg;

      /* Send the message to the new subscriber */
      status = SendMsg (sp->subscriberPort, subMsg->msgItem, subMsg,
                        sizeof (SubscriberMsg));

      /* If we successfully sent the request, assert that we are awaiting
       * one reply before completing the current streamer request.
       */
      if (status >= 0)
        streamCBPtr->repliesPending = 1;
      else
        streamCBPtr->repliesPending = 0;
    }
  else
    status = kDSNoMsgErr;

  return status;
}

/*******************************************************************************************
 * Routine to cause empty buffers to be prefilled with data before starting a
 *stream.
 *******************************************************************************************/
static int32
DoPreRollStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DSDataBufPtr bp;
  DataAcqMsgPtr acqMsg;

  status = kDSNoErr;

  /* Fill all free buffers with data */

  while ((bp = _GetFreeDataBuf (streamCBPtr)) != NULL)
    {
      acqMsg = (DataAcqMsgPtr)AllocPoolMem (streamCBPtr->dataMsgPool);

      if (acqMsg != NULL)
        {
          /* Ask data acquisition to fill a data buffer.
           */
          acqMsg->whatToDo = kAcqOpGetData;
          acqMsg->privatePtr = NULL;
          acqMsg->msg.data.bufferPtr = bp;
          acqMsg->msg.data.bufferSize = streamCBPtr->bufDataSize;

          status = SendMsg (streamCBPtr->acquirePort, acqMsg->msgItem, acqMsg,
                            sizeof (DataAcqMsg));
          if (status < 0)
            {
              CHECKRESULT ("DoPreRollStream()", status);
              break;
            }
        }
      else
        {
          status = kDSNoMsgErr;
          break;
        }
    }

  return status;
}

/*******************************************************************************************
 * Routine to enable the flow of buffered data to registered subscribers.
 *******************************************************************************************/
static int32
DoStartStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  SubscriberMsg subMsg;

  if (streamCBPtr->streamFlags & STRM_RUNNING)
    return kDSWasRunningErr;

  /* Allow data queuing, but declare the stream clock to be
   * invalid until something comes along and sets it.
   */
  streamCBPtr->streamFlags |= STRM_RUNNING;
  streamCBPtr->streamFlags &= ~STRM_CLOCK_VALID;

#if STARTSTREAM_SETS_THE_CLOCK
  /* If the clock was set at some point in time, reset the stream
   * clock to where we were when we left off.
   */
  if (streamCBPtr->streamFlags & STRM_CLOCK_WAS_VALID)
    {
      DSSetClock (streamCBPtr, streamCBPtr->lastValidClock);
    }
#endif

  /* Tell each subscriber that the stream is being started
   */
  subMsg.whatToDo = kStreamOpStart;
  subMsg.privatePtr = reqMsg;
  subMsg.msg.start.options = reqMsg->msg.start.options;

  _ForEachSubscriber (streamCBPtr, &subMsg);

  return kDSNoErr;
}

/*******************************************************************************************
 * Routine to cause streaming to stop. This causes the "stop" message to be
 *propagated to all subscribers and to the data acquisition proc.
 *******************************************************************************************/
static int32
DoStopStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  SubscriberMsg subMsg;

  status = kDSNoErr;

#if STARTSTREAM_SETS_THE_CLOCK
  /* If the clock was set at some point in time, remember
   * where we are in the stream so we can reset the clock upon
   * getting a start.
   */
  if (streamCBPtr->streamFlags & STRM_CLOCK_WAS_VALID)
    {
      DSGetClock (streamCBPtr, &streamCBPtr->lastValidClock);
    }
#endif

  /* Prevent additional data queuing, declare the stream
   * clock to be invalid.
   */
  streamCBPtr->streamFlags &= STRM_STOPPED;

  /* Tell each subscriber that the stream is being stopped
   */
  subMsg.whatToDo = kStreamOpStop;
  subMsg.privatePtr = reqMsg;
  subMsg.msg.stop.options = reqMsg->msg.stop.options;

  _ForEachSubscriber (streamCBPtr, &subMsg);

  return kDSNoErr;
}

/*******************************************************************************************
 * Routine to propagate a positioning operation through the system. This
 *consists of flushing any queued, completed data buffers and sending a
 *positioning message to the data acquisition client.
 *******************************************************************************************/
static int32
DoGoMarker (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DataAcqMsgPtr acqMsg;
  DSDataBufPtr bp;

  acqMsg = (DataAcqMsgPtr)AllocPoolMem (streamCBPtr->dataMsgPool);

  if (acqMsg != NULL)
    {
      /* Clear the EOF, if any, so that we will queue new
       * read requests if we were previously at end of file.
       */
      streamCBPtr->streamFlags &= ~STRM_EOF;

      /* Flush all filled buffers we currently own to prevent their
       * parsing and delivery. Simply move them to the free queue.
       */
      while ((bp = _GetFilledDataBuf (streamCBPtr)) != NULL)
        _FreeDataBuf (streamCBPtr, bp);

      /* Request the data acquisition client to re-position the stream.
       */
      acqMsg->whatToDo = kAcqOpGoMarker;
      acqMsg->privatePtr = reqMsg;
      acqMsg->msg.marker.value = reqMsg->msg.goMarker.markerValue;
      acqMsg->msg.marker.options = reqMsg->msg.goMarker.options;

      status = SendMsg (streamCBPtr->acquirePort, acqMsg->msgItem, acqMsg,
                        sizeof (DataAcqMsg));

      /* The "branch in progress" flag gets set externally to this
       * process by data acquisition once the translation of the branch
       * request has been made. The resulting stream time is saved in the
       * stream control block by a call to DSSetBranchDest(), and the
       * "branch in progress" flag is set then.
       */
      if (status >= 0)
        streamCBPtr->repliesPending = 1;
      else
        streamCBPtr->repliesPending = 0;
    }
  else
    status = kDSNoMsgErr;

  return status;
}

/*******************************************************************************************
 * Routine to connect a new data acquisition to the stream. First, if we are
 *currently connected we send a disconnect message to the current data
 *acquisition client. Then, we switch the aquireport in the stream control
 *block to the new port and send a connect message to the new data supplier.
 *The disconnect message should cause the first data supplier to cancel any
 *outstanding I/O's and send them back to us as "flushed", which we will simply
 *ignore and place back into the free buffer pool. We take care of flushing any
 *data we currently have queued.
 *******************************************************************************************/
static int32
DoConnect (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  int32 status;
  DSDataBufPtr bp;
  DataAcqMsgPtr acqMsg;

  /* If we are connecting to the same stream again, disregard
   * the connect request (works even if there's no current stream).
   */
  if (streamCBPtr->acquirePort == reqMsg->msg.connect.acquirePort)
    return kDSNoErr;

  /* If we are currently connected, tell the data acquisition thread that
   * we want to disconnect. This will cause it to return any of our buffers
   * that are "in progress" as with "flushed" status.
   */
  if (streamCBPtr->acquirePort != 0)
    {
      /* Send a disconnect message to the data acquisition client.
       */
      acqMsg = (DataAcqMsgPtr)AllocPoolMem (streamCBPtr->dataMsgPool);
      if (acqMsg != NULL)
        {
          acqMsg->whatToDo = kAcqOpDisconnect;
          acqMsg->privatePtr = NULL;

          status = SendMsg (streamCBPtr->acquirePort, acqMsg->msgItem, acqMsg,
                            sizeof (DataAcqMsg));
        }
      else
        status = kDSNoMsgErr;

      /* If something went wrong, bail out here.
       */
      if (status < 0)
        return status;
    }

  /* Flush any filled buffers we currently own to prevent their
   * parsing and delivery. Simply move them to the free queue.
   */
  while ((bp = _GetFilledDataBuf (streamCBPtr)) != NULL)
    _FreeDataBuf (streamCBPtr, bp);

  /* Switch the streamer's data acquisition port to the new port
   * specified.
   */
  streamCBPtr->acquirePort = reqMsg->msg.connect.acquirePort;

  /* Next, connect to the new port, if any. If we are given a NULL port as
   * input, then we are done (and all we've done is disconnect the existing
   * connection, if any).
   */
  if (streamCBPtr->acquirePort != 0)
    {
      /* Send a connect message to the new data acquisition client.
       */
      acqMsg = (DataAcqMsgPtr)AllocPoolMem (streamCBPtr->dataMsgPool);
      if (acqMsg != NULL)
        {
          acqMsg->whatToDo = kAcqOpConnect;
          acqMsg->privatePtr = reqMsg;
          acqMsg->msg.connect.streamCBPtr = streamCBPtr;

          status = SendMsg (streamCBPtr->acquirePort, acqMsg->msgItem, acqMsg,
                            sizeof (DataAcqMsg));

          /* If we successfully sent the request, assert that we are awaiting
           * one reply before completing the current streamer request.
           */
          if (status >= 0)
            streamCBPtr->repliesPending = 1;
          else
            streamCBPtr->repliesPending = 0;
        }
      else
        status = kDSNoMsgErr;
    }
  else
    status = kDSNoErr;

  return status;
}

/*******************************************************************************************
 * Routine to handle registration for end of stream notification.
 *******************************************************************************************/
static int32
DoRegisterEndOfStream (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
  if (streamCBPtr->endOfStreamMsg != NULL)
    {
      /* Reply to registrant to notify of losing its registration for
       * the end of stream condition. The new registrant gets the
       * notification when the condition happens.
       */
      ReplyMsg (streamCBPtr->endOfStreamMsg->msgItem, kDSEOSRegistrationErr,
                streamCBPtr->endOfStreamMsg, sizeof (DSRequestMsg));
    }

  /* Remember the address of the message to reply to later
   * when the end of stream condition occurs.
   */
  streamCBPtr->endOfStreamMsg = reqMsg;
  return kDSNoErr;
}

/*==========================================================================================
  ==========================================================================================
                                                        Data Streamer Init and
  Instantiation
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Routine to initialize all data acquisition activity. Called once at program
 *init time to allow allocation of context pool memory.
 *******************************************************************************************/
int32
InitDataStreaming (long maxNumberOfStreams)
{
  /* Create the memory pool for allocating data stream
   * contexts.
   */
  DSGlobals.streamContextPool
      = CreateMemPool (maxNumberOfStreams, sizeof (DSStreamCB));
  if (DSGlobals.streamContextPool == NULL)
    return kDSNoMemErr;

  /* Return success */
  return kDSNoErr;
}

/*******************************************************************************************
 * Routine to clean up global data streaming resources, called after all data
 *streaming had been shut down and is no longer needed. Currently, just
 *deallocates context block pool.
 *******************************************************************************************/
int32
CloseDataStreaming (void)
{

  DeleteMemPool (DSGlobals.streamContextPool);
  DSGlobals.streamContextPool = NULL;

  return kDSNoErr;
}

/*******************************************************************************************
 * Routine to instantiate a new data acquirer. Creates all necessary resoures
 *and starts the data acquisition thread running. Called to create a stream
 *interface for a data streaming file.
 *******************************************************************************************/
int32
NewDataStream (DSStreamCBPtr *pCtx, void *bufferListPtr, long bufferSize,
               long deltaPriority, long numSubsMsgs)
{
  int32 status;
  DSStreamCBPtr ctx;
  uint32 signalBits;

  /* Allocate a context */

  ctx = (DSStreamCBPtr)AllocPoolMem (DSGlobals.streamContextPool);
  if (ctx == NULL)
    return kDSNoMemErr;

  /* Pass creation arguments to the thread via the context block
   */
  ctx->bufDataSize = bufferSize;
  ctx->freeBufHead = (DSDataBufPtr)bufferListPtr;
  ctx->numSubsMsgs = numSubsMsgs;

  /* Allocate a signal to synchronize with the completion of the
   * initialization. The thread task will signal us with this after
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
      (void *)&DataStreamThread,  /* thread entrypoint */
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

  ReturnPoolMem (DSGlobals.streamContextPool, ctx);

  return status;
}

/*******************************************************************************************
 * Routine to format a "close stream" message and send it to the data stream
 *thread. Then we return the stream context back to the free pool.
 *******************************************************************************************/
int32
DisposeDataStream (Item msgItem, DSStreamCBPtr streamCBPtr)
{
  int32 status;
  DSRequestMsg reqMsg;

  reqMsg.whatToDo = kDSOpCloseStream;

  status = _SendRequestToDSThread (msgItem, false, /* wait for completion */
                                   streamCBPtr, &reqMsg);

  /* Get rid of the thread's resources */

  if (streamCBPtr->threadItem > 0)
    DisposeThread (streamCBPtr->threadItem);
  if (streamCBPtr->threadStackBlock)
    FreePtr (streamCBPtr->threadStackBlock);

  /* Return the context block to the free pool */

  ReturnPoolMem (DSGlobals.streamContextPool, streamCBPtr);

  return status;
}

/*==========================================================================================
  ==========================================================================================
                                                                        Data
  Stream thread
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * This routine is called repeatedly with the replies of requests we have sent
 *to the subscribers. Some replies signify the end of use of a data chunk that
 *we have sent to subscribers, while others are replies to requests we have
 *posted to them.
 *******************************************************************************************/
static void
HandleSubscriberReply (DSStreamCBPtr streamCBPtr, SubscriberMsgPtr subMsg,
                       Message *messagePtr)
{
  int32 status;
  DSRequestMsgPtr reqMsg;

  /* If the subscriber reply's private pointer points to the streamer
   * request message at the head of the request queue (the current request)
   * then decrement the repliesPending counter. If the counter is zero at
   * the end, then reply to the request at the head of the request
   * queue, permitting the next request to begin processing.
   */
  if ((subMsg->privatePtr != NULL)
      && (subMsg->privatePtr == streamCBPtr->requestMsgHead))
    {
      /* Decrement the reply counter that says how many subscribers must
       * reply to the current request before we can tell the original requestor
       * that the process has completed. If we get to zero, then tell the
       * requestor the request has completed.
       */
      if (--streamCBPtr->repliesPending == 0)
        {
          /* Delete the streamer request message from the head of
           * the request queue.
           */
          reqMsg = _GetNextDSRequest (streamCBPtr);

          /* If the completing request is a get channel status operation,
           * then we have a little extra work to do. Copy the result from
           * the completion reply to the caller's buffer.
           */
          if (subMsg->whatToDo == kStreamOpGetChan)
            *(reqMsg->msg.getChannel.channelStatusPtr)
                = messagePtr->msg_Result;

          /* Reply to the current request to indicate that we are
           * finished processing it.
           */
          status = ReplyMsg (reqMsg->msgItem, messagePtr->msg_Result, reqMsg,
                             sizeof (DSRequestMsg));
          CHECKRESULT (
              "DataStreamThread - HandleSubscriberReply - ReplyMsg() ",
              status);
        }
    }

  else if (subMsg->whatToDo == kStreamOpData)
    {
      /* Subscriber is finished using a data chunk we sent it some time
       * ago. Release the chunk, perhaps freeing the stream buffer for
       * requeuing for another read operation. In this case, we have no
       * request to reply to as this is the completion of a subscriber
       * request that the streamer itself initiated.
       */
      _ReleaseChunk (streamCBPtr, messagePtr->msg_Result,
                     (DSDataBufPtr)subMsg->privatePtr);
    }

  /* Some other messages are originated by the streamer itself.
   * These are, for now, ignored when subscribers reply to them.
   * Any other message reply we receive at this time is inappropriate,
   * so dump an error message if we get one?!
   */
  else if ((subMsg->whatToDo != kStreamOpAbort)
           && (subMsg->whatToDo != kStreamOpEOF))
    {
      ERR (("DataStreamThread - unexpected subscriber reply, whatToDo = %ld\n",
            subMsg->whatToDo));
    }

  /* Give the message memory back to the pool */
  ReturnPoolMem (streamCBPtr->subsMsgPool, subMsg);
}

/*******************************************************************************************
 * This routine is called repeatedly with the replies of requests we have sent
 *to the data acquisition client. Some of these requests are simply requests to
 *fill additional empty data buffers, while others implement stream
 *positioning, etc. Some replies will cause the completion of streamer
 *requests, those that are implemented by data acquisition (as opposed to those
 *implemented by subscribers).
 *******************************************************************************************/
static void
HandleDataAcqReply (DSStreamCBPtr streamCBPtr, DataAcqMsgPtr acqMsg,
                    Message *messagePtr)
{
  int32 status;
  DSRequestMsgPtr reqMsg;
  DSDataBufPtr bp;

  /* If the dataacq reply's private pointer points to the streamer
   * request message at the head of the request queue (the current request)
   * then decrement the repliesPending counter. If the counter is zero at
   * the end, then reply to the request at the head of the request
   * queue, permitting the next request to begin processing.
   */
  if ((acqMsg->privatePtr != NULL)
      && (acqMsg->privatePtr == streamCBPtr->requestMsgHead))
    {
      /* Decrement the reply counter that says how many repliese must
       * be receieved before we can tell the original requestor
       * that the process has completed. If we get to zero, then tell the
       * requestor the request has completed.
       */
      if (--streamCBPtr->repliesPending == 0)
        {
          /* Delete the current streamer request message from the head of
           * the request queue.
           */
          reqMsg = _GetNextDSRequest (streamCBPtr);

          /* Reply to the current request to indicate that we are
           * finished processing it.
           */
          status = ReplyMsg (reqMsg->msgItem, messagePtr->msg_Result, reqMsg,
                             sizeof (DSRequestMsg));
          CHECKRESULT ("DataStreamThread - HandleDataAcqReply - ReplyMsg() ",
                       status);
        }
      else
        /* Since there is one and only one data acquisition client, we should
         *NEVER ever have a repliesPending > 1. The only streamer requests that
         *are actually processed by data acquisition currently are:
         *
         *			Streamer req		Data acq req
         *			------------		------------
         *			kDSOpGoMarker	==>	kAcqOpGoMarker
         *			kDSOpConnect	==> kAcqOpConnect
         *
         * Any other request type is some kind of weird state error?!
         */
        ERR (("DataStreamThread - HandleDataAcqReply - "
              "streamCBPtr->repliesPending wrong = %ld\n",
              streamCBPtr->repliesPending));
    }

  /* Not a reply resulting from a streamer request. See if it is a reply to
   * a request to fill a data buffer and handle it if so.
   */
  else if (acqMsg->whatToDo == kAcqOpGetData)
    {
      bp = acqMsg->msg.data.bufferPtr;

      /* Handle replies from the data acquisition client.
       */
      if (messagePtr->msg_Result == 0)
        {
          /* Add filled buffer onto the end of the filled buffer list. Update
           * the head pointer only if the list is currently empty.
           */
          bp->next = 0;

          if (streamCBPtr->filledBufHead == NULL)
            {
              streamCBPtr->filledBufHead = bp;
              streamCBPtr->filledBufTail = bp;
            }
          else
            {
              streamCBPtr->filledBufTail->next = bp;
              streamCBPtr->filledBufTail = bp;
            }
        }

      else
        /* Some kind of non-successful completion occurred. This may not really
         * be an error, per se, but some kind of special completion, like EOF.
         * Always return these buffers to the free pool for eventual re-use.
         */
        {
          _FreeDataBuf (streamCBPtr, bp);

          switch (messagePtr->msg_Result)
            {
            case kDSEndOfFileErr:
              /* Set a flag to prevent further read queuing */
              streamCBPtr->streamFlags |= STRM_EOF;
              break;

            case kDSWasFlushedErr:
              /* This happens to queued reads when a positioning
               * operation happens. This is really not an error,
               * just a special case condition. It also happens
               * when switching streams with DSConnect() if the
               * existing data acquisition has buffers in progress.
               */
              break;

            default:
              ERR (("DataStreamThread - HandleDataAcqReply - kAcqOpGetData "
                    "failed with msg_Result = %ld\n",
                    messagePtr->msg_Result));
              OUTPUTSYSERR (messagePtr->msg_Result);

              /* Prevent additional data queuing */
              streamCBPtr->streamFlags &= STRM_STOPPED;
              break;
            }
        }
    }

  /* Some other messages are originated by the streamer itself.
   * These are, for now, ignored when data acq's reply to them.
   * Any other message reply we receive at this time is inappropriate,
   * so dump an error message if we get one?!
   */
  else if (acqMsg->whatToDo != kAcqOpDisconnect)
    {
      ERR (("DataStreamThread - HandleDataAcqReply - unexpected dataacq "
            "reply, whatToDo = %ld\n",
            acqMsg->whatToDo));
    }

  /* Give the message memory back to the pool */
  ReturnPoolMem (streamCBPtr->dataMsgPool, acqMsg);
}

/*******************************************************************************************
 * Routine to deliver all data currently awaiting deliver, if any.
 *******************************************************************************************/
static int32
DeliverData (DSStreamCBPtr streamCBPtr)
{
  DSDataBufPtr bp;
  StreamChunkPtr cp;
  SubscriberMsgPtr subMsg;
  int32 status;
  DSSubscriberPtr sp;

  /* If the stream is not running currently, then do not attempt to send
   * the data to subscribers.
   */
  if (!(streamCBPtr->streamFlags & STRM_RUNNING))
    return kDSNoErr;

  /* Deliver all data currently in the filled buffer list.
   */
  while ((bp = _GetFilledDataBuf (streamCBPtr)) != NULL)
    {
      /* Deliver all subscriber data chunks. Loop through the entire buffer,
       * skipping from chunk to chunk. Look up each owner for a chunk and call
       * that subscriber with a pointer to the data.
       */
      bp->useCount = 0;
      cp = (StreamChunkPtr)&bp->streamData;
      while (
          ((char *)cp)
          < ((char *)(((char *)bp->streamData) + streamCBPtr->bufDataSize
                      - sizeof (StreamChunk) - sizeof (cp->streamChunkData))))
        {
          /* First, validate the chunk size. It must be QUADBYTE aligned
           * for the stream data to be considered well formed. If the
           * size is not aligned, the pointer arithmetic will get us
           * an unaligned memory reference (ouch!). Also, check for impossible
           * chunk sizes: zero or negative sizes.
           */
          if (NOT_QUADBYTE_ALIGNED (cp->streamChunkSize)
              || (((long)cp->streamChunkSize) <= 0))
            {
              _AbortStream (streamCBPtr);
              return kDSBadChunkSizeErr;
            }

          /* Try to find the associated subscriber for the type of
           * data contained in the current chunk.
           */
          status = _GetSubscriberGivenType (streamCBPtr, cp->streamChunkType,
                                            &sp);

          /* If there is a subscriber for this data, pass a pointer
           * to the data to its subscriber. Increment the use count for
           * the buffer (decremented when the release proc is called).
           */
          if (status == kDSNoErr)
            {
              subMsg = ((SubscriberMsgPtr)AllocPoolMem (
                  streamCBPtr->subsMsgPool));
              if (subMsg != NULL)
                {
                  subMsg->whatToDo = kStreamOpData;
                  subMsg->privatePtr = (void *)bp;
                  subMsg->msg.data.buffer = (void *)cp;

                  /* Send the message to a subscriber */
                  status = SendMsg (sp->subscriberPort, subMsg->msgItem,
                                    subMsg, sizeof (SubscriberMsg));
                }
              else
                status = kDSNoMsgErr;

              /* If something went wrong, pass an abort to all
               * interested parties, and then give up.
               */
              if (status < 0)
                {
                  Message *msgPtr;

#define DISCOVER_STUPID_OS_MSG_BUG 1
#if DISCOVER_STUPID_OS_MSG_BUG
                  msgPtr = (Message *)LookupItem (subMsg->msgItem);
#endif
                  _AbortStream (streamCBPtr);
                  return status;
                }

              /* Increment use count */
              bp->useCount++;
            }
          else
            /* We don't care about subscriber data types that
             * are unknown. We just ignore them...
             */
            status = kDSNoErr;

          /* Advance the chunk pointer to the next chunk in
           * the current buffer
           */
          cp = (StreamChunkPtr)(((char *)cp) + cp->streamChunkSize);
        }

      /* If no subscriber claims any of the chunks in the buffer, then
       * fill the buffer again with data and keep going. Note that the
       * following routine will simply queue the buffer back to the free
       * list if the stream is stopped or at end of file.
       */
      if (bp->useCount == 0)
        status = _FillDataBuf (streamCBPtr, bp);
    }

  return status;
}

/*******************************************************************************************
 * This routine is called repeatedly with request messages for the data stream
 *thread. It acts as an overall dispatcher to the routines which actually
 *implement the actions requested by callers of the DataStream Library. Each
 *message is a single request, and each must be replied to once processed so
 *that the request's sender may know when the request has been processed. This
 *allows the sender to operate either synchronously or asynchronously with the
 *DataStream Library.
 *******************************************************************************************/
static int32
HandleDSRequest (DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg,
                 Boolean *pFKeepRunning)
{
  int32 status;

  /* Add the new streamer request message to the tail of the request
   * message queue. Then, if we are not currently awaiting the completion of
   * a request, start the next operation in the request queue.
   *
   * NOTE:	We may queue a new entry, but then initiate whatever is
   *currently at the head of the queue.
   */

  /* Do a range check on the whatToDo field to prevent queuing of
   * messages that are bogus.
   */
  if ((reqMsg->whatToDo < 0) || (reqMsg->whatToDo > kStreamOpAbort))
    return kDSInvalidDSRequest;

  _AddDSRequestToTail (streamCBPtr, reqMsg);
  if (streamCBPtr->repliesPending > 0)
    return kDSNoErr;

  /* Whenever initiating a new action, always start with whatever request
   * is currently at the head of the queue, and set the number of replies
   * required before completing the new request to zero.
   */
  reqMsg = streamCBPtr->requestMsgHead;
  streamCBPtr->repliesPending = 0;

  switch (reqMsg->whatToDo)
    {
    case kDSOpPreRollStream:
      status = DoPreRollStream (streamCBPtr, reqMsg);
      break;

    case kDSOpCloseStream:
      status = DoCloseStream (streamCBPtr, reqMsg);
      (*pFKeepRunning) = false;
      break;

    case kDSOpWaitEndOfStream:
      status = DoRegisterEndOfStream (streamCBPtr, reqMsg);
      break;

      /* The following message types have parameters passed
       * in the 'msg' extension of the request message.
       */

    case kDSOpStartStream:
      status = DoStartStream (streamCBPtr, reqMsg);
      break;

    case kDSOpStopStream:
      status = DoStopStream (streamCBPtr, reqMsg);
      break;

    case kDSOpSubscribe:
      status = DoSubscribe (streamCBPtr, reqMsg);
      break;

    case kDSOpGoMarker:
      status = DoGoMarker (streamCBPtr, reqMsg);
      break;

    case kDSOpGetChannel:
      status = DoGetChannel (streamCBPtr, reqMsg);
      break;

    case kDSOpSetChannel:
      status = DoSetChannel (streamCBPtr, reqMsg);
      break;

    case kDSOpControl:
      status = DoControl (streamCBPtr, reqMsg);
      break;

    case kDSOpConnect:
      status = DoConnect (streamCBPtr, reqMsg);
      break;

    default:
      status = kDSInvalidDSRequest;
    }

  /* If the request was completely processed, then reply to it
   * right away.
   */
  if (streamCBPtr->repliesPending == 0)
    {
      /* Delete the current streamer request message from the head of
       * the request queue.
       */
      reqMsg = _GetNextDSRequest (streamCBPtr);

      /* End of stream gets replied to when the condition actually occurrs,
       * so we avoid doing so here, even though repliesPending == zero.
       */
      if (reqMsg->whatToDo != kDSOpWaitEndOfStream)
        {
          /* Reply to the current request to indicate that we are
           * finished processing it.
           */
          status = ReplyMsg (reqMsg->msgItem, status, reqMsg,
                             sizeof (DSRequestMsg));
          CHECKRESULT ("DataStreamThread - HandleDSRequest - ReplyMsg() ",
                       status);
        }
    }

  return status;
}

/*******************************************************************************************
 * Utility routine to perform thread startup initialization. Handles all
 *resource setup and sets the context appropriately for communications with our
 *creator.
 *******************************************************************************************/
static int32
_InitializeThread (DSStreamCBPtr streamCBPtr)
{
  int32 status;
  DSDataBufPtr dp;
  long bufferCount;

  /* Initialize a few context block fields */

  streamCBPtr->creatorStatus
      = kDSInitErr; /* assume initialization will fail */
  streamCBPtr->streamFlags = 0;
  streamCBPtr->clockOffset = 0;
  streamCBPtr->numSubscribers = 0;
  streamCBPtr->filledBufHead = 0;
  streamCBPtr->filledBufTail = 0;
  streamCBPtr->acquirePort = 0;

  streamCBPtr->repliesPending = 0;
  streamCBPtr->requestMsgHead = 0;
  streamCBPtr->requestMsgTail = 0;

  /* Make sure that the buffer list we are passed contains only
   * QUADBYTE aligned buffers. Use this opportunity to insure that the
   * buffers we've been passed are properly aligned. Also, count the
   * buffers so we can allocate an appropriate amount of other resources.
   */
  bufferCount = 0;
  for (dp = streamCBPtr->freeBufHead; dp != NULL; dp = dp->next)
    {
      /* Check for QUADBYTE alignment.
       */
      if (NOT_QUADBYTE_ALIGNED (dp))
        return kDSBadBufAlignErr;

      dp->streamCBPtr = streamCBPtr;
      bufferCount++;
    }

  /* Initialize buffer counts. We keep track of the total number of
   * buffers and the number currently in the free list. We use this
   * count to know when we reach end of stream. This state is a combination
   * of reaching EOF on the data stream *and* knowing when, exactly, all
   * buffers have been drained (which is equivalent to knowing when all
   * buffers are back in the free list).
   */
  streamCBPtr->totalBufferCount = bufferCount;
  streamCBPtr->currentFreeBufferCount = bufferCount;
  streamCBPtr->endOfStreamMsg = NULL;

#if STARTSTREAM_SETS_THE_CLOCK
  /* Open the Audio Folio for this thread */
  if ((status = OpenAudioFolio ()) < 0)
    {
      streamCBPtr->creatorStatus = status;
      goto ABORTOPEN;
    }
#endif

  /* Create the message port which is used by any task wishing to
   * communicate work requests to this stream.
   */
  streamCBPtr->creatorStatus = kDSNoPortErr;
  streamCBPtr->requestPort = NewMsgPort (&streamCBPtr->requestPortSignal);
  if (streamCBPtr->requestPort <= 0)
    goto ABORTOPEN;

  /* Create the message port which is used to communicate with
   * the data acquisition module.
   */
  streamCBPtr->acqReplyPort = NewMsgPort (&streamCBPtr->acqReplyPortSignal);
  if (streamCBPtr->acqReplyPort <= 0)
    goto ABORTOPEN;

  /* Create the message port which is used to communicate with
   * the subscribers.
   */
  streamCBPtr->subsReplyPort = NewMsgPort (&streamCBPtr->subsReplyPortSignal);
  if (streamCBPtr->subsReplyPort <= 0)
    goto ABORTOPEN;

  /* Allocate a pool of message blocks that we will later send to
   * subscribers when we are passing their data to them.
   */
  streamCBPtr->creatorStatus = kDSNoMemErr;
  streamCBPtr->subsMsgPool
      = CreateMemPool (streamCBPtr->numSubsMsgs, sizeof (SubscriberMsg));
  if (streamCBPtr->subsMsgPool == NULL)
    goto ABORTOPEN;

    /* We need some additional data request messages in addition to the
     * one message needed for each data buffer. This is because we may need
     * to "talk" to data acquisition at a time when all buffers are in
     * progress. If we don't have some extra, we run out and die. There needs
     * to be one additional message for each type of async request we may need
     * to send. For now, these are: kAcqOpGoMarker, kAcqOpConnect,
     * kAcqOpDisconnect. We fudge and add a couple in case of back to back
     * requests...
     */
#define NUMBER_OF_ADDITIONAL_DATA_MSGS (3 * 2)

  streamCBPtr->dataMsgPool = CreateMemPool (
      NUMBER_OF_ADDITIONAL_DATA_MSGS + bufferCount, sizeof (DataAcqMsg));
  if (streamCBPtr->dataMsgPool == NULL)
    goto ABORTOPEN;

  /* Initialize each subscriber message pool entry with
   * the necessary system resources.
   */
  streamCBPtr->creatorStatus = kDSNoMsgErr;
  if (!FillPoolWithMsgItems (streamCBPtr->subsMsgPool,
                             streamCBPtr->subsReplyPort))
    goto ABORTOPEN;

  /* Initialize each data message with the necessary
   * system resources.
   */
  if (!FillPoolWithMsgItems (streamCBPtr->dataMsgPool,
                             streamCBPtr->acqReplyPort))
    goto ABORTOPEN;

  /* Looks like everything initialized just fine.
   */
  streamCBPtr->creatorStatus = 0;

ABORTOPEN:
  /* Inform our creator that we've finished with initialization. We are either
   * ready and able to accept requests for work, or we failed to initialize
   * ourselves. If the latter, we simply exit after informing our creator.
   * All resources we created are thankfully cleaned up for us by the system.
   */
  status = SendSignal (streamCBPtr->creatorTask, streamCBPtr->creatorSignal);
  if (status >= 0)
    status = streamCBPtr->creatorStatus;

  if (status < 0)
    {
      _TearDownStreamCB (streamCBPtr);
      return status;
    }
  else
    return 0;
}

/*******************************************************************************************
 * Main thread for running a data stream. Waits for new service requests, data
 *completion messages, and subscriber completion messages. Delivers any
 *received data to subscribers and repeats the entire thing forever. Process
 *exits when a "close" request is received or if some catestrophic error
 *occurrs.
 *******************************************************************************************/
static void
DataStreamThread (long notUsed, DSStreamCBPtr streamCBPtr)
{
  int32 status;
  DSRequestMsgPtr reqMsg;
  DataAcqMsgPtr acqMsg;
  SubscriberMsgPtr subMsg;
  DSDataBufPtr bp;
  Item msgItem;
  Message *messagePtr;
  uint32 signalBits;
  uint32 anySignal;
  Boolean fKeepRunning;

  /* Call a utility routine to perform all startup initialization.
   */
  if ((status = _InitializeThread (streamCBPtr)) != 0)
    exit (0);

  /* All resources are now allocated and ready to use. Our creator has been
   * informed that we are ready to accept requests for work. All that's left to
   * do is wait for work request messages to arrive.
   */
  anySignal = streamCBPtr->requestPortSignal | streamCBPtr->acqReplyPortSignal
              | streamCBPtr->subsReplyPortSignal;

  /***********************************************/
  /* Run the main thread loop until told to quit */
  /***********************************************/
  fKeepRunning = true;
  while (true)
    {
      /* Wait for a message on any of our message ports...
       */
      signalBits = WaitSignal (anySignal);

      /* Handle all work request messages. We do this first because someone
       * may be trying to shut off the stream or close it, and we want to
       * handle those requests before continuing to spew data.
       */
      if (signalBits & streamCBPtr->requestPortSignal)
        {
          while (PollForMsg (streamCBPtr->requestPort, NULL, NULL,
                             (void **)&reqMsg, &status))
            {
              status = HandleDSRequest (streamCBPtr, reqMsg, &fKeepRunning);
#if 1
              if (status != 0)
                printf ("reqMsg->whatToDo = %ld\n", reqMsg->whatToDo);
#endif
              CHECKRESULT ("DataStreamThread - HandleDSRequest() #1", status);

#if !EXIT_ONLY_AFTER_CLOSE_REPLIES
              /* If we're supposed to exit now as a result of a request, do
               * so...
               */
              if (fKeepRunning != true)
                goto THREAD_EXIT;
#endif
            }
        }

      /* Handle replies from subscribers (freeing data chunks, etc.)
       */
      if (signalBits & streamCBPtr->subsReplyPortSignal)
        {
          while (PollForMsg (streamCBPtr->subsReplyPort, &msgItem, &messagePtr,
                             (void **)&subMsg, &status))
            {
              HandleSubscriberReply (streamCBPtr, subMsg, messagePtr);

#if EXIT_ONLY_AFTER_CLOSE_REPLIES
              /* If a close stream request was previously received, and the
               * number of pending replies just got decremented to zero, then
               * we are now ready to exit because all subscribers have replied
               * to the close broadcast.
               */
              if ((fKeepRunning == false)
                  && (streamCBPtr->repliesPending == 0))
                goto THREAD_EXIT;
#endif
            }
        }

      /* Handle replies from data acquisition (arriving data, etc.)
       */
      if (signalBits & streamCBPtr->acqReplyPortSignal)
        {
          while (PollForMsg ((Item)streamCBPtr->acqReplyPort, &msgItem,
                             &messagePtr, (void **)&acqMsg, &status))
            {
              HandleDataAcqReply (streamCBPtr, acqMsg, messagePtr);
            }
        }

      /* Send out any data we have accumulated
       */
      status = DeliverData (streamCBPtr);
      CHECKRESULT ("DataStreamThread - DeliverData()", status);

      /* Try to fill any unfilled buffers that we are holding if and only if:
       * (1) there is currently a data acquisition client connected
       * (2) the stream is "running"
       * (3) the stream is not currently at EOF
       */
      if ((streamCBPtr->acquirePort != 0)
          && (streamCBPtr->streamFlags & STRM_RUNNING)
          && !(streamCBPtr->streamFlags & STRM_EOF))
        {
          while ((bp = _GetFreeDataBuf (streamCBPtr)) != NULL)
            _FillDataBuf (streamCBPtr, bp);
        }

      /* If there are no replies pending, then we may have just completed
       * a streamer request. If so, check to see if we can start another one.
       */
      if (streamCBPtr->repliesPending == 0)
        {
          if ((reqMsg = _GetNextDSRequest (streamCBPtr)) != NULL)
            {
              status = HandleDSRequest (streamCBPtr, reqMsg, &fKeepRunning);
              CHECKRESULT ("DataStreamThread - HandleDSRequest() #2", status);
            }
        }
    }

THREAD_EXIT:

#if EXIT_ONLY_AFTER_CLOSE_REPLIES
  _TearDownStreamCB (streamCBPtr);
#endif

  exit (0);
}
