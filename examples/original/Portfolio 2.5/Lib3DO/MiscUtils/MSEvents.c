
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: MSEvents.c,v 1.6 1994/11/17 21:27:31 vertex Exp $
**
**  Lib3DO routines to wait for and handle multiple messages and
**  signals concurrently.
**
**  Based on data passed to SetupMSEvents(), we build an internal
**  control structure that helps us wait for messages on any of the created
**  events.  Ideally, the OS would allow us to create an Item that points
**  to the internal data we allocate, since we don't let our clients see
**  the internals of this structure anyway.  Since we can't do that, we
**  do the next-best thing: allocate the structure, but on returning the
**  pointer to the client, we cast it to an int32 type, so it can more
**  or less be treated as an Item by the client.  The main advantage to
**  this (beside data hiding) is that it lets us return proper negative
**  error codes if anything goes wrong, instead of just returning a NULL
**  pointer.  Casting a pointer to an int is a bit dirty, but unless the
**  3DO platform ever gets into memory addresses so high that a negative
**  number could be a valid pointer, we're okay.
**
******************************************************************************/

#include "debug3do.h"
#include "mem.h"
#include "msgutils.h"
#include "operror.h"
#include "types.h"

typedef struct MSControl
{
  int32 activeSignals;
  int32 pendingEnabledSignals;
  int32 pendingDisabledSignals;
  int32 allocationMask;
  int32 numEvents;
  MSEventData *events;
} MSControl;

/*****************************************************************************
 * CleanupMSEvents()
 *	Delete resources acquired during setup process.
 *	This is also called from the setup routine to free the dregs of a
 *	failed creation.  (IE, it must cope with partially-allocated
 *resources.)
 ****************************************************************************/

void
CleanupMSEvents (MSEventHandle mseHandle)
{
  MSControl *msctl;
  MSEventData *event;
  int i;

  if (mseHandle <= 0)
    {
      return;
    }
  else
    {
      msctl = (MSControl *)mseHandle;
    }

  for (i = 0, event = msctl->events; i < msctl->numEvents; ++i, ++event)
    {
      if (msctl->allocationMask & (1L << i))
        {
          if (event->port > 0)
            {
              DeleteMsgPort (event->port);
              event->port = 0;
              event->signal = 0;
            }
          else if (event->signal > 0)
            {
              FreeSignal (event->signal);
              event->signal = 0;
            }
        }
    }

  FreeMem (msctl, sizeof (*msctl));
}

/*****************************************************************************
 * SetupMSEvents()
 *	Setup one or more events, based on the event data array passed
 *	in by the client.  Return a pseudo-item that the client can pass back
 *	to us for waiting and dispatching the events.  Returns negative on
 *error.
 ****************************************************************************/

MSEventHandle
SetupMSEvents (MSEventData *events, int32 numEvents, int32 reserved)
{
  int32 rv;
  int32 i;
  MSControl *msctl;
  MsgPort *mp;

  (void)reserved; /* no whining about unused vars please */

  if ((msctl = (MSControl *)AllocMem (sizeof (*msctl),
                                      MEMTYPE_ANY | MEMTYPE_FILL | 0))
      == NULL)
    {
      rv = NOMEM;
      DIAGNOSE_SYSERR (rv,
                       ("Can't create internal event control structure\n"));
      goto ERROR_EXIT;
    }
  msctl->numEvents = numEvents;
  msctl->events = events;

  for (i = 0; i < numEvents; ++i, ++events)
    {
      events->backLink = (MSEventHandle)msctl;
      if (events->name == NULL)
        {
          if (events->signal <= 0)
            {
              if ((events->signal = AllocSignal (0)) <= 0)
                {
                  rv = events->signal;
                  DIAGNOSE_SYSERR (rv, ("AllocSignal() failed\n"));
                  goto ERROR_EXIT;
                }
              msctl->allocationMask |= (1L << i);
            }
          events->port = 0;
        }
      else
        {
          if (events->port <= 0)
            {
              if ((events->port = CreateMsgPort (events->name, 0, 0)) < 0)
                {
                  rv = events->port;
                  DIAGNOSE_SYSERR (
                      rv, ("CreateMsgPort(%s) failed\n", events->name));
                  goto ERROR_EXIT;
                }
              msctl->allocationMask |= (1L << i);
            }
          if ((mp = (MsgPort *)LookupItem (events->port)) == NULL)
            {
              rv = BADITEM;
              DIAGNOSE_SYSERR (
                  rv, ("LookupItem(msgport %s) failed\n", events->name));
              goto ERROR_EXIT;
            }
          events->signal = mp->mp_Signal;
        }
      msctl->activeSignals |= events->signal;
    }

  rv = (int32)msctl;

ERROR_EXIT:

  if (rv < 0)
    {
      CleanupMSEvents ((MSEventHandle)msctl);
    }

  return rv;
}

/*****************************************************************************
 * EnableMSEvent()
 *	Re-enable an event that was previously disabled.  If the signal for the
 *	event was pending when it was disabled, send the signal back to
 *ourselves so that we process it on the next dispatch loop iteration.
 ****************************************************************************/

void
EnableMSEvent (MSEventData *event, int32 reserved)
{
  MSControl *msctl = (MSControl *)event->backLink;

  (void)reserved; /* no whining about unused vars please */

  msctl->activeSignals |= event->signal;

  if (msctl->pendingDisabledSignals & event->signal)
    {
      msctl->pendingDisabledSignals &= ~event->signal;
      SendSignal (CURRENTTASK->t.n_Item, event->signal);
    }
}

/*****************************************************************************
 * DisableMSEvent()
 *	Disable an event so that its handler doesn't get dispatched until it is
 *	re-enabled.  If the event is pending right now, remember that fact so
 *	that we can re-signal the event when it becomes re-enabled.
 ****************************************************************************/

void
DisableMSEvent (MSEventData *event, int32 reserved)
{
  MSControl *msctl = (MSControl *)event->backLink;

  (void)reserved; /* no whining about unused vars please */

  msctl->activeSignals &= ~event->signal;

  if (msctl->pendingEnabledSignals & event->signal)
    {
      msctl->pendingEnabledSignals &= ~event->signal;
      msctl->pendingDisabledSignals |= event->signal;
    }
}

/*****************************************************************************
 * DispatchMSEvents()
 *	Wait for events and dispatch them to their handlers.
 ****************************************************************************/

int32
DispatchMSEvents (MSEventHandle mseHandle, void *userContext, int32 reserved)
{
  int32 rv;
  int32 i;
  int32 theSignals;
  MSEventData *event;
  MSControl *msctl = (MSControl *)mseHandle;

  (void)reserved; /* no whining about unused vars please */

  for (;;)
    {
      for (i = 0; msctl->pendingEnabledSignals && i < msctl->numEvents; ++i)
        {
          event = &msctl->events[i];
          theSignals = msctl->pendingEnabledSignals & event->signal;
          if (theSignals != 0)
            {
              msctl->pendingEnabledSignals &= ~event->signal;
              if (event->port == 0)
                {
                  event->msgItem = theSignals;
                  if ((rv = event->handler (event, userContext)) != 0)
                    {
                      return rv;
                    }
                }
              else
                while ((event->msgItem = GetMsg (event->port)) > 0)
                  {
                    if ((event->msgPtr
                         = (Message *)LookupItem (event->msgItem))
                        == NULL)
                      {
                        DIAGNOSE (("LookupItem() failed for message item\n"));
                        continue;
                      }
                    if (event->msgPtr->msg.n_Flags & MESSAGE_SMALL)
                      {
                        event->msgValues
                            = (MsgValueTypes *)&event->msgPtr->msg_DataPtr;
                      }
                    else
                      {
                        event->msgValues
                            = (MsgValueTypes *)event->msgPtr->msg_DataPtr;
                      }
                    if ((rv = event->handler (event, userContext)) != 0)
                      {
                        return rv;
                      }
                  }
            }
        }
      msctl->pendingEnabledSignals = WaitSignal (msctl->activeSignals);
    }
}
