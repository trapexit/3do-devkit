/*****************************************************************************
 *	File:			TimerServices.c
 *
 *	Contains:		The timer services thread.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company & U.S. West Technologies. 
 *					All Rights Reserved.
 *
 *	History:
 *
 *	08/26/94  Ian	Changed from DLLUtils to Portfolio DLL routines.
 *
 *	07/19/94  Ian	Added trequest_verify().
 *
 *	05/30/94  Ian	Lots of little changes:
 *				  - Converted from the old MsgportMulti dispatcher to the
 *					new MSEvents dispatcher.  
 *				  - Changed IVDS-specific error	codes to 3DO standard codes. 
 *				  - Removed	the Sleep and GetTime functionality, since that's 
 *					now covered more efficiently by the TimerUtils package.
 *				  - Values expressed in microseconds passed in by clients can
 *					now be larger than 1 million.  We internally convert them
 *					to some number of seconds plus the remainder in usecs; the
 *					seconds resulting from conversion get added to the number
 *					of seconds specified by the client.  But all this happens
 *					within our private data structures, client values are
 *					not changed.
 *
 *	03/26/94  Ian	Busy day:
 *				  - Did a variety of cleanups -- more error checking where
 *					"Can't fail" assumptions were being made, the DLL link field
 *					is now at the start of the TimerControl structure, etc.
 *				  - It looks like the OS gang hasn't given us an OwnerData
 *					field in the message structure for backlinks, so 'plan b'
 *					has been implemented: use the item name as the backlink.
 *				  -	Implemented 'AtTime' functionality.
 *				  - Rewrote all Heartbeat-related logic to prevent 'drift' in
 *					the heartbeat frequency.  The old logic treated a heartbeat
 *					as a series of Delay requests, and the frequency of events
 *					would drift based on how long it took the owner to process
 *					each event and reply back to us (our own overhead added a
 *					bit too, so even signal-based notifications would drift).
 *					Now we treat heartbeats as a series of AtTime requests, with
 *					the time calculated by adding the delta to the last event time.
 *				  - Speeded up the GetTime processing by pre-allocating needed items
 *					and structures.  This is mostly because we call this a lot 
 *					internally now to process heartbeat timers.
 *
 *	03/17/94  Ian	Implemented pooled allocation of TimerControl structures
 *					and their attached resources.  This was not so much for
 *					performance as for resource tracking.  We now report any
 *					unreleased timercontrols at shutdown time and release
 *					them anyway.  Since the resource tracking code is 90% of
 *					what's needed for pooled allocation, we got that for free.
 *
 *	03/15/94  Ian	Fixed a bug caused by the OS gang fixing a bug of theirs.
 *					GetThisMsg() now returns an error if the receiver has
 *					received the message, so we can no longer consider a 
 *					failure of GetThisMsg() to be a problem.  Ideally, we'd
 *					consider anything but a "receiver already has the message"
 *					status to be a serious error, but GetThisMsg() returns a
 *					generic -1 status instead of a specific error code.
 *					(07/15/94: oops, my mistake; it seems -1 is the specific
 *					error code meaning receiver has the msg.  strange but true.)
 *
 *	02/15/94  RLG	Added support for the VBL timer as well as USEC.
 *					The VBL support includes Heartbeats, AtTime, and Delays, 
 *					but not GetTime.  (GetTime for a VBL is [NOT] best done by 
 *					just retrieving GrafBase->gf_VBLNumber.)
 *
 *	11/01/93  Ian 	New today.
 *
 *	Implementation notes:
 *
 * To do:
 *	Implement a usecount for startup/shutdown requests.
 ****************************************************************************/

#include "TimerServicesInternals.h"
#include "UMemory.h"
#include "Macros3DO.h"
#include "MsgUtils.h"
#include "Debug3DO.h"
#include "operror.h"
#include "device.h"
#include "io.h"
#include "string.h"
#include "kernel.h"

/*----------------------------------------------------------------------------
 * MSEvent stuff.
 *--------------------------------------------------------------------------*/

static int32 	handle_notifymsg_reply(MSEventData *msevent, void *userContext);
static int32 	handle_iomsg(MSEventData *msevent, void *userContext);
static int32 	handle_requestmsg(MSEventData *msevent, void *userContext);

static int32	mseHandle = -1;

static MSEventData 	ms_events[] = {
	{"TimerServicesReplyPort", 		handle_notifymsg_reply, NULL},
	{"TimerServicesIOPort",    		handle_iomsg,      		NULL},
	{TIMER_SERVICES_MSGPORT_NAME,	handle_requestmsg, 		NULL},
};

#define REPLY_PORT		0
#define IOMSG_PORT		1
#define REQUEST_PORT	2

#define	timer_reply_msgport 			ms_events[REPLY_PORT].port
#define timer_io_msgport				ms_events[IOMSG_PORT].port
#define timer_services_request_msgport	ms_events[REQUEST_PORT].port

/*----------------------------------------------------------------------------
 * resource control stuff and various allocate-once items and objects.
 *--------------------------------------------------------------------------*/

#define TIMER_THREAD_STACKSIZE	2048

static Item 	timer_dev;

static Item		readtimer_ioreq;
static IOReq *	readtimer_ioreqptr;
static IOInfo	readtimer_ioinfo;

static int32	startup_signal;
static Err		startup_status;
static Item 	startup_parent_task;

static int32	tctl_free_count;
static List		tctl_free_list;
static List		tctl_inuse_list;

#define MAX_FREE_TCTLS	10	// don't hold more than this in the free pool

/*----------------------------------------------------------------------------
 * misc flags and stuff.
 *--------------------------------------------------------------------------*/

#define TSIFLAG_IO_ACTIVE			0x00010000
#define TSIFLAG_MSG_OUTSTANDING		0x00020000
#define TSIFLAG_RELEASE_ON_REPLY	0x00040000

typedef enum ReleaseAction {
	DONT_RELEASE_TIMER,
	RELEASE_TIMER
} ReleaseAction;

typedef enum StartAction {
	RESET_HEARTBEAT,
	CONTINUE_HEARTBEAT
} StartAction;

static Err	 io_cancel(TimerControl *tctl);	// resolve forward ref

/*****************************************************************************
 * delete_timer_control()
 *	Delete a timer control structure and the resources attached to it.
 ****************************************************************************/

static void delete_timer_control(TimerControl *tctl)
{
	Err	 err;
	
	if (tctl != NULL) {
		if (tctl->ioreq > 0) {
			if ((err = DeleteIOReq(tctl->ioreq)) < 0) {
				DIAGNOSE_SYSERR(err, ("DeleteIOReq() failed\n"));
			}
		}
		if (tctl->notifymsg > 0) {
			if ((err = DeleteMsg(tctl->notifymsg)) < 0) {
				DIAGNOSE_SYSERR(err, ("DeleteMsg() failed\n"));
			}	
		}
		Free(tctl);
	}
}

/*****************************************************************************
 * create_timer_control()
 *	Create a timer control structure and attach the required resources 
 *	(IOReq, msg, etc) to it.
 ****************************************************************************/

static Err create_timer_control(TimerControl **retval)
{
	Err	 			err;
	TimerControl *	tctl;
	char			namebuf[12];
	
	if ((tctl = (TimerControl *)Malloc(sizeof(*tctl), MEMTYPE_ANY|MEMTYPE_FILL|0x00)) == NULL) {
		err = NOMEM;
		DIAGNOSE_SYSERR(err, ("Can't create internal timer control structure\n"));
		goto ERROR_EXIT;
	}
	
	if ((tctl->ioreq = CreateIOReq(NULL, 0, timer_dev, timer_io_msgport)) < 0) {
		err = tctl->ioreq;
		DIAGNOSE_SYSERR(err, ("CreateIORreq() failed\n"));
		goto ERROR_EXIT;
	}

	sprintf(namebuf, "0x%08p", tctl);	// encode backlink as msg item name
	if ((tctl->notifymsg = CreateSmallMsg(namebuf, 0, timer_reply_msgport)) < 0) {
		err = tctl->notifymsg;
		DIAGNOSE_SYSERR(err, ("CreateMsg() failed\n"));
		goto ERROR_EXIT;
	}

	tctl->ioreqptr = (IOReq *)LookupItem(tctl->ioreq);

	*retval = tctl;
	err = 0;
	
ERROR_EXIT:

	if (err < 0) {	
		delete_timer_control(tctl);
	}
	
	return err;

}

/*****************************************************************************
 * timer_services_cleanup()
 *	Release all resources acquired at init time, prepare to exit() thread.
 *	Called in response to receiving a SHUTDOWN message.
 ****************************************************************************/

static void timer_services_cleanup(void)
{
	TimerControl *	cur;
	TimerControl *	next;
	char *			ownerTaskName;
	Task *			ownerTaskPtr;
	
	for (cur = (TimerControl *)FirstNode(&tctl_inuse_list); IsNode(&tctl_inuse_list, cur); cur = next) {
		next = (TimerControl *)NextNode(cur);
		ownerTaskPtr  = (Task*)LookupItem(cur->ownerTask);
		ownerTaskName = (ownerTaskPtr) ? ownerTaskPtr->t.n_Name : "task has died";
		DIAGNOSE(("Unreleased timer at shutdown, ownertask = %08lx (%s)\n", cur->ownerTask, ownerTaskName));
		io_cancel(cur);
		delete_timer_control(cur);
	}
	
	for (cur = (TimerControl *)FirstNode(&tctl_free_list); IsNode(&tctl_free_list, cur); cur = next) {
		next = (TimerControl *)NextNode(cur);
		delete_timer_control(cur);
	}
	tctl_free_count	= 0;

	if (readtimer_ioreq > 0) {
		DeleteItem(readtimer_ioreq);
		readtimer_ioreq = 0;
	}

	if (timer_dev > 0) {
		CloseItem(timer_dev);
		timer_dev = 0;
	}

	if (mseHandle > 0) {
		CleanupMSEvents(mseHandle);
		mseHandle = 0;
	}
	
}

/*****************************************************************************
 * timer_services_init()
 *	Acquire resources needed to provide timer services:
 *	 - 	Open timer device.
 *	 -	Create the msgports we use to service requests from other tasks.
 *  Called immediately upon thread startup.
 ****************************************************************************/

static void timer_services_init(void)
{
	if ((timer_dev = OpenNamedDevice("timer", 0)) < 0) {
		startup_status = timer_dev;
		DIAGNOSE_SYSERR(startup_status, ("OpenNamedDevice(timer) failed\n"));
		goto ERROR_EXIT;
	}

	if ((mseHandle = SetupMSEvents(ms_events, ArrayElements(ms_events), 0)) < 0) {
		startup_status = mseHandle;
		DIAGNOSE_SYSERR(startup_status, ("SetupMSEvents() failed\n"));
		goto ERROR_EXIT;
	}
	
	if ((readtimer_ioreq = CreateIOReq(NULL, 150, timer_dev, 0)) < 0) {
		startup_status = readtimer_ioreq;
		DIAGNOSE_SYSERR(startup_status, ("CreateIORreq() failed\n"));
	}
	
	if ((readtimer_ioreqptr = (IOReq *)LookupItem(readtimer_ioreq)) == NULL) {
		startup_status = BADITEM;
		DIAGNOSE_SYSERR(startup_status, ("LookupItem() failed for readtimer_ioreq\n"));
		goto ERROR_EXIT;
	}

	memset(&readtimer_ioinfo, 0, sizeof(readtimer_ioinfo));
	readtimer_ioinfo.ioi_Command = CMD_READ;

	InitList(&tctl_inuse_list, NULL);
	InitList(&tctl_free_list,  NULL);

	startup_status = 0;

ERROR_EXIT:

	SendSignal(startup_parent_task, startup_signal);
	
	if (startup_status < 0) {
		timer_services_cleanup();
		exit((int)startup_status);
	}

}

/*****************************************************************************
 * release_timer_control()
 *	Put a timer control structure back into the pool of free resources.
 ****************************************************************************/

static void release_timer_control(TimerControl *tctl)
{
	tctl->ownerTask = 0xDEADD00D;
	RemNode((Node *)tctl);
	if (tctl_free_count < MAX_FREE_TCTLS) {
		++tctl_free_count;
		AddHead(&tctl_free_list, (Node *)tctl);
	} else {
		delete_timer_control(tctl);
	}
}

/*****************************************************************************
 * acquire_timer_control()
 *	Grab a timer control structure from the pool of free resources, or create
 *	a new one if the pool is empty.
 ****************************************************************************/

static Err acquire_timer_control(TimerControl **retval)
{
	Err				err;
	TimerControl *	tctl;

	if (IsEmptyList(&tctl_free_list)) {
		if ((err = create_timer_control(&tctl)) < 0) {
			goto ERROR_EXIT;
		}
	} else {
		--tctl_free_count;
		tctl = (TimerControl *)RemHead(&tctl_free_list);
		tctl->flags					= 0;
		tctl->notify.signal			= 0;
		tctl->ownerTask				= 0;
		tctl->ownerRequestMsg		= 0;
		tctl->ownerRequestReplyPort	= 0;
	}
	
	AddHead(&tctl_inuse_list, (Node *)tctl);
	*retval = tctl;
	err = 0;
	
ERROR_EXIT:

	return err;	
}

/*****************************************************************************
 * io_readtime()
 *	Read the current time from the USEC clock.  We do this a lot to service
 *	HEARTBEAT timers, so it needs to be short and sweet.
 ****************************************************************************/

static Err io_readtime(struct timeval *tv, uint8 unit)
{
	Err		err;

	readtimer_ioinfo.ioi_Unit	 		 = unit;
	readtimer_ioinfo.ioi_Recv.iob_Buffer = tv;
	readtimer_ioinfo.ioi_Recv.iob_Len	 = sizeof(*tv);

	if ((err = DoIO(readtimer_ioreq, &readtimer_ioinfo)) < 0) {
		DIAGNOSE_SYSERR(err, ("DoIO() failed\n"));
		goto ERROR_EXIT;
	}
	
	if ((err = readtimer_ioreqptr->io_Error) < 0) {
		DIAGNOSE_SYSERR(err, ("Timer device CMD_READ failed\n"));
		goto ERROR_EXIT;
	}

	err = 0;
	
ERROR_EXIT:

	return err;
}

/*****************************************************************************
 * io_cancel()
 *	Cancel an outstanding timer request, and if it had already completed, 
 *	withdraw the notification message we sent to the client earlier if the
 *	notification was by message (there's no way to withdraw a signal).
 *
 *	NOTE: The 1p2 OS release fixed the bug that would let you withdraw a 
 *	message while the recipient was 'holding' it (ie, had retrieved it but
 *	not yet replied to it).  If the receiver has dequeued the message, the
 *	GetThisMessage() function returns -1 to indicate such.  In this case,
 *	we leave the MSG_OUTSTANDING flag set.
 ****************************************************************************/

static Err io_cancel(TimerControl *tctl)
{
	Err	 err;
	
	if (tctl->flags & TSIFLAG_IO_ACTIVE) {
		tctl->flags &= ~TSIFLAG_IO_ACTIVE;
		AbortIO(tctl->ioreq);
		if ((err = WaitPort(timer_io_msgport, tctl->ioreqptr->io_MsgItem)) < 0) {
			DIAGNOSE_SYSERR(err, ("Failed to get pending timer ioreq completion msg\n"));
			goto ERROR_EXIT;
		}
	} 
	
	if (tctl->flags & TSIFLAG_MSG_OUTSTANDING) {
		if (GetThisMsg(tctl->notifymsg) != -1) {
			tctl->flags &= ~TSIFLAG_MSG_OUTSTANDING;
		}
	}
	
	err = 0;
	
ERROR_EXIT:

	return err;
}

/*****************************************************************************
 * io_start()
 *	Send a timer IO request to the timer device.
 ****************************************************************************/

static Err io_start(TimerControl *tctl, StartAction start_action)
{
	Err		err;
	IOInfo	ioinfo;

	memset(&ioinfo, 0, sizeof(ioinfo));
	ioinfo.ioi_User = (uint32)tctl;
	
	if (tctl->flags & TSFLAG_DELAY) {
		ioinfo.ioi_Command = TIMERCMD_DELAY;
	} else {
		ioinfo.ioi_Command = TIMERCMD_DELAYUNTIL;
	}
	
	if (tctl->flags & TSFLAG_HEARTBEAT) {
		if (start_action == RESET_HEARTBEAT) {
			if (tctl->flags & TSFLAG_VBL) {
				io_readtime(&tctl->last_tval, TIMER_UNIT_VBLANK);
			} else {
				io_readtime(&tctl->last_tval, TIMER_UNIT_USEC);
			}
		}
	} else {
		tctl->last_tval.tv_sec  = 0;
		tctl->last_tval.tv_usec = 0;
	}

	tctl->last_tval.tv_sec  += tctl->delta_tval.tv_sec;
	tctl->last_tval.tv_usec += tctl->delta_tval.tv_usec;
	
	if (tctl->flags & TSFLAG_VBL) {
		ioinfo.ioi_Unit 	= TIMER_UNIT_VBLANK;
		ioinfo.ioi_Offset	= tctl->last_tval.tv_usec;
	} else {
		if (tctl->last_tval.tv_usec >= 1000000) {
			tctl->last_tval.tv_sec  += tctl->last_tval.tv_usec / 1000000;
			tctl->last_tval.tv_usec  = tctl->last_tval.tv_usec % 1000000;
		}
		ioinfo.ioi_Unit 			= TIMER_UNIT_USEC;
		ioinfo.ioi_Send.iob_Buffer	= &tctl->last_tval;
		ioinfo.ioi_Send.iob_Len 	= sizeof(tctl->last_tval);
	}

	if ((err = SendIO(tctl->ioreq, &ioinfo)) < 0) {
		DIAGNOSE_SYSERR(err, ("SendIO() failed\n"));
		goto ERROR_EXIT;
	}
	
	tctl->flags |= TSIFLAG_IO_ACTIVE;

	err = 0;
	
ERROR_EXIT:
	
	return err;
}

/*****************************************************************************
 * trequest_verify()
 *	Walk the list of in-use timers verifying that the owning tasks still live.
 ****************************************************************************/

static Err trequest_verify(void)
{
	TimerControl *	cur;
	TimerControl *	next;
	Task *			ownerTaskPtr;
	
	for (cur = (TimerControl *)FirstNode(&tctl_inuse_list); IsNode(&tctl_inuse_list, cur); cur = next) {
		next = (TimerControl *)NextNode(cur);
		ownerTaskPtr = (Task*)LookupItem(cur->ownerTask);
		if (ownerTaskPtr == NULL) {
			DIAGNOSE(("Task %08lx died without releasing timer handle %08p; now released\n", 
				cur->ownerTask, cur));
			io_cancel(cur);
			release_timer_control(cur);
		}
	}
	
	return 0;
}

/*****************************************************************************
 * trequest_new_timer()
 *	Handle a client request for notification.
 *  A request for 0 delay sets up the timercontrol but doesn't start any
 *	I/O for it.  This allows a call that preallocates a timercontrol, but
 *	actual servicing won't happen until a TimerReset() call comes through
 *	to set non-zero time values.
 ****************************************************************************/

static TimerHandle trequest_new_timer(MsgValueTypes *mv)
{
	Err				err;
	TimerRequest *	trqst;
	TimerControl *	tctl = NULL;

	trqst = (TimerRequest *)mv;

	if ((err = acquire_timer_control(&tctl)) < 0) {
		goto ERROR_EXIT;
	}

	tctl->userdata1				= trqst->userdata1;
	tctl->userdata2				= trqst->userdata2;
	tctl->delta_tval			= trqst->tval;
	tctl->notify.signal			= trqst->notify;
	tctl->flags					= trqst->flags;
	tctl->ownerTask				= trqst->ownerTask;
	tctl->ownerRequestMsg		= trqst->ownerRequestMsg;
	tctl->ownerRequestMsgPtr	= trqst->ownerRequestMsgPtr;
	tctl->ownerRequestReplyPort	= trqst->ownerRequestReplyPort;

	if (trqst->tval.tv_sec != 0 || trqst->tval.tv_usec != 0) {
		if ((err = io_start(tctl, RESET_HEARTBEAT)) < 0) {
			goto ERROR_EXIT;
		}
	}
	
	return (TimerHandle)tctl; // cast tctl ptr to int32 opaque handle for client

ERROR_EXIT:

	release_timer_control(tctl);
	return err;
}

/*****************************************************************************
 * trequest_cancel_timer()
 *	Handle a client request to cancel a timer event requested earlier.
 ****************************************************************************/

static Err trequest_cancel_timer(MsgValueTypes *mv, ReleaseAction release_tctl)
{
	Err				err;
	TimerRequest *	trqst;
	TimerControl *	tctl;

	trqst 	= (TimerRequest *)mv;
	tctl	= (TimerControl *)trqst->thandle;
	
	if (tctl->ownerTask != trqst->ownerTask) {
		err = NOTOWNER;
		DIAGNOSE(("Cannot cancel timer, task %08lx is not owner of TimerControl handle %08p (owner is task %08lx)\n",
			trqst->ownerTask, tctl, tctl->ownerTask));
		goto ERROR_EXIT;
	}

	err = io_cancel(tctl);		

	if (release_tctl == RELEASE_TIMER) {
		if (tctl->flags & TSIFLAG_MSG_OUTSTANDING) {	
			tctl->flags |= TSIFLAG_RELEASE_ON_REPLY;
		} else {
			release_timer_control(tctl);
		}
	}
	
ERROR_EXIT:

	return err;
}
 
/*****************************************************************************
 * trequest_reset_timer()
 *	Handle a client request to cancel a prior timer event and start a new
 *	one using (potentially) different time.
 ****************************************************************************/

static Err trequest_reset_timer(MsgValueTypes *mv)
{
	Err				err;
	TimerRequest *	trqst;
	TimerControl *	tctl;

	trqst = (TimerRequest *)mv;
	tctl  = (TimerControl *)trqst->thandle;
	
	if (tctl->ownerTask != trqst->ownerTask) {
		err = NOTOWNER;
		DIAGNOSE(("Cannot reset timer, task %08lx is not owner of TimerControl handle %08p (owner is task %08lx)\n",
			trqst->ownerTask, tctl, tctl->ownerTask));
		goto ERROR_EXIT;
	}

	if ((err = io_cancel(tctl)) < 0) {
		goto ERROR_EXIT;
	}

	if (trqst->tval.tv_sec != 0 || trqst->tval.tv_usec != 0) {
		tctl->delta_tval = trqst->tval;
	}
	
	err = io_start(tctl, RESET_HEARTBEAT);

ERROR_EXIT:

	return err;
}

/*****************************************************************************
 * trequest_change_userdata()
 *	Handle a client request to change the userdata values for a timer.
 ****************************************************************************/

static Err trequest_change_userdata(MsgValueTypes *mv)
{
	Err				err;
	TimerRequest *	trqst;
	TimerControl *	tctl;

	trqst = (TimerRequest *)mv;
	tctl  = (TimerControl *)trqst->thandle;
	
	if (tctl->ownerTask != trqst->ownerTask) {
		err = NOTOWNER;
		DIAGNOSE(("Cannot change timer, task %08lx is not owner of TimerControl handle %08p (owner is task %08lx)\n",
			trqst->ownerTask, tctl, tctl->ownerTask));
		goto ERROR_EXIT;
	}

	tctl->userdata1 = trqst->userdata1;
	tctl->userdata2 = trqst->userdata2;

	err = 0;

ERROR_EXIT:

	return err;
}

/*****************************************************************************
 * handle_requestmsg()
 *	Handle requests from clients for timer event services.
 ****************************************************************************/

static int32 handle_requestmsg(MSEventData *msevent, void *userContext)
{
	Err				err;
	MsgValueTypes *	mv;
	
	(void)userContext;	// no whining about unused vars please

	mv = msevent->msgValues;

	switch (mv[0].msgid) {
	  case TSREQUEST_MSGID_SHUTDOWN:
		return msevent->msgItem;	// reply will be done from timer_services_thread() routine	  
	  case TSREQUEST_MSGID_OPEN:
	  	err = 0;
		break;
	  case TSREQUEST_MSGID_CLOSE:
	  	err = 0;
		break;
	  case TSREQUEST_MSGID_VERIFY:
		err = trequest_verify();
		break;
	  case TSREQUEST_MSGID_NEWTIMER:
		err = trequest_new_timer(mv);
		break;
	  case TSREQUEST_MSGID_CHANGEUDATA:
	  	err = trequest_change_userdata(mv);
		break;
	  case TSREQUEST_MSGID_RESET:
		err = trequest_reset_timer(mv);
		break;
	  case TSREQUEST_MSGID_CANCEL:
		err = trequest_cancel_timer(mv, RELEASE_TIMER);
		break;
	  case TSREQUEST_MSGID_SUSPEND:
	  	err = trequest_cancel_timer(mv, DONT_RELEASE_TIMER);
		break;
	  default:
		err = NOSUPPORT;
		DIAGNOSE(("Unknown message type %ld received by timer services request handler\n", mv[0].msgid));
		break;
	}

	ReplyMsg(msevent->msgItem, err, 0, 0);

	return 0;
}

/*****************************************************************************
 * handle_iomsg()
 *	Handle the completion of an IOReq from the timer device by sending a 
 *	notification message to the client that requested the event.
 ****************************************************************************/

static int32 handle_iomsg(MSEventData *msevent, void *userContext)
{
	Err				err;
	Item			ioreq;
	IOReq * 		ior;
	TimerControl *	tctl = NULL;

	(void)userContext;	// no whining about unused vars please

	ioreq = (Item)msevent->msgPtr->msg_DataPtr;
	if ((ior = (IOReq *)LookupItem(ioreq)) == NULL) {
		err = BADITEM;
		DIAGNOSE(("LookupItem() failed for timer IOReq\n"));
		goto ERROR_EXIT;
	}

	tctl = (TimerControl *)ior->io_Info.ioi_User;
	tctl->flags &= ~TSIFLAG_IO_ACTIVE;
	
	if ((err = ior->io_Error) < 0) {	 // should never happen
		DIAGNOSE_SYSERR(err, ("Timer device returned bad status in ioreq\n"));
		goto ERROR_EXIT;
	}

	if (tctl->flags & TSFLAG_SIGNAL) {
		err = SendSignal(tctl->ownerTask, tctl->notify.signal);
	} else {
		if (tctl->flags & TSIFLAG_MSG_OUTSTANDING) {	// can't send if client hasn't
			err = 0;									// replied to outstanding msg.
		} else {
			tctl->flags |= TSIFLAG_MSG_OUTSTANDING;
			err = SendSmallMsg(tctl->notify.msgport, tctl->notifymsg, tctl->userdata1, tctl->userdata2);
		}
	}
	
	if (err < 0) {
		DIAGNOSE_SYSERR(err, ("Can't send timer event notification to task %08lx\n", tctl->ownerTask));
		goto ERROR_EXIT;
	}

	if (tctl->flags & TSFLAG_HEARTBEAT) {
		err = io_start(tctl, CONTINUE_HEARTBEAT);
	}

ERROR_EXIT:

	if (err < 0) {
		DIAGNOSE_SYSERR(err, ("TimerControl %08p released due to error\n", tctl));
		release_timer_control(tctl);
	}
	
	return 0;

}

/*****************************************************************************
 * handle_notifymsg_reply()
 *	Handle a client's reply to an event notification message we sent earlier.
 *
 *	A wee bit of ugliness:  the message data structure doesn't have an OwnerData
 *	field we can use to backlink the message to our owning TimerControl.  So
 *	we encoded the TimerControl pointer as an ascii string and stored it in the
 *	message item name field when we created the message.  Now we convert the 
 *	name back to a pointer, and because we're paranoid we do a couple sanity
 *	checks on it before trying to use it as a pointer.
 *
 *	Another piece of ugliness is that the client could have retrieved an
 *	event message, then called TimerCancel() before replying to it.  When 
 *	this happens, the cancel routine sets the RELEASE_ON_REPLY flag and we
 *	release the timer when we get the reply.
 ****************************************************************************/

static int32 handle_notifymsg_reply(MSEventData *msevent, void *userContext)
{
	TimerControl *	tctl;

	(void)userContext;	// no whining about unused vars please

	tctl = (TimerControl *)strtol(msevent->msgPtr->msg.n_Name, NULL, 0);

#if DEBUG
	if ((int32)tctl <= 0 || tctl->notifymsg != msevent->msgItem) {
		DIAGNOSE(("Invalid timercontrol backlink, a TimerControl has been orphaned\n"));
		return 0;
	}
#endif

	if (tctl->flags & TSIFLAG_MSG_OUTSTANDING) {
		tctl->flags &= ~TSIFLAG_MSG_OUTSTANDING;
		if ((tctl->flags & TSIFLAG_RELEASE_ON_REPLY)) {
			release_timer_control(tctl);
		}
	}
	
	return 0;
}

/*****************************************************************************
 * timer_services_thread()
 *	Init timer services, run the message dispatching until shutdown is 
 *	requested, then clean up and exit.  A shutdown request causes the 
 *	request msg handler to return the request msg item number to the DispatchMSEvents
 *	routine, which then returns the value to us, and since it isn't negative
 *	(ie, not an error) it means we're supposed to shut down.  We do all our
 *	cleanups, then reply to the shutdown request message, which provides a 
 *	synchronous shutdown from the requestor's point of view.  If we do get
 *	an error status from DispatchMSEvents we ignore it and try to keep running.
 ****************************************************************************/

static void timer_services_thread(void)
{
	Item	shutdownMsg;
		
	timer_services_init();

	do	{
		if ((shutdownMsg = DispatchMSEvents(mseHandle, NULL, 0)) < 0) {
			DIAGNOSE(("DispatchMSEvents() failed; timer_services_thread continues\n"));
		}
	} while (shutdownMsg <= 0);
	
	timer_services_cleanup();

	ReplyMsg(shutdownMsg, 0, 0, 0);
	exit(0);
}

/*****************************************************************************
 * TimerServicesStartup()
 *	Start the timer services subsystem.
 ****************************************************************************/

Err TimerServicesStartup(int32 delta_priority)
{
	Err	 err;

	if (FindMsgPort(TIMER_SERVICES_MSGPORT_NAME) > 0) {
		err = 1;			// already running, just
		goto ERROR_EXIT;	// return 1 to indicate that.
	}

	startup_parent_task = CURRENTTASK->t.n_Item;
	startup_signal = AllocSignal(0);
	if (startup_signal < 0) {
		err = startup_signal;
		DIAGNOSE_SYSERR(err, ("AllocSignal() failed\n"));
		goto ERROR_EXIT;
	}

	err = CreateThread("TimerServicesThread", 
						(uint8)(CURRENTTASK->t.n_Priority + delta_priority),
						timer_services_thread, 
						TIMER_THREAD_STACKSIZE);
						
	if (err < 0) {
		DIAGNOSE_SYSERR(err, ("CreateThread() failed\n"));
		goto ERROR_EXIT;
	}

	WaitSignal(startup_signal);

	if ((err = startup_status) < 0) {
		DIAGNOSE_SYSERR(err, ("Can't start timer services thread\n"));
		goto ERROR_EXIT;
	}
	
	if ((err = TimerServicesOpen()) < 0) {
		DIAGNOSE_SYSERR(err, ("TimerServicesOpen() failed\n"));
		goto ERROR_EXIT;
	}

	err = 0;

ERROR_EXIT:

	if (startup_signal > 0) {
		FreeSignal(startup_signal);
		startup_signal = 0;
	}

	return err;
}

/*****************************************************************************
 * TimerServicesShutdown()
 *	Shut down the timer services subsystem.
 ****************************************************************************/

void TimerServicesShutdown(void)
{
	timer_services_request_(0, 0, 0, 0, 0, TSREQUEST_MSGID_SHUTDOWN, 0);
}

/*****************************************************************************
 * TimerServicesVerify()
 *	Verify that all clients owning timers are still alive.
 ****************************************************************************/

Err TimerServicesVerify(void)
{
	return timer_services_request_(0, 0, 0, 0, 0, TSREQUEST_MSGID_VERIFY, 0);
}

