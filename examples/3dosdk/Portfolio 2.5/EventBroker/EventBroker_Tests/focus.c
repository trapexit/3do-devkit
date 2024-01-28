
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: focus.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/focus
|||	focus - Talks to the event broker and switches
|||	        the focus to a different listener.
|||
|||	  Synopsis
|||
|||	    focus [listener]
|||
|||	  Description
|||
|||	    Lets you view the current focus holder, or change it.
|||
|||	  Arguments
|||
|||	    listener                     Name or hexadecimal address of a
message
|||	                                 port to which the focus should be
diverted.
|||	                                 If this is not specified, the program
lists
|||	                                 the current focus holder.
|||
|||	  Associated Files
|||
|||	    focus.c
|||
|||	  Location
|||
|||	    $c/focus
|||
|||	    examples/EventBroker/Event_Broker_Tests
|||
**/

#include "event.h"
#include "item.h"
#include "msgport.h"
#include "operror.h"
#include "stdio.h"
#include "string.h"
#include "types.h"

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
  Item eventItem;
  Item focusItem;
  Err err;
  Item switchItem;
  Message *msg;
  MsgPort *listenerPort;
  char focusPort[16];
  ListenerList *ll;
  int32 i;
  SetFocus focusRequest;

  if (argc > 2)
    {
      printf ("Usage: focus [listener]\n");
      return 0;
    }

  ebPortItem = FindMsgPort (EventPortName);
  if (ebPortItem >= 0)
    {
      msgPortItem = CreateMsgPort (NULL, 0, 0);
      if (msgPortItem >= 0)
        {
          msgItem = CreateBufferedMsg (NULL, 0, msgPortItem, 2048);
          if (msgItem >= 0)
            {
              err = SendEBMessage (ebPortItem, msgItem, EB_GetFocus);
              if (err >= 0)
                {
                  msg = (Message *)LookupItem (msgItem);
                  focusItem = (Item)msg->msg_Result;

                  if (argc == 1)
                    {
                      listenerPort = (MsgPort *)CheckItem (
                          focusItem, KERNELNODE, MSGPORTNODE);
                      if (listenerPort)
                        {
                          if (listenerPort->mp.n_Name)
                            {
                              printf ("Focus is held by '%s' on port 0x%x\n",
                                      listenerPort->mp.n_Name, focusItem);
                            }
                          else
                            {
                              printf ("Focus is held by unnamed port 0x%x\n",
                                      focusItem);
                            }
                        }
                      else
                        {
                          printf ("Nobody is holding the focus\n");
                        }
                    }
                  else
                    {
                      err = SendEBMessage (ebPortItem, msgItem,
                                           EB_GetListeners);
                      if (err >= 0)
                        {
                          ll = (ListenerList *)msg->msg_DataPtr;
                          switchItem = 0;

                          for (i = 0; !switchItem && i < ll->ll_Count; i++)
                            {
                              listenerPort = (MsgPort *)CheckItem (
                                  ll->ll_Listener[i].li_PortItem, KERNELNODE,
                                  MSGPORTNODE);
                              if (listenerPort
                                  && strcmp (listenerPort->mp.n_Name, argv[1])
                                         == 0)
                                {
                                  switchItem = ll->ll_Listener[i].li_PortItem;
                                }
                              else
                                {
                                  sprintf (focusPort, "0x%x",
                                           ll->ll_Listener[i].li_PortItem);
                                  if (strcmp (focusPort, argv[1]) == 0)
                                    switchItem
                                        = ll->ll_Listener[i].li_PortItem;
                                }
                            }

                          if (switchItem)
                            {
                              focusRequest.sf_Header.ebh_Flavor = EB_SetFocus;
                              focusRequest.sf_DesiredFocusListener
                                  = switchItem;

                              err = SendMsg (ebPortItem, msgItem,
                                             &focusRequest,
                                             sizeof focusRequest);
                              if (err >= 0)
                                {
                                  eventItem = WaitPort (msgPortItem, msgItem);
                                  if (eventItem >= 0)
                                    {
                                      err = (Err)msg->msg_Result;
                                      if (err < 0)
                                        {
                                          printf ("EB_SetFocus failed: ");
                                          PrintfSysErr (err);
                                        }
                                    }
                                  else
                                    {
                                      printf ("WaitPort() failed: ");
                                      PrintfSysErr (err);
                                    }
                                }
                              else
                                {
                                  printf ("SendMsg() failed: ");
                                  PrintfSysErr (err);
                                }
                            }
                          else
                            {
                              printf ("Cannot find listener %s\n", argv[1]);
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
