/*******************************************************************************************
 *	File:			SAnimSubscriber.c
 *
 *	Contains:		Streaming Cel Animation subscriber
 *
 *	Written by:		Neil Cormia
 *
 *	Copyright й 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/03/94	dtc		Fix - Bug #3402: added code to arbitrate
 *access to the dataqueue in DoSAnimData. Fix - Bug 2164: IsTimeForNextFrame
 *fixed to make looping to beginning of stream work.
 *	 09/20/94	dtc		Fixed un-terminated comment in
 *IsTimeForNextFrame. Replaced printf with ERR macro defined in Debug3DO.h.
 *	 08/22/94	dtc		Version 2.0.1d4
 *						Fixed memory leak.
 *_InitalizeSAnimThread() was zeroing the ctx's stackBlock field so
 *DisposeSAnimSubscriber() was never able to free the stackBlock. 8/17/94
 *dtc		Version 2.0.1d3 Fixed IsTimeForNextFrame to delivery the first
 *frame on time. 7/29/94	dtc		Version 2.0.1d2 Replaced NULL
 *with "SAnimSubsThread" as thread name when creating SAnimSubscriber thread in
 *NewSAnimSubscriber.
 *  07/19/94	DLD		Fixed a low-mem-abort bug that I introduced on
 *7/17. 07/17/94	DLD		Added AMAP chunks. 07/17/94	DLD
 *Fixed a bug in GetSAnimCel where individual channels would start playing
 *their frames before it was time for them.  This code is dovetailed in with
 *the FREE_DATA_WHEN_TIME_HAS_PASSED code. 06/29/94	rdg		removed
 *include of CPakSubscriber.h.... 05/24/94	DLD		Changed CCB and
 *POS chunk definitions!  Now the X.Y positions are assumed to be fixed point
 *values, not integers. If your animations all come out at 0,0, then you must
 *rebuild with SHIFT_CCB_POSITIONS defined to get the old behavior. 05/23/94
 *fyp		Fixed a low-memory access problem caused by a nil curFramePtr
 *						at the end of GetSAnimCel.
 *	11/23/93	jb		Version 1.2
 *						Added CloseSAnimSubscriber(), a routine
 *to deallocate resources allocated by InitSAnimSubscriber(). 10/19/93	jb
 *Version 1.1 Switch to using UMemory.c routines for memory allocation. Add
 *code identified by FREE_DATA_WHEN_TIME_HAS_PASSED to optionally free streamer
 *buffers automatically once a buffer's time has expired. Expiration is
 *determined by buffer time+duration being less than the current stream clock.
 *	9/15/93		jb		Add support for APOS chunk subtype.
 *	8/18/93		jb		Changed curTime in IsTimeForNextFrame()
 *to a SIGNED value to make comparisons work correctly. 6/22/93 njc
 *Changed subscriber structure from one msgPort for every channel to a simple
 *linked list with a semaphore to arbitrate access to the list. In addition, a
 *freeList is maintained that contains chunks that are to be returned to the
 *Data Streaming Library.
 *	5/25/93		ec		Added SAnimDuration & SAnimCurrTime;
 *	5/25/93		njc		Version 1.0a
 *	5/19/93		ec		Added reset code to FlushSAnimChannel.
 *	4/23/93		njc		All sorts of new routines to prepare
 *for first testing. 4/22/93		njc		New today.
 *
 *******************************************************************************************/

#include "audio.h"
#include "debug.h"
#include "filefunctions.h"
#include "folio.h"
#include "graphics.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "task.h"
#include "time.h"
#include "types.h"

#include "UMemory.h"

#include "DataStreamLib.h"
#include "MemPool.h"
#include "MsgUtils.h"
#include "ThreadHelper.h"

#include "Audio.h"
#include "Debug3DO.h"
#include "Graphics.h"
#include "debug.h"

#include "Form3DO.h"
#include "Parse3DO.h"

#include "SAnimSubscriber.h"
#include "SubscriberUtils.h"

#include "stdlib.h"  /* for the exit() routine */
#include "strings.h" /* for the memcpy() routine */

/* The following compile switch will cause the subscriber to always return
 * buffers to the streamer once their time has expired. This must be set
 * for end of stream detection to work.
 */
#define FREE_DATA_WHEN_TIME_HAS_PASSED 1

/**********************/
/* Subscriber globals */
/**********************/

struct SAnimSubscriberGlobals
{

  MemPoolPtr SAnimContextPool; /* The memory pool for subscriber contexts */
  MemPoolPtr SAnimRecPool; /* The memory pool for subscriber anim records */

} SAGlobals;

/****************************/
/* Local routine prototypes */
/****************************/
Boolean PollSAnimChunk (SAnimContextPtr ctx, SAnimRecPtr saRecPtr,
                        SubsChunkDataPtr *pChunkDataPtr,
                        SubscriberMsgPtr *subMsg);
static CCB *SetupNextSAFrame (CCB *ccb, StreamAnimFramePtr pSAFrame);
static Boolean IsTimeForNextFrame (SAnimContextPtr ctx, SAnimRecPtr saRecPtr);

static int32 DoSAnimData (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimGetChan (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimSetChan (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimControl (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimOpening (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimClosing (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimStop (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimSync (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimEOF (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSAnimAbort (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static SubscriberMsgPtr DoGetChunk (SAnimContextPtr ctx, SAnimRecPtr saRecPtr);
static int32 DoFreeChunk (SAnimContextPtr ctx, SubscriberMsgPtr subMsg);
static void DoReplyChunksToDSL (SAnimContextPtr ctx);

int32 _InitializeSAnimThread (SAnimContextPtr ctx);

void SAnimSubscriberThread (int32 notUsed, SAnimContextPtr ctx);

/**************************/
/* Local utility routines */
/**************************/

void _StopSAnimChannel (SAnimContextPtr ctx, long channelNumber);
void _FlushChannel (SAnimContextPtr ctx, long channelNumber);

/*==========================================================================================
  ==========================================================================================
                                                        Subscriber procedural
  interfaces
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Poll for the next msg in this streamed animation and return true if a
 *kStreamOpData msg is present. Pass a ptr to the chunk back in pChunkDataPtr.
 *******************************************************************************************/
Boolean
PollSAnimChunk (SAnimContextPtr ctx, SAnimRecPtr saRecPtr,
                SubsChunkDataPtr *pChunkDataPtr, SubscriberMsgPtr *subMsg)
{

  /* Poll for the next msg in this streamed animation
   * and extract a chunkPtr. Return true if a subscriber
   * message is returned from DoGetChunk.
   */
  if ((*subMsg = DoGetChunk (ctx, saRecPtr)) != NULL)
    *pChunkDataPtr = (SubsChunkDataPtr)(*subMsg)->msg.data.buffer;
  else
    return false;

  /* Return the chunkPtr */
  return true;
}

/*******************************************************************************************
 * Allocate and initialize a streamed animation state record.
 * GetSAnimCel will return NULL until the Header, CCB, and first frame have
 *been received.
 *******************************************************************************************/
int32
InitSAnimCel (DSStreamCBPtr streamCBPtr, SAnimContextPtr ctx,
              SAnimRecPtr *pSARecPtr, int32 channel, Boolean flushOnSync)
{
  SAnimRecPtr saRecPtr;
  SAnimChannelPtr chanPtr;

  saRecPtr = (SAnimRecPtr)AllocPoolMem (SAGlobals.SAnimRecPool);
  if (saRecPtr == NULL)
    return kDSNoMemErr;

  saRecPtr->curFramePtr = NULL;
  saRecPtr->curSubMsg = NULL;
  saRecPtr->channel = channel;
  saRecPtr->streamCBPtr = streamCBPtr;
  saRecPtr->lastCurTime = 0;
  *pSARecPtr = saRecPtr;

  chanPtr = ctx->channel + channel;
  chanPtr->fFlushOnSync = flushOnSync;

  return 0;
}

/*******************************************************************************************
 *	Return all sanim msgs to the SAnimSubscriber back end. The sanim
 *subscriber will return one subscriber chunk msg to the Data Streamer for each
 *of these msgs returned.
 *******************************************************************************************/
void
FlushSAnimChannel (SAnimContextPtr ctx, SAnimRecPtr saRecPtr, int32 channel)
{
  SubsChunkDataPtr pChunkData;
  SubscriberMsgPtr subMsg;

  if (saRecPtr->curSubMsg)
    {
      DoFreeChunk (ctx, saRecPtr->curSubMsg);
      saRecPtr->curSubMsg = NULL;
    }

  while (PollSAnimChunk (ctx, saRecPtr, &pChunkData, &subMsg))
    DoFreeChunk (ctx, subMsg);

  saRecPtr->curFramePtr = NULL;
  DoReplyChunksToDSL (ctx);
}

/*******************************************************************************************
 *
 *******************************************************************************************/
int32
DestroySAnimCel (SAnimContextPtr ctx, SAnimRecPtr saRecPtr, int32 channel)
{

  FlushSAnimChannel (ctx, saRecPtr, channel);
  ReturnPoolMem (SAGlobals.SAnimRecPool, (void *)saRecPtr);

  return 0;
}

/*******************************************************************************************
 *	Parse the data and assign the source and PLUT fields in the CCB.
 *******************************************************************************************/
CCB *
SetupNextSAFrame (CCB *ccb, StreamAnimFramePtr pSAFrame)
{
  char *pChunk;
  char *tempBuf;
  int32 tempSize;
  ulong chunk_ID;
  int32 *pchunkSize;

  tempBuf = ((char *)pSAFrame) + sizeof (StreamAnimFrame);
  pchunkSize = &(pSAFrame->chunkSize); /*еее HACK FOR COMPILER PROBLEM еее*/
  tempSize = *pchunkSize - sizeof (StreamAnimFrame);

  while ((pChunk = GetChunk (&chunk_ID, &tempBuf, &tempSize)) != NULL)
    {
      switch (chunk_ID)
        {
        case CHUNK_PLUT:
          ccb->ccb_PLUTPtr = &(((PLUTChunk *)pChunk)->PLUT[0]);
          break;

        case CHUNK_PDAT:
          ccb->ccb_SourcePtr = (CelData *)&(((PixelChunk *)pChunk)->pixels[0]);
          break;

        default:
          ERR (
              ("SetupNextSAFrame --- Unexpected chunk ID: '%4.s'", &chunk_ID));
          return NULL;
        }
    }

  return ccb;
}

/*******************************************************************************************
 * Return true if time for new frame.
 *******************************************************************************************/
Boolean
IsTimeForNextFrame (SAnimContextPtr ctx, SAnimRecPtr saRecPtr)
{
  StreamAnimFramePtr curFramePtr;
  int32 curTime;

  if (DSGetClock (saRecPtr->streamCBPtr, (uint32 *)&curTime) < 0)
    return false;

  /* Don't request a frame unless it's time for the 1st frame. */
  if (saRecPtr->saHeader.time > curTime)
    return false;

  /* Get a ptr to the current frame data */
  curFramePtr = saRecPtr->curFramePtr;

  /* If no frames read yet then get a frame */
  if (curFramePtr == NULL)
    return true;

  /* Check to see if the stream time looped and force
   * a new frame if it did.
   */
  if (saRecPtr->lastCurTime > curTime)
    {
      saRecPtr->lastCurTime = curFramePtr->time;
      return true;
    }

  saRecPtr->lastCurTime = curTime;

  /* If the current frame should still be displayed, return false.
   * If the current frame's starting time plus its duration are less
   * than the current time, then we need a new frame, so return true.
   */
  return (curTime > (curFramePtr->time + curFramePtr->duration)) ? true
                                                                 : false;
}

/*******************************************************************************************
 * Send a signal to the SAnim Subscriber telling it to Reply all subMsgs in the
 *freeList back to the DSL. This is necessary to release physical blocks for
 *re-use.
 *******************************************************************************************/
int32
SendFreeSAnimSignal (SAnimContextPtr ctx)
{
  int32 status = 0;

  if (ctx->freeListNotEmpty)
    {
      status = SendSignal (ctx->sanimTask, ctx->freeQueueSignal);
      ctx->freeListNotEmpty = false;
    }

  return status;
}

/*******************************************************************************************
 * Return the duration of the sanim (in 240ths), or -1 of the sanim frame
 *hasn't been read yet.  NOTE: this function returns the time of the BEGINING
 *of the last frame
 *******************************************************************************************/
int32
SAnimDuration (SAnimRecPtr saRecPtr)
{
  int32 frameRateAudioTime;

  frameRateAudioTime = 240 / saRecPtr->saHeader.frameRate;
  return (saRecPtr->saHeader.numFrames * frameRateAudioTime)
         - frameRateAudioTime;
}

/*******************************************************************************************
 * Return the current time of the sanim (in 240ths), or -1 of the sanim frame
 *hasn't been read yet.
 *******************************************************************************************/
int32
SAnimCurrTime (SAnimRecPtr saRecPtr)
{
  register StreamAnimFramePtr curFramePtr;

  /* Get a ptr to the current frame data */
  curFramePtr = saRecPtr->curFramePtr;

  /* If no frames read yet then can't tell what time it is */
  if (curFramePtr == NULL)
    return -1;

  return curFramePtr->time;
}

/*******************************************************************************************
 * Get the next Cel in the streamed animation. GetSAnimCel will return NULL
 *until the Header, CCB, and first frame have been received. When the First
 *frame comes in, the current audio time will be read and saved to determine
 *which frame to display each call.
 *******************************************************************************************/
CCB *
GetSAnimCel (SAnimContextPtr ctx, SAnimRecPtr saRecPtr)
{
  CCB *chunkCCB;
  CCB *pCCB = NULL;
  SubsChunkDataPtr pChunkData;
  SubscriberMsgPtr subMsg;
  StreamAnimFramePtr curFramePtr;
  StreamAnimPosPtr saPosPtr;
  StreamAnimMapPtr saMapPtr;

#if FREE_DATA_WHEN_TIME_HAS_PASSED
  uint32 curTime;
#endif

  /* If no current frame, set the CCB ptr return value to NULL */
  if (saRecPtr->curFramePtr == NULL)
    pCCB = NULL;
  else
    pCCB = (CCB *)&(saRecPtr->saCCB.ccb_Flags);

  /* Check to see if it is time to display the next frame or
   * if the stream has not supplied the first frame yet. If no
   * first frame yet or time for a new frame, call PollSAnimChunk.
   */
  while (IsTimeForNextFrame (ctx, saRecPtr))
    {
      if (PollSAnimChunk (ctx, saRecPtr, &pChunkData, &subMsg))
        {
          switch (pChunkData->subChunkType)
            {
            case AHDR_CHUNK_TYPE: /* SAnim Header Chunk */
              memcpy (&(saRecPtr->saHeader), pChunkData,
                      sizeof (StreamAnimHeader));
              saRecPtr->curFramePtr = NULL;

              DoFreeChunk (ctx, subMsg);
              break;

            case ACCB_CHUNK_TYPE: /* SAnim CCB Chunk */
              memcpy (&(saRecPtr->saCCB), pChunkData, sizeof (StreamAnimCCB));

              /* Make sure that this CCB's flags are set to use the 2nd corner
               * engine and to point absolutely (not relatively) to the source
               * and (if used) PLUT data.
               */
              chunkCCB = (CCB *)&(saRecPtr->saCCB.ccb_Flags);
              chunkCCB->ccb_Flags |= CCB_PPABS | CCB_SPABS | CCB_ACE | CCB_LCE;

#ifdef SHIFT_CCB_POSITIONS
              /* Make cel coordinates into 16.16 for cel engine */
              chunkCCB->ccb_XPos <<= 16;
              chunkCCB->ccb_YPos <<= 16;
#endif

              DoFreeChunk (ctx, subMsg);
              break;

            case FRME_CHUNK_TYPE: /* SAnim Frame Chunk */
              /* Return previous frame to the DSL for freeing */
              if (saRecPtr->curSubMsg != NULL)
                DoFreeChunk (ctx, saRecPtr->curSubMsg);

              saRecPtr->curFramePtr = (StreamAnimFramePtr)pChunkData;
              chunkCCB = (CCB *)&(saRecPtr->saCCB.ccb_Flags);

              /* Fill the CCB fields for this frame and remember the frame
               * subMsg so this frame can be released back to the subscriber
               * later.
               */
              pCCB = SetupNextSAFrame (chunkCCB, saRecPtr->curFramePtr);
              saRecPtr->curSubMsg = subMsg;
              break;

            case APOS_CHUNK_TYPE:
              if (pCCB != NULL)
                {
                  /* Get a ptr to the current frame data */
                  if ((curFramePtr = saRecPtr->curFramePtr) != NULL)
                    {
                      /* Set the CCB fields with the stuff in the chunk.
                       */
                      saPosPtr = (StreamAnimPosPtr)pChunkData;
#ifdef SHIFT_CCB_POSITIONS
                      pCCB->ccb_XPos = saPosPtr->newXValue << 16;
                      pCCB->ccb_YPos = saPosPtr->newYValue << 16;
#else
                      pCCB->ccb_XPos = saPosPtr->newXValue;
                      pCCB->ccb_YPos = saPosPtr->newYValue;
#endif
                      /* Set the current frame's start time and
                       * duration to the stuff in the chunk.
                       */
                      curFramePtr->time = saPosPtr->time;
                      curFramePtr->duration = saPosPtr->duration;
                    }
                }
              DoFreeChunk (ctx, subMsg);
              break;

            case AMAP_CHUNK_TYPE:
              if (pCCB != NULL)
                {
                  /* Get a ptr to the current frame data */
                  if ((curFramePtr = saRecPtr->curFramePtr) != NULL)
                    {
                      Point CelCorners[4];
                      long i;

                      /* Set the CCB fields with the stuff in the chunk.
                       */
                      saMapPtr = (StreamAnimMapPtr)pChunkData;

                      for (i = 0; i < 4; i++)
                        {
                          CelCorners[i].pt_X = saMapPtr->quad[i].x >> 16;
                          CelCorners[i].pt_Y = saMapPtr->quad[i].y >> 16;
                        }
                      MapCel (pCCB, CelCorners);

                      /* Set the current frame's start time and
                       * duration to the stuff in the chunk.
                       */
                      curFramePtr->time = saMapPtr->time;
                      curFramePtr->duration = saMapPtr->duration;
                    }
                }
              DoFreeChunk (ctx, subMsg);
              break;

            default:
              ERR (("GetSAnimCel --- Unknown Chunk Type: '%.4s'\n",
                    &(pChunkData->subChunkType)));
              break;

            } // switch
        }     // if
      else
        break; // exit the while loop if no more msgs

    } // while

#if FREE_DATA_WHEN_TIME_HAS_PASSED
  /* Check to see if what we are about to return is a "past frame".
   * If it is, then free the current frame and return NULL.
   */
  if (DSGetClock (saRecPtr->streamCBPtr, &curTime) >= 0)
    {
      if (saRecPtr->curFramePtr != NULL)
        {
          if (curTime > (saRecPtr->curFramePtr->time
                         + saRecPtr->curFramePtr->duration))
            {
              if (saRecPtr->curSubMsg)
                {
                  DoFreeChunk (ctx, saRecPtr->curSubMsg);
                  saRecPtr->curSubMsg = NULL;
                  saRecPtr->curFramePtr = NULL;
                }
            }

          /* Don't give up frames until it is time to do so! */
          else if (curTime < saRecPtr->curFramePtr->time)
            return NULL;
        }
    }
  return (saRecPtr->curFramePtr == NULL) ? NULL : pCCB;
#else
  return pCCB;
#endif
}

/*******************************************************************************************
 * Routine to initialize the subscriber. Creates the a memory pool for
 *allocating subscriber contexts.
 *******************************************************************************************/
int32
InitSAnimSubscriber (void)
{
  /* Create the memory pool for allocating data stream
   * contexts.
   */
  SAGlobals.SAnimContextPool
      = CreateMemPool (SANM_MAX_SUBSCRIPTIONS, sizeof (SAnimContext));
  if (SAGlobals.SAnimContextPool == NULL)
    return -1;

  /* Create the memory pool for allocating animation
   * state records.
   */
  SAGlobals.SAnimRecPool
      = CreateMemPool (SANM_MAX_CHANNELS, sizeof (SAnimRec));
  if (SAGlobals.SAnimRecPool == NULL)
    return -1;

  /* Return success */
  return 0;
}

/*******************************************************************************************
 * Routine to deallocate resources allocated by InitSAnimSubscriber()
 *******************************************************************************************/
int32
CloseSAnimSubscriber (void)
{
  DeleteMemPool (SAGlobals.SAnimContextPool);
  SAGlobals.SAnimContextPool = NULL;

  DeleteMemPool (SAGlobals.SAnimRecPool);
  SAGlobals.SAnimRecPool = NULL;

  return 0;
}

/*******************************************************************************************
 * Routine to instantiate a new subscriber. Creates the message port through
 *which all subsequent communications take place, as well as any other
 *necessary per-context resources. Creates the service thread and passes the
 *new context to it.
 *******************************************************************************************/
int32
NewSAnimSubscriber (SAnimContextPtr *pCtx, int32 numChannels, int32 priority)
{
  int32 status;
  SAnimContextPtr ctx;
  uint32 signalBits;

  /* Allocate a subscriber context */

  ctx = (SAnimContextPtr)AllocPoolMem (SAGlobals.SAnimContextPool);
  if (ctx == NULL)
    return kDSNoMemErr;

  /* Allocate a signal to synchronize with the completion of the
   * subscriber's initialization. It will signal us with this when
   * it has finished, successfully or not, when it is done initializing
   * itself.
   */
  ctx->creatorTask = CURRENTTASK->t.n_Item; /* see "kernel.h" for this */
  ctx->numChannels = numChannels;

  ctx->creatorSignal = AllocSignal (0);
  if (ctx->creatorSignal == 0)
    {
      status = kDSNoSignalErr;
      goto CLEANUP;
    }

  /* Create the thread that will handle all subscriber
   * responsibilities.
   */
  status = NewThread (
      (void *)&SAnimSubscriberThread,         /* thread entrypoint */
      4096,                                   /* initial stack size */
      (long)CURRENT_TASK_PRIORITY + priority, /* priority */
      "SAnimSubsThread",                      /* name */
      &ctx->threadStackBlock, /* where to remember stack block addr */
      0,                      /* initial R0 */
      ctx);                   /* initial R1 */

  if (status <= 0)
    goto CLEANUP;
  else
    ctx->threadItem = status;

  /* Wait here while the subscriber initializes itself. When its done,
   * look at the status returned to us in the context block to determine
   * if it was happy.
   */
  signalBits = WaitSignal (ctx->creatorSignal);
  if (signalBits != ctx->creatorSignal)
    return kDSSignalErr;

  /* We're done with this signal, so give it back */
  FreeSignal (ctx->creatorSignal);

  /* Check the initialization status of the subscriber. If anything
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
  /* Something went wrong in creating the new subscriber.
   * Release anything we created and return the status indicating
   * the cause of the failure.
   */
  if (ctx->threadStackBlock)
    FreePtr (ctx->threadStackBlock);

  ReturnPoolMem (SAGlobals.SAnimContextPool, ctx);

  return status;
}

/*******************************************************************************************
 * Routine to get rid of all subscriber resources we created. This assumes that
 *the subscriber is in a clean state where it has returned all resources that
 *were passed to it, like messages. Deleting the subscriber thread should free
 *up all the rest of the stuff the thread allocated. What is left is the
 *thread's stack space, which we allocated, and the context block.
 *******************************************************************************************/
int32
DisposeSAnimSubscriber (SAnimContextPtr ctx)
{

  if (ctx)
    {
      if (ctx->threadItem > 0)
        DisposeThread (ctx->threadItem);
      if (ctx->threadStackBlock)
        FreePtr (ctx->threadStackBlock);

      ReturnPoolMem (SAGlobals.SAnimContextPool, ctx);
    }

  return 0;
}

/*==========================================================================================
  ==========================================================================================
                                                Routines to handle subscriber
  messages
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Utility routine to disable further data flow for the given channel, and to
 *halt any associated physical processes associated with the channel (i.e.,
 *stop sound playing, etc.)
 *******************************************************************************************/
void
_StopSAnimChannel (SAnimContextPtr ctx, long channelNumber)
{
  SAnimChannelPtr chanPtr;

  if (channelNumber < SANM_MAX_CHANNELS)
    {
      chanPtr = ctx->channel + channelNumber;

      /* Prevent further data queuing */
      chanPtr->status &= ~CHAN_ENABLED;

      /* Stop any physical processes associated with the channel */
    }
}

/*******************************************************************************************
 * Utility routine to flush all msgs for the specified channel back to the DSL.
 *******************************************************************************************/
void
_FlushChannel (SAnimContextPtr ctx, long channelNumber)
{
  SAnimChannelPtr chanPtr;
  SubscriberMsgPtr msgPtr;
  SubscriberMsgPtr next;

  if (channelNumber < SANM_MAX_CHANNELS)
    {
      chanPtr = ctx->channel + channelNumber;

      /* Make sure we are the only code using this queue.
       */
      LockSemaphore (chanPtr->dataQueueSem, 1);

      /* Give back all queued chunks for this channel to the
       * stream parser. We do this by replying to all the
       * "chunk arrived" messages that we have queued.
       */
      msgPtr = chanPtr->dataQueue.head;

      while (msgPtr != NULL) // NJC
        {
          /* Get the pointer to the next message in the queue */
          next = (SubscriberMsgPtr)msgPtr->link;

          /* Reply to this chunk so that the stream parser
           * can eventually reuse the buffer space.
           */
          ReplyMsg (msgPtr->msgItem, kDSNoErr, msgPtr, 0);

          /* Continue with the next message in the queue */
          msgPtr = next;
        }

      chanPtr->dataQueue.head = NULL;
      chanPtr->dataQueue.tail = NULL;
      UnlockSemaphore (chanPtr->dataQueueSem);
    }
}

/*******************************************************************************************
 * Routine to queue arriving data chunks. Each chunk will be added to the end
 *of its channel's chunk list. When the App requests a chunk by sending a
 *request msg, the first chunk in the channel queue will be removed and sent
 *with the reply. Later, the App will send a msg telling the subscriber to
 *reply a chunk back to the DSL. This will tell the DSL to free the chunk.
 *******************************************************************************************/
static int32
DoSAnimData (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  int32 status;
  SubsChunkDataPtr chunkData;
  SAnimChannelPtr chanPtr;
  long channelNumber;
  int32 *channelPtr;

  chunkData = (SubsChunkDataPtr)subMsg->msg.data.buffer;

  channelPtr = (int32 *)&chunkData->channel;
  channelNumber = *channelPtr;

  /* If the channel number is greater than the max channel
   * that we have prepared for, just send the msg back to
   * the DSH to free it up.
   */
  if (channelNumber < SANM_MAX_CHANNELS)
    {
      chanPtr = ctx->channel + channelNumber;

      /* Make sure we are the only code using this queue.
       */
      LockSemaphore (chanPtr->dataQueueSem, 1);

      AddDataMsgToTail (&chanPtr->dataQueue, subMsg);

      UnlockSemaphore (chanPtr->dataQueueSem);
    }
  else
    {
      ReplyMsg (subMsg->msgItem, 0, subMsg, 0);
    }

  return status;
}

/*******************************************************************************************
 * Routine to return the status bits of a given channel.
 *******************************************************************************************/
static int32
DoSAnimGetChan (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  int32 status;
  long channelNumber;

  channelNumber = subMsg->msg.channel.number;

  if (channelNumber < SANM_MAX_CHANNELS)
    status = ctx->channel[channelNumber].status;
  else
    status = 0;

  return status;
}

/*******************************************************************************************
 * Routine to set the channel status bits of a given channel.
 *******************************************************************************************/
static int32
DoSAnimSetChan (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  long channelNumber;

  channelNumber = subMsg->msg.channel.number;

  if (channelNumber < SANM_MAX_CHANNELS)
    {
      /* Allow only bits that are Read/Write to be set by this call.
       *
       * NOTE: 	Any special actions that might need to be taken as as
       *			result of bits being set or cleared should be
       *taken now. If the only purpose served by status bits is to enable or
       *disable features, or some other communication, then the following is
       *all that is necessary.
       */
      ctx->channel[channelNumber].status
          |= (~CHAN_SYSBITS & subMsg->msg.channel.status);
    }

  return 0;
}

/*******************************************************************************************
 * Routine to perform an arbitrary subscriber defined action.
 *******************************************************************************************/
static int32
DoSAnimControl (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
#if 0
	int32			status;
	long			userDefinedArg1 = subMsg->msg.control.controlArg1;
	void*			userDefinedArg2 = subMsg->msg.control.controlArg2;
#endif

  /* This routine takes a long and a pointer as inputs. These are entirely
   * user defined and can be used to do everything from adjust audio volume
   * on a per channel basis, to ordering out for pizza.
   */

  return 0;
}

/*******************************************************************************************
 * Routine to get the next available chunk for a channel.
 *******************************************************************************************/
static SubscriberMsgPtr
DoGetChunk (SAnimContextPtr ctx, SAnimRecPtr saRecPtr)
{
  SubscriberMsgPtr subMsg;
  SAnimChannelPtr chanPtr;

  /* Get a ptr to the SAnimChannel structure for this saRec
   */
  chanPtr = ctx->channel + saRecPtr->channel;

  /* Return a ptr to the next available chunk in status.
   */
  LockSemaphore (chanPtr->dataQueueSem, 1);

  subMsg = GetNextDataMsg (&chanPtr->dataQueue);

  UnlockSemaphore (chanPtr->dataQueueSem);

  return subMsg;
}

/*******************************************************************************************
 * Routine to add a chunk to the free list. These chunks will be replied back
 *to the DSL later as a result of a signal sent by the client. (see
 *DoReplyChunksToDSL)
 *******************************************************************************************/
static int32
DoFreeChunk (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{

  /* Add the msg to the SAnim Subscriber's free list.
   */
  LockSemaphore (ctx->freeQueueSem, 1);

  AddDataMsgToTail (&ctx->freeList, subMsg);

  UnlockSemaphore (ctx->freeQueueSem);
  ctx->freeListNotEmpty = true;

  return 0;
}

/*******************************************************************************************
 * Routine to free all chunks in the freed chunk list back to the DSL.
 *******************************************************************************************/
static void
DoReplyChunksToDSL (SAnimContextPtr ctx)
{
  SubscriberMsgPtr subMsg;

  /* Add the msg to the SAnim Subscriber's free list.
   */
  LockSemaphore (ctx->freeQueueSem, 1);

  while ((subMsg = GetNextDataMsg (&ctx->freeList)) != NULL)
    ReplyMsg (subMsg->msgItem, 0, subMsg, 0);

  UnlockSemaphore (ctx->freeQueueSem);
}

/*******************************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream,
 *typically just after the stream is opened.
 *******************************************************************************************/
static int32
DoSAnimOpening (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  return 0;
}

/*******************************************************************************************
 * Routine to close down an open subscription.
 *******************************************************************************************/
static int32
DoSAnimClosing (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  return 0;
}

/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/
static int32
DoSAnimStop (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  long channelNumber;

  /* Stop all channels for this subscription.
   */
  for (channelNumber = 0; channelNumber < SANM_MAX_CHANNELS; channelNumber++)
    _StopSAnimChannel (ctx, channelNumber);

  return 0;
}

/*******************************************************************************************
 * Routine to resynh the SAnim channel queues based on the new stream time.
 *******************************************************************************************/
static int32
DoSAnimSync (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  int32 channelNumber;
  SAnimChannelPtr chanPtr;

  /* Reply all free chunks */
  DoReplyChunksToDSL (ctx);

  /* Reply all chunks for each channel back to the DSL */
  for (channelNumber = 0; channelNumber < SANM_MAX_CHANNELS; channelNumber++)
    {
      chanPtr = ctx->channel + channelNumber;

      if (chanPtr->fFlushOnSync == true)
        _FlushChannel (ctx, channelNumber);
    }

  ctx->fTimeChanged = true;
  return 0;
}

/*******************************************************************************************
 * Routine to
 *******************************************************************************************/
static int32
DoSAnimEOF (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  return 0;
}

/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop
 *everything.
 *******************************************************************************************/
static int32
DoSAnimAbort (SAnimContextPtr ctx, SubscriberMsgPtr subMsg)
{
  long channelNumber;

  /* Halt and flush all channels for this subscription.
   */
  for (channelNumber = 0; channelNumber < SANM_MAX_CHANNELS; channelNumber++)
    {
      _FlushChannel (ctx, channelNumber);

      /* Halt whatever activity is associated with the channel */
      _StopSAnimChannel (ctx, channelNumber);
    }

  return 0;
}

/*==========================================================================================
  ==========================================================================================
                                                                        The
  subscriber thread
  ==========================================================================================
  ==========================================================================================*/

int32
_InitializeSAnimThread (SAnimContextPtr ctx)
{
  int32 status;
  long channelNumber;
  SAnimChannelPtr chanPtr;
  char semName[8], semNum[2];

  /* Initialize fields in the context record */

  ctx->creatorStatus = -1; /* assume initialization will fail */
  ctx->requestPort = 0;
  ctx->cueItem = 0;
  ctx->cueSignal = 0;

  /* Open the Audio Folio for this thread */

  if ((status = OpenAudioFolio ()) < 0)
    {
      ctx->creatorStatus = status;
      goto BAILOUT;
    }

  ctx->sanimTask = CURRENTTASK->t.n_Item; /* see "kernel.h" for this */

  ctx->freeList.head = NULL;
  ctx->freeQueueSem = CreateSemaphore ("freeSAnimQueue", 100);
  ctx->freeQueueSignal = AllocSignal (0);
  ctx->freeListNotEmpty = false;
  ctx->fTimeChanged = false;

  /* Initialize channel structures */
  chanPtr = ctx->channel;
  for (channelNumber = 0; channelNumber < SANM_MAX_CHANNELS;
       channelNumber++, chanPtr++)
    {
      chanPtr->status = CHAN_ENABLED;
      chanPtr->dataQueue.head = NULL;
      chanPtr->dataQueue.tail = NULL;
      chanPtr->fFlushOnSync = false;

      /* Create a semaphore to manage access to the dataQueue list for this
       * channel
       */
      semName[0] = 's';
      semName[1] = 'a';
      semName[2] = 0;
      semNum[0] = '0' + (char)channelNumber; /* {"0","1",...} */
      semNum[1] = 0;
      strcat (semName, semNum);
      chanPtr->dataQueueSem = CreateSemaphore (semName, 100);
    }

  /* Create the message port where the new subscriber will accept
   * request messages.
   */
  status = NewMsgPort (&ctx->requestPortSignal);
  if (status <= 0)
    goto BAILOUT;
  else
    ctx->requestPort = status;

  /* Create an Audio Cue Item for timing the output
   * actions of this subscriber.
   */
  status = CreateItem (MKNODEID (AUDIONODE, AUDIO_CUE_NODE), NULL);
  if (status <= 0)
    goto BAILOUT;
  else
    ctx->cueItem = status;

  /* Get the signal associated with the cue so we
   * can wait on timed events in the thread's main loop.
   */
  if ((ctx->cueSignal = GetCueSignal (ctx->cueItem)) != 0)
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
 * This thread is started by a call to InitSAnimSubscriber(). It reads the
 *subscriber message port for work requests and performs appropriate actions.
 *The subscriber message definitions are located in "DataStreamLib.h".
 *******************************************************************************************/
void
SAnimSubscriberThread (int32 notUsed, SAnimContextPtr ctx)
{
  int32 status;
  SubscriberMsgPtr subMsg;
  uint32 signalBits;
  uint32 anySignal;
  long channelNumber;
  Boolean fKeepRunning = true;

  /* Call a utility routine to perform all startup initialization.
   */
  if ((status = _InitializeSAnimThread (ctx)) != 0)
    exit (0);

  /* All resources are now allocated and ready to use. Our creator has been
   * informed that we are ready to accept requests for work. All that's left to
   * do is wait for work request messages to arrive.
   */
  anySignal = ctx->cueSignal | ctx->requestPortSignal | ctx->freeQueueSignal;

  while (fKeepRunning)
    {
      signalBits = WaitSignal (anySignal);

      /***********************************************/
      /* Check for - process any free chunk requests */
      /***********************************************/
      if (signalBits & ctx->freeQueueSignal)
        {
          DoReplyChunksToDSL (ctx);
        }

      /********************************************************/
      /* Check for and process and incoming request messages. */
      /********************************************************/
      if (signalBits & ctx->requestPortSignal)
        {
          /* Process any new requests for service as determined by the incoming
           * message data.
           */
          while (PollForMsg (ctx->requestPort, NULL, NULL, (void **)&subMsg,
                             &status))
            {
              switch (subMsg->whatToDo)
                {
                case kStreamOpData: /* new data has arrived */
                  status = DoSAnimData (ctx, subMsg);
                  break;

                case kStreamOpGetChan: /* get logical channel status */
                  status = DoSAnimGetChan (ctx, subMsg);
                  break;

                case kStreamOpSetChan: /* set logical channel status */
                  status = DoSAnimSetChan (ctx, subMsg);
                  break;

                case kStreamOpControl: /* perform subscriber defined function
                                        */
                  status = DoSAnimControl (ctx, subMsg);
                  break;

                case kStreamOpSync: /* clock stream resynched the clock */
                  status = DoSAnimSync (ctx, subMsg);
                  break;

                  /************************************************************************
                   * NOTE:	The following msgs have no extended message
                   *arguments and only may use the whatToDo and context fields
                   *in the message.
                   ************************************************************************/

                case kStreamOpOpening: /* one time initialization call from DSH
                                        */
                  status = DoSAnimOpening (ctx, subMsg);
                  break;

                case kStreamOpClosing: /* stream is being closed */
                  status = DoSAnimClosing (ctx, subMsg);
                  fKeepRunning = false;
                  break;

                case kStreamOpStop: /* stream is being stopped */
                  status = DoSAnimStop (ctx, subMsg);
                  break;

                case kStreamOpEOF: /* physical EOF on data, no more to come */
                  status = DoSAnimEOF (ctx, subMsg);
                  break;

                case kStreamOpAbort: /* somebody gave up, stream is aborted */
                  status = DoSAnimAbort (ctx, subMsg);
                  break;

                default:;
                }

              /* Reply to the request we just handled unless this is a "data
               * arrived" message. These are replied to asynchronously as the
               * data is actually consumed and must not be replied to here.
               */
              if (subMsg->whatToDo != kStreamOpData)
                ReplyMsg (subMsg->msgItem, status, subMsg, 0);
            }
        }
    }

  /* Halt and flush all channels for this subscription.
   */
  for (channelNumber = 0; channelNumber < SANM_MAX_CHANNELS; channelNumber++)
    _FlushChannel (ctx, channelNumber);

  /* Exiting should cause all resources we allocated to be
   * returned to the system.
   */
  exit (0);
}
