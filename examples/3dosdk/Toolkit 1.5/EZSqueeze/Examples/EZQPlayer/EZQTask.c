/*
        File:		EZQTask.c

        Contains:	EZQ Task Code (Thread)

        Written by:	 by Bill Duvall

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

*/

#include "debug.h" /* for printf() */
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "portfolio.h"
#include "stdio.h"
#include "task.h"
#include "types.h"

#include "Init3DO.h"
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

#include "EZQTask.h"
#include "PlayEZQStream.h"
#include "SpawnThread.h"

EZQ_Parms EZQ_Task_Parms;

extern Boolean continuousPlayMode;

extern long skipFrameCount;

extern int32 InitEZQPlayerFromStreamHeader (PlayerPtr ctx,
                                            char *streamFileName);
extern void DismantlePlayer (PlayerPtr ctx);

static void killEZQTask (void);

static Signal continueSignal;
static Signal runSignal;
static Signal pauseSignal;
static Signal stepSignal;
static Signal stopSignal;
static Signal controlSignals;

int32
waitBufferFree (PlayerPtr ctx, Frame *frame)
{
  Signal result;
  do
    {
      Yield (); // WaitVBL(ctx->VBLIOReq, 0);
      result = controlSignals & WaitSignal (continueSignal | controlSignals);
      SendSignal (CURRENT_TASK, continueSignal);
      if (result != 0)
        {
          // printf("\n *** Signal Result = %08X [%08X] ***\n", result,
          // controlSignals);
          return (result); // abort the stream
        }
    }
  while (frame->valid);
  return 0;
}

int32
do_EZQStream (PlayerPtr ctx)
{
  int32 status;
  Signal signals;
  DSRequestMsg EOSMessage;
  int32 validFrameDecoded;

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

  /* If there is no audio in the stream to validate the stream clock,
   * then set the clock to zero at this point. This will enable streams
   * with no audio present to run.
   */

  if (ctx->audioContextPtr == NULL)
    {
      status = DSSetClock (ctx->streamCBPtr, 0);
      CHECK_DS_RESULT ("DSSetClock", status);
      if (status != 0)
        return status;
    }

  SendSignal (CURRENT_TASK, continueSignal);

  while (true)
    {
      if (signals = waitBufferFree (
              ctx, &EZQ_Task_Parms.frames[EZQ_Task_Parms.frameIndex]))
        {
          if (signals & stopSignal)
            {
              // printf("\n**STOPPED**");
              break; // abort the stream
            }
          else if (signals & pauseSignal)
            {
              // printf("\n**PAUSED**");
              continuousPlayMode = false;
              continue;
            }
          else if (signals & runSignal)
            {
              // printf("\n**RE-START**");
              continuousPlayMode = true;
              continue;
            }
          else if (signals & stepSignal)
            {
              // printf("\n**STEP**");
              continuousPlayMode = false;
            }
        }
      else if (!continuousPlayMode)
        continue;

      if (PollForMsg (ctx->messagePort, NULL, NULL, NULL, &status))
        break;

      /* In case there's no Cinepak in this "Cinepak stream" skip
       * trying to draw something that doesn't exist.
       */

      if (ctx->ezqContextPtr != nil)
        {
          /* Now try to unpack a frame (if one exists) */
          // printf("\n** Frame **\n");
          if (continuousPlayMode)
            {
              validFrameDecoded = DrawEZQToBuffer (
                  ctx->ezqContextPtr, ctx->ezqChannelPtr,
                  (Bitmap *)EZQ_Task_Parms.frames[EZQ_Task_Parms.frameIndex]
                      .pixMap,
                  0);
            }
          else
            validFrameDecoded = DrawStepEZQToBuffer (
                ctx->ezqContextPtr, ctx->ezqChannelPtr,
                (Bitmap *)EZQ_Task_Parms.frames[EZQ_Task_Parms.frameIndex]
                    .pixMap);
          if (validFrameDecoded)
            {
              // printf("\n** Decoded **\n");
              EZQ_Task_Parms.frames[EZQ_Task_Parms.frameIndex].valid = 1;
              EZQ_Task_Parms.frameIndex = (EZQ_Task_Parms.frameIndex + 1)
                                          % EZQ_Task_Parms.frameCount;
              // printf("*");
              // if ((ctx->frameCount++ % -64) == 0) printf("\n");
              ctx->frameCount++;
            }

          /* Free any streamer chunks back to the streamer (we're done with
           * them now) */
          SendFreeEZQSignal (ctx->ezqContextPtr);
        }
    }

  /* Unconditionally stop the stream */

  status = DSStopStream (ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH);
  CHECK_DS_RESULT ("DSStopStream: PlayEZQStream", status);

  if (ctx->ezqContextPtr
      != 0) /* Flush anything held by the Cinepak subscriber */
    FlushEZQChannel (ctx->ezqContextPtr, ctx->ezqChannelPtr, 0);

  return (2); /* All done -- return code */
}

void
EZQStreamTask (void)
{
  int32 status;
  int32 playerResult;
  PlayerPtr ctx;
  Player playerContext;

  /* Open the specified file, locate the stream header, and initialize
   * all resources necessary for performing stream playback.
   */

  char *streamFileName;
  PlayerFuntionContext *pfc;

  streamFileName = EZQ_Task_Parms.streamFileName;
  pfc = EZQ_Task_Parms.pfc;

  // printf("\n**EZQ Task Started , file = %s**", streamFileName);

  continueSignal = AllocSignal (0);
  // printf("\nContinue = %08X", continueSignal);
  runSignal = AllocSignal (0);
  // printf("\nrunSignal = %08X", runSignal);
  pauseSignal = AllocSignal (0);
  // printf("\npauseSignal = %08X", pauseSignal);
  stepSignal = AllocSignal (0);
  // printf("\nstepSignal = %08X", stepSignal);
  stopSignal = AllocSignal (0);
  // printf("\nstopSignal = %08X", stopSignal);

  controlSignals = runSignal | pauseSignal | stepSignal | stopSignal;

  status = InitEZQPlayerFromStreamHeader (&playerContext, streamFileName);
  if (status != 0)
    {
      EZQ_Task_Parms.result = status;
      printf ("\nInitEZQ Error, code = %08X", status);
      PrintfSysErr (status);
    }
  else
    {
      pfc->playerCtx = ctx = &playerContext;
      playerContext.frameCount = 0;
      playerResult = do_EZQStream (ctx);

      /* Get rid of stuff we allocated */
      /* printf("\nPlayed %d frames, skipped %d frames",
         playerContext.frameCount, playerContext.ezqContextPtr->framesDropped);
       */

      DismantlePlayer (ctx);
      EZQ_Task_Parms.result = playerResult;
    }
  // printf("\nAbout to Die");
  Yield ();
  EZQ_Task_Parms.running = 0;
}

void
startEZQTask (char *streamFileName, PlayerFuntionContext *pfc,
              int32 frameCount, char *bufferA, char *bufferB, char *bufferC)
{
  if (EZQ_Task_Parms.running)
    killEZQTask ();
  // printf("\n** Starting Cpak Task **");

  EZQ_Task_Parms.streamFileName = streamFileName;
  EZQ_Task_Parms.pfc = pfc;
  EZQ_Task_Parms.frameCount = frameCount;
  EZQ_Task_Parms.frameIndex = 0;
  EZQ_Task_Parms.frames[0].pixMap = bufferA;
  EZQ_Task_Parms.frames[1].pixMap = bufferB;
  EZQ_Task_Parms.frames[2].pixMap = bufferC;
  EZQ_Task_Parms.frames[0].valid = 0;
  EZQ_Task_Parms.frames[1].valid = 0;
  EZQ_Task_Parms.frames[2].valid = 0;
  EZQ_Task_Parms.task_Item
      = SpawnThread (EZQStreamTask, 8192, CURRENT_TASK_PRIORITY, "EZQ Task");
  // printf("EZQ Task is item %x\n", EZQ_Task_Parms.task_Item);
  EZQ_Task_Parms.result = 0;
  EZQ_Task_Parms.running = 1;
}

void
killEZQTask (void)
{
  if (EZQ_Task_Parms.running)
    {
      // printf("\n** Stopping Cpak Task **");
      DeleteThread (EZQ_Task_Parms.task_Item);
      EZQ_Task_Parms.running = 0;
    }
  // printf("Stopped EZQ Task, item %x\n", EZQ_Task_Parms.task_Item);
  EZQ_Task_Parms.result = 1;
}

void
runEZQTask (void)
{
  if (EZQ_Task_Parms.running)
    {
      // printf("\n** Call Run **");
      SendSignal (EZQ_Task_Parms.task_Item, runSignal);
    }
}

void
pauseEZQTask (void)
{
  if (EZQ_Task_Parms.running)
    {
      // printf("\n** Call Pause **");
      SendSignal (EZQ_Task_Parms.task_Item, pauseSignal);
    }
}

void
stepEZQTask (void)
{
  if (EZQ_Task_Parms.running)
    {
      // printf("\n** Call Step **");
      SendSignal (EZQ_Task_Parms.task_Item, stepSignal);
    }
}

void
stopEZQTask (void)
{
  // printf("\n** Call Stop **");
  if (EZQ_Task_Parms.running)
    SendSignal (EZQ_Task_Parms.task_Item, stopSignal);
}
