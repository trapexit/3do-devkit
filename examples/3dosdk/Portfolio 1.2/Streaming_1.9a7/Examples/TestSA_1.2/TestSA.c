/*******************************************************************************************
 *	File:			TestSA.c
 *
 *	Contains:		Test 3DO streamed audio player application
 *
 *	Usage:			TestSA <streamFile> ...
 *					"A" button:		Toggles stream
 *stop/start "B" button:		Advances current channel number (0 -
 *4). "C" button:		Rewinds the stream
 *
 *					"Start" button:	exits the program
 *
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/2/93		jb		Add addition new arg to NewDataStream()
 *	10/11/93	rdg		Make sure CLOSE_AUDIO_ON_SWITCH re-enables
 *channels after they are closed. 9/30/93		rdg		Change
 *CLOSE_AUDIO_ON_SWITCH to send close messages to channels 0-4 on rewind.
 *	9/29/93		rdg		Enable chans 1-4, don't init volume to
 *zero. 9/28/93		rdg		Fix volume control... was impossible to
 *get full max and min. 9/27/93		rdg		Add template tag for
 *16Bit, 22K, Stereo, 2:1 SDX2 9/20/93		rdg		Enable channel
 *2 Support up to 3 concurrent logical channels 9/17/93		rdg
 *Modified the code which parses the "C" button so that it will always call
 *StopWithFlush.  This way, if you are playing a stream and you... Stop the
 *stream with the "A" button. Rewind with the "C" button. Start again with the
 *"A" button. data will be flushed by the subscribers as it should be. Before
 *this mod you would hear the last audio buffer queued before the rewind when
 *you re-started the stream.
 *	9/14/93		rdg		New today; cannabalized from TestDS
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.2"

#define DATA_BUFFER_COUNT 6
#define DATA_BUFFER_SIZE (32 * 1024)

/* If true, this will cause a Close Channel message to be sent to
   channels 0-4 of the audio subscriber when the stream is rewound
   with the "C" button.  Close channel causes all resource which
   have been allocated for the channel to be freed.  This allows
   you to change data types on the fly without have to keep DSP
   instruments loaded that you no longer need.
 */
#define CLOSE_AUDIO_ON_SWITCH 0

#include "Init3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "DataAcq.h"
#include "DataStreamDebug.h"
#include "DataStreamLib.h"

#include "ControlSubscriber.h"
#include "SAudioSubscriber.h"

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

/*********************************************************************************************
 * Library and system globals
 *********************************************************************************************/
ScreenContext gScreenContext;    // needed for the lib3DO startup routine
SAudioContextPtr gSAudioCntxPtr; // audio context
CtrlContextPtr gCntrlCntxPtr;    // control subscriber context ptr

/******************************/
/* Utility routine prototypes */
/******************************/
static Boolean StartUp (void);
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

  if (!OpenGraphics (&gScreenContext, 2))
    return false;

  if (!OpenMacLink ())
    ;

  if (!OpenSPORT ())
    return false;

  if ((OpenAudioFolio () != 0))
    return false;

  if ((OpenMathFolio () != 0))
    return false;

  return true;
}

/*********************************************************************************************
 * Routine to initialize streaming
 *********************************************************************************************/
#define ROUND_TO_LONG(x) ((x + 3) & ~3)

static DSDataBufPtr
CreateBufferList (long numBuffers, long bufferSize)
{
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

  // Initialize streaming and open a new stream
#define NUMBER_OF_STREAMS 1
  status = InitDataStreaming (NUMBER_OF_STREAMS);
  CHECK_DS_RESULT ("InitDataStreaming", status);

  /* Initialize a new stream
   */
#define STREAMER_DELTA_PRI 6
  status = NewDataStream (streamCBPtrPtr, bufferList, bufferSize,
                          STREAMER_DELTA_PRI, 200);
  CHECK_DS_RESULT ("NewDataStream", status);

#define AUDIO_SUBSCRIBER_DELTA_PRI 10
  // always instantiate an audio subscriber too, never can tell when a guy
  //  will want one...
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

#define CTRL_SUBSCRIBER_DELTA_PRI (AUDIO_SUBSCRIBER_DELTA_PRI + 1)
  // and finally the control subscriber, we ALWAYS want one of these
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
  printf ("usage: %s <streamFile> \n", programName);
  printf ("\t\"A\" button:	Toggles stream stop/start\n");
  printf (
      "\t\"B\" button:	Toggles current channel number between 0 and 1.\n");
  printf ("\t\"C\" button:	Rewinds the stream.\n");
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
  DSStreamCBPtr streamCBPtr;
  DSDataBufPtr dataBufferList;
  Boolean fRunStream;
  long templateTags[7];
  SAudioCtlBlock ctlBlock;
  long volume;
  long pan;
  long curChannelNumber;

  AcqContextPtr dataContextPtr;
  char *streamFileName;

  fRunStream = true;
  curChannelNumber = 0;

  /* Initialize the library code */
  if (!StartUp ())
    {
      printf ("StartUp() initialization failed!\n");
      exit (0);
    }

  // sanity check, need at least the name of a file
  if (argc < 2)
    {
      printf ("Error: %s - invalid parameter list, need a stream file name.\n",
              argv[0]);
      Usage (argv[0]);
      exit (0);
    }

  streamFileName = argv[1];

  /* We need to create a message port and one message item
   * to communicate with the data streamer.
   */
  messagePort = NewMsgPort (NULL);
  CHECKRESULT ("NewMsgPort", messagePort);

  messageItem = CreateMsgItem (messagePort);
  CHECKRESULT ("CreateMsgItem", messageItem);

  /* Initialize the data buffer list */
  dataBufferList = CreateBufferList (DATA_BUFFER_COUNT, DATA_BUFFER_SIZE);

  /* Initialize data acquisition
   */
#define DATA_ACQ_DELTA_PRI 8
  status = InitDataAcq (1);
  CHECK_DS_RESULT ("InitDataAcq", status);

  status = NewDataAcq (&dataContextPtr, streamFileName, DATA_ACQ_DELTA_PRI);
  CHECK_DS_RESULT ("NewDataAcq", status);

  status = InitStreaming (&streamCBPtr, dataBufferList, DATA_BUFFER_SIZE,
                          messageItem);
  CHECKRESULT ("InitStreaming", status);

  /* Define which data types this audio subscriber must be able to play
   * without going to disk for the instrument (preload).
   */

  templateTags[0] = SA_44K_16B_S;
  templateTags[1] = SA_44K_16B_M;
  templateTags[2] = SA_22K_16B_M_SDX2;
  templateTags[3] = SA_22K_16B_S_SDX2;
  templateTags[4] = SA_44K_16B_M_SDX2;
  templateTags[5] = SA_44K_16B_S_SDX2;
  templateTags[6] = 0;

  ctlBlock.loadTemplates.tagListPtr = templateTags;

  status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                      kSAudioCtlOpLoadTemplates, &ctlBlock);
  CHECK_DS_RESULT ("DSControl", status);

  /* Enable audio channel #1 */
  status = DSSetChannel (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE, 1,
                         CHAN_ENABLED);
  CHECK_DS_RESULT ("DSSetChannel", status);

  /* Enable audio channel #2 */
  status = DSSetChannel (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE, 2,
                         CHAN_ENABLED);
  CHECK_DS_RESULT ("DSSetChannel", status);

  /* Enable audio channel #3 */
  status = DSSetChannel (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE, 3,
                         CHAN_ENABLED);
  CHECK_DS_RESULT ("DSSetChannel", status);

  /* Enable audio channel #4 */
  status = DSSetChannel (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE, 4,
                         CHAN_ENABLED);
  CHECK_DS_RESULT ("DSSetChannel", status);

  /* Attach the initial data acquisition to the stream
   */
  status = DSConnect (messageItem, NULL, streamCBPtr,
                      dataContextPtr->requestPort);
  CHECK_DS_RESULT ("DSConnect()", status);

  status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
  CHECK_DS_RESULT ("DSStartStream", status);

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

      if (joybits & JOYFIREB)
        {
          curChannelNumber += 1;

          /* wrap from 2 to 0 */
          if (curChannelNumber == 5)
            curChannelNumber = 0;

          printf ("Current channel = %ld.\n", curChannelNumber);
        }

      if (joybits & JOYFIREC)
        {
          // stop the current stream and flush the subscriber to make sure
          //  it doesn't hang onto any buffers.
          status = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
          CHECK_DS_RESULT ("DSStopStream", status);

          printf ("Rewinding stream...\n");

#if CLOSE_AUDIO_ON_SWITCH
          /* Send close message to all 5 channels */
          for (curChannelNumber = 0; curChannelNumber < 5; curChannelNumber++)
            {
              ctlBlock.closeChannel.channelNumber = curChannelNumber;
              status
                  = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                               kSAudioCtlOpCloseChannel, &ctlBlock);
              CHECK_DS_RESULT ("DSControl - closing channel", status);

              /* ReEnable audio channels so they will receive data */
              status = DSSetChannel (messageItem, NULL, streamCBPtr,
                                     SNDS_CHUNK_TYPE, curChannelNumber,
                                     CHAN_ENABLED);
              CHECK_DS_RESULT ("DSSetChannel", status);
            }

          /* Reset the current channel to 0 */
          curChannelNumber = 0;
          printf ("Current channel = %ld.\n", curChannelNumber);
#endif

          // tell the newly connected stream to reset itself to 0
          status = DSGoMarker (messageItem, NULL, streamCBPtr,
                               (unsigned long)0, GOMARKER_ABSOLUTE);
          CHECK_DS_RESULT ("DSGoMarker", status);

          if (fRunStream)
            {
              status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
              CHECK_DS_RESULT ("DSStartStream", status);
            }
        }

      if (joybits & JOYUP)
        {
          ctlBlock.amplitude.channelNumber = curChannelNumber;
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

              ctlBlock.amplitude.channelNumber = curChannelNumber;
              ctlBlock.amplitude.value = volume;
            }
          else
            {
              volume = 0x7FFF;

              ctlBlock.amplitude.channelNumber = curChannelNumber;
              ctlBlock.amplitude.value = volume;
            }

          status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                              kSAudioCtlOpSetVol, &ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
        }

      if (joybits & JOYDOWN)
        {
          ctlBlock.amplitude.channelNumber = curChannelNumber;
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

              ctlBlock.amplitude.channelNumber = curChannelNumber;
              ctlBlock.amplitude.value = volume;
            }
          else
            {
              volume = 0x0;

              ctlBlock.amplitude.channelNumber = curChannelNumber;
              ctlBlock.amplitude.value = volume;
            }

          status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                              kSAudioCtlOpSetVol, &ctlBlock);
          if (status < 0)
            {
              printf ("\n error sending control message!\n");
              printf (" status = %ld.\n", status);
            }
        }

      if (joybits & JOYLEFT)
        {
          ctlBlock.pan.channelNumber = curChannelNumber;
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

              ctlBlock.pan.channelNumber = curChannelNumber;
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
          ctlBlock.pan.channelNumber = curChannelNumber;
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

              ctlBlock.pan.channelNumber = curChannelNumber;
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
    }
  /* Close the stream */
  printf ("Stream stopping\n");
  status = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
  CHECK_DS_RESULT ("DSStopStream", status);

  printf ("Disposing the stream\n");
  status = DisposeDataStream (messageItem, streamCBPtr);
  CHECK_DS_RESULT ("DisposeDataStream", status);

  printf ("Disposing data acquisition.\n");
  DisposeDataAcq (dataContextPtr);

  /* Get rid of our message stuff */
  RemoveMsgItem (messageItem);
  RemoveMsgPort (messagePort);
  exit (0);
}
