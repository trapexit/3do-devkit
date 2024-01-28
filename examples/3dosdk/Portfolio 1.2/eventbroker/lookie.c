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
  lookie.c - a simple program which talks to the Event Broker and
  reports any events sent to it.
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

static void DumpEvent (EventBrokerHeader *hdr);

int
main (int argc, char **argv)
{
  Item msgPortItem;
  Item brokerPortItem;
  Item msgItem, eventItem;
  int32 sent;
  uint32 theSignal;
  Message *event;
  MsgPort *msgPort;

  ConfigurationRequest config;
  EventBrokerHeader *msgHeader;

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

  msgItem = CreateMsg (NULL, 0, msgPortItem);

  if (msgItem < 0)
    {
      printf ("Cannot create message: ");
      PrintfSysErr (msgItem);
      return 0;
    }

  config.cr_Header.ebh_Flavor = EB_Configure;
  config.cr_Category = LC_Observer;
  if (argc >= 2)
    {
      if (strcmp (argv[1], "focus") == 0)
        {
          config.cr_Category = LC_FocusListener;
        }
      else if (strcmp (argv[1], "hybrid") == 0)
        {
          config.cr_Category = LC_FocusUI;
        }
      else
        {
          printf ("Listener category not 'focus' or 'hybrid'\n");
        }
    }
  memset (&config.cr_TriggerMask, 0x00, sizeof config.cr_TriggerMask);
  memset (&config.cr_CaptureMask, 0x00, sizeof config.cr_CaptureMask);
  config.cr_QueueMax = 0; /* let the Broker set it */
  ;
  config.cr_TriggerMask[0]
      = EVENTBIT0_ControlButtonPressed | EVENTBIT0_ControlButtonReleased
        | EVENTBIT0_ControlButtonUpdate | EVENTBIT0_MouseButtonPressed
        | EVENTBIT0_MouseButtonReleased | EVENTBIT0_MouseUpdate
        | EVENTBIT0_MouseMoved | EVENTBIT0_GivingFocus | EVENTBIT0_LosingFocus
        | EVENTBIT0_LightGunButtonPressed | EVENTBIT0_LightGunButtonReleased
        | EVENTBIT0_LightGunUpdate | EVENTBIT0_LightGunFireTracking
        | EVENTBIT0_StickButtonPressed | EVENTBIT0_StickButtonReleased
        | EVENTBIT0_StickUpdate | EVENTBIT0_StickMoved;
  config.cr_TriggerMask[2] = EVENTBIT2_ControlPortChange;

  sent = SendMsg (brokerPortItem, msgItem, &config, sizeof config);

  if (sent < 0)
    {
      printf ("Error sending config message: ");
      PrintfSysErr (sent);
      return 0;
    }

  while (TRUE)
    {
      theSignal = WaitSignal (msgPort->mp_Signal);
      if (theSignal & msgPort->mp_Signal)
        {
          while (TRUE)
            {
              eventItem = GetMsg (msgPortItem);
              if (eventItem < 0)
                {
                  printf ("Error 0x%x getting message: ", eventItem);
                  PrintfSysErr (eventItem);
                  return 0;
                }
              else if (eventItem == 0)
                {
                  break;
                }
              event = (Message *)LookupItem (eventItem);
              msgHeader = (EventBrokerHeader *)event->msg_DataPtr;
              if (eventItem == msgItem)
                {
                  if ((int32)event->msg_Result < 0)
                    {
                      printf ("Lookie says broker refused configuration "
                              "request: ");
                      PrintfSysErr ((int32)event->msg_Result);
                      return 0;
                    }
                  printf ("Lookie says broker has accepted event-config "
                          "request\n");
                }
              else
                {
                  DumpEvent (msgHeader);
                  ReplyMsg (eventItem, 0, NULL, 0);
                }
            }
        }
      else
        {
          printf ("Lookie-loo unexpected signal 0x%x\n", theSignal);
        }
    }
}

void
LightGun (EventFrame *frame, char *action)
{
  LightGunEventData *data;
  data = (LightGunEventData *)frame->ef_EventData;
  printf ("  LightGun %s 0x%x pod %d position %d generic %d\n", action,
          data->lged_ButtonBits, frame->ef_PodNumber, frame->ef_PodPosition,
          frame->ef_GenericPosition);
  printf ("    counter %d line-hits %d\n", data->lged_Counter,
          data->lged_LinePulseCount);
}

void
StickDump (EventFrame *frame, char *action)
{
  StickEventData *stk;
  stk = (StickEventData *)frame->ef_EventData;
  printf ("  Stick %s 0x%x pod %d position %d generic %d\n", action,
          stk->stk_ButtonBits, frame->ef_PodNumber, frame->ef_PodPosition,
          frame->ef_GenericPosition);
  printf ("    position(%5d,%5d,%5d)\n", stk->stk_HorizPosition,
          stk->stk_VertPosition, stk->stk_DepthPosition);
}

static void
DumpEvent (EventBrokerHeader *hdr)
{
  EventFrame *frame;
  MouseEventData *med;
  switch (hdr->ebh_Flavor)
    {
    case EB_EventRecord:
      frame = (EventFrame *)(hdr + 1); /* Gawd that's ugly! */
      printf ("Lookie got an event record at 0x%x time %d:\n", hdr,
              frame->ef_SystemTimeStamp);
      while (TRUE)
        {
          if (frame->ef_ByteCount == 0)
            {
              printf (" End of record\n\n");
              break;
            }
          printf (" Frame of %d bytes:", frame->ef_ByteCount);
          switch (frame->ef_EventNumber)
            {
            case EVENTNUM_ControlButtonPressed:
              printf ("  Control buttons pressed 0x%x pod %d position %d "
                      "generic %d\n",
                      frame->ef_EventData[0], frame->ef_PodNumber,
                      frame->ef_PodPosition, frame->ef_GenericPosition);
              break;
            case EVENTNUM_ControlButtonReleased:
              printf ("  Control buttons released 0x%x pod %d position %d "
                      "generic %d\n",
                      frame->ef_EventData[0], frame->ef_PodNumber,
                      frame->ef_PodPosition, frame->ef_GenericPosition);
              break;
            case EVENTNUM_ControlButtonUpdate:
              printf ("  Control button update 0x%x pod %d position %d "
                      "generic %d\n",
                      frame->ef_EventData[0], frame->ef_PodNumber,
                      frame->ef_PodPosition, frame->ef_GenericPosition);
              break;
            case EVENTNUM_ControlButtonArrived:
              printf ("  Control button arrival 0x%x pod %d position %d "
                      "generic %d\n",
                      frame->ef_EventData[0], frame->ef_PodNumber,
                      frame->ef_PodPosition, frame->ef_GenericPosition);
              break;
            case EVENTNUM_MouseButtonPressed:
              med = (MouseEventData *)frame->ef_EventData;
              printf ("  Mouse button pressed 0x%x at (%d,%d)\n",
                      med->med_ButtonBits, med->med_HorizPosition,
                      med->med_VertPosition);
              printf ("    pod %d position %d generic %d\n",
                      frame->ef_PodNumber, frame->ef_PodPosition,
                      frame->ef_GenericPosition);
              break;
            case EVENTNUM_MouseButtonReleased:
              med = (MouseEventData *)frame->ef_EventData;
              printf ("  Mouse button released 0x%x at (%d,%d)\n",
                      med->med_ButtonBits, med->med_HorizPosition,
                      med->med_VertPosition);
              printf ("    pod %d position %d generic %d\n",
                      frame->ef_PodNumber, frame->ef_PodPosition,
                      frame->ef_GenericPosition);
              break;
            case EVENTNUM_MouseMoved:
              med = (MouseEventData *)frame->ef_EventData;
              printf ("  Mouse moved to (%d,%d) buttons 0x%x\n",
                      med->med_HorizPosition, med->med_VertPosition,
                      med->med_ButtonBits);
              printf ("    pod %d position %d generic %d\n",
                      frame->ef_PodNumber, frame->ef_PodPosition,
                      frame->ef_GenericPosition);
              break;
            case EVENTNUM_MouseUpdate:
              med = (MouseEventData *)frame->ef_EventData;
              printf ("  Mouse data update 0x%x at (%d,%d)\n",
                      med->med_ButtonBits, med->med_HorizPosition,
                      med->med_VertPosition);
              printf ("    pod %d position %d generic %d\n",
                      frame->ef_PodNumber, frame->ef_PodPosition,
                      frame->ef_GenericPosition);
              break;
            case EVENTNUM_LightGunButtonPressed:
              LightGun (frame, "buttons pressed");
              break;
            case EVENTNUM_LightGunButtonReleased:
              LightGun (frame, "buttons released");
              break;
            case EVENTNUM_LightGunUpdate:
              LightGun (frame, "update");
              break;
            case EVENTNUM_LightGunFireTracking:
              LightGun (frame, "fire-tracking");
              break;
            case EVENTNUM_StickButtonPressed:
              StickDump (frame, "buttons pressed");
              break;
            case EVENTNUM_StickButtonReleased:
              StickDump (frame, "buttons released");
              break;
            case EVENTNUM_StickUpdate:
              StickDump (frame, "update");
              break;
            case EVENTNUM_StickMoved:
              StickDump (frame, "moved");
              break;
            case EVENTNUM_EventQueueOverflow:
              printf ("  Event queue overflowed, some events lost\n");
              break;
            case EVENTNUM_ControlPortChange:
              printf ("  The Control Port configuration has changed\n");
              break;
            case EVENTNUM_GivingFocus:
              printf ("  We have been given focus\n");
              break;
            case EVENTNUM_LosingFocus:
              printf ("  We are losing focus\n");
              break;
            default:
              printf ("  Event %d\n", frame->ef_EventNumber);
              break;
            }
          frame = (EventFrame *)(frame->ef_ByteCount + (char *)frame);
        }
      break;
    default:
      printf ("Lookie got event-message type %d\n", hdr->ebh_Flavor);
      break;
    }
}
