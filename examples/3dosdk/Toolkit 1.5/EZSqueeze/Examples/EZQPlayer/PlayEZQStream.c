/*******************************************************************************************
 *	File:			PlayEZQStream.c
 *
 *	Contains:		high level EZSqueeze stream playback function
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/6/94			WSD		Change to spawn a separate task
 *for decompression to allow 3 display buffers
 *	12/3/93			jb		Change test for stream header
 *version to != from >
 *	12/2/93			jb		Add "close" routines to
 *DismantlePlayer() Set stream clock to zero at start stream time if there is
 *no audio present in the stream to allow silent movies to play properly. Add
 *addition new arg to NewDataStream()
 *	11/22/93		jb		Add call to DismantlePlayer() at the
 *end of PlayCPakStream() 10/25/93		jb		Fix order of
 *disposals in DismantlePlayer() 10/20/93		jb		Do
 *Cinepak only playback; need to get this out by the Developer's Symposium...
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
#include "EZQSubscriber.h"
#include "SAudioSubscriber.h"

#include "3DO_Player.h"
#include "DSStreamHeader.h"
#include "PlayEZQStream.h"

#include "EZQTask.h"

extern void EraseScreen (ScreenContext *sc, int32 screenNum);

// extern long decodedFrame; // for double buffering
extern long skipFrameCount;
#define DoubleBuffer

/************************************/
/* Local utility routine prototypes */
/************************************/
int32 InitEZQPlayerFromStreamHeader (PlayerPtr ctx, char *streamFileName);
void DismantlePlayer (PlayerPtr ctx);

/*******************************************************************************************
 * Routine to play a Cinepak stream file. Stream may or may not contain audio
 *data. Audio channel selection is based upon info found in the stream header.
 *******************************************************************************************/

Boolean continuousPlayMode = 1;

//************************************************************************************
//
//	ClosePlayerFunction -- cleanup of one time initialization for the
//player functions
//
//************************************************************************************
static Boolean
_FreeMsgItem (void *argumentNotUsed, void *poolEntry)
{
  DeleteItem (((DSRequestMsgPtr)poolEntry)->msgItem);
  return true;
}

/*******************************************************************************************
 * Routine to play a Cinepak stream file. Stream may or may not contain audio
 *data. Audio channel selection is based upon info found in the stream header.
 *******************************************************************************************/

int32
PlayEZQStream (ScreenContext *screenContextPtr, char *streamFileName,
               PlayEZQUserFn userFn, void *userContext,
               PlayerFuntionContext *pfc)
{
  int32 i;
  int32 playerResult;
  int32 screen_ID, oldScreen_ID;

  startEZQTask (streamFileName, pfc, 3,
                (char *)screenContextPtr->sc_Bitmaps[0],
                (char *)screenContextPtr->sc_Bitmaps[1],
                (char *)screenContextPtr->sc_Bitmaps[2]);

  screen_ID = 0;
  oldScreen_ID = EZQ_Task_Parms.frameCount - 1;

  for (i = 0; i < EZQ_Task_Parms.frameCount; i++)
    EraseScreen (screenContextPtr, i);

  DisplayScreen (screenContextPtr->sc_Screens[oldScreen_ID], 0);

  while (EZQ_Task_Parms.running)
    {
      if (EZQ_Task_Parms.frames[screen_ID].valid)
        {
          DisplayScreen (screenContextPtr->sc_Screens[screen_ID], 0);
          EZQ_Task_Parms.frames[oldScreen_ID].valid
              = 0; // allow this one to be used
          oldScreen_ID = screen_ID;
          screen_ID = (screen_ID + 1) % EZQ_Task_Parms.frameCount;
        }
      else
        Yield ();
      if ((userFn)
          && ((playerResult = (*userFn) ((void *)pfc, userContext)) != 0))
        {
          switch (playerResult)
            {
            case kStopMovie:
              stopEZQTask ();
              break;
            case kDSStepFrame:
              pauseEZQTask ();
              break;
            case kDSContinuousFrame:
              runEZQTask ();
              break;
            case kDSDrawSingleFrame:
              stepEZQTask ();
              break;
            }
        }
    }

  for (i = 0; i < EZQ_Task_Parms.frameCount; i++)
    EraseScreen (screenContextPtr, i);

  return EZQ_Task_Parms.result;
}

/*******************************************************************************************
 * Routine to get the stream header info, and perform all necessary allocations
 * and initializations necessary for stream playback.
 *******************************************************************************************/
int32
InitEZQPlayerFromStreamHeader (PlayerPtr ctx, char *streamFileName)
{
  int32 status;
  long subscriberIndex;
  long channelNum;
  SAudioCtlBlock ctlBlock;
  DSHeaderSubsPtr subsPtr;
  Boolean fStreamHasAudio;

  /* Assume no audio */
  fStreamHasAudio = false;

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
  ctx->ezqChannelPtr = 0;
  ctx->ezqContextPtr = 0;

  if ((ctx->VBLIOReq = GetVBLIOReq ()) < 0)
    {
      status = ctx->VBLIOReq;
      return status;
    }
  // printf("\n*1*\n");
  /* Get the stream header loaded */
  status = FindAndLoadStreamHeader (&ctx->hdr, streamFileName);
  if (status != 0)
    return status;

  // printf("\n*2*\n");

  /* Make sure this playback code is compatible with the version of the
   * data in the stream file.
   */
  if (ctx->hdr.headerVersion != DS_STREAM_VERSION)
    return kPSVersionErr;

  // printf("\n*3*\n");
  /* Allocate the stream buffers and build the linked list of
   * buffers that are input to the streamer.
   */
  ctx->bufferList
      = CreateBufferList (ctx->hdr.streamBuffers, ctx->hdr.streamBlockSize);
  if (ctx->bufferList == NULL)
    return kPSMemFullErr;

  // printf("\n*4*\n");
  /* We need to create a message port and one message item
   * to communicate with the data streamer.
   */
  ctx->messagePort = NewMsgPort (NULL);
  if (ctx->messagePort < 0)
    goto CLEANUP;

  // printf("\n*5*\n");
  ctx->messageItem = CreateMsgItem (ctx->messagePort);
  if (ctx->messageItem < 0)
    goto CLEANUP;

  // printf("\n*6*\n");
  ctx->endOfStreamMessageItem = CreateMsgItem (ctx->messagePort);
  if (ctx->endOfStreamMessageItem < 0)
    goto CLEANUP;

  // printf("\n*7*\n");
  /* Initialize data acquisition for use with 1 file */
  status = InitDataAcq (1);
  if (status != 0)
    goto CLEANUP;

  // printf("\n*8*\n");
  /* Initialize data acquisition for the specified file */
  status = NewDataAcq (&ctx->acqContext, streamFileName,
                       ctx->hdr.dataAcqDeltaPri);
  if (status != 0)
    goto CLEANUP;

  // printf("\n*9*\n");
  /* Initialize for 1 streamer thread */
  status = InitDataStreaming (1);
  if (status != 0)
    goto CLEANUP;

  // printf("\n*10*\n");
  status = NewDataStream (
      &ctx->streamCBPtr,         /* output: stream control block ptr */
      ctx->bufferList,           /* pointer to buffer list */
      ctx->hdr.streamBlockSize,  /* size of each buffer */
      ctx->hdr.streamerDeltaPri, /* streamer thread relative priority */
      ctx->hdr.numSubsMsgs);     /* number of subscriber messages */
  if (status != 0)
    goto CLEANUP;

  // printf("\n*11*\n");
  /* Connect the stream to its data supplier */
  status = DSConnect (ctx->messageItem, NULL, ctx->streamCBPtr,
                      ctx->acqContext->requestPort);
  CHECK_DS_RESULT ("DSConnect()", status);
  if (status != 0)
    goto CLEANUP;

  // printf("\n*12*\n");
  /* Loop through the subscriber descriptor table and initialize all
   * subscribers specified in the table.
   */
  for (subscriberIndex = 0;
       ctx->hdr.subscriberList[subscriberIndex].subscriberType != 0;
       subscriberIndex++)
    {
      subsPtr = ctx->hdr.subscriberList + subscriberIndex;

      //		printf(" SubscriberType = %x\n",
      //subsPtr->subscriberType );
      switch (subsPtr->subscriberType)
        {
        case FILM_CHUNK_TYPE:
          status = InitEZQSubscriber ();
          CHECK_DS_RESULT ("InitEZQSubscriber", status);
          if (status != 0)
            goto CLEANUP;

          status = NewEZQSubscriber (&ctx->ezqContextPtr, 1,
                                     subsPtr->deltaPriority);
          CHECK_DS_RESULT ("NewEZQSubscriber", status);
          if (status != 0)
            goto CLEANUP;

          status = DSSubscribe (
              ctx->messageItem,                 // control msg item
              NULL,                             // synchronous call
              ctx->streamCBPtr,                 // stream context block
              (DSDataType)FILM_CHUNK_TYPE,      // subscriber data type
              ctx->ezqContextPtr->requestPort); // subscriber message port
          CHECK_DS_RESULT ("DSSubscribe for cinepak", status);
          if (status != 0)
            goto CLEANUP;

          status = InitEZQCel (
              ctx->streamCBPtr,    // Need this for DSGetClock in sub
              ctx->ezqContextPtr,  // The subscriber's context
              &ctx->ezqChannelPtr, // The channel's context
              0,                   // The channel number
              true);               // true = flush on synch msg from DS
          CHECK_DS_RESULT ("InitEZQCel", status);
          if (status != 0)
            goto CLEANUP;
          break;

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

          fStreamHasAudio = true;
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
          printf ("InitEZQPlayerFromStreamHeader() - unknown subscriber in "
                  "stream header: '%.4s'\n",
                  (char *)&subsPtr->subscriberType);
          status = kPSUnknownSubscriber;
          goto CLEANUP;
        }
    }

  // printf("\n*13*\n");
  /* If the stream has audio, then do some additional initializations.
   */
  if (fStreamHasAudio)
    {
      /* Preload audio instrument templates, if any are specified
       */
      if (ctx->hdr.preloadInstList != 0)
        {
          ctlBlock.loadTemplates.tagListPtr = ctx->hdr.preloadInstList;

          status = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                              SNDS_CHUNK_TYPE, kSAudioCtlOpLoadTemplates,
                              &ctlBlock);
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
              status
                  = DSSetChannel (ctx->messageItem, NULL, ctx->streamCBPtr,
                                  SNDS_CHUNK_TYPE, channelNum, CHAN_ENABLED);
              CHECK_DS_RESULT ("DSSetChannel", status);
              if (status != 0)
                goto CLEANUP;
            }
        }

      /* Set the audio clock to use the selected channel */
      ctlBlock.clock.channelNumber = ctx->hdr.audioClockChan;
      status
          = DSControl (ctx->messageItem, NULL, ctx->streamCBPtr,
                       SNDS_CHUNK_TYPE, kSAudioCtlOpSetClockChan, &ctlBlock);
      CHECK_DS_RESULT ("DSControl - setting audio clock chan", status);
    }

  if (status == 0)
    return 0;

  // printf("\n*14*\n");
CLEANUP:
  // printf("\n*15*\n");
  DismantlePlayer (ctx);
  return status;
}

/*******************************************************************************************
 * Routine to free all resources associated with a Player structure. Assumes
 *that all relevant fields are set to ZERO when the struct is initialized.
 *
 *	NOTE:	THE ORDER OF THE FOLLOWING DISPOSALS IS IMPORTANT. DO NOT
 *CHANGE UNLESS YOU KNOW WHAT YOU ARE DOING.
 *
 *******************************************************************************************/
void
DismantlePlayer (PlayerPtr ctx)
{
  if (ctx->streamCBPtr)
    {
      DisposeDataStream (ctx->messageItem, ctx->streamCBPtr);
      CloseDataStreaming ();
    }

  if (ctx->acqContext)
    {
      DisposeDataAcq (ctx->acqContext);
      CloseDataAcq ();
    }

  if (ctx->controlContextPtr)
    {
      DisposeCtrlSubscriber (ctx->controlContextPtr);
      CloseCtrlSubscriber ();
    }

  if (ctx->ezqChannelPtr)
    DestroyEZQCel (ctx->ezqContextPtr, ctx->ezqChannelPtr,
                   ctx->ezqChannelPtr->channel);

  if (ctx->ezqContextPtr)
    {
      DisposeEZQSubscriber (ctx->ezqContextPtr);
      CloseEZQSubscriber ();
    }

  if (ctx->audioContextPtr)
    {
      DisposeSAudioSubscriber (ctx->audioContextPtr);
      CloseSAudioSubscriber ();
    }

  if (ctx->bufferList)
    FreePtr (ctx->bufferList);
  if (ctx->messageItem)
    RemoveMsgItem (ctx->messageItem);
  if (ctx->endOfStreamMessageItem)
    RemoveMsgItem (ctx->endOfStreamMessageItem);
  if (ctx->messagePort)
    RemoveMsgPort (ctx->messagePort);
  if (ctx->VBLIOReq)
    DeleteItem (ctx->VBLIOReq);
}
