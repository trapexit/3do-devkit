
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: luckie.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/luckie
|||	luckie - Uses the event broker to read events from the first control
pad.
|||
|||	  Synopsis
|||
|||	    luckie [anything]
|||
|||	  Description
|||
|||	    Uses the event broker to monitor and report activity for the first
control
|||	    pad plugged in to the control port.
|||
|||	  Arguments
|||
|||	    anything                     If you supply no arguments to this
program,
|||	                                 it asks GetControlPad() to put the
task to
|||	                                 sleep when waiting for an event. If
you
|||	                                 supply an argument, GetControlPad()
will not
|||	                                 put the task to sleep, and the program
will
|||	                                 poll the control pad.
|||
|||	  Associated Files
|||
|||	    luckie.c
|||
|||	  Location
|||
|||	    examples/EventBroker/Event_Broker_Tests
|||
**/

#include "event.h"
#include "stdio.h"
#include "types.h"

/*****************************************************************************/

int
main (int32 argc, char **argv)
{
  Err err;
  ControlPadEventData cp;

  printf ("Initializing event utility\n");

  err = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (err < 0)
    {
      printf ("Unable to initialize the event utility: ");
      PrintfSysErr (err);
      return 0;
    }

  do
    {
      err = GetControlPad (1, argc == 1, &cp);
      if (err < 0)
        {
          printf ("GetControlPad() failed: ");
          PrintfSysErr (err);
          break;
        }
      printf ("Control pad 1: update %d, bits 0x%x\n", err,
              cp.cped_ButtonBits);
    }
  while ((cp.cped_ButtonBits & ControlStart) == 0);

  printf ("Shutting down luckie\n");

  KillEventUtility ();

  return 0;
}
