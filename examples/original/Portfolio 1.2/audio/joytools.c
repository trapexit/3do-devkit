/*
** SImple Joypad Access
** By:  Phil Burk
*/

/*
** Copyright (C) 1993, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "audio.h"
#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "folio.h"
#include "graphics.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#include "audiodemo.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

static ControlPadEventData cped;

/******************************************************************/
int32
InitJoypad (void)
{
  int32 Result;

  Result = InitEventUtility (1, 0, LC_FocusListener);
  if (Result < 0)
    {
      ERR (("InitJoypad: error in InitEventUtility\n"));
      PrintfSysErr (Result);
    }
  return Result;
}

/******************************************************************/
int32
TermJoypad (void)
{
  int32 Result;

  Result = KillEventUtility ();
  if (Result < 0)
    {
      ERR (("TermJoypad: error in KillEventUtility\n"));
      PrintfSysErr (Result);
    }
  return Result;
}

/******************************************************************/
int32
ReadJoypad (uint32 *Buttons)
{
  int32 Result;
  Result = GetControlPad (1, FALSE, &cped);
  if (Result < 0)
    {
      ERR (("ReadJoypad: error in GetControlPad\n"));
      PrintfSysErr (Result);
    }
  *Buttons = cped.cped_ButtonBits;
  DBUG (("ReadJoypad: *Buttons = 0x%8x, err = 0x%x\n", *Buttons, Result));
  return Result;
}

/******************************************************************/
int32
WaitJoypad (uint32 Mask, uint32 *Buttons)
{
  static int32 oldbutn = 0;
  int32 Result;
  int32 butn;
  do
    {
      Result = GetControlPad (1, TRUE, &cped);
      if (Result < 0)
        {
          PrintfSysErr (Result);
          return Result;
        }
      butn = cped.cped_ButtonBits;
      if (butn == 0)
        oldbutn = 0;
    }
  while ((butn & Mask) == (oldbutn & Mask));

  oldbutn = butn;
  DBUG (("butn = 0x%x\n", butn));
  *Buttons = butn;
  return Result;
}
