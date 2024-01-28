
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: timersleep.c,v 1.6 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/timersleep
|||	timersleep - Demonstrates how to use the timer device to wait for an
|||	    amount of time specified on the command-line.
|||
|||	  Synopsis
|||
|||	    timersleep \<num seconds> \<num microseconds>
|||
|||	  Description
|||
|||	    This program demonstrates how to use the timer device to wait for a
|||	    certain amount of time.
|||
|||	    The program does the following:
|||
|||	    * Parses the command-line arguments
|||
|||	    * Opens the timer device
|||
|||	    * Creates an IOReq
|||
|||	    * Initializes an IOInfo structure
|||
|||	    * Calls DoIO() to perform the wait operation
|||
|||	    * Cleans up
|||
|||	    Note that Portfolio provides convenience routines to make using the
timer
|||	    device easier. For example, CreateTimerIOReq(), DeleteTimerIOReq(),
and
|||	    WaitTime(). This example intends to show how in general one can
|||	    communicate with devices in the Portfolio environment. For more
|||	    information on the timer convenience routines, see the timer device
|||	    documentation.
|||
|||	  Arguments
|||
|||	    num seconds                  Number of seconds to wait.
|||
|||	    num microseconds             Number of microseconds to wait.
|||
|||	  Associated Files
|||
|||	    timersleep.c
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

  if (argc == 3)
    {
      tv.tv_Seconds = strtoul (argv[1], 0, 0);
      tv.tv_Microseconds = strtoul (argv[2], 0, 0);

      deviceItem = OpenNamedDevice ("timer", 0);
      if (deviceItem >= 0)
        {
          ioreqItem = CreateIOReq (0, 0, deviceItem, 0);
          if (ioreqItem >= 0)
            {
              ior = (IOReq *)LookupItem (ioreqItem);

              memset (&ioInfo, 0, sizeof (ioInfo));
              ioInfo.ioi_Command = TIMERCMD_DELAY;
              ioInfo.ioi_Unit = TIMER_UNIT_USEC;
              ioInfo.ioi_Send.iob_Buffer = &tv;
              ioInfo.ioi_Send.iob_Len = sizeof (tv);

              err = DoIO (ioreqItem, &ioInfo);
              if (err >= 0)
                {
                  printf ("slept\n");
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
    }
  else
    {
      printf ("Usage: timersleep <num seconds> <num microseconds>\n");
    }

  return 0;
}
