#ifndef __TIMEHELPER__
#define __TIMEHELPER__

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: timehelper.h,v 1.3 1994/11/19 16:38:52 ceckhaus Exp $
**
******************************************************************************/

#include "time.h"

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
bool WaitTimeVal (TimerHelperPtr theTimer, struct timeval *tv);
void MicroSecTimeDelta (struct timeval *firstTime, struct timeval *lastTime,
                        struct timeval *deltaTime);

#endif
