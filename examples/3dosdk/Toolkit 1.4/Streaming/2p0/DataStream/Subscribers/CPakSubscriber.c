/*******************************************************************************************
 *	File:			CPakSubscriber.c
 *
 *	Contains:		Streaming CinePak subscriber
 *
 *	Written by:		Neil Cormia
 *
 *	Copyright й 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	11/23/93	jb		Version 1.2
 *						Added CloseCPakSubscriber() to clean up
 *resources allocated by the InitCPakSubscriber() routine. 10/19/93	jb
 *Version 1.1 Switch to using UMemory.c routines for memory allocation 10/14/93
 *jb		Add code identified by FREE_DATA_WHEN_TIME_HAS_PASSED to
 *optionally free streamer buffers automatically once a buffer's time has
 *expired. Expiration is determined by buffer time+duration being less than the
 *current stream clock. 8/25/93		jb		Force image width and
 *height to be a multiple of 4 in AllocMovieBuff(). 6/22/93		njc
 *Changed subscriber structure from one msgPort for every channel to a simple
 *linked list with a semaphore to arbitrate access to the list. In addition, a
 *freeList is maintained that contains chunks that are to be returned to the
 *Data Streaming Library. 5/25/93		njc		Version 1.0a
 *	5/19/93		ec		Added reset code to FlushCPakChannel.
 *	4/29/93		njc		New today.
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
#include "Graphics.h"
#include "debug.h"
#include "operamath.h"

#include "codec.h"

#include "Form3DO.h"
#include "Parse3DO.h"

#include "CPakSubscriber.h"
#include "SubscriberUtils.h"

#include "stdlib.h"  /* for the exit() routine */
#include "strings.h" /* for the memcpy() routine */

#define SKIP_FRAME 0

/* The following compile switch will cause the subscriber to always return
 * buffers to the streamer once their time has expired. This must be set
 * for end of stream detection to work.
 */
#define FREE_DATA_WHEN_TIME_HAS_PASSED 1

#define Convert32_F20(x) ((x) << 20)
/* convert 32 bit integer to 12.20 fraction */

/**********************/
/* Subscriber globals */
/**********************/

struct CPakSubscriberGlobals
{

  MemPoolPtr CPakContextPool; /* The memory pool for subscriber contexts */
  MemPoolPtr CPakRecPool;     /* The memory pool for subscriber anim records */

} CPGlobals;

/****************************/
/* Local routine prototypes */
/****************************/
Boolean PollCPakChunk (CPakContextPtr ctx, CPakRecPtr cpRecPtr,
                       SubsChunkDataPtr *pChunkDataPtr,
                       SubscriberMsgPtr *subMsg);

static Boolean IsTimeForNextFrame (CPakRecPtr cpRecPtr);

static int32 DoData (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoGetChan (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSetChan (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoControl (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoOpening (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoClosing (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoStop (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoSync (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoEOF (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static int32 DoAbort (CPakContextPtr ctx, SubscriberMsgPtr subMsg);

static SubscriberMsgPtr DoGetChunk (CPakContextPtr ctx, CPakRecPtr cpRecPtr);
static int32 DoFreeChunk (CPakContextPtr ctx, SubscriberMsgPtr subMsg);
static void DoReplyChunksToDSL (CPakContextPtr ctx);

static int32 _InitializeThread (CPakContextPtr ctx);

/**************************/
/* Local utility routines */
/**************************/

static void _StopChannel (CPakContextPtr ctx, long channelNumber);
static void _FlushChannel (CPakContextPtr ctx, long channelNumber);
void GetDstStuff (long *baseAddr, long *rowBytes, long width, long height,
                  Bitmap *bitmap);

/*==========================================================================================
  ==========================================================================================
                                                        Subscriber procedural
  interfaces
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Allocate a buffer for the Cinepak decompressor. The decompressor will dump
 *data into this buffer in LR form. The Cel Engine can draw this format (on RED
 *systems) to the frame buf.
 *******************************************************************************************/
#define FORCE_MULTIPLE_OF_4(x) ((x + 3) & ~3)
int32
AllocMovieBuff (ImageDesc *imagePtr, CinePakHeaderPtr pCPHeader)
{
  ubyte *movieFrameBuff;

  /* Force width and height to be a multiple of 4
   */
  imagePtr->width = FORCE_MULTIPLE_OF_4 (pCPHeader->width);
  imagePtr->height = FORCE_MULTIPLE_OF_4 (pCPHeader->height);

  imagePtr->rowBytes = imagePtr->width * BYTES_PER_PIXEL;

  /* еее We need to make sure that we have an even number of rows */
  movieFrameBuff = (ubyte *)NewPtr (imagePtr->rowBytes * imagePtr->height,
                                    MEMTYPE_ANY | (MEMTYPE_FILL | 0x00));

  if (movieFrameBuff == NULL)
    {
      printf ("Failed to allocate movie image buffer.\n");
      return (-1);
    }

  imagePtr->baseAddr = (int32)(movieFrameBuff);

  return 0;
}

/*******************************************************************************************
 * Free the decompression buffer we created for the Cinepak .
 *******************************************************************************************/
void
FreeMovieBuff (ImageDesc *imagePtr)
{
  int32 *tempPtr;

  tempPtr = (int32 *)imagePtr->baseAddr;

  if (tempPtr != NULL)
    {
      FreePtr (tempPtr);
      imagePtr->baseAddr = 0;
    }
}

/*******************************************************************************************
 * Set the LR Form Cel fields to valid values.
 *******************************************************************************************/
void
SetLRFormCel (ImageDesc *imagePtr, CCB *ccb)
{
  /* Init the LR Form CCB
   */
  ccb->ccb_Flags = CCB_LDSIZE   /* Load hdx-y and vdx-y from CCB */
                   | CCB_LDPRS  /* Load ddx-y from CCB */
                   | CCB_LDPPMP /* Load the PPMP word from CCB */
                   | CCB_CCBPRE /* Load the Pramble words from CCB */
                   | CCB_ACW    /* Pixels facing forward will be seen */
                   | CCB_ACCW   /* Pixels facing backward will be seen */
                   | CCB_SPABS  /* SourcePtr is a ptr, not an offset */
                   | CCB_YOXY | CCB_LCE /* Lock the corner engines together */
                   | CCB_ACE;           /* Aenable Corner Engine */

  ccb->ccb_NextPtr = 0;
  ccb->ccb_PLUTPtr = 0;

  ccb->ccb_XPos = 0;
  ccb->ccb_YPos = 0;
  ccb->ccb_HDX = Convert32_F20 (1);
  ccb->ccb_HDY = 0;
  ccb->ccb_VDX = 0;
  ccb->ccb_VDY = Convert32_F16 (1);
  ccb->ccb_HDDX = 0;
  ccb->ccb_HDDY = 0;
  ccb->ccb_PIXC = 0x1F811F00;

  ccb->ccb_Width = imagePtr->width;
  ccb->ccb_Height = imagePtr->height;

  ccb->ccb_PRE0
      = (((ccb->ccb_Height >> 1) - 1) << PRE0_VCNT_SHIFT)
        +             /* number of vertical data lines in sprite data -1 */
        PRE0_LINEAR + /* use PIN for IPN (0x10) */
        PRE0_BPP_16;  /* set bits/pixel to 16 (0x6) see HDWR spec 3.6.3 */

  /* imagePtr->width is really the number of pixels but,
   * in this case, pixels are 16 bits and each line is
   * twice as long (2 lines are interleaved in LRForm)
   * so imagePtr->width happens to give us the number of
   * longwords wide for each (double) line.
   */
  ccb->ccb_PRE1 = ((imagePtr->width - 2) << 16)
                  + /* offset (in words) from line start to next line start. */
                  (ccb->ccb_Width - 1)
                  + /* number of horizontal pixels to render -1 */
                  PRE1_LRFORM;

  ccb->ccb_SourcePtr = (CelData *)imagePtr->baseAddr;
}

/*******************************************************************************************
 * Poll for the next msg in this streamed animation and return true if a
 *kStreamOpData msg is present. Pass a ptr to the chunk back in pChunkDataPtr.
 *******************************************************************************************/
Boolean
PollCPakChunk (CPakContextPtr ctx, CPakRecPtr cpRecPtr,
               SubsChunkDataPtr *pChunkDataPtr, SubscriberMsgPtr *subMsg)
{

  /* Poll for the next msg in this streamed video
   * and extract a chunkPtr. Return true if a subscriber
   * message is returned from DoGetChunk.
   */
  if ((*subMsg = DoGetChunk (ctx, cpRecPtr)) != NULL)
    *pChunkDataPtr = (SubsChunkDataPtr)(*subMsg)->msg.data.buffer;
  else
    return false;

  /* Return the chunkPtr */
  return true;
}

/*******************************************************************************************
 * Allocate and initialize a streamed CinePak state record. GetCPakCel will
 *return NULL until the Header and first frame have been received. When the
 *First frame comes in, the current audio time will be read and saved to
 *determine which frame to display each call.
 *******************************************************************************************/
int32
InitCPakCel (DSStreamCBPtr streamCBPtr, CPakContextPtr ctx,
             CPakRecPtr *pCPRecPtr, int32 channel, Boolean flushOnSync)
{
  CPakRecPtr cpRecPtr;
  CPakChannelPtr chanPtr;

  cpRecPtr = (CPakRecPtr)AllocPoolMem (CPGlobals.CPakRecPool);
  if (cpRecPtr == NULL)
    return kDSNoMemErr;

  cpRecPtr->curFramePtr = NULL;
  cpRecPtr->curSubMsg = NULL;
  cpRecPtr->channel = channel;
  cpRecPtr->streamCBPtr = streamCBPtr;
  cpRecPtr->lastCurTime = 0;
  *pCPRecPtr = cpRecPtr;

  chanPtr = ctx->channel + channel;
  chanPtr->fFlushOnSync = flushOnSync;

  return 0;
}

/*******************************************************************************************
 *	Return all CPak msgs to the CPakSubscriber back end. The CPak
 *subscriber will return one subscriber chunk msg to the Data Streamer for each
 *of these msgs returned.
 *******************************************************************************************/
void
FlushCPakChannel (CPakContextPtr ctx, CPakRecPtr cpRecPtr, int32 channel)
{
  SubsChunkDataPtr pChunkData;
  SubscriberMsgPtr subMsg;

  if (cpRecPtr->curSubMsg)
    {
      DoFreeChunk (ctx, cpRecPtr->curSubMsg);
      cpRecPtr->curSubMsg = NULL;
    }

  while (PollCPakChunk (ctx, cpRecPtr, &pChunkData, &subMsg))
    DoFreeChunk (ctx, subMsg);

  cpRecPtr->curFramePtr = NULL;
  DoReplyChunksToDSL (ctx);
}

/*******************************************************************************************
 *
 *******************************************************************************************/
int32
DestroyCPakCel (CPakContextPtr ctx, CPakRecPtr cpRecPtr, int32 channel)
{
  FreeMovieBuff (&(ctx->channel[channel].imageData));
  FlushCPakChannel (ctx, cpRecPtr, channel);
  ReturnPoolMem (CPGlobals.CPakRecPool, (void *)cpRecPtr);

  return 0;
}

/*******************************************************************************************
 * Return the duration of the film (in 240ths), or -1 if the film frame hasn't
 *been read yet.  NOTE: this function returns the time of the BEGINING of the
 *last frame
 *******************************************************************************************/
int32
CPakDuration (CPakRecPtr cpRecPtr)
{
  int32 frameRateAudioTime;

  if (cpRecPtr->curSubMsg == NULL)
    return -1;

  frameRateAudioTime = 240 / cpRecPtr->cpHeader.scale;
  return (cpRecPtr->cpHeader.count * frameRateAudioTime) - frameRateAudioTime;
}

/*******************************************************************************************
 * Return the current time of the film (in 240ths), or -1 of the film frame
 *hasn't been read yet.
 *******************************************************************************************/
int32
CPakCurrTime (CPakRecPtr cpRecPtr)
{
  register CinePakFramePtr curFramePtr;

  /* Get a ptr to the current frame data */
  curFramePtr = cpRecPtr->curFramePtr;

  /* If no frames read yet then can't tell what time it is */
  if (curFramePtr == NULL)
    return -1;

  return curFramePtr->time;
}

/*******************************************************************************************
 * Return true if time for new frame or no frames read yet. Pass back the frame
 *to get.
 *******************************************************************************************/
Boolean
IsTimeForNextFrame (CPakRecPtr cpRecPtr)
{
  CinePakFramePtr curFramePtr;
  uint32 curTime;

  if (DSGetClock (cpRecPtr->streamCBPtr, &curTime) < 0)
    return false;

  /* Check to see if the stream time looped and force
   * a new frame if it did.
   */
  if (cpRecPtr->lastCurTime > curTime)
    {
      cpRecPtr->lastCurTime = curTime;
      return true;
    }

  cpRecPtr->lastCurTime = curTime;

  /* Get a ptr to the current frame data */
  curFramePtr = cpRecPtr->curFramePtr;

  /* If no frames read yet then get a frame */
  if (curFramePtr == NULL)
    return true;

  /* If the current frame should still be displayed, return false.
   * If the current frame's starting time plus its duration are less
   * than the current time, then we need a new frame, so return true.
   */
  return (curTime > (curFramePtr->time + curFramePtr->duration)) ? true
                                                                 : false;
}

/*******************************************************************************************
 * Send a signal to the CPak Subscriber telling it to Reply all subMsgs in the
 *freeList back to the DSL. This is necessary to release physical blocks for
 *re-use.
 *******************************************************************************************/
int32
SendFreeCPakSignal (CPakContextPtr ctx)
{
  int32 status = 0;

  if (ctx->freeListNotEmpty)
    {
      status = SendSignal (ctx->cpakTask, ctx->freeQueueSignal);
      ctx->freeListNotEmpty = false;
    }

  return status;
}

/*******************************************************************************************
 * Get the next compressed frame data in the streamed CinePak film. GetCPakCel
 *will return NULL until the Header and first frame have been received. When
 *the First frame comes in, the current audio time will be read and saved to
 *determine which frame to display each call.
 *******************************************************************************************/
CCB *
GetCPakCel (CPakContextPtr ctx, CPakRecPtr cpRecPtr)
{
  SubsChunkDataPtr pChunkData;
  CPakChannelPtr pChannel;
  ImageDesc *pImageData;
  CCB *retCCB;
  SubscriberMsgPtr subMsg;
#if FREE_DATA_WHEN_TIME_HAS_PASSED
  uint32 curTime;
#endif

  pChannel = &(ctx->channel[cpRecPtr->channel]);
  pImageData = &(pChannel->imageData);
  retCCB = &(cpRecPtr->cpCCB);

  /* Check to see if it is time to display the next frame or
   * if the stream has not supplied the first frame yet. If no
   * first frame yet or time for a new frame, call PollCPakChunk.
   */
  while (IsTimeForNextFrame (cpRecPtr))
    {
      if (PollCPakChunk (ctx, cpRecPtr, &pChunkData, &subMsg))
        {
          switch (pChunkData->subChunkType)
            {
            case FHDR_CHUNK_TYPE: /* CPak Header Chunk */

              /* Make a copy of the header so we can free up the block that
               * contains it.
               */
              memcpy (&(cpRecPtr->cpHeader), pChunkData,
                      sizeof (CinePakHeader));

              /* Create the LR Form data buffer that we will fill when
               * decompressing.
               */
              if (pImageData->baseAddr == 0)
                AllocMovieBuff (pImageData, &(cpRecPtr->cpHeader));

              SetLRFormCel (pImageData, &(cpRecPtr->cpCCB));

              cpRecPtr->curFramePtr = NULL;

              DoFreeChunk (ctx, subMsg);
              break;

            case FRME_CHUNK_TYPE: /* CPak Frame Chunk */
              cpRecPtr->curFramePtr = (CinePakFramePtr)pChunkData;

              if (cpRecPtr->curSubMsg)
                DoFreeChunk (ctx, cpRecPtr->curSubMsg);

              PreDecompress (
                  ctx->filmCodec,
                  (void *)(((CinePakFramePtr)pChunkData)->frameData));
              Decompress (ctx->filmCodec,
                          (void *)(((CinePakFramePtr)pChunkData)->frameData),
                          (char *)pImageData->baseAddr, pImageData->rowBytes);

              /* Remember the frame subMsg so this frame
               * can be released back to the subscriber later.
               */
              cpRecPtr->curSubMsg = subMsg;
              break;

            default:
              printf ("GetCPakCel --- Unknown Chunk Type: '%.4s'",
                      &(pChunkData->subChunkType));
              break;
            }
        }
      else
        break; // exit the while loop if no more msgs
    }

#if FREE_DATA_WHEN_TIME_HAS_PASSED
  /* Check to see if what we are about to return is a "past frame".
   * If it is, then free the current frame and return NULL.
   */
  if (DSGetClock (cpRecPtr->streamCBPtr, &curTime) >= 0)
    {
      if (curTime
          > (cpRecPtr->curFramePtr->time + cpRecPtr->curFramePtr->duration))
        {
          if (cpRecPtr->curSubMsg)
            {
              DoFreeChunk (ctx, cpRecPtr->curSubMsg);
              cpRecPtr->curSubMsg = NULL;
              cpRecPtr->curFramePtr = NULL;
            }
        }
    }
#endif
  return (cpRecPtr->curFramePtr == NULL) ? NULL : retCCB;
}

/*******************************************************************************************
 * Get the next compressed frame data in the streamed CinePak film.
 *DrawCPakToBuffer will return NULL until the Header and first frame have been
 *received. When the First frame comes in, the current audio time will be read
 *and saved to determine which frame to display each call.
 *******************************************************************************************/
void
DrawCPakToBuffer (CPakContextPtr ctx, CPakRecPtr cpRecPtr, Bitmap *bitmap)
{
  long baseAddr, rowBytes;
  SubsChunkDataPtr pChunkData;
  CPakChannelPtr pChannel;
  ImageDesc *pImageData;
  SubscriberMsgPtr subMsg;
#if FREE_DATA_WHEN_TIME_HAS_PASSED
  uint32 curTime;
#endif

#if SKIP_FRAME
  Boolean newFrame = false;
#endif

  pChannel = &(ctx->channel[cpRecPtr->channel]);
  pImageData = &(pChannel->imageData);

  /* Check to see if it is time to display the next frame or
   * if the stream has not supplied the first frame yet. If no
   * first frame yet or time for a new frame, call PollCPakChunk.
   */

  while (IsTimeForNextFrame (cpRecPtr))
    {

      if (PollCPakChunk (ctx, cpRecPtr, &pChunkData, &subMsg))
        {

          switch (pChunkData->subChunkType)
            {
            case FHDR_CHUNK_TYPE: /* CPak Header Chunk */

              /* Make a copy of the header so we can free up the block that
               * contains it.
               */
              memcpy (&(cpRecPtr->cpHeader), pChunkData,
                      sizeof (CinePakHeader));

              /* These are used by GetDstStuff to calc the offset into the
               * Frame Buffer */
              pImageData->width = cpRecPtr->cpHeader.width;
              pImageData->height = cpRecPtr->cpHeader.height;

              DoFreeChunk (ctx, subMsg);
              cpRecPtr->curFramePtr = NULL;
              break;

            case FRME_CHUNK_TYPE: /* CPak Frame Chunk */
              cpRecPtr->curFramePtr = (CinePakFramePtr)pChunkData;

              if (cpRecPtr->curSubMsg)
                DoFreeChunk (ctx, cpRecPtr->curSubMsg);

#if SKIP_FRAME
              newFrame = true;
#else

              GetDstStuff (&baseAddr, &rowBytes, pImageData->width,
                           pImageData->height, bitmap);
              PreDecompress (
                  ctx->filmCodec,
                  (void *)(((CinePakFramePtr)pChunkData)->frameData));
              Decompress (ctx->filmCodec,
                          (void *)(((CinePakFramePtr)pChunkData)->frameData),
                          (char *)baseAddr, rowBytes);
#endif
              /* Remember the frame subMsg so this frame
               * can be released back to the subscriber later.
               */
              cpRecPtr->curSubMsg = subMsg;
              break;

            default:
              printf ("GetCPakCel --- Unknown Chunk Type: '%.4s'",
                      &(pChunkData->subChunkType));
              break;
            } // switch
        }     // if
      else
        break; // exit the while loop if no more msgs

    } // while

#if SKIP_FRAME
  if (newFrame == true)
    {
      GetDstStuff (&baseAddr, &rowBytes, pImageData->width, pImageData->height,
                   bitmap);
      PreDecompress (ctx->filmCodec,
                     (void *)(((CinePakFramePtr)pChunkData)->frameData));
      Decompress (ctx->filmCodec,
                  (void *)(((CinePakFramePtr)pChunkData)->frameData),
                  (char *)baseAddr, rowBytes);
    }
#endif

#if FREE_DATA_WHEN_TIME_HAS_PASSED
  /* Check to see if what we are about to return is a "past frame".
   * If it is, then free the current frame and return NULL.
   */
  if (DSGetClock (cpRecPtr->streamCBPtr, &curTime) >= 0)
    {
      if (curTime
          > (cpRecPtr->curFramePtr->time + cpRecPtr->curFramePtr->duration))
        {
          if (cpRecPtr->curSubMsg)
            {
              DoFreeChunk (ctx, cpRecPtr->curSubMsg);
              cpRecPtr->curSubMsg = NULL;
              cpRecPtr->curFramePtr = NULL;
            }
        }
    }
#endif
}

/*******************************************************************************************
 *	Although the frame buffer displays is 320 by 240, only about 288 by 216
 *pixels are in the visible area. All display calculations are therefore based
 *on a rectangle that is roughly centered in the frame buffer:
 *	VisibleRect(TLBR)= [12,18,12+216,18+288];
 *******************************************************************************************/
void
GetDstStuff (long *baseAddr, long *rowBytes, long width, long height,
             Bitmap *bitmap)
{
  long hOff;
  long vOff;

  hOff = ((320 - width) / 2) * 4;
  vOff = (((240 - height) / 2) & 0xFFFFFFFC) * (320 * 2);

  //	hOff = ((320 - (width-50))/2)*4;
  //	vOff = (((240 - (height-2))/2) & 0xFFFFFFFC) * (320*2);

  *baseAddr = (long)bitmap->bm_Buffer + hOff + vOff;
  *rowBytes = bitmap->bm_Width * 2;
}

/*******************************************************************************************
 * Routine to initialize the subscriber. Creates the a memory pool for
 *allocating subscriber contexts.
 *******************************************************************************************/
int32
InitCPakSubscriber (void)
{
  /* Create the memory pool for allocating data stream
   * contexts.
   */
  CPGlobals.CPakContextPool
      = CreateMemPool (CPAK_MAX_SUBSCRIPTIONS, sizeof (CPakContext));
  if (CPGlobals.CPakContextPool == NULL)
    return -1;

  /* Create the memory pool for allocating animation
   * state records.
   */
  CPGlobals.CPakRecPool = CreateMemPool (CPAK_MAX_CHANNELS, sizeof (CPakRec));
  if (CPGlobals.CPakContextPool == NULL)
    return -1;

  /* Return success */
  return 0;
}

/*******************************************************************************************
 * Routine to deallocate resources allocated by InitCPakSubscriber()
 *******************************************************************************************/
int32
CloseCPakSubscriber (void)
{
  /* Create the memory pool for allocating data stream
   * contexts.
   */
  DeleteMemPool (CPGlobals.CPakContextPool);
  CPGlobals.CPakContextPool = NULL;

  DeleteMemPool (CPGlobals.CPakRecPool);
  CPGlobals.CPakRecPool = NULL;

  /* Return success */
  return 0;
}

/*******************************************************************************************
 * Routine to instantiate a new subscriber. Creates the message port through
 *which all subsequent communications take place, as well as any other
 *necessary per-context resources. Creates the service thread and passes the
 *new context to it.
 *******************************************************************************************/
int32
NewCPakSubscriber (CPakContextPtr *pCtx, int32 numChannels, int32 priority)
{
  int32 status;
  CPakContextPtr ctx;
  uint32 signalBits;

  /* Allocate a subscriber context */

  ctx = (CPakContextPtr)AllocPoolMem (CPGlobals.CPakContextPool);
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
      (void *)&CPakSubscriberThread,          /* thread entrypoint */
      4096,                                   /* initial stack size */
      (long)CURRENT_TASK_PRIORITY + priority, /* priority */
      NULL,                                   /* name */
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

  ReturnPoolMem (CPGlobals.CPakContextPool, ctx);

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
DisposeCPakSubscriber (CPakContextPtr ctx)
{

  if (ctx)
    {
      if (ctx->threadItem > 0)
        DisposeThread (ctx->threadItem);
      if (ctx->threadStackBlock)
        FreePtr (ctx->threadStackBlock);

      ReturnPoolMem (CPGlobals.CPakContextPool, ctx);
    }

  // dipose the codec and codec handler we created
  DisposeCodec (ctx->filmCodec);
  DisposeCodecHandler (ctx->codecHndlr);

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
static void
_StopChannel (CPakContextPtr ctx, long channelNumber)
{
  CPakChannelPtr chanPtr;

  if (channelNumber < CPAK_MAX_CHANNELS)
    {
      chanPtr = ctx->channel + channelNumber;

      /* Prevent further data queuing */
      chanPtr->status &= ~CHAN_ENABLED;

      /* Stop any physical processes associated with the channel */
    }
}

/*******************************************************************************************
 * Utility routine to disable further data flow for the given channel, and to
 *cause any associated physical processes associated with the channel to stop.
 *******************************************************************************************/
static void
_FlushChannel (CPakContextPtr ctx, long channelNumber)
{
  CPakChannelPtr chanPtr;
  SubscriberMsgPtr msgPtr;
  SubscriberMsgPtr next;

  if (channelNumber < CPAK_MAX_CHANNELS)
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
          ReplyMsg (msgPtr->msgItem, kDSNoErr, msgPtr,
                    sizeof (SubscriberMsgPtr));

          /* Continue with the next message in the queue */
          msgPtr = next;
        }

      chanPtr->dataQueue.head = NULL;
      chanPtr->dataQueue.tail = NULL;
      UnlockSemaphore (chanPtr->dataQueueSem);
    }
}

/*******************************************************************************************
 * Routine to queue arriving data chunks. All we do here is place the new data
 *into the msg queue for its respective channel. Something else dequeues the
 *data and uses ReplyMsg() to send the empty buffer back to the stream parser.
 *******************************************************************************************/
static int32
DoData (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  int32 status = kDSNoErr;
  SubsChunkDataPtr chunkData;
  CPakChannelPtr chanPtr;
  long channelNumber;
  int32 *channelPtr;

  chunkData = (SubsChunkDataPtr)subMsg->msg.data.buffer;
  channelPtr = &chunkData->channel; /*еее HACK FOR COMPILER PROBLEM еее*/
  channelNumber = *channelPtr;

  if (channelNumber < ctx->numChannels)
    {
      chanPtr = ctx->channel + channelNumber;
      AddDataMsgToTail (&chanPtr->dataQueue, subMsg);
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
DoGetChan (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  int32 status;
  long channelNumber;

  channelNumber = subMsg->msg.channel.number;

  if (channelNumber < CPAK_MAX_CHANNELS)
    status = ctx->channel[channelNumber].status;
  else
    status = 0;

  return status;
}

/*******************************************************************************************
 * Routine to set the channel status bits of a given channel.
 *******************************************************************************************/
static int32
DoSetChan (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  long channelNumber;

  channelNumber = subMsg->msg.channel.number;

  if (channelNumber < CPAK_MAX_CHANNELS)
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
DoControl (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
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
 * Routine to do whatever is necessary when a subscriber is added to a stream,
 *typically just after the stream is opened.
 *******************************************************************************************/
static int32
DoOpening (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  return 0;
}

/*******************************************************************************************
 * Routine to close down an open subscription.
 *******************************************************************************************/
static int32
DoClosing (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  return 0;
}

/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/
static int32
DoStop (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  long channelNumber;

  /* Stop all channels for this subscription.
   */
  for (channelNumber = 0; channelNumber < CPAK_MAX_CHANNELS; channelNumber++)
    _StopChannel (ctx, channelNumber);

  return 0;
}

/*******************************************************************************************
 * Routine to
 *******************************************************************************************/
static int32
DoSync (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  int32 channelNumber;
  CPakChannelPtr chanPtr;

  /* Reply all free chunks */
  DoReplyChunksToDSL (ctx);

  /* Reply all chunks for each channel back to the DSL */
  for (channelNumber = 0; channelNumber < CPAK_MAX_CHANNELS; channelNumber++)
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
DoEOF (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  return 0;
}

/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop
 *everything.
 *******************************************************************************************/
static int32
DoAbort (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{
  long channelNumber;

  /* Halt and flush all channels for this subscription.
   */
  for (channelNumber = 0; channelNumber < CPAK_MAX_CHANNELS; channelNumber++)
    {
      _FlushChannel (ctx, channelNumber);

      /* Halt whatever activity is associated with the channel */
      _StopChannel (ctx, channelNumber);
    }
  return 0;
}

/*******************************************************************************************
 * Routine to get the next available chunk for a channel.
 *******************************************************************************************/
static SubscriberMsgPtr
DoGetChunk (CPakContextPtr ctx, CPakRecPtr cpRecPtr)
{
  SubscriberMsgPtr subMsg;
  CPakChannelPtr chanPtr;

  /* Get a ptr to the SAnimChannel structure for this saRec
   */
  chanPtr = ctx->channel + cpRecPtr->channel;

  /* Return a ptr to the next available chunk in status.
   */
  LockSemaphore (chanPtr->dataQueueSem, 1);

  subMsg = GetNextDataMsg (&chanPtr->dataQueue);

  UnlockSemaphore (chanPtr->dataQueueSem);

  return subMsg;
}

/*******************************************************************************************
 * Routine to free a chunk back to the DSL.
 *******************************************************************************************/
static int32
DoFreeChunk (CPakContextPtr ctx, SubscriberMsgPtr subMsg)
{

  /* Add the msg to the SAnim Subscriber's free list.
   */
  LockSemaphore (ctx->freeQueueSem, 1);

  AddDataMsgToTail (&ctx->freeList, subMsg);

  UnlockSemaphore (ctx->freeQueueSem);
  ctx->freeListNotEmpty = true;

  SendFreeCPakSignal (ctx);

  return 0;
}

/*******************************************************************************************
 * Routine to free all chunks in the freed chunk list back to the DSL.
 *******************************************************************************************/
static void
DoReplyChunksToDSL (CPakContextPtr ctx)
{
  SubscriberMsgPtr subMsg;

  /* Add the msg to the SAnim Subscriber's free list.
   */
  LockSemaphore (ctx->freeQueueSem, 1);

  while ((subMsg = GetNextDataMsg (&ctx->freeList)) != NULL)
    ReplyMsg (subMsg->msgItem, 0, subMsg, 0);

  UnlockSemaphore (ctx->freeQueueSem);
}

/*==========================================================================================
  ==========================================================================================
                                                                        The
  subscriber thread
  ==========================================================================================
  ==========================================================================================*/

static int32
_InitializeThread (CPakContextPtr ctx)
{
  int32 status;
  long channelNumber;
  CPakChannelPtr chanPtr;
  char semName[8], semNum[2];

  /* Initialize fields in the context record */
  ctx->creatorStatus = -1; /* assume initialization will fail */
  ctx->localTimeOrigin = 0;
  ctx->requestPort = 0;
  ctx->cueItem = 0;
  ctx->cueSignal = 0;

  /* Open the Audio Folio for this thread */

  if ((status = OpenAudioFolio ()) < 0)
    {
      ctx->creatorStatus = status;
      goto BAILOUT;
    }

  ctx->cpakTask = CURRENTTASK->t.n_Item; /* see "kernel.h" for this */

  ctx->freeList.head = NULL;
  ctx->freeQueueSem = CreateSemaphore ("freeCPakQueue", 100);
  ctx->freeQueueSignal = AllocSignal (0);
  ctx->freeListNotEmpty = false;

  /* Create a codec handler and codec for decompressing */
  ctx->codecHndlr = CreateCodecHandler ();
  ctx->filmCodec = CreateCodec (ctx->codecHndlr, 0);
  ;

  /* Initialize channel structures */
  chanPtr = ctx->channel;
  for (channelNumber = 0; channelNumber < CPAK_MAX_CHANNELS;
       channelNumber++, chanPtr++)
    {
      chanPtr->status = CHAN_ENABLED;
      chanPtr->dataQueue.head = NULL;
      chanPtr->dataQueue.tail = NULL;
      chanPtr->fFlushOnSync = false;
      chanPtr->imageData.baseAddr
          = 0; /* This tells us we have no LR Form Cel yet */

      /* Create a semaphore to manage access to the dataQueue list for this
       * channel
       */
      semName[0] = 'c';
      semName[1] = 'p';
      semName[2] = 0;
      semNum[0] = (char)('0' + channelNumber); /* {"0","1",...} */
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
 * This thread is started by a call to InitCPakSubscriber(). It reads the
 *subscriber message port for work requests and performs appropriate actions.
 *The subscriber message definitions are located in "DataStreamLib.h".
 *******************************************************************************************/
void
CPakSubscriberThread (int32 notUsed, CPakContextPtr ctx)
{
  int32 status;
  SubscriberMsgPtr subMsg;
  uint32 signalBits;
  uint32 anySignal;
  long channelNumber;
  Boolean fKeepRunning = true;

  /* Call a utility routine to perform all startup initialization.
   */
  if ((status = _InitializeThread (ctx)) != 0)
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
      /* Check for and process any timer expirations */
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
                  status = DoData (ctx, subMsg);
                  break;

                case kStreamOpGetChan: /* get logical channel status */
                  status = DoGetChan (ctx, subMsg);
                  break;

                case kStreamOpSetChan: /* set logical channel status */
                  status = DoSetChan (ctx, subMsg);
                  break;

                case kStreamOpControl: /* perform subscriber defined function
                                        */
                  status = DoControl (ctx, subMsg);
                  break;

                case kStreamOpSync: /* clock stream resynched the clock */
                  status = DoSync (ctx, subMsg);
                  break;

                  /************************************************************************
                   * NOTE:	The following msgs have no extended message
                   *arguments and only may use the whatToDo and context fields
                   *in the message.
                   ************************************************************************/

                case kStreamOpOpening: /* one time initialization call from DSH
                                        */
                  status = DoOpening (ctx, subMsg);
                  break;

                case kStreamOpClosing: /* stream is being closed */
                  status = DoClosing (ctx, subMsg);
                  fKeepRunning = false;
                  break;

                case kStreamOpStop: /* stream is being stopped */
                  status = DoStop (ctx, subMsg);
                  break;

                case kStreamOpEOF: /* physical EOF on data, no more to come */
                  status = DoEOF (ctx, subMsg);
                  break;

                case kStreamOpAbort: /* somebody gave up, stream is aborted */
                  status = DoAbort (ctx, subMsg);
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
  for (channelNumber = 0; channelNumber < CPAK_MAX_CHANNELS; channelNumber++)
    _FlushChannel (ctx, channelNumber);

  /* Exiting should cause all resources we allocated to be
   * returned to the system.
   */
  exit (0);
}
