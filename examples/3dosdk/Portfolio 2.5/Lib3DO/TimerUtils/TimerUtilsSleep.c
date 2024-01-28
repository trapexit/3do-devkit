
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: TimerUtilsSleep.c,v 1.3 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO timer utility routines to sleep.
**
******************************************************************************/

#include "debug3do.h"
#include "device.h"
#include "io.h"
#include "time.h"
#include "timerutils.h"

/*----------------------------------------------------------------------------
 * SleepUSec()
 *	Sleep for the amount of time specified by seconds+microseconds.  Works
 *	okay even if microseconds is greater than 1 million.
 *--------------------------------------------------------------------------*/

Err
SleepUSec (Item ioreq, uint32 seconds, uint32 useconds)
{
  Err rv;
  Item theIOReq;
  IOInfo ioinfo = { 0 };
  struct timeval tval;

  if ((theIOReq = ioreq) <= 0)
    {
      if ((theIOReq = GetTimerIOReq ()) < 0)
        {
          rv = theIOReq;
          goto ERROR_EXIT;
        }
    }

  if (useconds >= 1000000)
    {
      seconds += useconds / 1000000;
      useconds = useconds % 1000000;
    }

  tval.tv_sec = seconds;
  tval.tv_usec = useconds;

  ioinfo.ioi_Command = TIMERCMD_DELAY;
  ioinfo.ioi_Unit = TIMER_UNIT_USEC;
  ioinfo.ioi_Send.iob_Buffer = &tval;
  ioinfo.ioi_Send.iob_Len = sizeof (tval);

  if ((rv = DoIO (theIOReq, &ioinfo)) < 0)
    {
      DIAGNOSE_SYSERR (rv, ("DoIO(usec timer delay) failed\n"));
      goto ERROR_EXIT;
    }

  rv = 0;

ERROR_EXIT:

  if (theIOReq != ioreq)
    {
      DeleteIOReq (theIOReq);
    }

  return rv;
}

/*----------------------------------------------------------------------------
 * SleepMSec()
 *	Sleep for the specified number of milliseconds (can be > 1000).
 *--------------------------------------------------------------------------*/

Err
SleepMSec (Item ioreq, uint32 mseconds)
{
  uint32 secs;
  uint32 usecs;

  secs = mseconds / 1000L;
  usecs = (mseconds % 1000L) * 1000L;
  return SleepUSec (ioreq, secs, usecs);
}

/*----------------------------------------------------------------------------
 * SleepHSec()
 *	Sleep for the specified number of hundredths of seconds (can be > 100).
 *--------------------------------------------------------------------------*/

Err
SleepHSec (Item ioreq, uint32 hseconds)
{
  uint32 secs;
  uint32 usecs;

  secs = hseconds / 100L;
  usecs = (hseconds % 100L) * 10000L;
  return SleepUSec (ioreq, secs, usecs);
}

/*----------------------------------------------------------------------------
 * SleepTSec()
 *	Sleep for the specified number of tenths of seconds (can be > 10).
 *--------------------------------------------------------------------------*/

Err
SleepTSec (Item ioreq, uint32 tseconds)
{
  uint32 secs;
  uint32 usecs;

  secs = tseconds / 10L;
  usecs = (tseconds % 10L) * 100000L;
  return SleepUSec (ioreq, secs, usecs);
}

/*----------------------------------------------------------------------------
 * Sleep()
 *	Sleep for the specified number of seconds.
 *--------------------------------------------------------------------------*/

Err
Sleep (Item ioreq, uint32 seconds)
{
  return SleepUSec (ioreq, seconds, 0);
}
