/*****

$Id:

$Log:
*****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  luckie.c - a simple program which uses the Event Utility Library
  to read events from the first Control Pad.
*/

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "event.h"
#include "io.h"
#include "item.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "msgport.h"
#include "nodes.h"
#include "operror.h"
#include "types.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

#include "strings.h"

int
main (int argc, char **argv)
{
  Err err;
  ControlPadEventData cp;
  printf ("Initializing event utility\n");
  err = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (err < 0)
    {
      PrintfSysErr (err);
      return 0;
    }
  do
    {
      err = GetControlPad (1, argc == 1, &cp);
      if (err < 0)
        {
          PrintfSysErr (err);
          return 0;
        }
      printf ("Control pad 1: update %d, bits 0x%x\n", err,
              cp.cped_ButtonBits);
    }
  while ((cp.cped_ButtonBits & ControlStart) == 0);
  printf ("Shutting down luckie\n");
  err = KillEventUtility ();
  if (err < 0)
    {
      PrintfSysErr (err);
    }
  return 0;
}
