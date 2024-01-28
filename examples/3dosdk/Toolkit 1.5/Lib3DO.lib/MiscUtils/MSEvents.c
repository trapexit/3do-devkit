/*****************************************************************************
 *	File:			MSEvent.c
 *
 *	Contains:		Routines to wait for and handle multiple messages
 *and signals concurrently.
 *
 *	Copyright (c) 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	07/29/94  Ian	Tweaked so that an event can include multiple signals
 *					and when the handler is dispatched the msgItem
 *field holds a mask of which signal(s) triggered the event. If this feature is
 *used, a signal handler must handle all signals found in the msgItem field on
 *a single call. 06/13/94  Ian	Changed setup and cleanup routines so that
 *clients can pre-allocate their own msgports (indicated by a non-zero port
 *item in the data array) and we just go with it (and, of course, we don't try
 *to de-alloc it at cleanup time). 06/08/94  Ian	Fixed a bug in message
 *dispatching: it now loops on a message port until all messages are cleared
 *from it before moving on to the next signal/port.
 *	05/29/94  Ian	Complete rewrite.
 *	11/05/93  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	Based on data passed to SetupMSEvents(), we build an internal
 *	control structure that helps us wait for messages on any of the created
 *	events.  Ideally, the OS would allow us to create an Item that points
 *	to the internal data we allocate, since we don't let our clients see
 *	the internals of this structure anyway.  Since we can't do that, we
 *	do the next-best thing: allocate the structure, but on returning the
 *	pointer to the client, we cast it to an int32 type, so it can more
 *	or less be treated as an Item by the client.  The main advantage to
 *	this (beside data hiding) is that it lets us return proper negative
 *	error codes if anything goes wrong, instead of just returning a NULL
 *	pointer.  Casting a pointer to an int is a bit dirty, but unless the
 *	3DO platform ever gets into memory addresses so high that a negative
 *	number could be a valid pointer, we're okay.
 *
 ****************************************************************************/

#include "Debug3DO.h"
#include "MsgUtils.h"
#include "UMemory.h"
#include "operror.h"

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

  Free (msctl);
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

  (void)reserved; // no whining about unused vars please

  if ((msctl = (MSControl *)Malloc (sizeof (*msctl),
                                    MEMTYPE_DRAM | MEMTYPE_FILL | 0))
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
      if (events->name == NULL)
        {
          if (events->signal <= 0)
            {
              if ((events->signal = AllocSignal (0)) < 0)
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

  (void)reserved; // no whining about unused vars please

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

  (void)reserved; // no whining about unused vars please

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

  (void)reserved; // no whining about unused vars please

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
