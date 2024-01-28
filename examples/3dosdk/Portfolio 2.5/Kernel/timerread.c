
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: timerread.c,v 1.5 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/timerread
|||	timerread - Demonstrates how to use the timer device to read the
current
|||	    system time.
|||
|||	  Synopsis
|||
|||	    timerread
|||
|||	  Description
|||
|||	    This program demonstrates how to use the timer device to read the
current
|||	    system time.
|||
|||	    The program does the following:
|||
|||	    * Opens the timer device
|||
|||	    * Creates an IOReq
|||
|||	    * Initializes an IOInfo structure
|||
|||	    * Calls DoIO() to perform the read operation
|||
|||	    * Prints out the current time
|||
|||	    * Cleans up
|||
|||	    Note that Portfolio provides convenience routines to make using the
timer
|||	    device easier. For example, CreateTimerIOReq() and
DeleteTimerIOReq().
|||	    This example intends to show how in general one can communicate
with
|||	    devices in the Portfolio environment. For more information on the
timer
|||	    convenience routines, see the timer device documentation.
|||
|||	  Associated Files
|||
|||	    timerread.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include "device.h"
#include "io.h"
#include "item.h"
#include "operror.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "types.h"

int
main (int32 argc, char **argv)
{
  Item deviceItem;
  Item ioreqItem;
  IOReq *ior;
  IOInfo ioInfo;
  TimeVal tv;
  Err err;

  deviceItem = OpenNamedDevice ("timer", 0);
  if (deviceItem >= 0)
    {
      ioreqItem = CreateIOReq (0, 0, deviceItem, 0);
      if (ioreqItem >= 0)
        {
          ior = (IOReq *)LookupItem (ioreqItem);

          memset (&ioInfo, 0, sizeof (ioInfo));
          ioInfo.ioi_Command = CMD_READ;
          ioInfo.ioi_Unit = TIMER_UNIT_USEC;
          ioInfo.ioi_Recv.iob_Buffer = &tv;
          ioInfo.ioi_Recv.iob_Len = sizeof (tv);

          err = DoIO (ioreqItem, &ioInfo);
          if (err >= 0)
            {
              printf ("Seconds %u, microseconds %u\n", tv.tv_Seconds,
                      tv.tv_Microseconds);
            }
          else
            {
              printf ("DoIO() failed: ");
              PrintfSysErr (err);
            }
          DeleteIOReq (ioreqItem);
        }
      else
        {
          printf ("CreateIOReq() failed: ");
          PrintfSysErr (ioreqItem);
        }
      CloseNamedDevice (deviceItem);
    }
  else
    {
      printf ("OpenNamedDevice() failed: ");
      PrintfSysErr (deviceItem);
    }

  return 0;
}
