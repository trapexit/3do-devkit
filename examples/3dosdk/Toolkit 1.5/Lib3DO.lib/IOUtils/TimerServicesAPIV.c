/*****************************************************************************
 *	File:			TimerServicesAPIV.c
 *
 *	Contains:		Routines to send service requests to
 *TimerServices.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company & U.S. West
 *Technologies. All Rights Reserved.
 *
 *	History:
 *	07/26/94  Ian	Split API into 3 modules.
 *	05/31/94  Ian	Reworked internals.  Added new API routines.
 *	03/26/94  Ian	Combined API routines into one  source module.
 *	11/01/93  Ian 	New today.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "TimerServicesInternals.h"

/*----------------------------------------------------------------------------
 * TimerMsgAfterDelayVBL()
 *	Create a timer that will deliver a message after the specified delay.
 *	If the time values are zero the timer is created but not started until
 *	a TimerReset() is done on it.  After the message is delivered the timer
 *	is automatically suspended and you won't get another message unless
 *	you restart or reset it.
 *--------------------------------------------------------------------------*/

TimerHandle
TimerMsgAfterDelayVBL (Item msgport, uint32 fields, uint32 userdata1,
                       uint32 userdata2)
{
  return timer_services_request_ (msgport, 0, fields, userdata1, userdata2,
                                  TSREQUEST_MSGID_NEWTIMER,
                                  TSFLAG_VBL | TSFLAG_MSG | TSFLAG_DELAY);
}

/*----------------------------------------------------------------------------
 * TimerMsgAtTimeVBL()
 *	Create a timer that will deliver a message at the specified time.
 *	If the time values are zero the timer is created but not started until
 *	a TimerReset() is done on it.  After the message is delivered the timer
 *	is automatically suspended and you won't get another message unless
 *	you restart or reset it.
 *--------------------------------------------------------------------------*/

TimerHandle
TimerMsgAtTimeVBL (Item msgport, uint32 fields, uint32 userdata1,
                   uint32 userdata2)
{
  return timer_services_request_ (msgport, 0, fields, userdata1, userdata2,
                                  TSREQUEST_MSGID_NEWTIMER,
                                  TSFLAG_VBL | TSFLAG_MSG | TSFLAG_ATTIME);
}

/*----------------------------------------------------------------------------
 * TimerMsgHeartbeatVBL()
 *	Create a timer that will deliver messages at the specified frequency.
 *	If the time values are zero the timer is created but not started until
 *	a TimerReset() is done on it.  The timer will automatically restart
 *itself as soon as a message is sent, so that you receive a continuous stream
 *	of messages until you explicitly suspend or cancel the timer.
 *--------------------------------------------------------------------------*/

TimerHandle
TimerMsgHeartbeatVBL (Item msgport, uint32 fields, uint32 userdata1,
                      uint32 userdata2)
{
  return timer_services_request_ (msgport, 0, fields, userdata1, userdata2,
                                  TSREQUEST_MSGID_NEWTIMER,
                                  TSFLAG_VBL | TSFLAG_MSG | TSFLAG_HEARTBEAT);
}

/*----------------------------------------------------------------------------
 * TimerSignalAfterDelayVBL()
 *	Create a timer that will deliver a signal after the specified delay.
 *	If the time values are zero the timer is created but not started until
 *	a TimerReset() is done on it.  After the signal is delivered the timer
 *	is automatically suspended and you won't get another signal unless
 *	you restart or reset it.
 *--------------------------------------------------------------------------*/

TimerHandle
TimerSignalAfterDelayVBL (int32 signal, uint32 fields)
{
  return timer_services_request_ (signal, 0, fields, 0, 0,
                                  TSREQUEST_MSGID_NEWTIMER,
                                  TSFLAG_VBL | TSFLAG_SIGNAL | TSFLAG_DELAY);
}

/*----------------------------------------------------------------------------
 * TimerSignalAtTimeVBL()
 *	Create a timer that will deliver a signal at the specified time.
 *	If the time values are zero the timer is created but not started until
 *	a TimerReset() is done on it.  After the signal is delivered the timer
 *	is automatically suspended and you won't get another signal unless
 *	you restart or reset it.
 *--------------------------------------------------------------------------*/

TimerHandle
TimerSignalAtTimeVBL (int32 signal, uint32 fields)
{
  return timer_services_request_ (signal, 0, fields, 0, 0,
                                  TSREQUEST_MSGID_NEWTIMER,
                                  TSFLAG_VBL | TSFLAG_SIGNAL | TSFLAG_ATTIME);
}

/*----------------------------------------------------------------------------
 * TimerSignalHeartbeatVBL()
 *	Create a timer that will deliver signals at the specified frequency.
 *	If the time values are zero the timer is created but not started until
 *	a TimerReset() is done on it.  The timer will automatically restart
 *itself as soon as a signal is sent, so that you receive a continuous stream
 *	of signals until you explicitly suspend or cancel the timer.
 *--------------------------------------------------------------------------*/

TimerHandle
TimerSignalHeartbeatVBL (int32 signal, uint32 fields)
{
  return timer_services_request_ (
      signal, 0, fields, 0, 0, TSREQUEST_MSGID_NEWTIMER,
      TSFLAG_VBL | TSFLAG_SIGNAL | TSFLAG_HEARTBEAT);
}
