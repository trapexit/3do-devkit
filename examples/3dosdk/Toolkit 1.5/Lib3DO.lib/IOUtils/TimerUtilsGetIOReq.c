/*****************************************************************************
 *	File:			TimerUtilsGetIOReq.c
 *
 *	Contains:		Timer utilities routine to obtain a timer
 *IOReq.
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
 *	The performance-boost technique of caching the timer device item number
 *	in a static variable will fail horribly if the OpenItem() call ever
 *	starts returning unique item numbers instead of just returning the same
 *	item number that was passed in to it.  Perhaps before that happens the
 *	OS gang will give us task-private items we can use for such caching.
 ****************************************************************************/

#include "Debug3DO.h"
#include "TimerUtils.h"
#include "device.h"
#include "io.h"

extern int32
    ItemOpened (Item); // handy function the OS guys never told us about.

static Item timer_dev;

/*----------------------------------------------------------------------------
 * GetTimerIOReq()
 *	Return an IOReq item suitable for use with the other timer utility
 *	functions.  Handy if you're gonna be calling the utilities a lot and
 *	want to avoid the overhead of creating/deleting an ioreq each time.
 *	Use DeleteItem() to dispose of it when you don't need it anymore.
 *--------------------------------------------------------------------------*/

Item
GetTimerIOReq (void)
{
  Item tDev;

  if ((tDev = timer_dev) <= 0)
    {
      if ((timer_dev = tDev = FindDevice ("timer")) < 0)
        {
          DIAGNOSE_SYSERR (tDev, ("FindDevice(timer) failed\n"));
          return tDev;
        }
    }

  if (ItemOpened (tDev) != 0)
    {
      if ((tDev = OpenItem (tDev, NULL)) < 0)
        {
          DIAGNOSE_SYSERR (tDev, ("OpenItem(timer_device) failed\n"));
          return tDev;
        }
    }

  return CreateIOReq (NULL, 0, tDev, 0);
}
