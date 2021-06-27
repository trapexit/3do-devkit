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
  focus.c - a simple program which talks to the Event Broker and
  switches the focus to a different listener
*/

#include "types.h"
#include "item.h"
#include "kernel.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "msgport.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "operror.h"
#include "event.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

#include "strings.h"

#define messageSize 2048
#define numGenerics 6


int main(int argc, char **argv)
{
  Item msgPortItem;
  Item brokerPortItem;
  Item msgItem, eventItem;
  Item focusItem;
  Item switchItem;
  int32 sent;
  Message *event;
  MsgPort *msgPort, *listenerPort;
  ListenerList *ll;
  char focusPort[12];
  SetFocus focusRequest;
  int i;

  EventBrokerHeader queryHeader, *msgHeader;

  TagArg msgTags[3];

  if (argc > 2) {
    printf("Usage: focus [listener]\n");
    return -1;
  }

  brokerPortItem = FindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),
				 EventPortName);

  if (brokerPortItem < 0) {
    printf("Can't find Event Broker port: ");
    PrintfSysErr(brokerPortItem);
    return 0;
  }
  
  msgPortItem = CreateMsgPort(argv[0], 0, 0);

  if (msgPortItem < 0) {
    printf("Cannot create event-listener port: ");
    PrintfSysErr(msgPortItem);
    return 0;
  }

  msgPort = (MsgPort *) LookupItem(msgPortItem);

  msgTags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
  msgTags[0].ta_Arg = (void *) msgPortItem;
  msgTags[1].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
  msgTags[1].ta_Arg = (void *) messageSize;
  msgTags[2].ta_Tag = TAG_END;

  msgItem = CreateItem(MKNODEID(KERNELNODE,MESSAGENODE), msgTags);
  
  if (msgItem < 0) {
    printf("Cannot create message: ");
    PrintfSysErr(msgItem);
    return 0;
  }

  queryHeader.ebh_Flavor = EB_GetFocus;

  sent = SendMsg(brokerPortItem, msgItem, &queryHeader, sizeof queryHeader);

  if (sent < 0) {
    printf("Error sending get-focus message: ");
    PrintfSysErr(sent);
    return 0;
  }

  eventItem = WaitPort(msgPortItem, msgItem);

  if (eventItem < 0) {
    printf("Error getting get-focus reply: ");
    PrintfSysErr(eventItem);
    return 0;
  }

  event = (Message *) LookupItem(eventItem);
  msgHeader = (EventBrokerHeader *) event->msg_DataPtr;

  if ((int32) event->msg_Result < 0) {
    printf("cpdump says broker refused get-focus request: ");
    PrintfSysErr((int32) event->msg_Result);
    return 0;
  }

  focusItem = (Item) event->msg_Result;

  if (argc == 1) {
    listenerPort = (MsgPort *) CheckItem(focusItem,
					 KERNELNODE, MSGPORTNODE);
    if (listenerPort) {
      if (listenerPort->mp.n_Name) {
	printf("Focus is held by '%s' on port 0x%x\n",
	       listenerPort->mp.n_Name,
	       focusItem);
      } else {
	printf("Focus is held by unnamed port 0x%x\n", focusItem);
      }
    } else {
      printf("Nobody is holding the focus\n");
    }
    return 0;
  }

  queryHeader.ebh_Flavor = EB_GetListeners;

  sent = SendMsg(brokerPortItem, msgItem, &queryHeader, sizeof queryHeader);

  if (sent < 0) {
    printf("Error sending get-listeners message: ");
    PrintfSysErr(sent);
    return 0;
  }

  eventItem = WaitPort(msgPortItem, msgItem);

  if (eventItem < 0) {
    printf("Error getting get-listeners reply: ");
    PrintfSysErr(eventItem);
    return 0;
  }

  event = (Message *) LookupItem(eventItem);
  msgHeader = (EventBrokerHeader *) event->msg_DataPtr;

  if ((int32) event->msg_Result < 0) {
    printf("cpdump says broker refused get-listeners request: ");
    PrintfSysErr((int32) event->msg_Result);
    return 0;
  }

  ll = (ListenerList *) msgHeader;
  switchItem = 0;

  for (i = 0; !switchItem && i < ll->ll_Count; i++) {
    listenerPort = (MsgPort *) CheckItem(ll->ll_Listener[i].li_PortItem,
					 KERNELNODE, MSGPORTNODE);
    if (listenerPort && strcmp(listenerPort->mp.n_Name, argv[1]) == 0) {
      switchItem = ll->ll_Listener[i].li_PortItem;
    } else {
      sprintf(focusPort, "0x%x", ll->ll_Listener[i].li_PortItem);
      if (strcmp(focusPort, argv[1]) == 0) {
	switchItem = ll->ll_Listener[i].li_PortItem;
      }
    }
  }
  if (!switchItem) {
    printf("Cannot find listener %s\n", argv[1]);
    return -1;
  }

  focusRequest.sf_Header.ebh_Flavor = EB_SetFocus;
  focusRequest.sf_DesiredFocusListener = switchItem;

  sent = SendMsg(brokerPortItem, msgItem, &focusRequest, sizeof focusRequest);

  if (sent < 0) {
    printf("Error sending set-focus message: ");
    PrintfSysErr(sent);
    return 0;
  }

  eventItem = WaitPort(msgPortItem, msgItem);

  if (eventItem < 0) {
    printf("Error getting set-focus reply: ");
    PrintfSysErr(eventItem);
    return 0;
  }

  event = (Message *) LookupItem(eventItem);
  msgHeader = (EventBrokerHeader *) event->msg_DataPtr;

  if ((int32) event->msg_Result < 0) {
    printf("cpdump says broker refused set-focus request: ");
    PrintfSysErr((int32) event->msg_Result);
    return 0;
  }

  return 0;
}
