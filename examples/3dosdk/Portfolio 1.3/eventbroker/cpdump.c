/*****

$Id:

$Log:
*****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  cpdump.c - a simple program which talks to the Event Broker and
  prints out a summary of the Control Port hashup.
*/

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "event.h"
#include "io.h"
#include "item.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "msgport.h"
#include "nodes.h"
#include "operror.h"
#include "types.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

#include "strings.h"

#define messageSize 2048
#define numGenerics 16

const char *generics[numGenerics] = {
  "control pad",      "mouse",    "gun",      "glasses controller",
  "audio controller", "keyboard", "lightgun", "joy stick",
  "IR controller",    "type 9",   "type 10",  "type 11",
  "type 12",          "type 13",  "type 14",  "type 15",
};

const char *lcat[4] = {
  "hear-no-evil",
  "focus only",
  "observer",
  "focus for UI, observe others",
};

int
main (int argc, char **argv)
{
  Item msgPortItem;
  Item brokerPortItem;
  Item msgItem, eventItem;
  Item focusItem;
  int32 sent;
  Message *event;
  MsgPort *msgPort, *listenerPort;
  PodDescriptionList *pdl;
  ListenerList *ll;
  int i, j;

  EventBrokerHeader queryHeader, *msgHeader;

  TagArg msgTags[3];

  brokerPortItem
      = FindNamedItem (MKNODEID (KERNELNODE, MSGPORTNODE), EventPortName);

  if (brokerPortItem < 0)
    {
      printf ("Can't find Event Broker port: ");
      PrintfSysErr (brokerPortItem);
      return 0;
    }

  msgPortItem = CreateMsgPort (argv[0], 0, 0);

  if (msgPortItem < 0)
    {
      printf ("Cannot create event-listener port: ");
      PrintfSysErr (msgPortItem);
      return 0;
    }

  msgPort = (MsgPort *)LookupItem (msgPortItem);

  msgTags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
  msgTags[0].ta_Arg = (void *)msgPortItem;
  msgTags[1].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
  msgTags[1].ta_Arg = (void *)messageSize;
  msgTags[2].ta_Tag = TAG_END;

  msgItem = CreateItem (MKNODEID (KERNELNODE, MESSAGENODE), msgTags);

  if (msgItem < 0)
    {
      printf ("Cannot create message: ");
      PrintfSysErr (msgItem);
      return 0;
    }

  queryHeader.ebh_Flavor = EB_DescribePods;

  sent = SendMsg (brokerPortItem, msgItem, &queryHeader, sizeof queryHeader);

  if (sent < 0)
    {
      printf ("Error sending describe-pods message: ");
      PrintfSysErr (sent);
      return 0;
    }

  eventItem = WaitPort (msgPortItem, msgItem);

  if (eventItem < 0)
    {
      printf ("Error getting describe-pods reply: ");
      PrintfSysErr (eventItem);
      return 0;
    }

  event = (Message *)LookupItem (eventItem);
  msgHeader = (EventBrokerHeader *)event->msg_DataPtr;

  if ((int32)event->msg_Result < 0)
    {
      printf ("cpdump says broker refused describe-pods request: ");
      PrintfSysErr ((int32)event->msg_Result);
      return 0;
    }

  pdl = (PodDescriptionList *)msgHeader;

  printf ("There are %d pods on the chain:\n\n", pdl->pdl_PodCount);

  for (i = 0; i < pdl->pdl_PodCount; i++)
    {
      printf ("Position %d number %d type 0x%x ", i,
              pdl->pdl_Pod[i].pod_Number, pdl->pdl_Pod[i].pod_Type);
      printf ("flags 0x%x bits-in %d bits-out %d", pdl->pdl_Pod[i].pod_Flags,
              pdl->pdl_Pod[i].pod_BitsIn, pdl->pdl_Pod[i].pod_BitsOut);
      for (j = 0; j < numGenerics; j++)
        {
          if (pdl->pdl_Pod[i].pod_GenericNumber[j])
            {
              printf (" generic %s %d", generics[j],
                      pdl->pdl_Pod[i].pod_GenericNumber[j]);
            }
        }
      if (pdl->pdl_Pod[i].pod_LockHolder)
        {
          printf (" locked by 0x%x", pdl->pdl_Pod[i].pod_LockHolder);
        }
      printf ("\n");
    }

  queryHeader.ebh_Flavor = EB_GetFocus;

  sent = SendMsg (brokerPortItem, msgItem, &queryHeader, sizeof queryHeader);

  if (sent < 0)
    {
      printf ("Error sending get-focus message: ");
      PrintfSysErr (sent);
      return 0;
    }

  eventItem = WaitPort (msgPortItem, msgItem);

  if (eventItem < 0)
    {
      printf ("Error getting get-focus reply: ");
      PrintfSysErr (eventItem);
      return 0;
    }

  event = (Message *)LookupItem (eventItem);
  msgHeader = (EventBrokerHeader *)event->msg_DataPtr;

  if ((int32)event->msg_Result < 0)
    {
      printf ("cpdump says broker refused get-focus request: ");
      PrintfSysErr ((int32)event->msg_Result);
      return 0;
    }

  focusItem = (Item)event->msg_Result;

  queryHeader.ebh_Flavor = EB_GetListeners;

  sent = SendMsg (brokerPortItem, msgItem, &queryHeader, sizeof queryHeader);

  if (sent < 0)
    {
      printf ("Error sending get-listeners message: ");
      PrintfSysErr (sent);
      return 0;
    }

  eventItem = WaitPort (msgPortItem, msgItem);

  if (eventItem < 0)
    {
      printf ("Error getting get-listeners reply: ");
      PrintfSysErr (eventItem);
      return 0;
    }

  event = (Message *)LookupItem (eventItem);
  msgHeader = (EventBrokerHeader *)event->msg_DataPtr;

  if ((int32)event->msg_Result < 0)
    {
      printf ("cpdump says broker refused get-listeners request: ");
      PrintfSysErr ((int32)event->msg_Result);
      return 0;
    }

  ll = (ListenerList *)msgHeader;

  printf ("\nThere are %d listeners on the chain:\n\n", ll->ll_Count);

  for (i = 0; i < ll->ll_Count; i++)
    {
      printf ("Port 0x%x type %s", ll->ll_Listener[i].li_PortItem,
              lcat[ll->ll_Listener[i].li_Category]);
      listenerPort = (MsgPort *)CheckItem (ll->ll_Listener[i].li_PortItem,
                                           KERNELNODE, MSGPORTNODE);
      if (ll->ll_Listener[i].li_PortItem == focusItem)
        {
          printf (" HAS FOCUS");
        }
      if (listenerPort)
        {
          if (listenerPort->mp.n_Name)
            {
              printf (" name %s\n", listenerPort->mp.n_Name);
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
