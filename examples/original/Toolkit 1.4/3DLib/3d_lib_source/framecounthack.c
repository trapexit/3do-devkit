/*** Copyright © 1993 The 3DO Company ***/

/********************************************************************
 * All rights reserved.
 * This material constitutes confidential and proprietary information
 * of the 3DO Company and shall not be used by any Person or for any
 * purpose except as expressly authorized in writing by the 3DO Company.
 ********************************************************************/

/*
 * This example shows how to read a timer to see how many seconds
 * and miscroseconds have elapsed since the system was started.
 * This code can be used to instrument your program to see how many
 * frames per second you are achieving.
 * The program requires a routine, PutNumXY,to put a number on the display.
 */

#include "Portfolio.h"

#include "StdLib.h" /* using exit() */
#include "io.h"
#include "kernelnodes.h"
#include "operror.h"
#include "time.h"
#include "timer.h"

#include "Parse3DO.h"
#include "init3DO.h"
#include "utils3DO.h"

#include "uso.h"

int32 nObjCt;

void PutNumXY (Item bitmap, int x, int y, char *buff);

static TagArg IOTags[] = {
  CREATEIOREQ_TAG_DEVICE,
  0,
  TAG_END,
  0,
};

static Item timerDevice, timerRequest;
static IOReq *timerReq;
static IOInfo timerInfo;
static char timername[] = "timer";
static uint32 prevTime;

void
InitInstrumentation (void)
{
  /* Timer initialization */
  timerDevice = OpenItem (
      FindNamedItem (MKNODEID (KERNELNODE, DEVICENODE), timername), 0);

  if (timerDevice < 0)
    {
      PrintfSysErr (timerDevice);
      return;
    }

  IOTags[0].ta_Arg = (void *)timerDevice;
  timerRequest = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), IOTags);

  if (timerRequest < 0)
    {
      PrintfSysErr (timerRequest);
      exit (0);
    }

  timerReq = (IOReq *)LookupItem (timerRequest);
  timerInfo.ioi_Unit = 1; /* Unit one is the microsecond timer */
#if 0
/* ASSERT: static structures are pre-zeroed */
	timerInfo.ioi_Offset = 0;
	timerInfo.ioi_Flags = 0;
#endif
}

uint32
CurrentSeconds (struct timeval *tv)
{
  timerInfo.ioi_Command = CMD_READ;
  timerInfo.ioi_Recv.iob_Buffer = tv;
  timerInfo.ioi_Recv.iob_Len = sizeof (struct timeval);
  DoIO (timerRequest, &timerInfo);
  /* Ignore tv->tv_usec */
  return tv->tv_sec;
}

static uint32 Gtime = 0;

/* Call this from your program every frame (for example)
 * just before DisplayScreen to do a frame count.
 */

void
collecttimedata (Item bitmap, int32 display)
{
  static int framecount;
  static int framecount2;
  static char buff[100];
  frac16 fps, delta_time;
  frac16 DivSF16 (frac16 f1, frac16 f2);
  struct timeval tv;

  framecount++;

  if (display)
    {
      framecount2 = framecount << 16;

      Gtime = CurrentSeconds (&tv); // seconds
      PutNumXY (bitmap, 20, 240 - 50, buff);

      delta_time = ((Gtime - prevTime) << 16);
      fps = DivSF16 (framecount2, delta_time);

      sprintf (buff, "fps %4s", Frac16str (fps));

      prevTime = Gtime;
      framecount = 1;
    }
  else
    {
      // printf("bb framecount2=%ld, gtime=%ld,
      // prevtime=%ld\n",framecount2,Gtime,prevTime);
      PutNumXY (bitmap, 20, 240 - 50, buff);
    }
}

void
PutNumXY (Item bitmap, int x, int y, char *buff)
{
  GrafCon gc;

  memset (&gc, 0, sizeof (GrafCon));
  gc.gc_PenX = x;
  gc.gc_PenY = y;

  /*
          sprintf(buff, "frame : %d  deltime : %d fps %d", framecount,
     Gtime-prevTime, framecount/(Gtime-prevTime));
  */

  DrawText8 (&gc, bitmap, buff);
}
