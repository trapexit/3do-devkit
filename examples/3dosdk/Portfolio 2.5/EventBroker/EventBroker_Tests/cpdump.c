
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: cpdump.c,v 1.11 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/cpdump
|||	cpdump - Queries the event broker and prints out a summary of what's
|||	    connected to the control port.
|||
|||	  Synopsis
|||
|||	    cpdump
|||
|||	  Description
|||
|||	    Sends three messages to the event broker (EB_DescribePods,
EB_GetFocus,
|||	    and EB_GetListeners) and prints out the values that the event
broker
|||	    returns for these messages.
|||
|||	  Associated Files
|||
|||	    cpdump.c
|||
|||	  Location
|||
|||	    $c/cpdump
|||
|||	    examples/EventBroker
|||
**/

#include "event.h"
#include "item.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "msgport.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

/*****************************************************************************/

static const char *generics[] = {
  "control pad",      "mouse",    "gun",       "glasses controller",
  "audio controller", "keyboard", "light gun", "joystick",
  "IR controller",    "type 9",   "type 10",   "type 11",
  "type 12",          "type 13",  "type 14",   "type 15",
};
#define NUM_GENERICS (sizeof (generics) / sizeof (char *))

static const char *lcat[4] = {
  "hear-no-evil",
  "focus only",
  "observer",
  "focus for UI, observe others",
};

/*****************************************************************************/

static Err
SendEBMessage (Item ebPortItem, Item msgItem, enum EventBrokerFlavor flavor)
{
  EventBrokerHeader ebHdr;
  Err err;
  Message *msg;

  ebHdr.ebh_Flavor = flavor;

  err = SendMsg (ebPortItem, msgItem, &ebHdr, sizeof (EventBrokerHeader));
  if (err >= 0)
    {
      msg = (Message *)LookupItem (msgItem);

      err = WaitPort (msg->msg_ReplyPort, msgItem);
      if (err >= 0)
        {
          if ((Err)msg->msg_Result < 0)
            {
              printf ("Event broker failed: ");
              PrintfSysErr (msg->msg_Result);
            }

          return (Err)msg->msg_Result;
        }
      else
        {
          printf ("Error waiting for reply: ");
          PrintfSysErr (err);
        }
    }
  else
    {
      printf ("Error sending message: ");
      PrintfSysErr (err);
    }

  return err;
}

/*****************************************************************************/

int
main (int32 argc, char **argv)
{
  Item ebPortItem;
  Item msgPortItem;
  Item msgItem;
  Item focusItem;
  Err err;
  MsgPort *listenerPort;
  PodDescriptionList *pdl;
  ListenerList *ll;
  int i, j;
  Message *msg;

  ebPortItem = FindMsgPort (EventPortName);
  if (ebPortItem >= 0)
    {
      msgPortItem = CreateMsgPort (NULL, 0, 0);
      if (msgPortItem >= 0)
        {
          msgItem = CreateBufferedMsg (NULL, 0, msgPortItem, 2048);
          if (msgItem >= 0)
            {
              err = SendEBMessage (ebPortItem, msgItem, EB_DescribePods);
              if (err >= 0)
                {
                  msg = (Message *)LookupItem (msgItem);
                  pdl = (PodDescriptionList *)msg->msg_DataPtr;

                  printf ("There are %d pods on the chain:\n\n",
                          pdl->pdl_PodCount);

                  for (i = 0; i < pdl->pdl_PodCount; i++)
                    {
                      printf ("Position %d number %d type 0x%x ", i,
                              pdl->pdl_Pod[i].pod_Number,
                              pdl->pdl_Pod[i].pod_Type);
                      printf ("flags 0x%x bits-in %d bits-out %d",
                              pdl->pdl_Pod[i].pod_Flags,
                              pdl->pdl_Pod[i].pod_BitsIn,
                              pdl->pdl_Pod[i].pod_BitsOut);

                      for (j = 0; j < NUM_GENERICS; j++)
                        {
                          if (pdl->pdl_Pod[i].pod_GenericNumber[j])
                            {
                              printf (" generic %s %d", generics[j],
                                      pdl->pdl_Pod[i].pod_GenericNumber[j]);
                            }
                        }

                      if (pdl->pdl_Pod[i].pod_LockHolder)
                        {
                          printf (" locked by 0x%x",
                                  pdl->pdl_Pod[i].pod_LockHolder);
                        }
                      printf ("\n");
                    }
                }

              err = SendEBMessage (ebPortItem, msgItem, EB_GetFocus);
              if (err >= 0)
                {
                  msg = (Message *)LookupItem (msgItem);
                  focusItem = (Item)msg->msg_Result;

                  err = SendEBMessage (ebPortItem, msgItem, EB_GetListeners);
                  if (err >= 0)
                    {
                      msg = (Message *)LookupItem (msgItem);
                      ll = (ListenerList *)msg->msg_DataPtr;

                      printf ("\nThere are %d listeners on the chain:\n\n",
                              ll->ll_Count);

                      for (i = 0; i < ll->ll_Count; i++)
                        {
                          printf ("Port 0x%x type %s",
                                  ll->ll_Listener[i].li_PortItem,
                                  lcat[ll->ll_Listener[i].li_Category]);

                          listenerPort = (MsgPort *)CheckItem (
                              ll->ll_Listener[i].li_PortItem, KERNELNODE,
                              MSGPORTNODE);
                          if (ll->ll_Listener[i].li_PortItem == focusItem)
                            printf (" HAS FOCUS");

                          if (listenerPort)
                            {
                              if (listenerPort->mp.n_Name)
                                {
                                  printf (" name %s\n",
                                          listenerPort->mp.n_Name);
                                }
                              else
                                {
                                  printf (" unnamed\n");
                                }
                            }
                          else
                            {
                              printf (" can't find port to report name!\n");
                            }
                        }
                    }
                }
              DeleteMsg (msgItem);
            }
          else
            {
              printf ("Cannot create message: ");
              PrintfSysErr (msgItem);
            }
          DeleteMsgPort (msgPortItem);
        }
      else
        {
          printf ("Cannot create event-listener port: ");
          PrintfSysErr (msgPortItem);
        }
    }
  else
    {
      printf ("Can't find Event Broker port: ");
      PrintfSysErr (ebPortItem);
    }

  return 0;
}
