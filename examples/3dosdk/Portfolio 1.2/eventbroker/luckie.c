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

#include "types.h"
#include "item.h"
#include "kernel.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "msgport.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "operror.h"
#include "event.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

#include "strings.h"


int main(int argc, char **argv)
{
  Err err;
  ControlPadEventData cp;
  printf("Initializing event utility\n");
  err = InitEventUtility(1, 0, LC_FocusListener);
  if (err < 0) {
    PrintfSysErr(err);
    return 0;
  }
  do {
    err = GetControlPad (1, argc == 1, &cp);
    if (err < 0) {
      PrintfSysErr(err);
      return 0;
    }
    printf("Control pad 1: update %d, bits 0x%x\n", err, cp.cped_ButtonBits);
  } while ((cp.cped_ButtonBits & ControlStart) == 0);
  printf("Shutting down luckie\n");
  err = KillEventUtility();
  if (err < 0) {
    PrintfSysErr(err);
  }
  return 0;
}

