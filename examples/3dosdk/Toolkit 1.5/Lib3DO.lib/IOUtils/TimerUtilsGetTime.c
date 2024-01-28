/*****************************************************************************
 *	File:			TimerUtilsGetTime.c
 *
 *	Contains:		Timer utilities for reading the current time.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company & U.S. West
 *Technologies. All Rights Reserved.
 *
 *	History:
 *	07/26/94  Ian	Split API into 3 modules.
 *	06/02/94  Ian	Added GetVBLTime() after getting details on the driver
 *					protocol from the OS guys.
 *	04/23/94  Ian	Added HSec and MSec calls (after rationalizing to
 *myself that they can be useful despite the overflow issue detailed in the
 *TimerUtils.h implementation notes).
 *
 *	03/27/94  Ian 	New today.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "time.h"
#include "Debug3DO.h"
#include "TimerUtils.h"
#include "device.h"
#include "io.h"

/*----------------------------------------------------------------------------
 * get_time()
 *	Internal routine to read either the VBL or USEC timer.
 *--------------------------------------------------------------------------*/

static Err
get_time (Item ioreq, uint32 *hiorder, uint32 *loworder, int32 unit)
{
  Err rv;
  Item theIOReq;
  struct timeval tval;
  IOInfo ioinfo = { 0 };

  if ((theIOReq = ioreq) <= 0)
    {
      if ((theIOReq = GetTimerIOReq ()) < 0)
        {
          rv = theIOReq;
          goto ERROR_EXIT;
        }
    }

  ioinfo.ioi_Command = CMD_READ;
  ioinfo.ioi_Unit = (uint8)unit;
  ioinfo.ioi_Recv.iob_Buffer = &tval;
  ioinfo.ioi_Recv.iob_Len = sizeof (tval);

  if ((rv = DoIO (theIOReq, &ioinfo)) < 0)
    {
      DIAGNOSE_SYSERR (rv, ("DoIO(vbl timer read) failed\n"));
      goto ERROR_EXIT;
    }

  if (hiorder)
    {
      *hiorder = tval.tv_sec;
    }

  if (loworder)
    {
      *loworder = tval.tv_usec;
    }

  rv = tval.tv_sec;

ERROR_EXIT:

  if (theIOReq > 0 && theIOReq != ioreq)
    {
      DeleteItem (theIOReq);
    }

  return rv;
}

/*----------------------------------------------------------------------------
 * GetVBLTime()
 *	Return the hi/low order values from the system VBL timer.  If you want
 *	just the low-order part, you can pass a NULL hiorder pointer.
 *--------------------------------------------------------------------------*/

Err
GetVBLTime (Item ioreq, uint32 *hiorder, uint32 *loworder)
{
  return get_time (ioreq, hiorder, loworder, TIMER_UNIT_VBLANK);
}

/*----------------------------------------------------------------------------
 * GetUSecTime()
 *	Return the hi/low order values from the system USEC timer.  If you want
 *	just the seconds you can pass NULL pointers and use the return value.
 *--------------------------------------------------------------------------*/

int32
GetUSecTime (Item ioreq, uint32 *seconds, uint32 *useconds)
{
  return get_time (ioreq, seconds, useconds, TIMER_UNIT_USEC);
}

/*----------------------------------------------------------------------------
 * GetMSecTime()
 *	Return the elapsed milliseconds since power-on.
 *--------------------------------------------------------------------------*/

int32
GetMSecTime (Item ioreq)
{
  Err rv;
  uint32 secs;
  uint32 usecs;

  if ((rv = GetUSecTime (ioreq, &secs, &usecs)) >= 0)
    {
      rv = (secs * 1000L) + (usecs / 1000L);
    }

  return rv;
}

/*----------------------------------------------------------------------------
 * GetHSecTime()
 *	Return elapsed hundredths of a second since power-on.
 *--------------------------------------------------------------------------*/

int32
GetHSecTime (Item ioreq)
{
  Err rv;
  uint32 secs;
  uint32 usecs;

  if ((rv = GetUSecTime (ioreq, &secs, &usecs)) >= 0)
    {
      rv = (secs * 100L) + (usecs / 10000L);
    }

  return rv;
}

/*----------------------------------------------------------------------------
 * GetTSecTime()
 *	Get elapsed tenths of a second since power-on.
 *--------------------------------------------------------------------------*/

int32
GetTSecTime (Item ioreq)
{
  Err rv;
  uint32 secs;
  uint32 usecs;

  if ((rv = GetUSecTime (ioreq, &secs, &usecs)) >= 0)
    {
      rv = (secs * 10L) + (usecs / 100000L);
    }

  return rv;
}

/*----------------------------------------------------------------------------
 * GetTime()
 *	Get elapsed seconds since power-on.
 *--------------------------------------------------------------------------*/

int32
GetTime (Item ioreq)
{
  return GetUSecTime (ioreq, NULL, NULL);
}
