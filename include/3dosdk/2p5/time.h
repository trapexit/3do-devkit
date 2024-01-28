#ifndef __TIME_H
#define __TIME_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: time.h,v 1.15 1994/10/07 20:36:36 vertex Exp $
**
**  Kernel time management definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __DEVICE_H
#include "device.h"
#endif


/*****************************************************************************/


/* If you get a pointer to the timer device item, you can look at the
 * vblank counter within it.
 */
typedef struct TimerDevice
{
	Device	timerdev_dev;
	uint32  timerdev_VBlankCountOverFlow;
	uint32  timerdev_VBlankCount;
} TimerDevice;


/* units of the timer device */
#define TIMER_UNIT_VBLANK	0	/* use io_Offset for arg */
#define TIMER_UNIT_USEC		1	/* uses TimeVal for arg  */

/* commands specific to the timer device */
#define TIMERCMD_DELAY		3	/* delay for   */
#define TIMERCMD_DELAYUNTIL	4	/* delay until */
#define TIMERCMD_METRONOME	5	/* auto-repeat */


/*****************************************************************************/


/* For use with TIMER_UNIT_USEC */
typedef struct timeval
{
    int32 tv_sec;         /* seconds          */
    int32 tv_usec;        /* and microseconds */
} TimeVal;

/* synonyms */
#define tv_Seconds      tv_sec
#define tv_Microseconds tv_usec


/*****************************************************************************/


/* For use with TIMER_UNIT_VBLANK */
typedef struct VBlankTimeVal
{
    uint32 vbltv_VBlankHi32;   /* upper 32 bits of vblank counter */
    uint32 vbltv_VBlankLo32;   /* lower 32 bits of vblank counter */
} VBlankTimeVal;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/* sample the current system time with very low overhead */
uint32 __swi(KERNELSWI+38) SampleSystemTime(void);
void SampleSystemTimeTV(TimeVal *time);

/* timer device convenience routines */
Item CreateTimerIOReq(void);
Err DeleteTimerIOReq(Item ioreq);
Err WaitTime(Item ioreq, uint32 seconds, uint32 micros);
Err WaitUntil(Item ioreq, uint32 seconds, uint32 micros);
Err StartMetronome(Item ioreq, uint32 seconds, uint32 micros, int32 signal);
Err StopMetronome(Item ioreq);

/* time arithmetic convenience routines */
void AddTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result);
void SubTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result);
int32 CompareTimes(const TimeVal *tv1, const TimeVal *tv2);


#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#define TimeLaterThan(t1,t2)        (CompareTimes((t1),(t2)) > 0)
#define TimeLaterThanOrEqual(t1,t2) (CompareTimes((t1),(t2)) >= 0)


/*****************************************************************************/


#endif /* __TIME_H */
