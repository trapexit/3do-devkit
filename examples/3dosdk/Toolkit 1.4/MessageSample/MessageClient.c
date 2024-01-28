/*
**
**			MessageClient.c
**				A sample that demonstrates the Kernel's messaging
*capabilities
**
**				GREEN Hardware, 4B2 release
**
**			7/22/93		NMD		Started
**			8/10/93		NMD		Added time delay to
*simulate real kids
*/

#include "Init3DO.h"
#include "Kernel.h"
#include "Mem.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "String.h"
#include "Task.h"
#include "Time.h"
#include "Timer.h"
#include "Utils3DO.h"
#include "event.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "io.h"

#include "MarcoMessages.h"

#ifdef DEBUG
#define CHECKRESULTSTATUS(res, s)                                             \
  {                                                                           \
    printf ("%s - ", s);                                                      \
    PrintfSysErr (res);                                                       \
  }

#define FAILNILSTATUS(s)                                                      \
  {                                                                           \
    printf ("%s - returned NULL", s);                                         \
  }
#endif

#ifndef DEBUG
#define CHECKRESULTSTATUS(res, s)
#define FAILNILSTATUS(s)
#endif

#define CHECKRESULT(res, s)                                                   \
  if (res < 0)                                                                \
    {                                                                         \
      CHECKRESULTSTATUS (res, s);                                             \
      goto FAILED;                                                            \
    }
#define FAILNIL(ptr, s)                                                       \
  if (ptr == 0)                                                               \
    {                                                                         \
      FAILNILSTATUS (s);                                                      \
      goto FAILED;                                                            \
    }

typedef struct
{
  uint32 type_mode;
  Item devItem;
  Item ioReqItem;
  struct timeval deltaStart;
} TimerHelper, *TimerHelperPtr;

#define TM_TYPE_MICROSEC 0x00000000
#define TM_TYPE_VBL 0x00010000

// NOTE:	When we're in VBL mode, the tv_usec field of the timevalue
// 		structure contains the vbl count.

#define TM_MODE_ABSOLUTE 0x00000000
#define TM_MODE_DELTA 0x00000001

#define TM_RESET true   // Reset the counter in DELTA mode
#define TM_NORMAL false // Leave the counter alone DELTA mode
TimerHelperPtr InitTimer (uint32 mode);
void FreeTimer (TimerHelperPtr theTimer);
bool GetTime (TimerHelperPtr theTimer, bool reset, struct timeval *tv);
bool WaitTime (TimerHelperPtr theTimer, struct timeval *tv);

typedef struct PlayerNode
{
  Node theNode;
  Item msgPortItem;
} PlayerNode, *PlayerNodePtr;

#define CURRENT_TASK_PRIORITY (KernelBase->kb_CurrentTask->t.n_Priority)
#define THREAD_PARENT                                                         \
  ((Item)KernelBase->kb_CurrentTask->t_ThreadTask->t.n_Item)

#define XMAX 320
#define YMAX 240
#define XMIN 0
#define YMIN 0

#define kNumJoyPads 1
#define kTaskFocus true

ScreenContext gTheScreen;
bool gAlive = true;

int32 gMarcoSignalFlag;

bool ProcessIncommingMessages (Item thePort);

int
main (int argc, char **argv)
{
  int32 err;

  int32 poloSignalFlag;
  Item poloPort;

  Item marcoPort;

  TagArg targs[4];
  Item myMsg;

  //	Item					incommingItem;
  //	Message					*incommingMsg;

  //	Haven't implimented the cool delay effects...
  //
  //	int32					delayTime;
  //	int32					delayCounter;

  //	Establish a Message port for this task
  //

  poloSignalFlag = AllocSignal (0);
  CHECKRESULT (poloSignalFlag, "(main)AllocSignal");

  poloPort = CreateMsgPort ("POLO", 0, poloSignalFlag);
  CHECKRESULT (poloPort, "(main) CreateMsgPort");

  //	Locate the message port for the Marco Server
  //
  marcoPort = FindNamedItem (MKNODEID (KERNELNODE, MSGPORTNODE), "MARCO");
  CHECKRESULT (marcoPort, "(main) FindNamedItem");

  // Create a persistant message to use for communication with the Marco
  //	Server.
  //
  targs[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
  targs[0].ta_Arg = (void *)poloPort;
  targs[1].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
  targs[1].ta_Arg = (void *)8;
  targs[2].ta_Tag = TAG_END;

  myMsg = CreateItem (MKNODEID (KERNELNODE, MESSAGENODE), targs);
  CHECKRESULT (myMsg, "CreateItem");

  // Announce our presence to the server
  //
  err = SendSmallMsg (marcoPort, myMsg, kMarco_RegisterPlayer, 0);
  CHECKRESULT (err, "SendSmallMsg");

  printf ("\n *** Polo process registered *** \n");

  //	While we're supposed to be doing our thing
  //
  while (gAlive)
    {

      ProcessIncommingMessages (poloPort);
    }

  printf ("\n\nClient Exit\n");

  return TRUE;

FAILED:

  return FALSE;
}

bool
ProcessIncommingMessages (Item thePort)
{
  int32 err;

  Item playerMsgItem;
  Message *playerMsg;

  uint32 reply;

  TimerHelperPtr timer;
  struct timeval delay;

  playerMsgItem = WaitPort (thePort, 0);
  CHECKRESULT (playerMsgItem, "GetMsg");
  FAILNIL (playerMsgItem, "GetMsg");

  playerMsg = (Message *)LookupItem (playerMsgItem);
  FAILNIL (playerMsg, "LookupItem");

  // First, Make sure that we got a Short Message (the only kind
  //	we know how to deal with)
  //
  if (playerMsg->msg.n_Flags & MESSAGE_SMALL)
    {

      // Setup the default reply
      //
      reply = kMarco_OK;

      switch ((int32)playerMsg->msg_DataPtr)
        {
        case kMarco_ShoutMarco:

          // To simulate real life, we'll wait a small ammount of time
          // before we respond to the message.  This will give that
          // wonderfull staggerd reply effect when more than one
          // client is running...
          //

          timer = InitTimer (TM_TYPE_MICROSEC);

          delay.tv_sec = 0;
          delay.tv_usec = (ReadHardwareRandomNumber () & 7) * 250000;
          WaitTime (timer, &delay);

          reply = kMarco_ShoutPolo;

          FreeTimer (timer);

          break;

        case kMarco_YouCantPlay:
          // Too many players registered - our process isn't welcome
          //
          printf ("\n\nWaaa, too many players...\n\n");
        case kMarco_GameOver:
          gAlive = false;
          break;

        default:
          goto FAILED;
        }

      //	Reply to the message we recieved.  Note that simply sending
      //another 	message is at best bad form, and at worst detrimental to system
      //	performance...
      //
      if (playerMsg->msg_Result != kMarco_Recieved)
        {
          err = ReplySmallMsg (playerMsgItem, kMarco_Recieved, reply, 0);
          CHECKRESULT (err, "replySmallMsg");
        }
    }

  return 1;
FAILED:
  return 0;
}

//
/////////// Timer Helper Code - Simplifies working with Time delays ///////
//

TimerHelperPtr
InitTimer (uint32 mode)
{
  TimerHelperPtr theTimer;

  // Allocate space for my TimerHelper record, open a Timer Device
  // and set up an ioRequest
  //
  theTimer = (TimerHelperPtr)AllocMem (sizeof (TimerHelper), MEMTYPE_DMA);
  FAILNIL (theTimer, "InitTimer: AllocMem");

  memset (theTimer, 0, sizeof (TimerHelper));

  theTimer->devItem = OpenNamedDevice ("timer", 0);
  CHECKRESULT (theTimer->devItem, "InitTimer: OpenNamedDevice");

  theTimer->ioReqItem = CreateIOReq (0, 0, theTimer->devItem, 0);
  CHECKRESULT (theTimer->ioReqItem, "InitTimer: CreateIOReq");

  theTimer->type_mode = mode;

  if (GetTime (theTimer, true, &(theTimer->deltaStart)))
    return theTimer;

FAILED:

  if (theTimer->ioReqItem)
    DeleteItem (theTimer->ioReqItem);

  if (theTimer->devItem)
    CloseItem (theTimer->devItem);

  if (theTimer)
    FreeMem (theTimer, sizeof (TimerHelper));

  return NULL;
}

void
FreeTimer (TimerHelperPtr theTimer)
{
  // Dispose of all allocted items, devices and memory
  //

  if (theTimer->ioReqItem)
    DeleteIOReq (theTimer->ioReqItem);
  if (theTimer->devItem)
    CloseItem (theTimer->devItem);

  if (theTimer)
    FreeMem (theTimer, sizeof (TimerHelper));
}

bool
GetTime (TimerHelperPtr theTimer, bool reset, struct timeval *tv)
{
  int32 retval;
  IOInfo ioInfo;

  // Read the current time, and return either an absolute or delta based
  // on the mode the TimerHelper is in.
  //
  memset (&ioInfo, 0, sizeof (IOInfo));

  ioInfo.ioi_Command = CMD_READ;
  ioInfo.ioi_Unit = (theTimer->type_mode & TM_TYPE_VBL) ? TIMER_UNIT_VBLANK
                                                        : TIMER_UNIT_USEC;

  ioInfo.ioi_Recv.iob_Buffer = tv;
  ioInfo.ioi_Recv.iob_Len = sizeof (struct timeval);

  retval = DoIO (theTimer->ioReqItem, &ioInfo);
  CHECKRESULT (retval, "GetTime: DoIO");

  if (reset)
    {
      theTimer->deltaStart.tv_sec = tv->tv_sec;
      theTimer->deltaStart.tv_usec = tv->tv_usec;
    }

  if (theTimer->type_mode & TM_MODE_DELTA)
    {
      tv->tv_sec = tv->tv_sec - theTimer->deltaStart.tv_sec;
      tv->tv_usec = tv->tv_usec - theTimer->deltaStart.tv_usec;
    }

  return 1;

FAILED:

  return 0;
}

bool
WaitTime (TimerHelperPtr theTimer, struct timeval *tv)
{
  int32 retval;
  IOInfo ioInfo;

  memset (&ioInfo, 0, sizeof (ioInfo));

  ioInfo.ioi_Command = TIMERCMD_DELAY;
  ioInfo.ioi_Unit = (theTimer->type_mode & TM_TYPE_VBL) ? TIMER_UNIT_VBLANK
                                                        : TIMER_UNIT_USEC;

  ioInfo.ioi_Send.iob_Buffer = tv;
  ioInfo.ioi_Send.iob_Len = sizeof (struct timeval);

  retval = DoIO (theTimer->ioReqItem, &ioInfo);
  CHECKRESULT (retval, "WaitTime: DoIO");

  retval = WaitIO (theTimer->ioReqItem);
  CHECKRESULT (retval, "WaitTime: Wait");

  return 1;

FAILED:

  return 0;
}
