/*
**
**			TimerHelper.c
**				A simple timing library...
**
**				GREEN Hardware, 1.1 release
**
**			8/1/93		NMD		Started
**			8/4/93		NMD		Fixed a bug in WaitTime
*that was causing it not wait.
**			9/24/93		NMD		Cleaned up source a bit
*(librarified it).
**
*/

#include "Init3DO.h"
#include "Kernel.h"
#include "Mem.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "String.h"
#include "Task.h"
#include "Utils3DO.h"
#include "event.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "stdio.h"

#include "Time.h"
#include "TimeHelper.h"
#include "Timer.h"
#include "io.h"

////////////////////////////////////////////// Debugging Macros
//////////////////////////////////////////////////////
#ifdef DEBUG
#define CHECKRESULTSTATUS(res, s)                                             \
  {                                                                           \
    printf ("%s - ", s);                                                      \
    PrintfSysErr (res);                                                       \
    printf ("\n");                                                            \
  }

#define FAILNILSTATUS(s)                                                      \
  {                                                                           \
    printf ("%s - returned NULL\n", s);                                       \
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

TimerHelperPtr
InitTimer (uint32 mode)
{
  TimerHelperPtr theTimer;

  theTimer = (TimerHelperPtr)AllocMem (sizeof (Timer), MEMTYPE_DMA);
  FAILNIL (theTimer, "InitTimer: AllocMem");

  theTimer->devItem = OpenNamedDevice ("timer", 0);
  CHECKRESULT (theTimer->devItem, "InitTimer: OpenNamedDevice");

  theTimer->ioReqItem = CreateIOReq (0, 0, theTimer->devItem, 0);
  CHECKRESULT (theTimer->ioReqItem, "InitTimer: CreateIOReq");

  theTimer->type_mode = mode;

  if (GetTime (theTimer, true, &(theTimer->deltaStart)))
    return theTimer;

FAILED:

  if (theTimer)
    FreeMem (theTimer, sizeof (TimerHelper));

  return NULL;
}

void
FreeTimer (TimerHelperPtr theTimer)
{
  DeleteIOReq (theTimer->ioReqItem);
  CloseItem (theTimer->devItem);

  FreeMem (theTimer, sizeof (TimerHelper));
}

bool
GetTime (TimerHelperPtr theTimer, bool reset, struct timeval *tv)
{
  int32 retval;
  IOInfo ioInfo;

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
      MicroSecTimeDelta (&theTimer->deltaStart, tv, tv);
      // tv->tv_sec = tv->tv_sec - theTimer->deltaStart.tv_sec;
      // tv->tv_usec = tv->tv_usec - theTimer->deltaStart.tv_usec;
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
  CHECKRESULT (retval, "GetTime: DoIO");

  return 1;

FAILED:

  return 0;
}

void
MicroSecTimeDelta (struct timeval *firstTime, struct timeval *lastTime,
                   struct timeval *deltaTime)
{
  deltaTime->tv_sec = lastTime->tv_sec - firstTime->tv_sec;
  deltaTime->tv_usec = lastTime->tv_usec - firstTime->tv_usec;
  if (deltaTime->tv_usec < 0)
    {
      deltaTime->tv_usec += 1000000;
      --deltaTime->tv_sec;
    }
}
