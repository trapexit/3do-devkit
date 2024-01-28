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
  maus.c - a simple program which uses the Event Utility Library
  to read events from the first mouse.
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
  MouseEventData maus;
  printf ("Initializing event utility\n");
  err = InitEventUtility (1, 1, LC_FocusListener);
  if (err < 0)
    {
      PrintfSysErr (err);
      return 0;
    }
  do
    {
      err = GetMouse (1, argc == 1, &maus);
      if (err < 0)
        {
          PrintfSysErr (err);
          return 0;
        }
      printf ("Mouse 0x%x at (%d,%d)\n", maus.med_ButtonBits,
              maus.med_HorizPosition, maus.med_VertPosition);
    }
  while (maus.med_ButtonBits != MouseLeft + MouseMiddle + MouseRight);
  printf ("Shutting down maus\n");
  err = KillEventUtility ();
  if (err < 0)
    {
      PrintfSysErr (err);
    }
  return 0;
}
