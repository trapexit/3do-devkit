/*******************************************************************************************
 *	File:			PlaySAnim.c
 *
 *	Contains:		Multi-channel streamed animation file player
 *
 *	Usage:			PlaySAnim <SAnimStreamFile>
 *
 *					"A" button:		Toggles stream
 *stop/start
 *
 *					"B" button:		Toggles printf display
 *of the stream clock
 *
 *					"C" button:		Switches to playing the
 *next stream in the command line list. Rewinds the stream in between switching
 *streams.
 *
 *					"Start" button:	exits the program
 *
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/2/93		jb		Add addition new arg to NewDataStream()
 *	9/20/93		jb		Version 1.1
 *						Added multiple channel rendering. Added
 *conditional compilation for FORCE_CELS_ONTO_SCREEN. 9/20/93		jb
 *Version 1.0 Stripped stuff out of TestDS to make this tool which is oriented
 *more towards playing animation files created with the Director Extractor
 *tool.
 *	=======		==		===========
 *	9/15/93		jb		Version 2.3
 *						Add "gSaChannel" global to allow changing
 *of animation channel number with debugger when playing test streams. 8/25/93
 *jb		Fix test of uninitialized 'status' after call to
 *InitDataStreaming() by actually assigning the result of the call to the
 *'status' variable. 8/11/93		jb		Version 2.2 Add
 *CLOSE_AUDIO_ON_SWITCH code 8/5/93		jb		Version 2.1
 *						Switch to using new interfaces that
 *require the async flag. 8/2/93		jb		Version 2.0
 *						Changes to support changes in Dragontail
 *graphics folio. Fix bug that happened when Cinepak movie was first file in
 *						command line. Forgot to set audio
 *subscriber clock channel to match the audio in the Cinepak example data
 *(channel 1). Use 'curChannelNumber' variable to track the current audio clock
 *channel, and then let the pan and volume controls operate on all audio by
 *referring to this channel to make changes. 6/29/93		ec
 *Version 1.8.1 Show clock time, frames/sec, and calls/sec on 3DO screen.
 *	6/29/93		jb		Version 1.8
 *						Allow use of channel #1 for Cinepak
 *audio 6/25/93		jb		Version 1.7 Make control subscriber one
 *notch higer than all other subscribers so that sync/flush will never flush
 *good data. 6/24/93		rdg		Version 1.6
 *	6/23/93		rdg		In main(), send control message to
 *audio subscriber to preload instrument templates for 44K_16Bit_Stereo and
 *44K_16Bit_Mono so the subscriber won't have to seek for them when the stream
 *starts. Alter main loop to control volume and panning via control messages
 *						when the arrow buttons are
 *pressed. Initalize volume and panning using kSAudioCtlOpGetVol and ...Pan.
 *	6/23/93		jb		Version 1.5
 *						Change FindMathFolio() to OpenMathFolio()
 *for Dragon O/S release 6/22/93		jb		Version 1.4 Add
 *flushing option to start/stop stream calls. Flush when we're stopping and
 *intending to start at a different part of the stream. 6/19/93 jb
 *Version 1.3 Display the stream clock with printf as the stream plays 6/1/93
 *jb		Version 1.2.7 Remove all variable initializers in local
 *variable declarations to avoid register bashing by the ARM C compiler.
 *						Dispose of data acquisition threads at
 *exit time. 5/30/93		jb		Version 0.2
 *	5/30/93		jb		Change 'debugNext' field in DSDataBuf
 *struct to 'permanentNext'
 *	5/20/93		jb		Integrate Eric's changes and switch to
 *using DSConnect() to attach data acquisition to a stream.
 *	5/18/93		jb		Remove all references to DSHRunStream()
 *and all the funky timing stuff that went with it. 4/19/93		jb
 *Add DSHCloseStream() call at end, change to using updated interface to
 *NewDataAcq() and NewTestSubscriber(). 4/17/93		jb New today
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.2"

/* The following compile switches control various parameters for initializing
 * and creating the streaming environment.
 */
#define NUMBER_OF_STREAMS 1
#define DATA_BUFFER_COUNT 4
#define DATA_BUFFER_SIZE (32 * 1024)
#define DATA_ACQ_DELTA_PRI 8
#define STREAMER_DELTA_PRI 6
#define SUBSCRIBER_DELTA_PRI 7
#define AUDIO_SUBSCRIBER_DELTA_PRI 10
#define CTRL_SUBSCRIBER_DELTA_PRI (AUDIO_SUBSCRIBER_DELTA_PRI + 1)

#define NUM_DIRECTOR_CHANNELS 24

/* Setting the following compile switch will cause cels that would appear
 * offscreen to be centered on the screen if the X or Y position exceed the
 * following maxima.
 */
#define FORCE_CELS_ONTO_SCREEN 1
#define MAX_SAFE_X_POS (320L << 16)
#define MAX_SAFE_Y_POS (240L << 16)

#include "Init3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "DataAcq.h"
#include "DataStreamDebug.h"
#include "DataStreamLib.h"

#include "ControlSubscriber.h"
#include "SAnimSubscriber.h"
#include "SAudioSubscriber.h"

/* If true, this will cause a closechannel message to be sent to
   the audio subscriber when the stream is switched, or rewound
   with the "C" button.
 */
#define CLOSE_AUDIO_ON_SWITCH 1

/*************************************/
/* Handy macro for diagnostic output */
/*************************************/
#ifndef CHECKRESULT
#define CHECKRESULT(name, result)                                             \
  if (((long)result) < 0)                                                     \
    {                                                                         \
      printf ("Failure in %s: %d ($%x)\n", name, ((long)result),              \
              ((long)result));                                                \
      PrintfSysErr (result);                                                  \
      exit (0);                                                               \
    }
#endif

/* Stream timer defines */
enum
{
  kInvalidClock = -1
};

/*********************************************************************************************
 * Library and system globals
 *********************************************************************************************/
ScreenContext gScreenContext;
ubyte *gBackgroundPtr = NULL;
long gScreenSelect = 0;
Item VBLIOReq;

/***********************************/
/* Stream related global variables */
/***********************************/
SAnimContextPtr gSaAnimCntxPtr; /* ANIM context ptr	*/
SAnimRecPtr gSaRecChanPtr[NUM_DIRECTOR_CHANNELS];

SAudioContextPtr gSAudioCntxPtr; /* audio context */
long gAudioChannel = 0;

CtrlContextPtr gCntrlCntxPtr; /* control subscriber context ptr */

/******************************/
/* Utility routine prototypes */
/******************************/
static Boolean StartUp (void);
static void EraseScreen (ScreenContext *sc, int32 screenNum);
static int32 InitStreaming (DSStreamCBPtr *streamCBPtrPtr,
                            DSDataBufPtr bufferList, long bufferSize,
                            Item messageItem);

/*===========================================================================================
 ============================================================================================
                                                                        Utility
 Routines
 ============================================================================================
 ===========================================================================================*/

/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
static Boolean
StartUp (void)
{
  gScreenContext.sc_nScreens = 2;

  VBLIOReq = GetVBLIOReq ();

  if (!OpenGraphics (&gScreenContext, 2))
    return false;

  if (!OpenMacLink ())
    ;

  if (!OpenSPORT ())
    return false;

  if (!OpenAudio ())
    return false;

  if ((OpenMathFolio () != 0))
    return false;

  return true;
}

/*********************************************************************************************
 * Routine to erase the current screen.
 *********************************************************************************************/
void
EraseScreen (ScreenContext *sc, int32 screenNum)
{
  Item VRAMIOReq;

  VRAMIOReq = GetVRAMIOReq ();
  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[screenNum]->bm_Buffer, 0,
                sc->sc_nFrameBufferPages, -1);
  DeleteItem (VRAMIOReq);
}

/*********************************************************************************************
 * Routine to create a buffer pool to be used by the streamer. All buffers are
 *made to be the same size, bufferSize, and are returned as a linked list.
 *********************************************************************************************/
static DSDataBufPtr
CreateBufferList (long numBuffers, long bufferSize)
{
#define ROUND_TO_LONG(x) ((x + 3) & ~3)
  DSDataBufPtr bp;
  DSDataBufPtr lastBp;
  long totalBufferSpace;
  long actualEntrySize;

  actualEntrySize = sizeof (DSDataBuf) + ROUND_TO_LONG (bufferSize);
  totalBufferSpace = numBuffers * actualEntrySize;

  bp = (DSDataBufPtr)malloc (totalBufferSpace);
  if (bp == NULL)
    {
      printf ("malloc() of buffer list space failed!\n");
      exit (0);
    }

  /* Make a linked list of entries */
  lastBp = NULL;
  while (numBuffers-- > 0)
    {
      bp->permanentNext = lastBp;
      bp->next = lastBp;
      lastBp = bp;

      bp = (DSDataBufPtr)(((char *)bp) + actualEntrySize);
    }

  /* Return a pointer to the first buffer in the list
   */
  return lastBp;
}

/*********************************************************************************************
 * Routine to perform high level streaming initialization.
 *********************************************************************************************/
static int32
InitStreaming (DSStreamCBPtr *streamCBPtrPtr, DSDataBufPtr bufferList,
               long bufferSize, Item messageItem)
{
  int32 status;
  long channelNum;

  /***********************************/
  /* Initialize the streaming system */
  /***********************************/
  status = InitDataStreaming (NUMBER_OF_STREAMS);
  CHECK_DS_RESULT ("InitDataStreaming", status);

  status = NewDataStream (streamCBPtrPtr, bufferList, bufferSize,
                          STREAMER_DELTA_PRI, 200);
  CHECK_DS_RESULT ("NewDataStream", status);

  /*********************************/
  /* Init the animation subscriber */
  /*********************************/
  status = InitSAnimSubscriber ();
  CHECK_DS_RESULT ("InitSAnimSubscriber", status);

  status = NewSAnimSubscriber (&gSaAnimCntxPtr, NUMBER_OF_STREAMS,
                               SUBSCRIBER_DELTA_PRI);
  CHECK_DS_RESULT ("NewSAnimSubscriber", status);

  /* Attach the anim subscriber to the stream */
  status
      = DSSubscribe (messageItem,                  // control msg item
                     NULL,                         // synchronous call
                     *streamCBPtrPtr,              // stream context block
                     (DSDataType)SANM_CHUNK_TYPE,  // subscriber data type
                     gSaAnimCntxPtr->requestPort); // subscriber message port
  CHECK_DS_RESULT ("DSSubscribe for sanm", status);

  for (channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++)
    {
      status
          = InitSAnimCel (*streamCBPtrPtr, // Need this for DSGetClock in sub.
                          gSaAnimCntxPtr,  // The subscriber's context
                          gSaRecChanPtr + channelNum, // The channel's context
                          channelNum,                 // The channel number
                          true); // true = flush on synch msg from streamer
      CHECK_DS_RESULT ("InitSAnimCel", status);
    }

  /*****************************/
  /* Init the audio subscriber */
  /*****************************/
  status = InitSAudioSubscriber ();
  CHECK_DS_RESULT ("InitCtrlSubscriber", status);

  status = NewSAudioSubscriber (&gSAudioCntxPtr, *streamCBPtrPtr,
                                AUDIO_SUBSCRIBER_DELTA_PRI);
  CHECK_DS_RESULT ("NewSAudioSubscriber", status);

  status
      = DSSubscribe (messageItem,                  // control msg item
                     NULL,                         // synchronous call
                     *streamCBPtrPtr,              // stream context block
                     (DSDataType)SNDS_CHUNK_TYPE,  // subscriber data type
                     gSAudioCntxPtr->requestPort); // subscriber message port
  CHECK_DS_RESULT ("DSSubscribe for audio subscriber", status);

  /*******************************/
  /* Init the control subscriber */
  /*******************************/
  status = InitCtrlSubscriber ();
  CHECK_DS_RESULT ("InitCtrlSubscriber", status);

  status = NewCtrlSubscriber (&gCntrlCntxPtr, *streamCBPtrPtr,
                              CTRL_SUBSCRIBER_DELTA_PRI);
  CHECK_DS_RESULT ("NewCtrlSubscriber", status);

  status = DSSubscribe (messageItem,                 // control msg item
                        NULL,                        // synchronous call
                        *streamCBPtrPtr,             // stream context block
                        (DSDataType)CTRL_CHUNK_TYPE, // subscriber data type
                        gCntrlCntxPtr->requestPort); // subscriber message port
  CHECK_DS_RESULT ("DSSubscribe for control subscriber", status);

  return status;
}

/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void
Usage (char *programName)
{
  printf ("%s version %s\n", programName, PROGRAM_VERSION_STRING);
  printf ("usage: %s <-c | -a | -s> <streamFile> \n", programName);
  printf ("\t\"A\" button:	Toggles stream stop/start\n");
  printf ("\t\"B\" button:	Toggles stream timing\n");
  printf ("\t\"C\" button:	rewind to start of stream\n");
}

/*********************************************************************************************
 * Show stream frame timing
 *********************************************************************************************/
static Item gTimerRequest;
static IOInfo gTimerInfo;

/*
 * start up a timer for ourselves
 */
void
InitInstrumentation (void)
{
  Item timerDevice;
  IOReq *timerReq;
  TagArg ioTags[2];

  ioTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioTags[0].ta_Arg = 0;
  ioTags[1].ta_Tag = TAG_END;
  ioTags[1].ta_Arg = 0;

  /* Timer initialization */
  timerDevice = OpenItem (
      FindNamedItem (MKNODEID (KERNELNODE, DEVICENODE), "timer"), 0);
  if (timerDevice < 0)
    {
      PrintfSysErr (timerDevice);
      return;
    }

  ioTags[0].ta_Arg = (void *)timerDevice;
  gTimerRequest = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), ioTags);

  if (gTimerRequest < 0)
    {
      PrintfSysErr (gTimerRequest);
      exit (0);
    }
  timerReq = (IOReq *)LookupItem (gTimerRequest);
  gTimerInfo.ioi_Unit = 1;
}

/*
 * Get the current second count from our private timer
 */
uint32
CurrentSeconds (void)
{
  static int32 gTimerHasStarted = 0;
  struct timeval tmVal;

  /* has the timer been initialized??? */
  if (gTimerHasStarted == 0)
    {
      InitInstrumentation ();
      gTimerHasStarted = 1;
    }

  /* Read elapsed time */
  gTimerInfo.ioi_Command = CMD_READ;
  gTimerInfo.ioi_Recv.iob_Buffer = &tmVal;
  gTimerInfo.ioi_Recv.iob_Len = sizeof (tmVal);
  DoIO (gTimerRequest, &gTimerInfo);

  /* we want seconds... */
  return tmVal.tv_sec;
}

/*
 * draw the current timer & stream clock to the screen
 */
void
PutNumXY (int32 callsPerSec, int32 secCount, uint32 streamClock, int32 x,
          int32 y)
{
  static char gTimeStr[20];
  static char gClockStr[24];
  static char gCallsPerSecStr[20];
  static int32 gLastSecCount = -1;
  static int32 gLastStreamClock = -1;
  static int32 gLastCallsPerSec = -1;
  GrafCon localGrafCon;

  /* Don't reset the strings unless we have a new time/clock, just draw the
   * last string we made
   */
  if (secCount != gLastSecCount)
    {
      sprintf (gTimeStr, "frame/sec: %2lu", secCount);
      gLastSecCount = secCount;
    }

  if (streamClock != gLastStreamClock)
    {
      if (streamClock != kInvalidClock)
        sprintf (gClockStr, "stream clock: %2lu", streamClock);
      else
        sprintf (gClockStr, "stream clock: *INVALID*");
      gLastStreamClock = streamClock;
    }

  if (callsPerSec != gLastCallsPerSec)
    {
      sprintf (gCallsPerSecStr, "calls/sec: %2lu", callsPerSec);
      gLastCallsPerSec = callsPerSec;
    }

#if 0
	localGrafCon.gc_PenX = x;
	localGrafCon.gc_PenY = y;
	DrawText8(&localGrafCon, gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], 
					gTimeStr);
	localGrafCon.gc_PenX = x;
	localGrafCon.gc_PenY = y + 12;
	DrawText8(&localGrafCon, gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], 
					gCallsPerSecStr);
#endif

  localGrafCon.gc_PenX = x;
  localGrafCon.gc_PenY = y + 24;
  DrawText8 (&localGrafCon,
             gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen],
             gClockStr);
}

/*
 * Calculate and show frame timing
 */
void
ShowStreamTiming (DSStreamCBPtr streamCBPtr)
{
  static void *gLastFrame = NULL;
  static int32 gCallsPerSec;
  static int32 gCallCount = 0;
  static int32 gPrevTime = -1;
  static int32 gFramesPerSec;
  static int32 gFrameCount = 0;
  static uint32 gStreamClock;
  int32 currTime;
  void *tempFrame;

  tempFrame = (void *)gSaRecChanPtr[0]->curFramePtr;

  currTime = CurrentSeconds ();
  ++gCallCount;

  // do we have a different frame from last time? if so updat the global
  //  frame/sec counter
  if (gLastFrame != tempFrame)
    ++gFrameCount;

  // has a second passed? if so, update the number of times
  //  that we've been through this proc in the last second
  if (currTime != gPrevTime)
    {
      // update the fps counter
      gFramesPerSec = gFrameCount;
      gFrameCount = 0;

      // update the stream time
      if (DSGetClock (streamCBPtr, &gStreamClock) != kDSNoErr)
        gStreamClock = kInvalidClock;

      gPrevTime = currTime;
      gCallsPerSec = gCallCount;
      gCallCount = 0;
    }

  // remember this frame for next time through
  gLastFrame = tempFrame;

  // draw the last second's average count again...
  PutNumXY (gCallsPerSec, gFramesPerSec, gStreamClock, 20, 195);
}

/*===========================================================================================
 ============================================================================================
                                                                        Main
 Program Logic
 ============================================================================================
 ===========================================================================================*/

/*********************************************************************************************
 * Main program
 *********************************************************************************************/
int
main (int argc, char **argv)
{
  long status;
  long joybits;
  Item messagePort;
  Item messageItem;
  Boolean fRunStream;
  Boolean fDisplayStreamClock;
  DSDataBufPtr dataBufferList;
  DSStreamCBPtr streamCBPtr;
  AcqContextPtr dataContextPtr;
  SAudioCtlBlock ctlBlock;
  long channelNum;
  CCB *ccbListHead;
  CCB *ccbListTail;
  CCB *ccbPtr;

  fRunStream = true;
  fDisplayStreamClock = false;

  /* Initialize the library code */
  if (!StartUp ())
    {
      printf ("StartUp() initialization failed!\n");
      exit (0);
    }

  // sanity check, need at least the name of a file
  if (argc < 2)
    {
      printf ("### %s - invalid parameter list, need at least a stream file "
              "name.\n",
              argv[0]);
      Usage (argv[0]);
      exit (0);
    }

  /* We need to create a message port and one message item
   * to communicate with the data streamer.
   */
  messagePort = NewMsgPort (NULL);
  CHECKRESULT ("NewMsgPort", messagePort);

  messageItem = CreateMsgItem (messagePort);
  CHECKRESULT ("CreateMsgItem", messageItem);

  /* Initialize the data buffer list */
  dataBufferList = CreateBufferList (DATA_BUFFER_COUNT, DATA_BUFFER_SIZE);

  /* Initialize data acquisition */
  status = InitDataAcq (1 /* file count == 1 */);
  CHECK_DS_RESULT ("InitDataAcq", status);

  status = NewDataAcq (&dataContextPtr, argv[1], DATA_ACQ_DELTA_PRI);
  CHECK_DS_RESULT ("NewDataAcq 1", status);

  status = InitStreaming (&streamCBPtr, dataBufferList, DATA_BUFFER_SIZE,
                          messageItem);
  CHECKRESULT ("InitStreaming", status);

  /* Define which data types this audio subscriber must be able to play
   * without going to disk for the instrument (preload).
   */
  {
    long templateTags[4];

    templateTags[0] = SA_44K_16B_S;
    templateTags[1] = SA_44K_16B_M;
    templateTags[2] = SA_22K_16B_M_SDX2;
    templateTags[3] = 0;

    ctlBlock.loadTemplates.tagListPtr = templateTags;
    status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                        kSAudioCtlOpLoadTemplates, &ctlBlock);
    CHECK_DS_RESULT ("DSControl", status);
  }

  /* Attach the initial data acquisition to the stream
   */
  status = DSConnect (messageItem, NULL, streamCBPtr,
                      dataContextPtr->requestPort);
  CHECK_DS_RESULT ("DSConnect()", status);

  status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
  CHECK_DS_RESULT ("DSStartStream", status);

  /* The SANM Stream has no SYNC CTRL message at its front so we
   * have to jump-start this stream. If This stream did have a SYNC
   * message, the GOTO CTRL message at the end of the stream would
   * cause the Control Subscriber to process the SYNC immediatly upon
   * receipt of the SYNC chunk. This would almost certainly be BEFORE
   * the SAnim subscriber has completed processing the last frame Cel.
   * Thus, the last frames in the looping SAnim would never be seen.
   */
  status = DSSetClock (streamCBPtr, 0);
  CHECKRESULT ("DSSetClock()", status);

  /* Set the audio clock to use the selected channel */
  ctlBlock.clock.channelNumber = 0;
  status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                      kSAudioCtlOpSetClockChan, &ctlBlock);
  CHECK_DS_RESULT ("DSControl - setting audio clock chan = 1", status);

  EraseScreen (&gScreenContext, gScreenContext.sc_curScreen);

  /******************/
  /* Run the stream */
  /******************/
  while (true)
    {
      joybits = ReadControlPad (0);
      if (joybits & JOYSTART)
        break;

      /* The "A" button toggles running the stream
       */
      if (joybits & JOYFIREA)
        {
          /* Flip the logical value of the "stream running" flag
           */
          fRunStream = !fRunStream;

          /* If we're transitioning from stopped to running, start the
           * stream. Otherwise, stop the stream.
           */
          if (fRunStream)
            {
              status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
              CHECK_DS_RESULT ("DSStartStream", status);
              printf ("Stream started.\n");

              /* Start clock for animation subscriber in case there's
               * no audio data in the stream to do this for us.
               */
              status = DSSetClock (streamCBPtr, 0);
              CHECKRESULT ("DSSetClock()", status);
            }
          else
            {
              /* Note: we don't use the SOPT_FLUSH option when we stop
               * here so that the "A" button acts like a pause/resume
               * key. Using the SOPT_FLUSH option will cause buffered
               * data to be flushed, so that once resumed, any queued
               * video will drop frames to "catch up". This is not what
               * is usually desired for pause/resume.
               */
              status = DSStopStream (messageItem, NULL, streamCBPtr, 0);
              CHECK_DS_RESULT ("DSStopStream", status);
              printf ("Stream stopped.\n");
            }

          printf (" stream flags = %lx\n", streamCBPtr->streamFlags);
        }

      /* The "B" button toggles displaying the stream clock
       */
      if (joybits & JOYFIREB)
        {
          fDisplayStreamClock = !fDisplayStreamClock;
          if (fDisplayStreamClock)
            printf ("Stream timing on.\n");
          else
            printf ("Stream timing off.\n");
        }

      if (joybits & JOYFIREC)
        {
          // stop the current stream and flush the subscriber to make sure
          //  it doesn't hang onto any buffers.
          if (fRunStream)
            {
              status
                  = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
              CHECK_DS_RESULT ("DSStopStream", status);
            }

          for (channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS;
               channelNum++)
            FlushSAnimChannel (gSaAnimCntxPtr, gSaRecChanPtr[channelNum], 0);

          // tell the newly connected stream to reset itself to 0 so that we
          // get the
          //  ccb from the file directly (the subscriber can't respond to our
          //  polling until it gets the CCB, which is only found at the
          //  begining of the file)
          status = DSGoMarker (messageItem, NULL, streamCBPtr,
                               (unsigned long)0, GOMARKER_ABSOLUTE);
          CHECK_DS_RESULT ("DSGoMarker", status);

          if (fRunStream)
            {
              status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
              CHECK_DS_RESULT ("DSStartStream", status);

              /* Start clock for animation subscriber in case there's
               * no audio data in the stream to do this for us.
               */
              status = DSSetClock (streamCBPtr, 0);
              CHECKRESULT ("DSSetClock()", status);
            }
        }

      if (joybits & JOYUP)
        {
          long volume;

          ctlBlock.amplitude.channelNumber = gAudioChannel;
          status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                              kSAudioCtlOpGetVol, &ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
          volume = ctlBlock.amplitude.value;

          if (volume < 0x7A00)
            {
              volume += 0x500;

              ctlBlock.amplitude.channelNumber = gAudioChannel;
              ctlBlock.amplitude.value = volume;

              status
                  = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                               kSAudioCtlOpSetVol, &ctlBlock);
              if (status < 0)
                {
                  printf ("\n error sending control message!\n");
                  printf (" status = %ld.\n", status);
                }
            }
        }

      if (joybits & JOYDOWN)
        {
          long volume;

          ctlBlock.amplitude.channelNumber = gAudioChannel;
          status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                              kSAudioCtlOpGetVol, &ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
          volume = ctlBlock.amplitude.value;

          if (volume > 0x600)
            {
              volume -= 0x500;

              ctlBlock.amplitude.channelNumber = gAudioChannel;
              ctlBlock.amplitude.value = volume;

              status
                  = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                               kSAudioCtlOpSetVol, &ctlBlock);
              if (status < 0)
                {
                  printf ("\n error sending control message!\n");
                  printf (" status = %ld.\n", status);
                }
            }
        }

      if (joybits & JOYLEFT)
        {
          long pan;

          ctlBlock.pan.channelNumber = gAudioChannel;
          status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                              kSAudioCtlOpGetPan, &ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
          pan = ctlBlock.pan.value;

          if (pan > 0x600)
            {
              pan -= 0x500;

              ctlBlock.pan.channelNumber = gAudioChannel;
              ctlBlock.pan.value = pan;

              status
                  = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                               kSAudioCtlOpSetPan, &ctlBlock);
              if (status < 0)
                {
                  printf ("\n error sending control message!\n");
                  printf (" status = %ld.\n", status);
                }
            }
        }

      if (joybits & JOYRIGHT)
        {
          long pan;

          ctlBlock.pan.channelNumber = gAudioChannel;
          status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                              kSAudioCtlOpGetPan, &ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
          pan = ctlBlock.pan.value;

          if (pan < 0x7A00)
            {
              pan += 0x500;

              ctlBlock.pan.channelNumber = gAudioChannel;
              ctlBlock.pan.value = pan;

              status
                  = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                               kSAudioCtlOpSetPan, &ctlBlock);
              if (status < 0)
                {
                  printf ("\n error sending control message!\n");
                  printf (" status = %ld.\n", status);
                }
            }
        }

      /****************************************************************/
      /* Get the next animation frames from the subscriber and draw 	*/
      /* them on the screen.
       */
      /****************************************************************/
      ccbListHead = NULL;
      ccbListTail = NULL;

      for (channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++)
        {
          /* Get the next frame */
          ccbPtr = GetSAnimCel (gSaAnimCntxPtr, gSaRecChanPtr[channelNum]);
          if (ccbPtr != NULL)
            {
              /* Always mark the current cel as the "end of list" */
              LAST_CEL (ccbPtr);

              /* If there's already one in the list, add the new one
               * onto the end of the list, else set the head pointer to
               * point to the one we just got (make tail point there too).
               */
              if (ccbListTail != NULL)
                {
                  ccbListTail->ccb_Flags
                      &= ~CCB_LAST; /* clear "last ccb" bit */
                  ccbListTail->ccb_Flags
                      |= CCB_NPABS; /* set "next ptr absolute" bit */
                  ccbListTail->ccb_NextPtr
                      = ccbPtr; /* point to new addition */
                }
              else
                /* Add first and only entry to list */
                ccbListHead = ccbPtr;

              /* Always remember the last one in the list */
              ccbListTail = ccbPtr;

#if FORCE_CELS_ONTO_SCREEN
              if ((ccbPtr->ccb_XPos > MAX_SAFE_X_POS)
                  || (ccbPtr->ccb_YPos > MAX_SAFE_Y_POS))
                CenterCelOnScreen (ccbPtr);
#endif
            }
        }

      /* Draw the ccb list if it is non-empty */
      if (ccbListHead != NULL)
        {
          gScreenContext.sc_curScreen = 1 - gScreenContext.sc_curScreen;

          WaitVBL (VBLIOReq, 1);
          EraseScreen (&gScreenContext, gScreenContext.sc_curScreen);
          status = DrawScreenCels (
              gScreenContext.sc_Screens[gScreenContext.sc_curScreen],
              ccbListHead);
          if (status != 0)
            printf ("DrawCels() error...\n");

          /* Tell the SAnimSubscriber to reply all freed chunks back to the
           * streamer */
          SendFreeSAnimSignal (gSaAnimCntxPtr);
        }

      /* Display the stream clock if we're supposed to do so, show the new
       * image
       */
      if (fDisplayStreamClock)
        ShowStreamTiming (streamCBPtr);

      DisplayScreen (gScreenContext.sc_Screens[gScreenContext.sc_curScreen],
                     0);
    }

  /* Close the stream */
  printf ("Stream stopping\n");
  status = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
  CHECK_DS_RESULT ("DSStopStream", status);

  /* flush anything held by the anim subscriber */
  for (channelNum = 0; channelNum < NUM_DIRECTOR_CHANNELS; channelNum++)
    FlushSAnimChannel (gSaAnimCntxPtr, gSaRecChanPtr[channelNum], 0);

  printf ("Disposing the stream\n");
  status = DisposeDataStream (messageItem, streamCBPtr);
  CHECK_DS_RESULT ("DisposeDataStream", status);

  DisposeDataAcq (dataContextPtr);

  /* Get rid of our message stuff */
  RemoveMsgItem (messageItem);
  RemoveMsgPort (messagePort);
  exit (0);
}
