
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: TimerServicesAPI.c,v 1.4 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO routines to send service requests to TimerServices.
**
******************************************************************************/

#include "debug3do.h"
#include "kernel.h"
#include "operror.h"
#include "string.h"
#include "time.h"
#include "timerservicesinternals.h"

static Item timer_services_msgport;

/*****************************************************************************
 * Common service routine used by public API routines...
 ****************************************************************************/

/*----------------------------------------------------------------------------
 * timer_services_request_()
 *	Format a request message from the specified parms and send it off to
 *	the service thread.  When the request is to manipulate an existing
 *	timer we use the msg and port resources we allocated when we created
 *	the timer, otherwise we create those resources herein.
 *--------------------------------------------------------------------------*/

TimerHandle
timer_services_request_ (int32 notify, uint32 seconds, uint32 useconds,
                         uint32 userdata1, uint32 userdata2, int32 msgid,
                         int32 requestdata)
{
  Err err;
  TimerRequest trqst;
  TimerControl *tctl;
  Message *msgPtr;
  Item msg = 0;
  Item replyPort = 0;

  memset (&trqst, 0, sizeof (trqst));
  trqst.userdata1 = userdata1;
  trqst.userdata2 = userdata2;
  trqst.tval.tv_sec = seconds;
  trqst.tval.tv_usec = useconds;
  trqst.notify = notify;
  trqst.ownerTask = CURRENTTASK->t.n_Item;
  trqst.msgid = msgid;

  switch (msgid)
    {

    case TSREQUEST_MSGID_NEWTIMER:
    case TSREQUEST_MSGID_SHUTDOWN:
    case TSREQUEST_MSGID_OPEN:
    case TSREQUEST_MSGID_CLOSE:
    case TSREQUEST_MSGID_VERIFY:

      if ((replyPort = CreateMsgPort (NULL, 0, 0)) <= 0)
        {
          err = replyPort;
          DIAGNOSE_SYSERR (err, ("CreateMsgPort() failed\n"));
          goto ERROR_EXIT;
        }
      if ((msg = CreateMsg (NULL, 0, replyPort)) <= 0)
        {
          err = msg;
          DIAGNOSE_SYSERR (err, ("CreateMsg() failed\n"));
          goto ERROR_EXIT;
        }
      if ((msgPtr = (Message *)LookupItem (msg)) == NULL)
        {
          err = BADITEM;
          DIAGNOSE_SYSERR (err, ("LookupItem() failed\n"));
          goto ERROR_EXIT;
        }
      trqst.ownerRequestMsg = msg;
      trqst.ownerRequestMsgPtr = msgPtr;
      trqst.ownerRequestReplyPort = replyPort;
      trqst.flags = requestdata;
      break;

    default:

      trqst.thandle = requestdata;
      tctl = (TimerControl *)requestdata;
      msg = tctl->ownerRequestMsg;
      msgPtr = tctl->ownerRequestMsgPtr;
      replyPort = tctl->ownerRequestReplyPort;
      break;
    }

  if ((err = SendMsg (timer_services_msgport, msg, &trqst, sizeof (trqst)))
      < 0)
    {
      DIAGNOSE_SYSERR (err, ("SendMsg() failed, %08lx %08lx %08lx\n",
                             timer_services_msgport, msg, replyPort));
      goto ERROR_EXIT;
    }

  if ((err = WaitPort (replyPort, msg)) < 0)
    {
      DIAGNOSE_SYSERR (err, ("WaitMsgPort() failed\n"));
      goto ERROR_EXIT;
    }

  err = msgPtr->msg_Result;

ERROR_EXIT:

  switch (msgid)
    {
    case TSREQUEST_MSGID_NEWTIMER:
      if (err > 0)
        {
          break;
        }
      /* fall thru on err < 0 */
    case TSREQUEST_MSGID_CANCEL:
    case TSREQUEST_MSGID_SHUTDOWN:
    case TSREQUEST_MSGID_OPEN:
    case TSREQUEST_MSGID_CLOSE:
    case TSREQUEST_MSGID_VERIFY:
      DeleteMsg (msg); /* clean up resources owned on this side of the world */
      DeleteMsgPort (replyPort);
      break;
    }

  return err;
}

/*****************************************************************************
 * functions that work on both VBL and USEC timers...
 ****************************************************************************/

/*----------------------------------------------------------------------------
 * TimerServicesOpen()
 *	Open timer services -- used when some other task has started the
 *service thread and this task (and its threads) want to use the services.
 *Also, the TimerServicesStartup() routine calls this internally to init the
 *	API routines for the task that starts the service thread.
 *--------------------------------------------------------------------------*/

Err
TimerServicesOpen (void)
{
  Err err;

  if ((timer_services_msgport = FindMsgPort (TIMER_SERVICES_MSGPORT_NAME)) < 0)
    {
      err = timer_services_msgport;
      goto ERROR_EXIT;
    }

  err = timer_services_request_ (0, 0, 0, 0, 0, TSREQUEST_MSGID_OPEN, 0);

ERROR_EXIT:

  return err;
}

/*----------------------------------------------------------------------------
 * TimerServicesClose()
 *	Close timer services -- used to inform the service thread that this
 *task (and its threads) are done using timer services.  The service thread
 *	won't shut down (because it was started by some other task) but it will
 *	verify that all timers created by this task and its threads have been
 *	released/cancelled.
 *--------------------------------------------------------------------------*/

void
TimerServicesClose (void)
{
  timer_services_request_ (0, 0, 0, 0, 0, TSREQUEST_MSGID_CLOSE, 0);
}

/*----------------------------------------------------------------------------
 * TimerRestart()
 *	Restart a timer.  If the timer was suspended this starts it again using
 *	the same time values as last time.  If the timer is active, this
 *cancels the current operation and starts it again using the same time values
 *as last time.
 *--------------------------------------------------------------------------*/

Err
TimerRestart (TimerHandle thandle)
{
  return timer_services_request_ (0, 0, 0, 0, 0, TSREQUEST_MSGID_RESET,
                                  thandle);
}

/*----------------------------------------------------------------------------
 * TimerReset()
 *	Restart a timer using new time values.  This behaves just like a
 *restart in terms of whether the timer was suspended or active, but it also
 *lets you specify new timer values.  If the timer values are zero, it
 *effectively suspends the timer.
 *--------------------------------------------------------------------------*/

Err
TimerReset (TimerHandle thandle, uint32 seconds, uint32 useconds_or_fields)
{
  return timer_services_request_ (0, seconds, useconds_or_fields, 0, 0,
                                  TSREQUEST_MSGID_RESET, thandle);
}

/*----------------------------------------------------------------------------
 * TimerCancel()
 *	Cancel a timer and release all resources associated with it.  This
 *	suspends a timer, withdraws any pending notification message, and then
 *	deletes the timer.
 *--------------------------------------------------------------------------*/

Err
TimerCancel (TimerHandle thandle)
{
  return timer_services_request_ (0, 0, 0, 0, 0, TSREQUEST_MSGID_CANCEL,
                                  thandle);
}

/*----------------------------------------------------------------------------
 * TimerSuspend()
 *	Suspend a timer.  If a notification message is pending it is withdrawn.
 *	The timer then lies dormant until restarted, reset, or cancelled.
 *--------------------------------------------------------------------------*/

Err
TimerSuspend (TimerHandle thandle)
{
  return timer_services_request_ (0, 0, 0, 0, 0, TSREQUEST_MSGID_SUSPEND,
                                  thandle);
}

/*----------------------------------------------------------------------------
 * TimerChangeUserdata()
 *	Change the userdata values associated with a timer.  Doesn't affect
 *	timing in progress, if any.
 *--------------------------------------------------------------------------*/

Err
TimerChangeUserdata (TimerHandle thandle, uint32 userdata1, uint32 userdata2)
{
  return timer_services_request_ (0, 0, 0, userdata1, userdata2,
                                  TSREQUEST_MSGID_CHANGEUDATA, thandle);
}
