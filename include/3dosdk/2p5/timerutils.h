#ifndef __TIMERUTILS_H
#define __TIMERUTILS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: timerutils.h,v 1.2 1994/10/05 17:34:41 vertex Exp $
**
**  Lib3DO timer services and utilities.
**
**  The Get and Sleep functions are 'passive' timer utilities; they execute in
**  the context of the calling task and don't return until the requested action
**  is completed.  The TimerMsg and TimerSignal functions are 'active' timer
**  utilties which run in the context of a separate service thread and notify
**  the caller of completion via message or signal.
**
**  For all functions which take an ioreq parm, that parm is optional.  If
**  you pass a zero the functions will allocate an ioreq internally and delete
**  it before returning.  For repeated calls to these functions it is more
**  efficient to allocate an ioreq and pass it in on each call (like you have
**  to do with WaitVBL() and VRAM operations).
**
**  All functions which work with separate second and microsecond values have
**  internal logic to handle microsecond inputs greater than 1 million.  (For
**  example, you can express 2 seconds as 2,0 or 0,2000000).  Values returned
**  to you from Get functions always have the values normalized (usecs will be
**  less than 1 million.)
**
**  Functions with USec in the name deal with separate seconds and microseconds
**  values.  Functions with TSec in the name deal with a single value which
**  expresses time in 10ths of a second.  Functions with HSec deal with 100ths
**  of a second.  Functions with MSec deal will 1000ths (milliseconds).
**  Functions with no qualifier in the name deal with whole seconds.
**
**  Getting time in terms of 10ths, 100ths, or 1000ths of a second is a
**  convenience; you can get accurate intervals expressed as an easy-to-handle
**  single integer value, but the values will overflow an int32 representation
**  eventually.  For 10ths of a second, it takes about 7 years of continuous
**  running for the values to overflow.  For 100ths it takes roughly 250 days
**  of continuous running to overflow.  And for 1000ths it takes roughly 25
**  days (surprise surprise) to overflow.  Only the GetXXXXTime() functions
**  are sensitive to the overflow problem; the Sleep routines will always work
**  just fine regardless of how long the machine has been on.
**
**  All this overflows-in-some-number-of-days stuff assumes that the system
**  microsecond clock starts at zero seconds at power-on (which is true now,
**  and seems likely to continue to be the case).  It also means that the
**  functions start returning bogus values after the machine has been powered
**  on for that long, not just that you can't time intervals longer than that.
**
**  The overflow periods can be doubled if you choose to interpret the returned
**  values as a uint32, but this of course prevents you from detecting error
**  return values.  Of course, errors aren't very likely as long as you pass
**  a valid IOReq item.  That is, anything that's likely to go wrong in
**  obtaining the time or sleeping is also likely to bring the whole machine
**  down in various other ways.  The timer device is central to the OS's
**  proper functioning, and little can go wrong in working with it, other
**  than invalid IOReq items passed in by you, or failure to allocate an
**  IOReq (also indicative of an impending crash, I think) if you let these
**  routines do the allocation for you.
**
******************************************************************************/


#include "types.h"

/*----------------------------------------------------------------------------
 * A couple handy macros for dealing with microseconds.
 *	The parameters to these macros CAN be true floating point constants.  The
 *	compiler will resolve them into integer constants at compile time, and
 *	no runtime fp support is needed.  For example, HZ_TO_USEC(2.5) is valid.
 *	You can express up to 2147 seconds (about 35 minutes) in microseconds
 *	without overflowing an int32.
 *--------------------------------------------------------------------------*/

#define HZ_TO_USEC(val)		(uint32)(1000000 / (val))	/* hertz to microseconds */
#define SEC_TO_USEC(val)	(uint32)(1000000 * (val))	/* seconds to microseconds */
#define MSEC_TO_USEC(val)	(uint32)(1000    * (val))	/* milliseconds to microseconds */

/*****************************************************************************
 * Passive utilites.
 ****************************************************************************/

#ifdef __cplusplus
  extern "C" {
#endif

Item		GetTimerIOReq(void);

int32		GetVBLTime(Item ioreq, uint32 *hiorder, uint32 *loworder);

int32		GetUSecTime(Item ioreq, uint32 *seconds, uint32 *useconds);
int32		GetMSecTime(Item ioreq);
int32		GetHSecTime(Item ioreq);
int32		GetTSecTime(Item ioreq);
int32		GetTime(Item ioreq);

Err			SleepUSec(Item ioreq, uint32 seconds, uint32 useconds);
Err			SleepMSec(Item ioreq, uint32 mseconds);
Err			SleepHSec(Item ioreq, uint32 hseconds);
Err			SleepTSec(Item ioreq, uint32 tseconds);
Err			Sleep(Item ioreq, uint32 seconds);

/*****************************************************************************
 * TimerServices package...
 ****************************************************************************/

typedef int32 TimerHandle;

/*----------------------------------------------------------------------------
 * Create a timer functions.
 *	Functions that return a TimerHandle type will return negative on error.
 *--------------------------------------------------------------------------*/

TimerHandle	TimerMsgAtTime(Item msgport, uint32 seconds, uint32 microseconds, uint32 userdata1, uint32 userdata2);
TimerHandle	TimerMsgAfterDelay(Item msgport, uint32 seconds, uint32 microseconds, uint32 userdata1, uint32 userdata2);
TimerHandle	TimerMsgHeartbeat(Item msgport, uint32 seconds, uint32 microseconds, uint32 userdata1, uint32 userdata2);

TimerHandle	TimerSignalAtTime(int32 signal, uint32 seconds, uint32 microseconds);
TimerHandle	TimerSignalAfterDelay(int32 signal, uint32 seconds, uint32 microseconds);
TimerHandle	TimerSignalHeartbeat(int32 signal, uint32 seconds, uint32 microseconds);

TimerHandle	TimerMsgAtTimeVBL(Item msgport, uint32 fields, uint32 userdata1, uint32 userdata2);
TimerHandle	TimerMsgAfterDelayVBL(Item msgport, uint32 fields, uint32 userdata1, uint32 userdata2);
TimerHandle	TimerMsgHeartbeatVBL(Item msgport, uint32 fields, uint32 userdata1, uint32 userdata2);

TimerHandle	TimerSignalAtTimeVBL(int32 signal, uint32 fields);
TimerHandle	TimerSignalAfterDelayVBL(int32 signal, uint32 fields);
TimerHandle	TimerSignalHeartbeatVBL(int32 signal, uint32 fields);

/*----------------------------------------------------------------------------
 * Manipulate an existing timer functions.
 *--------------------------------------------------------------------------*/

Err		 	TimerCancel(TimerHandle thandle);
Err	 		TimerSuspend(TimerHandle thandle);
Err			TimerRestart(TimerHandle thandle);
Err		 	TimerReset(TimerHandle thandle, uint32 seconds, uint32 microseconds_or_fields);
Err			TimerChangeUserdata(TimerHandle thandle, uint32 userdata1, uint32 userdata2);

/*----------------------------------------------------------------------------
 * Open/Close functions.
 *	These are used by a task that didn't directly start the service thread
 *	but wants to use the service thread started by some other task.  Only
 *	the main task needs to call Open, then any of its threads can use the
 *	services as well.
 *--------------------------------------------------------------------------*/

Err			TimerServicesOpen(void);
void		TimerServicesClose(void);

/*----------------------------------------------------------------------------
 * Startup/Shutdown functions.
 *	These are used by a task that wants to start and own the service thread.
 *	After calling Startup the task & any of its threads can use the	services.
 *--------------------------------------------------------------------------*/

Err		 	TimerServicesStartup(int32 delta_priority);
void		TimerServicesShutdown(void);
Err			TimerServicesVerify(void);

#ifdef __cplusplus
  }
#endif

#endif /* __TIMERUTILS_H */
