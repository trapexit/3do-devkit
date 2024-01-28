/*******************************************************************************************
 *	File:			TestDS.c
 *
 *	Contains:		simple 3DO stream player application
 *
 *	Usage:			TestDS <-c | -a | -s> <streamFile> ...
 *						-c	Cinepak stream with or without
 *sound -a	ANIM stream with or without sound -s	sound only stream
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
 *	11/23/93	jb		Call all "close" routines to get rid of
 *one-time initialization allocations. Fixes memory leaks. 10/21/93	rdg
 *Make sure that the "C" button always calls stopstream with the flush option
 *so that any queued data will be thrown out before we rewind and start again.
 *	10/19/93	jb		Switch to using OpenAudioFolio() instead of
 *OpenAudio() to get rid of the "pop" when the application starts up. 10/13/93
 *jb		Remove call to SendFreeCPakSignal() 10/4/93		jb
 *Add end of stream test code. NOTE: THIS ENTIRE FILE NEEDS TO BE TRASHED AND
 *REWRITTEN. PLEASE, PLEASE, PLEASE DO NOT CUT AND PASTE FROM THIS CODE IF YOU
 *WANT A CLEAN EXAMPLE!!! 8/25/93		jb		Fix test of
 *uninitialized 'status' after call to InitDataStreaming() by actually
 *assigning the result of the call to the 'status' variable.
 *	8/11/93		jb		Version 2.2
 *						Add CLOSE_AUDIO_ON_SWITCH code
 *	8/5/93		jb		Version 2.1
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

#define PROGRAM_VERSION_STRING "2.3"

#include "Init3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "DataAcq.h"
#include "DataStreamDebug.h"
#include "DataStreamLib.h"

#include "CPakSubscriber.h"
#include "ControlSubscriber.h"
#include "SAnimSubscriber.h"
#include "SAudioSubscriber.h"

#include "Debug3do.h"
#include "UMemory.h"

/**************************************************************/
/* Buffer sizes, number of buffers, and other setup constants */
/**************************************************************/
#define NUMBER_OF_STREAMS 1
#define DATA_BUFFER_COUNT 4
#define DATA_BUFFER_SIZE (32 * 1024)
#define kMaxAcqCount 20

/*******************************/
/* Thread priority definitions */
/*******************************/
#define STREAMER_DELTA_PRI 6
#define SUBSCRIBER_DELTA_PRI 7
#define DATA_ACQ_DELTA_PRI 8
#define AUDIO_SUBSCRIBER_DELTA_PRI 10
#define CTRL_SUBSCRIBER_DELTA_PRI (AUDIO_SUBSCRIBER_DELTA_PRI + 1)

/* If true, this will cause a closechannel message to be sent to
   the audio subscriber when the stream is switched, or rewound
   with the "C" button.
 */
#define CLOSE_AUDIO_ON_SWITCH 0

/* ¥¥¥TEMPORARY¥¥¥ Compile switch to turn on/off whether we explicitly
 * free Cinepak buffers. Should be set to zero for testing new automatic
 * memory freeing scheme.
 */
#define SEND_FREE_CPAK_SIGNAL_ENABLED 0

/*************************************/
/* Handy macro for diagnostic output */
/*************************************/
#ifndef CHECKRESULT
#define CHECKRESULT(name, result)                                             \
  if (((long)result) < 0)                                                     \
    {                                                                         \
      kprintf ("Failure in %s: %d ($%x)\n", name, ((long)result),             \
               ((long)result));                                               \
      PrintfSysErr (result);                                                  \
      exit (0);                                                               \
    }
#endif

// stream type defines
enum
{
  kNotDefined = -1,
  kCinePakStream,
  kAnimStream,
  kSoundStream
};

// stream timer defines
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

CPakRecPtr gCnaRecChanPtr;
CPakContextPtr gCnaPakCntxPtr; // cinepak context pointer

SAnimRecPtr gSaRecChanPtr;
SAnimContextPtr gSaAnimCntxPtr; // ANIM context ptr

SAudioContextPtr gSAudioCntxPtr; // audio context

CtrlContextPtr gCntrlCntxPtr; // control subscriber context ptr

Item VBLIOReq; // IO Parameter for timer in WaitVBL

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

#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define NTSC_FREQUENCY 60
#define NTSC_TYPE DI_TYPE_NTSC
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define PAL_FREQUENCY 50
#define PAL_TYPE DI_TYPE_PAL2

/*
        Purpose:	Create a screen context record with the number of
                                associated screens specified that matches the
   current video mode of the target machine (i.e., NTSC or PAL). Opens the
   Graphics Folio before doing anything...
        NOTE:		A 320x240 by 16-bit deep screen is 153,600 bytes.
                                A 388x288 by 16-bit deep screen is 211,184
   bytes. Thus the standard 2-screen usage will EITHER allocate buffers of 300K
   (NTSC) or 432K (PAL) (the overhead for either is the same).  Be sure you
   have enough memory! Inputs:	sc - A pointer to a screen context record (must
   be allocated). nScreens - number of screens to allocate (1-6) Outputs:
   OpenGraphicsNTSCPAL - returns TRUE if the screen context record was
   allocated.  FALSE if nScreens is out-of-range or the screen group could not
   be added.
*/
bool
OpenGraphicsNTSCPAL (ScreenContext *sc, uint32 nScreens)
{

  int32 width, height, diType, i;
  Item item;
  uint32 bmw[MAXSCREENS];
  uint32 bmh[MAXSCREENS];
  bool rslt = FALSE;
  TagArg taScreenTags[] = {
    /* Don't change the order of these without changing assumtion below */
    CSG_TAG_DISPLAYHEIGHT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SCREENCOUNT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SCREENHEIGHT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_BITMAPCOUNT,
    (void *)1,
    CSG_TAG_BITMAPWIDTH_ARRAY,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_BITMAPHEIGHT_ARRAY,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SPORTBITS,
    (void *)(MEMTYPE_VRAM | MEMTYPE_STARTPAGE),
    CSG_TAG_DISPLAYTYPE,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_DONE,
    (void *)0,
  };

  // Check for bad arguments
  if ((nScreens > MAXSCREENS) || (nScreens < 1))
    goto EXIT_MyOpenGraphics;

  // Open the graphics folio
  i = OpenGraphicsFolio ();
  if (i < 0)
    {
      DIAGNOSE (("Cannot open graphics folio (Error = %ld)\n", i));
      goto EXIT_MyOpenGraphics;
    }

  // Determine NTSC/PAL operating environment
  width = NTSC_WIDTH; // Assume NTSC
  height = NTSC_HEIGHT;
  diType = NTSC_TYPE;
  if (GrafBase->gf_VBLFreq == PAL_FREQUENCY)
    {
      width = PAL_WIDTH;
      height = PAL_HEIGHT;
      diType = PAL_TYPE;
    }
  else if (GrafBase->gf_VBLFreq != NTSC_FREQUENCY)
    {
      DIAGNOSE (("Unknown screen refresh rate %ld\n", GrafBase->gf_VBLFreq));
      goto EXIT_MyOpenGraphics;
    }

  // Set up the width and height arrays now that we know NTSC/PAL
  for (i = 0; i < nScreens; ++i)
    {
      bmw[i] = width;
      bmh[i] = height;
    }

  // Now create the screen group, add it to the display mechanism
  sc->sc_nScreens = (int32)nScreens;
  sc->sc_curScreen = (int32)0;
  i = GrafBase->gf_VRAMPageSize;
  sc->sc_nFrameBufferPages = (width * height * 2 + (i - 1)) / i;
  sc->sc_nFrameByteCount = sc->sc_nFrameBufferPages * i;

  taScreenTags[0].ta_Arg = (void *)height;          // CSG_TAG_DISPLAYHEIGHT
  taScreenTags[1].ta_Arg = (void *)sc->sc_nScreens; // CSG_TAG_SCREENCOUNT
  taScreenTags[2].ta_Arg = (void *)height;          // CSG_TAG_SCREENHEIGHT
  taScreenTags[4].ta_Arg = (void *)bmw;    // CSG_TAG_BITMAPWIDTH_ARRAY
  taScreenTags[5].ta_Arg = (void *)bmh;    // CSG_TAG_BITMAPHEIGHT_ARRAY
  taScreenTags[7].ta_Arg = (void *)diType; // CSG_TAG_DISPLAYTYPE

  item = CreateScreenGroup (&(sc->sc_Screens[0]), taScreenTags);
  if (item < 0)
    {
#if DEBUG
      PrintfSysErr (item);
#endif
      DIAGNOSE (("Cannot create screen group\n"));
      return FALSE;
    }
  i = AddScreenGroup (item, NULL);
  if (item < 0)
    {
#if DEBUG
      PrintfSysErr ((Item)i);
#endif
      DIAGNOSE (("Cannot add screen group\n"));
      return FALSE;
    }

  // Get the information about the screen group and update the ScreenContext
  // structure
  for (i = 0; i < sc->sc_nScreens; i++)
    {
      Screen *screen;
      screen = (Screen *)LookupItem (sc->sc_Screens[i]);

      if (screen == NULL)
        {
          DIAGNOSE (("Cannot locate screens\n"));
          return FALSE;
        }
      sc->sc_BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
      sc->sc_Bitmaps[i] = screen->scr_TempBitmap;
      EnableHAVG (sc->sc_Screens[i]);
      EnableVAVG (sc->sc_Screens[i]);
    }

  rslt = TRUE;

EXIT_MyOpenGraphics:
  return (rslt);
}

/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
static Boolean
StartUp (void)
{
  gScreenContext.sc_nScreens = 2;

  VBLIOReq = GetVBLIOReq ();

  if (!OpenGraphicsNTSCPAL (&gScreenContext, 2))
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

DSDataBufPtr
CreateBufferList (long numBuffers, long bufferSize)
{
#define ROUND_TO_LONG(x) ((x + 3) & ~3)
  DSDataBufPtr bp;
  DSDataBufPtr firstBp;
  long totalBufferSpace;
  long actualEntrySize;
  long bufferNum;

  actualEntrySize = sizeof (DSDataBuf) + ROUND_TO_LONG (bufferSize);
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

  /* Return a pointer to the first buffer in the list
   */
  return firstBp;
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
  status = InitDataStreaming (NUMBER_OF_STREAMS);
  CHECK_DS_RESULT ("InitDataStreaming", status);

  /* Initialize a new stream
   */
  status = NewDataStream (streamCBPtrPtr, bufferList, bufferSize,
                          STREAMER_DELTA_PRI, 200);
  CHECK_DS_RESULT ("NewDataStream", status);

  // init the anim streamer...
  status = InitSAnimSubscriber ();
  CHECK_DS_RESULT ("InitSAnimSubscriber", status);

  status = NewSAnimSubscriber (&gSaAnimCntxPtr, NUMBER_OF_STREAMS,
                               SUBSCRIBER_DELTA_PRI);
  CHECK_DS_RESULT ("NewSAnimSubscriber", status);

  // Attach the anim subscriber to the stream
  status
      = DSSubscribe (messageItem,                  // control msg item
                     NULL,                         // synchronous call
                     *streamCBPtrPtr,              // stream context block
                     (DSDataType)SANM_CHUNK_TYPE,  // subscriber data type
                     gSaAnimCntxPtr->requestPort); // subscriber message port
  CHECK_DS_RESULT ("DSSubscribe for sanm", status);

  status = InitSAnimCel (*streamCBPtrPtr, // Need this for DSGetClock in sub.
                         gSaAnimCntxPtr,  // The subscriber's context
                         &gSaRecChanPtr,  // The channel's context
                         0,               // The channel number
                         true);           // true = flush on synch msg from DSL
  CHECK_DS_RESULT ("InitSAnimCel", status);

  // init the cinepak subscriber
  status = InitCPakSubscriber ();
  CHECK_DS_RESULT ("InitCPakSubscriber", status);

  status = NewCPakSubscriber (&gCnaPakCntxPtr, NUMBER_OF_STREAMS,
                              SUBSCRIBER_DELTA_PRI);
  CHECK_DS_RESULT ("NewCPakSubscriber", status);

  status
      = DSSubscribe (messageItem,                  // control msg item
                     NULL,                         // synchronous call
                     *streamCBPtrPtr,              // stream context block
                     (DSDataType)FILM_CHUNK_TYPE,  // subscriber data type
                     gCnaPakCntxPtr->requestPort); // subscriber message port
  CHECK_DS_RESULT ("DSSubscribe for cinepak", status);

  status = InitCPakCel (*streamCBPtrPtr, // Need this for DSGetClock in sub.
                        gCnaPakCntxPtr,  // The subscriber's context
                        &gCnaRecChanPtr, // The channel's context
                        0,               // The channel number
                        true);           // true = flush on synch msg from DSL
  CHECK_DS_RESULT ("InitCPakCel", status);

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

void
EraseScreen (ScreenContext *sc, int32 screenNum)
{
  Item VRAMIOReq;

  VRAMIOReq = GetVRAMIOReq ();
  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[screenNum]->bm_Buffer, 0,
                sc->sc_nFrameBufferPages, -1);
  DeleteItem (VRAMIOReq);
}

/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void
Usage (char *programName)
{
  kprintf ("%s version %s\n", programName, PROGRAM_VERSION_STRING);
  kprintf ("usage: %s <-c | -a | -s> <streamFile> \n", programName);
  kprintf ("\t\t-c 	cinepak stream \n");
  kprintf ("\t\t-a 	anim stream\n");
  kprintf ("\t\t-s 	sound only stream\n");
  kprintf ("\t\"A\" button:	Toggles stream stop/start\n");
  kprintf ("\t\"B\" button:	Toggles stream timing\n");
  kprintf ("\t\"C\" button:	Switches to playing the next stream\n");
  kprintf ("\t				in the command line list. Rewinds\n");
  kprintf ("\t				the stream in between switching "
           "streams.\n");
}

/*********************************************************************************************
 * Figure out which type of file we're supposed to show
 *********************************************************************************************/
int32
StreamFileType (char streamIdent)
{
  if ((streamIdent == 'c') || (streamIdent == 'C'))
    return kCinePakStream;
  else if ((streamIdent == 'a') || (streamIdent == 'A'))
    return kAnimStream;
  else if ((streamIdent == 's') || (streamIdent == 'S'))
    return kSoundStream;
  else
    return kNotDefined;
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
  TagArg ioTags[] = {
    CREATEIOREQ_TAG_DEVICE,
    0,
    TAG_END,
    0,
  };

  // Timer initialization
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

  // has the timer been initialized???
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

  // we want seconds...
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

  // don't reset the strings unless we have a new time/clock, just draw the
  // last
  //  string we made
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

  localGrafCon.gc_PenX = x;
  localGrafCon.gc_PenY = y;
  DrawText8 (&localGrafCon,
             gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen],
             gTimeStr);
  localGrafCon.gc_PenX = x;
  localGrafCon.gc_PenY = y + 12;
  DrawText8 (&localGrafCon,
             gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen],
             gCallsPerSecStr);
  localGrafCon.gc_PenX = x;
  localGrafCon.gc_PenY = y + 24;
  DrawText8 (&localGrafCon,
             gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen],
             gClockStr);
}

/*
 * Calculate and show timing for a cinepak stream
 */
void
ShowStreamTiming (int32 streamType, DSStreamCBPtr streamCBPtr)
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

  // frame counting for audio doesn't make sense, so...
  if (streamType == kSoundStream)
    return;

  // only count the frame if it is different than last time
  if (streamType == kCinePakStream)
    tempFrame = (void *)gCnaRecChanPtr->curFramePtr;
  else
    tempFrame = (void *)gSaRecChanPtr->curFramePtr;

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
  Item EndOfStreamMessageItem;
  DSRequestMsgPtr msgPtr;
  DSRequestMsg EOSMessage;
  Boolean endOfStreamFlag;
  DSStreamCBPtr streamCBPtr;
  DSDataBufPtr dataBufferList;
  Boolean fRunStream;
  int32 parmNdx;
  int32 acqNdx;
  int32 fileCount;
  int32 currStreamType;
  int32 currStreamNum;
  Boolean fDisplayStreamClock;
  long templateTags[4];
  SAudioCtlBlock ctlBlock;
  long volume;
  long pan;
  long curChannelNumber;

  AcqContextPtr dataContextPtr[kMaxAcqCount];
  int32 streamFileType[kMaxAcqCount];
  char *streamFileName[kMaxAcqCount];

  fRunStream = true;
  fDisplayStreamClock = false;
  fileCount = 0;
  currStreamType = 0;
  currStreamNum = 0;

  /* Initialize the library code */
  if (!StartUp ())
    {
      kprintf ("StartUp() initialization failed!\n");
      exit (0);
    }

  // sanity check, need at least the name of a file
  if (argc < 3)
    {
      kprintf ("### %s - invalid parameter list, need at least a stream file "
               "name.\n",
               argv[0], argv[parmNdx]);
      Usage (argv[0]);
      exit (0);
    }

  // check to make sure we have a file type flag for each file name (remember
  // that the
  //  first param is the program name)
  if (argc % 2 == 0)
    {
      kprintf ("### %s - invalid parameter list, need a stream type specified "
               "for each stream file name.\n",
               argv[0], argv[parmNdx]);
      Usage (argv[0]);
      exit (0);
    }
  // now check to make sure that all of the file types passed
  for (parmNdx = 1; parmNdx < argc; parmNdx += 2)
    {

      // not an option? must be a file name.  move it down so all file
      //  names are sequential
      if (*argv[parmNdx] != '-')
        {
          kprintf ("### %s - \"%s\" is not an option.\n", argv[0],
                   argv[parmNdx]);
          Usage (argv[0]);
          exit (0);
        }

      // figure out which stream type the is and grab a pointer to the file
      // name
      streamFileType[fileCount] = StreamFileType (*(argv[parmNdx] + 1));
      if (streamFileType[fileCount] == kNotDefined)
        {
          kprintf ("### %s - \"%c\" is not a valid file type.\n", argv[0],
                   *(argv[parmNdx] + 1));
          Usage (argv[0]);
          exit (0);
        }
      streamFileName[fileCount] = argv[parmNdx + 1];
      fileCount++;
    }

  /* We need to create a message port and one message item
   * to communicate with the data streamer.
   */
  messagePort = NewMsgPort (NULL);
  CHECKRESULT ("NewMsgPort", messagePort);

  messageItem = CreateMsgItem (messagePort);
  CHECKRESULT ("CreateMsgItem", messageItem);

  EndOfStreamMessageItem = CreateMsgItem (messagePort);
  CHECKRESULT ("CreateMsgItem", EndOfStreamMessageItem);

  /* Initialize the data buffer list */
  dataBufferList = CreateBufferList (DATA_BUFFER_COUNT, DATA_BUFFER_SIZE);

  /* Initialize data acquisition
   */
  status = InitDataAcq (fileCount);
  CHECK_DS_RESULT ("InitDataAcq", status);

  for (acqNdx = 0; acqNdx < fileCount; acqNdx++)
    {
      AcqContextPtr *tempAcqPtrPtr;

      tempAcqPtrPtr = &dataContextPtr[acqNdx];
      status = NewDataAcq (tempAcqPtrPtr, streamFileName[acqNdx],
                           DATA_ACQ_DELTA_PRI);
      CHECK_DS_RESULT ("NewDataAcq 1", status);
    }

  status = InitStreaming (&streamCBPtr, dataBufferList, DATA_BUFFER_SIZE,
                          messageItem);
  CHECKRESULT ("InitStreaming", status);

  /* Define which data types this audio subscriber must be able to play
   * without going to disk for the instrument (preload).
   */

  templateTags[0] = SA_44K_16B_S;
  templateTags[1] = SA_44K_16B_M;
  templateTags[2] = SA_22K_16B_M_SDX2;
  templateTags[3] = 0;

  ctlBlock.loadTemplates.tagListPtr = templateTags;

  status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                      kSAudioCtlOpLoadTemplates, &ctlBlock);
  CHECK_DS_RESULT ("DSControl", status);

  /* Enable audio channel #1 which is used by the Cinepak
   * movie, if present.
   */
  status = DSSetChannel (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE, 1,
                         CHAN_ENABLED);
  CHECK_DS_RESULT ("DSSetChannel", status);

  /* Attach the initial data acquisition to the stream
   */
  status = DSConnect (messageItem, NULL, streamCBPtr,
                      dataContextPtr[0]->requestPort);
  CHECK_DS_RESULT ("DSConnect()", status);

  status = DSStartStream (messageItem, NULL, streamCBPtr, 0);
  CHECK_DS_RESULT ("DSStartStream", status);

  /* Register for end of stream notification */
  status
      = DSWaitEndOfStream (EndOfStreamMessageItem, &EOSMessage, streamCBPtr);
  CHECK_DS_RESULT ("DSWaitEndOfStream", status);

  currStreamType = streamFileType[0];

  /* The SANM Stream has no SYNC CTRL message at its front so we
   * have to jump-start this stream. If This stream did have a SYNC
   * message, the GOTO CTRL message at the end of the stream would
   * cause the Control Subscriber to process the SYNC immediatly upon
   * receipt of the SYNC chunk. This would almost certainly be BEFORE
   * the SAnim subscriber has completed processing the last frame Cel.
   * Thus, the last frames in the looping SAnim would never be seen.
   */
  if (currStreamType == kAnimStream)
    {
      status = DSSetClock (streamCBPtr, 0);
      CHECKRESULT ("DSSetClock()", status);
    }

  /* Our Cinepak test data has audio in channel #1 only, so we must
   * tell the audio subscriber to use channel #1 as the clock channel
   * if a Cinepak movie is the first thing we are going to play.
   */
  if (currStreamType == kCinePakStream)
    curChannelNumber = 1;
  else
    curChannelNumber = 0;

  /* Set the audio clock to use the selected channel */
  ctlBlock.clock.channelNumber = curChannelNumber;
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

      /* Poll for EOS. Yuk!!! You would normally want to do this in some other
       * place than the main display loop, ideally, a separate thread.
       */
      endOfStreamFlag
          = PollForMsg (messagePort, NULL, NULL, (void **)&msgPtr, &status);
      if (endOfStreamFlag)
        {
          /* Register for end of stream notification */
          status = DSWaitEndOfStream (EndOfStreamMessageItem, &EOSMessage,
                                      streamCBPtr);
          CHECK_DS_RESULT ("DSWaitEndOfStream", status);

          printf ("End of stream reached\n");
          continue;
        }

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
              kprintf ("Stream started.\n");

              if (currStreamType == kAnimStream)
                {
                  status = DSSetClock (streamCBPtr, 0);
                  CHECKRESULT ("DSSetClock()", status);
                }
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
              kprintf ("Stream stopped.\n");
            }

          kprintf (" stream flags = %lx\n", streamCBPtr->streamFlags);
        }

      /* The "B" button toggles displaying the stream clock
       */
      if (joybits & JOYFIREB)
        {
          fDisplayStreamClock = !fDisplayStreamClock;
          if (fDisplayStreamClock)
            kprintf ("Stream timing on.\n");
          else
            kprintf ("Stream timing off.\n");
        }

      if (joybits & JOYFIREC)
        {
          // stop the current stream and flush the subscriber to make sure
          //  it doesn't hang onto any buffers.
          status = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
          CHECK_DS_RESULT ("DSStopStream", status);

#if CLOSE_AUDIO_ON_SWITCH
          ctlBlock.closeChannel.channelNumber = curChannelNumber;
          status = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                              kSAudioCtlOpCloseChannel, &ctlBlock);
          CHECK_DS_RESULT ("DSControl - closing channel", status);

          /* Reenable this channel for the next time audio data arrives */
          status = DSSetChannel (messageItem, NULL, streamCBPtr,
                                 SNDS_CHUNK_TYPE, 1, CHAN_ENABLED);
          CHECK_DS_RESULT ("DSSetChannel", status);
#endif

          switch (currStreamType)
            {
            case kAnimStream:
              // flush anything held by the anim subscriber
              FlushSAnimChannel (gSaAnimCntxPtr, gSaRecChanPtr, 0);
              break;

            case kCinePakStream:
              FlushCPakChannel (gCnaPakCntxPtr, gCnaRecChanPtr, 0);

              /* Switch the audio clock back to using channel zero */
              curChannelNumber = 0;
              ctlBlock.clock.channelNumber = curChannelNumber;
              status
                  = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                               kSAudioCtlOpSetClockChan, &ctlBlock);
              CHECK_DS_RESULT ("DSControl - setting audio clock chan = 0",
                               status);
              break;
            }

          // bump to the next stream, let the user know what we're doing
          ++currStreamNum;
          currStreamNum = (currStreamNum >= fileCount) ? 0 : currStreamNum;
          kprintf ("Switching to stream %s\n", streamFileName[currStreamNum]);

          // and figure out which type of anim it is
          currStreamType = streamFileType[currStreamNum];

          /* If we're switching INTO a Cinepak stream, set the audio clock
           * channel to channel #1.
           */
          if (currStreamType == kCinePakStream)
            {
              /* Switch the audio clock to use channel #1 */
              curChannelNumber = 1;
              ctlBlock.clock.channelNumber = curChannelNumber;
              status
                  = DSControl (messageItem, NULL, streamCBPtr, SNDS_CHUNK_TYPE,
                               kSAudioCtlOpSetClockChan, &ctlBlock);
              CHECK_DS_RESULT ("DSControl - setting audio clock chan = 1",
                               status);
            }

          status = DSConnect (messageItem, NULL, streamCBPtr,
                              dataContextPtr[currStreamNum]->requestPort);
          CHECK_DS_RESULT ("DSConnect", status);

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

              if (currStreamType == kAnimStream)
                {
                  status = DSSetClock (streamCBPtr, 0);
                  CHECKRESULT ("DSSetClock()", status);
                }
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

      // call the appropriate subscriber.  the audio subscriber, of course,
      // doesn't
      //  need any time for it to continue
      switch (currStreamType)
        {
        case kAnimStream:
          {
            CCB *tempCCB = NULL;

            // get the next frame by passing 0 as 'nextFrameNumber'
            tempCCB = GetSAnimCel (gSaAnimCntxPtr, gSaRecChanPtr);

            // draw the next ccb, assuming we got one...
            if (tempCCB != NULL)
              {
                gScreenContext.sc_curScreen = 1 - gScreenContext.sc_curScreen;
                CenterCelOnScreen (tempCCB);
                LAST_CEL (tempCCB);
                WaitVBL (VBLIOReq, 1);
                EraseScreen (&gScreenContext, gScreenContext.sc_curScreen);
                status = DrawScreenCels (
                    gScreenContext.sc_Screens[gScreenContext.sc_curScreen],
                    tempCCB);
                if (status != 0)
                  kprintf ("DrawCels() error...\n");

                /* Tell the SAnimSubscriber to reply all freed chunks back to
                 * the DSL */
                SendFreeSAnimSignal (gSaAnimCntxPtr);
              }
            break;
          }

        case kCinePakStream:
          // if the stream is currently rolling, draw the next
          WaitVBL (VBLIOReq, 1);
          DrawCPakToBuffer (
              gCnaPakCntxPtr, gCnaRecChanPtr,
              gScreenContext.sc_Bitmaps[gScreenContext.sc_curScreen]);

#if SEND_FREE_CPAK_SIGNAL_ENABLED
          /* Tell the CPakSubscriber to reply all freed chunks back to the DSL
           */
          SendFreeCPakSignal (gCnaPakCntxPtr);
#endif
          break;
        }

      /* Display the stream clock if we're supposed to do so, show the new
       * image
       */
      if (fDisplayStreamClock)
        ShowStreamTiming (currStreamType, streamCBPtr);
      DisplayScreen (gScreenContext.sc_Screens[gScreenContext.sc_curScreen],
                     0);
    }

  /* Close the stream */
  kprintf ("Stream stopping\n");
  status = DSStopStream (messageItem, NULL, streamCBPtr, SOPT_FLUSH);
  CHECK_DS_RESULT ("DSStopStream", status);

  switch (currStreamType)
    {
    case kAnimStream:
      // flush anything held by the anim subscriber
      FlushSAnimChannel (gSaAnimCntxPtr, gSaRecChanPtr, 0);
      break;

    case kCinePakStream:
      FlushCPakChannel (gCnaPakCntxPtr, gCnaRecChanPtr, 0);
      break;
    }

  kprintf ("Disposing the stream\n");
  status = DisposeDataStream (messageItem, streamCBPtr);
  CHECK_DS_RESULT ("DisposeDataStream", status);

  /* Tell data acquisition threads to go away */
  for (acqNdx = 0; acqNdx < fileCount; acqNdx++)
    {
      AcqContextPtr tempAcqPtr;

      kprintf ("Disposing data acquisition #%ld\n", acqNdx);
      tempAcqPtr = dataContextPtr[acqNdx];
      DisposeDataAcq (tempAcqPtr);
    }

  /************************************************************/
  /* Deallocate all one-time initialization things to insure	*/
  /* there are no memory leaks.
   */
  /************************************************************/
  status = DisposeCtrlSubscriber (gCntrlCntxPtr);
  CHECK_DS_RESULT ("DisposeCtrlSubscriber", status);

  status = DisposeSAudioSubscriber (gSAudioCntxPtr);
  CHECK_DS_RESULT ("DisposeSAudioSubscriber", status);

  status = DestroyCPakCel (gCnaPakCntxPtr, gCnaRecChanPtr, 0);
  CHECK_DS_RESULT ("DestroyCPakCel", status);

  status = DisposeCPakSubscriber (gCnaPakCntxPtr);
  CHECK_DS_RESULT ("DisposeCPakSubscriber", status);

  status = DisposeSAnimSubscriber (gSaAnimCntxPtr);
  CHECK_DS_RESULT ("DisposeSAnimSubscriber", status);

  status = CloseSAnimSubscriber ();
  CHECK_DS_RESULT ("CloseSAnimSubscriber", status);

  status = CloseCPakSubscriber ();
  CHECK_DS_RESULT ("CloseCPakSubscriber", status);

  status = CloseSAudioSubscriber ();
  CHECK_DS_RESULT ("CloseCtrlSubscriber", status);

  status = CloseCtrlSubscriber ();
  CHECK_DS_RESULT ("CloseCtrlSubscriber", status);

  status = CloseDataAcq ();
  CHECK_DS_RESULT ("CloseDataAcq", status);

  status = CloseDataStreaming ();
  CHECK_DS_RESULT ("CloseDataStreaming", status);

  /* Get rid of our message stuff */
  RemoveMsgItem (messageItem);
  RemoveMsgPort (messagePort);

  FreePtr (dataBufferList);

  ScavengeMem ();

  exit (0);
}
