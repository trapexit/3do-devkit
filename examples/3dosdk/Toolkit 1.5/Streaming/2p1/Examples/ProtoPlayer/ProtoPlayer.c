/*******************************************************************************************
 *	File:			ProtoPlayer.c
 *
 *	Contains:		Simple example of playing test data through the
 *Prototype Subscriber.
 *
 *	Usage:			ProtoPlayer <streamFile>
 *					"A" button:		Rewinds the
 *stream
 *
 *					"Stop" button:	exits the program
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright й 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  09/01/94	dtc		Replaced kprintf with printf and changed
 *version number to 1.1. 4/04/94		rdg		New today.. cut
 *and past from many sources.
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.1"

/**************************************************************
 * Buffer sizes, number of buffers, and other setup constants
 **************************************************************/
#define NUMBER_OF_STREAMS 1          // Number of streams to initalize
#define DATA_BUFFER_COUNT 6          // How many streaming buffers to create
#define DATA_BUFFER_SIZE (32 * 1024) // Size of the streaming buffers

/********************************************************
 * Thread priority definitions, relative to main task
 ********************************************************/
#define STREAMER_DELTA_PRI 6
#define SUBSCRIBER_DELTA_PRI 7
#define DATA_ACQ_DELTA_PRI 8
#define CTRL_SUBSCRIBER_DELTA_PRI 11

/************************
 * Header File Includes
 ************************/
#include "Init3DO.h"
#include "JoyPad.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "DataAcq.h"
#include "DataStreamDebug.h"
#include "DataStreamLib.h"

#include "ControlSubscriber.h"
#include "ProtoSubscriber.h"

#include "UMemory.h"

/*************************************
 * Handy macro for quadbyte alignment
 *************************************/
#define ROUND_TO_LONG(x) ((x + 3) & ~3)

/*************************************
 * Handy macro for diagnostic output
 *************************************/
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

/******************************/
/* Utility routine prototypes */
/******************************/
static void Usage (char *programName);
static DSDataBufPtr CreateBufferList (long numBuffers, long bufferSize);

/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void
Usage (char *programName)
{
  printf ("%s version %s\n", programName, PROGRAM_VERSION_STRING);
  printf ("usage: %s <streamFile> \n", programName);
  printf ("\t\"A\" button:	Rewinds stream\n");
  printf ("\t\"Stop\" button:	Closes stream\n");
}

/*********************************************************************************************
 * Routine to initialize streaming data buffers.
 *********************************************************************************************/

static DSDataBufPtr
CreateBufferList (long numBuffers, long bufferSize)
{
  DSDataBufPtr bp;
  DSDataBufPtr firstBp;
  long totalBufferSpace;
  long actualEntrySize;
  long bufferNum;

  /* Calculate the size each buffer needs to be to include quadbyte
   * alignment and header
   */
  actualEntrySize = sizeof (DSDataBuf) + ROUND_TO_LONG (bufferSize);

  /* Calculate total buffer space needed */
  totalBufferSpace = numBuffers * actualEntrySize;

  /* Try to allocate the needed memory */
  firstBp = (DSDataBufPtr)NewPtr (totalBufferSpace, MEMTYPE_ANY);
  if (firstBp == NULL)
    return NULL;

  /* Loop to take the big buffer and split it into "N" buffers
   * of size "bufferSize", linked together.
   */
  for (bp = firstBp, bufferNum = 0; bufferNum < (numBuffers - 1); bufferNum++)
    {
      bp->next = (DSDataBufPtr)(((char *)bp) + actualEntrySize);
      bp->permanentNext = bp->next;

      /* Advance to point to the next entry */
      bp = bp->next;
    }

  /* Make last entry's forward link point to nil */
  bp->next = NULL;
  bp->permanentNext = NULL;

  /* Return a pointer to the first buffer in the list */
  return firstBp;
}

/*********************************************************************************************
 * Initalize the DataStreamer and play the Prototype subscriber's test data.
 *********************************************************************************************/

int
main (int argc, char **argv)
{
  long status;         // Completion status of various things
  JoyPadState jpState; // JoyPad context

  char *streamFileName; // Name of the test stream file to play

  DSDataBufPtr dataBufferList; // List of buffers to use for streaming

  DSStreamCBPtr streamCBPtr; // Stream Context Block

  AcqContextPtr dataAcqCtxPtr; // DataAcq Context Block

  CtrlContextPtr controlCtxPtr; // Control Subscriber Context Block
  ProtoContextPtr protoCtxPtr;  // Prototype Subscriber Context Block

  Item messagePort; // Port for communicating with the streamer
  Item messageItem;

  /********************************************************
   * Sanity check, need at least the name of a stream file
   ********************************************************/
  if (argc < 2)
    {
      printf ("Error %s - need at least a stream file name.\n", argv[0]);
      Usage (argv[0]);
      exit (0);
    }

  /********************************************************
   * We need to create a message port and one message item
   * to communicate with the data streamer.
   ********************************************************/
  messagePort = NewMsgPort (NULL);
  CHECKRESULT ("NewMsgPort", messagePort);

  messageItem = CreateMsgItem (messagePort);
  CHECKRESULT ("CreateMsgItem", messageItem);

  /********************************************************
   * Get stream file name from command line paramter list
   ********************************************************/
  streamFileName = argv[1];

  /********************************************************
   * Create a linked list of streaming buffers.
   ********************************************************/
  dataBufferList = CreateBufferList (DATA_BUFFER_COUNT, DATA_BUFFER_SIZE);

  /********************************************************
   * Initialize streaming and create a new streamer thread
   ********************************************************/

  /* First initalize the pool of streaming contexts */
  status = InitDataStreaming (NUMBER_OF_STREAMS);
  CHECK_DS_RESULT ("InitDataStreaming", status);

  /* Allocate a context block and start the streamer thread running */
  status = NewDataStream (&streamCBPtr, dataBufferList, DATA_BUFFER_SIZE,
                          STREAMER_DELTA_PRI, 200);
  CHECK_DS_RESULT ("NewDataStream", status);

  /********************************************************
   * Create a data acquisition thread for supplying the
   * streamer with data.
   *******************************************************/

  /* First initalize the DataAcq's pool of contexts */
  status = InitDataAcq (NUMBER_OF_STREAMS);
  CHECK_DS_RESULT ("InitDataAcq", status);

  /* Allocate a context block and start the DataAcq thread running */
  status = NewDataAcq (&dataAcqCtxPtr, streamFileName, DATA_ACQ_DELTA_PRI);
  CHECK_DS_RESULT ("NewDataAcq", status);

  /********************************************************
   * Create a ProtoSubscriber thread for testing.
   *******************************************************/

  /* First initalize the subcriber's pool of contexts */
  status = InitProtoSubscriber ();
  CHECK_DS_RESULT ("InitProtoSubscriber", status);

  /* Allocate a context block and start the subscriber thread running */
  status
      = NewProtoSubscriber (&protoCtxPtr, streamCBPtr, SUBSCRIBER_DELTA_PRI);
  CHECK_DS_RESULT ("NewProtoSubscriber", status);

  /* Tell the streamer thread that we exist and what data type
   * to send us.
   */
  status = DSSubscribe (messageItem,                 // control msg item
                        NULL,                        // synchronous call
                        streamCBPtr,                 // stream context block
                        (DSDataType)PRTO_CHUNK_TYPE, // subscriber data type
                        protoCtxPtr->requestPort);   // subscriber message port
  CHECK_DS_RESULT ("DSSubscribe for ProtoSubscriber", status);

  /********************************************************
   * Create a Control Subscriber thread in case the user
   * has woven CTRL chunks into the stream for looping or
   * other wierd stuff.
   *******************************************************/

  status = InitCtrlSubscriber ();
  CHECK_DS_RESULT ("InitCtrlSubscriber", status);

  status = NewCtrlSubscriber (&controlCtxPtr, streamCBPtr,
                              CTRL_SUBSCRIBER_DELTA_PRI);
  CHECK_DS_RESULT ("NewCtrlSubscriber", status);

  status = DSSubscribe (messageItem,                 // control msg item
                        NULL,                        // synchronous call
                        streamCBPtr,                 // stream context block
                        (DSDataType)CTRL_CHUNK_TYPE, // subscriber data type
                        controlCtxPtr->requestPort); // subscriber message port
  CHECK_DS_RESULT ("DSSubscribe for control subscriber", status);

  /********************************************************
   * Attach the initial data acquisition to the stream.
   *******************************************************/
  status
      = DSConnect (messageItem, NULL, streamCBPtr, dataAcqCtxPtr->requestPort);
  CHECK_DS_RESULT ("DSConnect()", status);

  /********************************************************
   * Start the stream running.
   *******************************************************/

  /* Ask the streamer to start data moving. */
  status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
  CHECK_DS_RESULT ("DSStartStream", status);

  /* Since we have no audio subscriber to drive the clock,
   * we initalize it to zero manually.
   */
  status = DSSetClock (streamCBPtr, 0);
  CHECKRESULT ("DSSetClock()", status);

  /********************************************
   * Run the stream and check for user action
   ********************************************/
  while (true)
    {
      /* Read the control pad. If the start button is pressed, then
       * cause the stream to stop playing and exit everything.
       */
      GetJoyPad (&jpState, 1);
      if (jpState.xBtn)
        break;

      /* The "A" button causes a rewind */
      if (jpState.aBtn)
        {
          /* Stop the current stream with the flush option so all subscribers
           * will flush any data they currently have.  Otherwise they might
           * hang onto it forever... keeping stream buffers in use forever.
           */
          status = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
          CHECK_DS_RESULT ("DSStopStream", status);

          /* Jump back to the beginning of the stream */
          status = DSGoMarker (messageItem, NULL, streamCBPtr,
                               (unsigned long)0, GOMARKER_ABSOLUTE);
          CHECK_DS_RESULT ("DSGoMarker", status);

          /* Re-start the stream */
          status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
          CHECK_DS_RESULT ("DSStartStream", status);

          /* Re-set the stream clock */
          status = DSSetClock (streamCBPtr, 0);
          CHECKRESULT ("DSSetClock()", status);
        }
    }

  /************************************************************
   * We're exiting now so stop the stream and shut down.
   ************************************************************/

  /* Close the stream */
  printf ("Stream stopping\n");

  status = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
  CHECK_DS_RESULT ("DSStopStream", status);

  /************************************************************
   * Deallocate all one-time initialization things to insure
   * there are no memory leaks.
   ************************************************************/
  printf ("Disposing the stream with all subscribers.\n");

  /* Notify all subscribers that the stream is shutting down
   * and make the streamer thread go away
   */
  status = DisposeDataStream (messageItem, streamCBPtr);
  CHECK_DS_RESULT ("DisposeDataStream", status);

  /* Shut down this subscriber instance */
  status = DisposeCtrlSubscriber (controlCtxPtr);
  CHECK_DS_RESULT ("DisposeCtrlSubscriber", status);

  /* Free all of the subscriber contexts allocated by InitCtrlSubscriber() */
  status = CloseCtrlSubscriber ();
  CHECK_DS_RESULT ("CloseCtrlSubscriber", status);

  status = DisposeProtoSubscriber (protoCtxPtr);
  CHECK_DS_RESULT ("DisposeProtoSubscriber", status);

  status = CloseProtoSubscriber ();
  CHECK_DS_RESULT ("CloseProtoSubscriber", status);

  /* еее This function returns void for some wierd reason. */
  DisposeDataAcq (dataAcqCtxPtr);

  status = CloseDataAcq ();
  CHECK_DS_RESULT ("CloseDataAcq", status);

  status = CloseDataStreaming ();
  CHECK_DS_RESULT ("CloseDataStreaming", status);

  /* Get rid of our message stuff */
  RemoveMsgItem (messageItem);
  RemoveMsgPort (messagePort);

  /* Free the buffer space */
  FreePtr (dataBufferList);

  exit (0);
}
