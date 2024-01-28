
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: TimerUtilsGetIOReq.c,v 1.3 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO timer utilities routine to obtain a timer IOReq.
**
**  The performance-boost technique of caching the timer device item number
**  in a static variable will fail horribly if the OpenItem() call ever
**  starts returning unique item numbers instead of just returning the same
**  item number that was passed in to it.  Perhaps before that happens the
**  OS gang will give us task-private items we can use for such caching.
**
******************************************************************************/

#include "debug3do.h"
#include "device.h"
#include "io.h"
#include "kernel.h"
#include "timerutils.h"

static Item timer_dev;

/*----------------------------------------------------------------------------
 * GetTimerIOReq()
 *	Return an IOReq item suitable for use with the other timer utility
 *	functions.  Handy if you're gonna be calling the utilities a lot and
 *	want to avoid the overhead of creating/deleting an ioreq each time.
 *	Use DeleteIOReq() to dispose of it when you don't need it anymore.
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

  if (IsItemOpened (CURRENTTASK->t.n_Item, tDev) != 0)
    {
      if ((tDev = OpenItem (tDev, NULL)) < 0)
        {
          DIAGNOSE_SYSERR (tDev, ("OpenItem(timer_device) failed\n"));
          return tDev;
        }
    }

  return CreateIOReq (NULL, 0, tDev, 0);
}
