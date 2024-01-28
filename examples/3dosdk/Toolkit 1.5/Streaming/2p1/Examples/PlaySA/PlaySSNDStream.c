/*******************************************************************************************
 *	File:			PlaySSNDStream.c
 *
 *	Contains:		high level Audio stream playback function
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/05/94		dtc		Added include <stdio.h>
 *	12/3/93			jb		Change test for stream header
 *version to != from >
 *	12/2/93			jb		Add addition new arg to
 *NewDataStream()
 *	10/25/93		rdg		Modify to play SSND only
 *streams. Move connect to dataAcq into Init routine.
 *	10/20/93		jb		Do Cinepak only playback; need to get
 *this out by the Developer's Symposium...
 *	10/14/93		jb		New today
 *
 *******************************************************************************************/

#include "Init3DO.h"
#include "Portfolio.h"
#include "UMemory.h"
#include "Utils3DO.h"

#include "ControlSubscriber.h"
#include "DataAcq.h"
#include "DataStreamDebug.h"
#include "DataStreamLib.h"
#include "SAudioSubscriber.h"

#include "DSStreamHeader.h"
#include "PlaySSNDStream.h"

#include <stdio.h>

/************************************/
/* Local utility routine prototypes */
/************************************/
static int32 InitSSNDPlayerFromStreamHeader (PlayerPtr ctx,
                                             char *streamFileName);
void DismantlePlayer (PlayerPtr ctx);

/*******************************************************************************************
 * Routine to play an SSND stream file. Stream may or may not contain audio
 *data. Audio channel selection is based upon info found in the stream header.
 *******************************************************************************************/
int32
PlaySSNDStream (char *streamFileName, PlaySSNDUserFn userFn, void *userCB,
                int32 callBackInterval)
{
  int32 status;
  int32 playerResult;
  PlayerPtr ctx;
  Player playerContext;
  DSRequestMsg EOSMessage;
  Item aCue;

  /* Open the specified file, locate the stream header, and initialize
   * all resources necessary for performing stream playback.
   */
  status = InitSSNDPlayerFromStreamHeader (&playerContext, streamFileName);
  if (status != 0)
    return status;

  ctx = &playerContext;

  /* Remember the user function pointer and context, if any */
  ctx->userFn = userFn;
  ctx->userCB = userCB;

  /* Create an audio cue so we can call back periodically */
  aCue = CreateItem (MKNODEID (AUDIONODE, AUDIO_CUE_NODE), NULL);
  if (aCue <= 0)
    return (int32)aCue;

  /* Start the stream running */
  status = DSStartStream (ctx->messageItem, NULL, ctx->streamCBPtr, 0);
  CHECK_DS_RESULT ("DSStartStream", status);
  if (status != 0)
    return status;

  /* Register for end of stream notification */
  status = DSWaitEndOfStream (ctx->endOfStreamMessageItem, &EOSMessage,
                              ctx->streamCBPtr);
  CHECK_DS_RESULT ("DSWaitEndOfStream", status);
  if (status != 0)
    return status;

  playerResult = 0;

  while (true)
    {
      /* If a user function is defined, then call it. If the user function
       * returns a non-zero value, then we should exit.
       */
      if (ctx->userFn)
        {
          playerResult = (*ctx->userFn) ((void *)ctx);

          if (playerResult)
            break;
        }

      /* Check for end of stream and print message if done...
       * Don't break out of loop unless instructed to by UI button press
       */
      if (PollForMsg (ctx->messagePort, NULL, NULL, NULL, &status))
        {
          status = DSStopStream (ctx->messageItem, NULL, ctx->streamCBPtr,
                                 SOPT_FLUSH);
          CHECK_DS_RESULT ("DSStopStream", status);

          printf (
              "\nReceived  End of Stream  message and stopped stream... \n");
        }

      /* Sleep until it's time to call the user function again */
      SleepUntilTime (aCue, callBackInterval + GetAudioTime ());
    }

  /* Unconditionally stop the stream */
  status = DSStopStream (ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH);
  CHECK_DS_RESULT ("DSStopStream", status);

  /* Free everything */
  DismantlePlayer (ctx);

  return playerResult;
}

/*******************************************************************************************
 * Routine to get the stream header info, and perform all necessary allocations
 * and initializations necessary for stream playback.
 *******************************************************************************************/
static int32
InitSSNDPlayerFromStreamHeader (PlayerPtr ctx, char *streamFileName)
{
  int32 status;
  long subscriberIndex;
  long channelNum;
  SAudioCtlBlock ctlBlock;
  DSHeaderSubsPtr subsPtr;

  /* Initialize fields to zero so that cleanup can
   * tell what has been allocated.
   */
  ctx->bufferList = 0;
  ctx->streamCBPtr = 0;
  ctx->acqContext = 0;
  ctx->messageItem = 0;
  ctx->endOfStreamMessageItem = 0;
  ctx->messagePort = 0;
  ctx->controlContextPtr = 0;
  ctx->audioContextPtr = 0;

  /* Get the stream header loaded */
  status = FindAndLoadStreamHeader (&ctx->hdr, streamFileName);
  if (status != 0)
    return status;

  /* Make sure this playback code is compatible with the version of the
   * data in the stream file.
   */
  if (ctx->hdr.headerVersion != DS_STREAM_VERSION)
    return kPSVersionErr;

  /* Allocate the stream buffers and build the linked list of
   * buffers that are input to the streamer.
   */
  ctx->bufferList
      = CreateBufferList (ctx->hdr.streamBuffers, ctx->hdr.streamBlockSize);
  if (ctx->bufferList == NULL)
    return kPSMemFullErr;

  /* We need to create a message port and one message item
   * to communicate with the data streamer.
   */
  ctx->messagePort = NewMsgPort (NULL);
  if (ctx->messagePort < 0)
    goto CLEANUP;

  ctx->messageItem = CreateMsgItem (ctx->messagePort);
  if (ctx->messageItem < 0)
    goto CLEANUP;

  ctx->endOfStreamMessageItem = CreateMsgItem (ctx->messagePort);
  if (ctx->endOfStreamMessageItem < 0)
    goto CLEANUP;

  /* Initialize data acquisition for use with 1 file */
  status = InitDataAcq (1);
  if (status != 0)
    goto CLEANUP;

  /* Initialize data acquisition for the specified file */
  status = NewDataAcq (&ctx->acqContext, streamFileName,
                       ctx->hdr.dataAcqDeltaPri);
  if (status != 0)
    goto CLEANUP;

  /* Initialize for 1 streamer thread */
  status = InitDataStreaming (1);
  if (status != 0)
    goto CLEANUP;

  status = NewDataStream (
      &ctx->streamCBPtr,         /* output: stream control block ptr */
      ctx->bufferList,           /* pointer to buffer list */
      ctx->hdr.streamBlockSize,  /* size of each buffer */
      ctx->hdr.streamerDeltaPri, /* streamer thread relative priority */
      ctx->hdr.numSubsMsgs);     /* number of subscriber messages */
  if (status != 0)
    goto CLEANUP;

  /* Connect the stream to its data supplier */
  status = DSConnect (ctx->messageItem, NULL, ctx->streamCBPtr,
                      ctx->acqContext->requestPort);
  CHECK_DS_RESULT ("DSConnect()", status);

  /* Loop through the subscriber descriptor table and initialize all
   * subscribers specified in the table.
   */
  for (subscriberIndex = 0;
       ctx->hdr.subscriberList[subscriberIndex].subscriberType != 0;
       subscriberIndex++)
    {
      subsPtr = ctx->hdr.subscriberList + subscriberIndex;

      switch (subsPtr->subscriberType)
        {
        case SNDS_CHUNK_TYPE:
          status = InitSAudioSubscriber ();
          CHECK_DS_RESULT ("InitCtrlSubscriber", status);
          if (status != 0)
            goto CLEANUP;

          status = NewSAudioSubscriber (
              &ctx->audioContextPtr, ctx->streamCBPtr, subsPtr->deltaPriority);
          CHECK_DS_RESULT ("NewSAudioSubscriber", status);
          if (status != 0)
            goto CLEANUP;

          status = DSSubscribe (
              ctx->messageItem,                   // control msg item
              NULL,                               // synchronous call
              ctx->streamCBPtr,                   // stream context block
              (DSDataType)SNDS_CHUNK_TYPE,        // subscriber data type
              ctx->audioContextPtr->requestPort); // subscriber message port
          CHECK_DS_RESULT ("DSSubscribe for audio subscriber", status);
          if (status != 0)
            goto CLEANUP;

          break;

        case CTRL_CHUNK_TYPE:
          status = InitCtrlSubscriber ();
          CHECK_DS_RESULT ("InitCtrlSubscriber", status);
          if (status != 0)
            goto CLEANUP;

          status
              = NewCtrlSubscriber (&ctx->controlContextPtr, ctx->streamCBPtr,
                                   subsPtr->deltaPriority);
          CHECK_DS_RESULT ("NewCtrlSubscriber", status);
          if (status != 0)
            goto CLEANUP;

          status = DSSubscribe (
              ctx->messageItem,                     // control msg item
              NULL,                                 // synchronous call
              ctx->streamCBPtr,                     // stream context block
              (DSDataType)CTRL_CHUNK_TYPE,          // subscriber data type
              ctx->controlContextPtr->requestPort); // subscriber message port
          CHECK_DS_RESULT ("DSSubscribe for control subscriber", status);
          if (status != 0)
            goto CLEANUP;
          break;

        default:
          status = kPSUnknownSubscriber;
          printf ("\n InitSSNDPlayerFromStreamHeader -- Unknown subscriber "
                  "ident %.4s found in stream header!\n",
                  (char *)&subsPtr->subscriberType);
          goto CLEANUP;
        }
    }

  /* Preload audio instrument templates, if any are specified
   */
  if (ctx->hdr.preloadInstList != 0)
    {
      ctlBlock.loadTemplates.tagListPtr = ctx->hdr.preloadInstList;

      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpLoadTemplates, &ctlBlock);
      if (status != 0)
        goto CLEANUP;
    }

  /* Enable any audio channels whose enable bit is set.
   * NOTE: Channel zero is enabled by default, so we don't check it.
   */
  for (channelNum = 1; channelNum < 32; channelNum++)
    {
      /* If the bit corresponding to the channel number is set,
       * then tell the audio subscriber to enable that channel.
       */
      if (ctx->hdr.enableAudioChan & (1L << channelNum))
        {
          status = DSSetChannel (ctx->messageItem, NULL, ctx->streamCBPtr,
                                 SNDS_CHUNK_TYPE, channelNum, CHAN_ENABLED);
          CHECK_DS_RESULT ("DSSetChannel", status);
          if (status != 0)
            goto CLEANUP;
        }
    }

  /* Set the audio clock to use the selected channel */
  ctlBlock.clock.channelNumber = ctx->hdr.audioClockChan;
  status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                      SNDS_CHUNK_TYPE, kSAudioCtlOpSetClockChan, &ctlBlock);
  CHECK_DS_RESULT ("DSControl - setting audio clock chan", status);

  if (status == 0)
    return 0;

CLEANUP:
  DismantlePlayer (ctx);
  return status;
}

/*******************************************************************************************
 * Routine to free all resources associated with a Player structure. Assumes
 *that all relevant fields are set to ZERO when the struct is initialized.
 *******************************************************************************************/
void
DismantlePlayer (PlayerPtr ctx)
{
  if (ctx->streamCBPtr)
    DisposeDataStream (ctx->messageItem, ctx->streamCBPtr);

  if (ctx->acqContext)
    DisposeDataAcq (ctx->acqContext);

  if (ctx->controlContextPtr)
    DisposeCtrlSubscriber (ctx->controlContextPtr);

  if (ctx->audioContextPtr)
    DisposeSAudioSubscriber (ctx->audioContextPtr);

  if (ctx->bufferList)
    FreePtr (ctx->bufferList);

  if (ctx->messageItem)
    RemoveMsgItem (ctx->messageItem);
  if (ctx->endOfStreamMessageItem)
    RemoveMsgItem (ctx->endOfStreamMessageItem);
  if (ctx->messagePort)
    RemoveMsgPort (ctx->messagePort);
}
