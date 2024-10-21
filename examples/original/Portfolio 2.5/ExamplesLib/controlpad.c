
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: controlpad.c,v 1.16 1995/01/16 19:48:35 vertex Exp $
**
**  Utility functions to query control pads
**
******************************************************************************/

#include "controlpad.h"
#include "debug3do.h"
#include "event.h"
#include "mem.h"
#include "types.h"

static Item gCPadErrorItem = -1;
static bool gBrokerInitialized = false;
static uint32 *gPreviousButtons = NULL;
static int32 gNPads = 0;

/*
 Error messages associated with the specified error codes
*/
static char *gCPadErrors[] = {
  "no error",

  /* INITCONTROLPAD_ERR */
  "InitControlPad has not yet been called",

  /* PADNUMBER_ERR */
  "Queried pad has not been initialized",

  /* INITEDUTIL_ERR */
  "Event broker has already been initialized",

  /* ALLOCMEM_ERR */
  "Unable to allocate memory",

  /* ALREADYALLOC_ERR */
  "Item or memory has already been allocated",

};
#define MAX_ERROR_LEN 45

/*
 Tag Args for error text item to return opera style errors
*/
static TagArg gCPadErrorTags[]
    = { TAG_ITEM_NAME,
        (void *)"ControlPad",
        ERRTEXT_TAG_OBJID,
        (void *)((ER_TASK << ERR_IDSIZE) | ER_CPAD),
        ERRTEXT_TAG_MAXERR,
        (void *)(sizeof (gCPadErrors) / sizeof (char *)),
        ERRTEXT_TAG_TABLE,
        (void *)gCPadErrors,
        ERRTEXT_TAG_MAXSTR,
        (void *)MAX_ERROR_LEN,
        TAG_END,
        0 };

static int32
ControlPadSanityCheck (int32 *pWhichPad)
{

  int32 status = 0;

  /*
          check to make sure we've intialized the event broker.  Print out
          the error, because if we haven't called InitControlPad, then we
     haven't created the error text item, which would otherwise contain the
     error text.  Sigh.
  */

  if (!gBrokerInitialized)
    {
      status = INITCONTROLPAD_ERR;
      ERR (("%s:: InitControlPad() has not yet been called\n", __FILE__));
      goto CLEANUP;
    }

  /*
                  Check that we aren't querying a pad for which we haven't
     initialized.
  */

  if (*pWhichPad > gNPads)
    {
      status = PADNUMBER_ERR;
      CPERR (status);
      goto CLEANUP;
    }

  /*
          stupid sanity check to make sure that we are at least checking pad 1,
     the first pad
  */

  if (*pWhichPad == 0)
    {
      *pWhichPad += 1;
    }

CLEANUP:
  return status;
}

/**
|||	AUTODOC PUBLIC examples/initcontrolpad
|||	InitControlPad - Connects a task to the event broker for control pad
|||	    interaction.
|||
|||	  Synopsis
|||
|||	    int32 InitControlPad (int32 nPads)
|||
|||	  Description
|||
|||	    Initializes global static resources needed for interfacing with the
event
|||	    broker. Uses some high-level convenience functions rather than
opting for
|||	    the low-level method of communicating with the event broker. See
other
|||	    source files for examples of that level of programming.
|||
|||	    The function creates an array in which to keep the past states of
all the
|||	    control pads.
|||
|||	  Arguments
|||
|||	    nPads                        Number of control pads to monitor.
|||
|||	  Return Value
|||
|||	    Returns 0 if successful or a negative error code if an error
occurs.
|||
|||	  Caveats
|||
|||	    This function is neither multi-thread or multi-task capable or
safe. You
|||	    should call this function ONCE AND ONLY ONCE as an initialization
routine,
|||	    before calling DoControlPad(). Doing otherwise could cause
undesirable
|||	    effects.
|||
|||	    The main task needs to call KillControlPad() prior to exiting.
|||
|||	  Associated Files
|||
|||	    controlpad.c, controlpad.h
|||
|||	  Location
|||
|||	    examples/ExamplesLib
|||
|||	  See Also
|||
|||	    DoControlPad(), KillControlPad()
|||
**/
int32
InitControlPad (int32 nPads)
{

  int32 status = 0;

  /*
     Create an error text item to report opera style errors
  */
  if (gCPadErrorItem < 0)
    {
      status
          = CreateItem (MKNODEID (KERNELNODE, ERRORTEXTNODE), gCPadErrorTags);
      if (status < 0)
        {
          CPERR (status);
          goto CLEANUP;
        }

      gCPadErrorItem = status;
    }
  else
    {
      status = ALREADYALLOC_ERR;
      CPERR (status);
      goto CLEANUP;
    }

  /*
     call a convenience routine to configure an interface to the event broker.
     Register to monitor nPads, no mice, and to receive all event
     messages (focus-independent)
  */
  if (!gBrokerInitialized)
    {
      status = InitEventUtility (nPads, 0, LC_Observer);
      if (status < 0)
        {
          CPERR (status);
          goto CLEANUP;
        }
    }
  else
    {
      status = INITEDUTIL_ERR;
      CPERR (status);
      goto CLEANUP;
    }

  gBrokerInitialized = true;
  gNPads = nPads;

  /*
          allocate an array of uint32's to hold the previous states of the
     control pads. This is used to "de-bounce" the control keys
  */

  if (!gPreviousButtons)
    {
      gPreviousButtons = (uint32 *)AllocMem (
          nPads * sizeof (uint32), MEMTYPE_TRACKSIZE | MEMTYPE_FILL | 0x00);
      if (gPreviousButtons == NULL)
        {
          status = ALLOCMEM_ERR;
          CPERR (status);
          goto CLEANUP;
        }
    }
  else
    {
      status = ALREADYALLOC_ERR;
      CPERR (status);
      goto CLEANUP;
    }

CLEANUP:
  return status;
}

/**
|||	AUTODOC PUBLIC examples/docontrolpad
|||	DoControlPad - Queries the specified control pad and de-bounces
specified
|||	    key presses.
|||
|||	  Synopsis
|||
|||	    int32 DoControlPad (int32 whichPad, uint32 *pButton,int32
continuousBits)
|||
|||	  Description
|||
|||	    Queries the state of the specified control pad, and de-bounces the
keys,
|||	    based upon which keys the user is interested in receiving
continuous data
|||	    on button presses, or discrete data.
|||
|||	    This function fills pButton with the current state of the control
pad.
|||	    That is, it returns values defined in event.h:
|||
|||	    ControlUp ControlDownControlLeft ControlRightControlA ControlB
ControlC
|||	    ControlStart ControlX ControlLeftShift ControlRightShift
|||
|||	  Arguments
|||
|||	    whichPad                     Index of the control pad to query
|||
|||	    pButton                      Pointer to a uint32, in which the
state of
|||	                                 the requested control pad is returned.
|||
|||	    continuousBits               Events from event.h for which the user
would
|||	                                 like to get a continuous stream of
event
|||	                                 states.
|||
|||	  Return Value
|||
|||	    Returns 0 if successful or a negative error code if an error
occurs.
|||
|||	  Caveats
|||
|||	    This function is neither multi-thread or multi-task capable or
safe. You
|||	    should call this from the task or thread which has already called
|||	    InitControlPad(). This is because of system restrictions on item
ownership
|||	    and using items. InitControlPad() and InitEventUtility() create
message
|||	    ports and message items, which only the task which has created them
can
|||	    use. Hence, DoControlPad() can only be called by that same task.
Other
|||	    tasks or threads which attempt this, will get errors due to not
owning the
|||	    appropriate message ports and messages.
|||
|||	    To make this multi-thread capable, you would need to spawn off a
thread
|||	    that communicates directly with the event broker, and with all the
other
|||	    tasks that will consume event information.
|||
|||	  Associated Files
|||
|||	    controlpad.c, controlpad.h
|||
|||	  Location
|||
|||	    examples/ExamplesLib
|||
|||	  See Also
|||
|||	    InitControlPad(), KillControlPad()
|||
**/
int32
DoControlPad (int32 whichPad, uint32 *pButton, int32 continuousBits)
{

  ControlPadEventData currentPad;
  uint32 button = 0;
  int32 status = 0;

  *pButton = button;

  status = ControlPadSanityCheck (&whichPad);
  if (status < 0)
    {
      CPERR (status)
      goto CLEANUP;
    }

  /*
          actually query the control pad, taking a snapshot of its current
     state. We do NOT wait for an event to occur, we come back immediately
  */

  status = GetControlPad (whichPad, false, &currentPad);
  if (status < 0)
    {
      CPERR (status);
      goto CLEANUP;
    }

  /*
          do some logic to separate discrete and continuous events from the
     snapshot remember what the control pad state was the last time, and then
     act accordingly
  */

  button = currentPad.cped_ButtonBits;

  *pButton = (button & (button ^ gPreviousButtons[whichPad - 1]))
             | (button & continuousBits);

  gPreviousButtons[whichPad - 1] = button; /* the 1st pad is pad 1, not 0 */

CLEANUP:

  return status;
}

/**
|||	AUTODOC PUBLIC examples/returnpreviouscontrolpad
|||	ReturnPreviousControlPad - Returns the state of the control pad as it
is
|||	    remembered.
|||
|||	  Synopsis
|||
|||	    int32 ReturnPreviousControlPad (int32 whichPad)
|||
|||	  Description
|||
|||	    Returns the state of the specified control pad as it is remembered
from
|||	    the last time DoControlPad() was called and a new state for the
control
|||	    pad existed.
|||
|||	  Arguments
|||
|||	    whichPad                     Index of the control pad to query.
|||
|||	  Return Value
|||
|||	    Returns 0 if successful or a negative error code if an error
occurs.
|||
|||	  Caveats
|||
|||	    This function is neither multi-thread or multi-task capable or
safe.
|||
|||	    Because of system restrictions on item ownership and using items,
you
|||	    should call this function from the task or thread which called
|||	    InitControlPad().
|||
|||	    InitControlPad() and InitEventUtility() create message ports and
message
|||	    items which only the task which has created them can use. Hence,
|||	    DoControlPad() can only be called by that same task. If other tasks
or
|||	    threads attempt this errors result because those tasks or threads
do not
|||	    own the appropriate message ports and messages.
|||
|||	    To make this function multi-thread capable, you would need to spawn
a
|||	    thread that communicates directly with the event broker, and with
all the
|||	    other tasks that will consume event information.
|||
|||	  Associated Files
|||
|||	    controlpad.c, controlpad.h
|||
|||	  Location
|||
|||	    examples/Exampleslib
|||
|||	  See Also
|||
|||	    InitControlPad(), KillControlPad()
|||
**/
int32
ReturnPreviousControlPad (int32 whichPad, uint32 *pButton)
{

  int32 status = 0;

  status = ControlPadSanityCheck (&whichPad);
  if (status < 0)
    {
      CPERR (status)
      goto CLEANUP;
    }

  *pButton = gPreviousButtons[whichPad - 1];

CLEANUP:
  return status;
}

/**
|||	AUTODOC PUBLIC examples/killcontrolpad
|||	KillControlPad - Disconnects the task from the event broker and frees
|||	    resources.
|||
|||	  Synopsis
|||
|||	    int32 KillControlPad (void)
|||
|||	  Description
|||
|||	    Deletes all resources that were created for interacting with the
event
|||	    broker. These resources include the array of previous button
states, and
|||	    the message ports, messages, etc., created by InitEventUtility().
|||
|||	  Return Value
|||
|||	    Returns 0 if successful or a negative error code if an error
occurs.
|||
|||	  Caveats
|||
|||	    This function MUST be called upon the main tasks completion. It
should be
|||	    called ONCE and ONLY ONCE per initialization. It must be called by
the
|||	    same task that called InitControlPad().
|||
|||	  Associated Files
|||
|||	    controlpad.c, controlpad.h
|||
|||	  Location
|||
|||	    examples/ExampleLib
|||
|||	  See Also
|||
|||	    InitControlPad(), KillControlPad()
|||
**/
int32
KillControlPad (void)
{

  int32 status = 0;

  DeleteItem (gCPadErrorItem);
  gCPadErrorItem = -1;

  FreeMem (gPreviousButtons, -1);
  gPreviousButtons = NULL;

  if (gBrokerInitialized)
    {
      status = KillEventUtility ();
      if (status < 0)
        {
          CPERR (status);
          return status;
        }

      gBrokerInitialized = false;
    }

  return status;
}
