/*
**
**			TimerHelper.h
**				The header for a simple timing library...
**
**				GREEN Hardware, 1.1 release
**
**
*/

#ifndef __TIMEHELPER__
#define __TIMEHELPER__

#ifndef _TIME_H
#include "time.h"
#endif

typedef struct
{
  uint32 type_mode; // high 16 bits for type, low for mode
  Item devItem;
  Item ioReqItem;
  struct timeval deltaStart;
} TimerHelper, *TimerHelperPtr;

#define TM_TYPE_MICROSEC 0x00000000
#define TM_TYPE_VBL 0x00010000

#define TM_MODE_ABSOLUTE 0x00000000
#define TM_MODE_DELTA 0x00000001

#define TM_RESET true
#define TM_NORMAL false

TimerHelperPtr InitTimer (uint32 mode);
void FreeTimer (TimerHelperPtr theTimer);
bool GetTime (TimerHelperPtr theTimer, bool reset, struct timeval *tv);
bool WaitTime (TimerHelperPtr theTimer, struct timeval *tv);
void MicroSecTimeDelta (struct timeval *fistTime, struct timeval *lastTime,
                        struct timeval *deltaTime);

#endif
