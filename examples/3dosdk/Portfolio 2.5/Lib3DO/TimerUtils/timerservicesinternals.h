#ifndef __TIMERSERVICESINTERNALS_H
#define __TIMERSERVICESINTERNALS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: timerservicesinternals.h,v 1.3 1994/10/05 17:34:41 vertex Exp $
**
**  Lib3DO implementation-private header file for timer services.
**
******************************************************************************/


#include "timerutils.h"
#include "list.h"
#include "nodes.h"
#include "driver.h"
#include "msgport.h"
#include "time.h"

#define TIMER_SERVICES_MSGPORT_NAME "TimerServicesMsgPort"

/*----------------------------------------------------------------------------
 * Timer Services message IDs.
 *	These values show up in the msgid field of a TimerRequest.
 *--------------------------------------------------------------------------*/

enum {
	TSREQUEST_MSGID_SHUTDOWN,		/*	shutdown the service thread please */
	TSREQUEST_MSGID_VERIFY,			/*	verify that clients owning timers live */
	TSREQUEST_MSGID_OPEN,			/*	open services for this task & its threads */
	TSREQUEST_MSGID_CLOSE,			/*	close services for this task & its threads */
	TSREQUEST_MSGID_NEWTIMER,		/*	create a new timer using request parms */
	TSREQUEST_MSGID_RESET,			/*	reset a timer that's already running */
	TSREQUEST_MSGID_SUSPEND,		/*	suspend a timer until told to restart it */
	TSREQUEST_MSGID_CANCEL,			/*	cancel a timer and free its resources */
	TSREQUEST_MSGID_CHANGEUDATA		/*	change userdata attached to a timer */
};

/*----------------------------------------------------------------------------
 * Timer Services Flags.
 *	These values show up in the flags field of a TimerRequest.
 *--------------------------------------------------------------------------*/

#define TSFLAG_USEC				0x00000001
#define TSFLAG_VBL				0x00000002
#define TSFLAG_MSG				0x00000010
#define TSFLAG_SIGNAL			0x00000020
#define TSFLAG_DELAY			0x00000100
#define TSFLAG_ATTIME			0x00000200
#define TSFLAG_HEARTBEAT		0x00000400

/*----------------------------------------------------------------------------
 * TimerControl structure.
 *	Used for internal tracking of timer requests on the service thread side.
 *	Also used in read-only fashion from the API side.
 *	If you add anything to this structure, go to acquire_timer_control() in
 *	the service thread and add code there to zero/init the new field(s).
 *	The ownerTask field is used by the service thread to validate requests in
 *	pretty much the same way as the OS validates item ownership.  The other
 *	ownerXXXX fields are values the service thread remembers as an assist
 *	for the API routines on the owner task's side of the world.
 *--------------------------------------------------------------------------*/

typedef struct TimerControl {
	MinNode			links;
	uint32			userdata1;
	uint32			userdata2;
	struct timeval	delta_tval;
	struct timeval	last_tval;
	uint32			flags;
	union {
	  int32			  signal;
	  Item			  msgport;
	}				notify;
	Item			notifymsg;
	IOReq *			ioreqptr;
	Item			ioreq;
	Item			ownerTask;
	Item			ownerRequestMsg;
	Message *		ownerRequestMsgPtr;
	Item			ownerRequestReplyPort;
} TimerControl;

/*----------------------------------------------------------------------------
 * Timer Services request structure.
 *	This is sent as a message from the API routines to the service thread.
 *--------------------------------------------------------------------------*/

typedef struct TimerRequest {
	int32			msgid;
	uint32			userdata1;
	uint32			userdata2;
	struct timeval	tval;
	int32			notify;
	uint32			flags;
	TimerHandle		thandle;
	Item			ownerTask;
	Item			ownerRequestMsg;
	Message *		ownerRequestMsgPtr;
	Item			ownerRequestReplyPort;
} TimerRequest;

TimerHandle timer_services_request_(int32 notify,
									uint32 seconds, uint32 useconds,
									uint32 userdata1, uint32 userdata2,
									int32 msgid, int32 requestdata);

#endif /* __TIMERSERVICESINTERNALS_H */
