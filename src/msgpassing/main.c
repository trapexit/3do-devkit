
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: msgpassing.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/msgpassing
|||	msgpassing - Demonstrates sending and receiving messages between two
|||	    threads.
|||
|||	  Synopsis
|||
|||	    msgpassing
|||
|||	  Description
|||
|||	    Demonstrates how to send messages between threads or tasks.
|||
|||	    The main() routine of the program creates a message port where it
can
|||	    receive messages. It then spawns a thread. This thread creates its
own
|||	    message port, and a message. The thread then sends the message to
the
|||	    parent message port. Once the parent receives the message, it
passes it on
|||	    to the thread.
|||
|||	  Associated Files
|||
|||	    msgpassing.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include "displayutils.h"
#include "event.h"
#include "graphics.h"
#include "item.h"
#include "kernel.h"
#include "msgport.h"
#include "operror.h"
#include "stdio.h"
#include "task.h"
#include "types.h"

/*****************************************************************************/

/* a signal mask used to sync the thread with the parent */
int32 parentSig;

/*****************************************************************************/

static void
ClearScreen (ScreenContext *sc)
{
  GrafCon gc;
  Rect r;

  r.rect_XLeft = 0;
  r.rect_YTop = 0;
  r.rect_XRight = 320;
  r.rect_YBottom = 240;
  SetFGPen (&gc, 0);
  FillRect (sc->sc_BitmapItems[sc->sc_curScreen], &gc, &r);
}

static void
DrawText (ScreenContext *sc, int32 x, int32 y, const char *text, Color color)
{
  GrafCon gc;

  SetFGPen (&gc, color);
  MoveTo (&gc, x, y);
  DrawText8 (&gc, sc->sc_BitmapItems[sc->sc_curScreen], (ubyte *)text);
}

static void
Present (ScreenContext *sc)
{
  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  sc->sc_curScreen = 1 - sc->sc_curScreen;
}

static void
DrawStep (ScreenContext *sc, int32 step, Message *msg)
{
  char line[80];

  ClearScreen (sc);
  DrawText (sc, 16, 18, "Message Passing", MakeRGB15 (31, 31, 31));
  DrawText (sc, 20, 44, "Parent task", MakeRGB15 (31, 24, 0));
  DrawText (sc, 176, 44, "Child thread", MakeRGB15 (31, 24, 0));
  DrawText (sc, 156, 60, "|", MakeRGB15 (12, 12, 12));
  DrawText (sc, 156, 76, "|", MakeRGB15 (12, 12, 12));
  DrawText (sc, 156, 92, "|", MakeRGB15 (12, 12, 12));
  DrawText (sc, 156, 108, "|", MakeRGB15 (12, 12, 12));
  DrawText (sc, 156, 124, "|", MakeRGB15 (12, 12, 12));
  DrawText (sc, 156, 140, "|", MakeRGB15 (12, 12, 12));
  DrawText (sc, 156, 156, "|", MakeRGB15 (12, 12, 12));
  DrawText (sc, 156, 172, "|", MakeRGB15 (12, 12, 12));

  if (step >= 1)
    DrawText (sc, 20, 68, "Create ParentPort", MakeRGB15 (24, 24, 24));
  if (step >= 2)
    DrawText (sc, 20, 84, "Create child thread", MakeRGB15 (24, 24, 24));
  if (step >= 3)
    {
      DrawText (sc, 176, 68, "Create ChildPort", MakeRGB15 (24, 24, 24));
      DrawText (sc, 176, 84, "Create ChildMsg", MakeRGB15 (24, 24, 24));
      DrawText (sc, 176, 100, "Signal ready", MakeRGB15 (24, 24, 24));
    }
  if (step >= 4)
    {
      DrawText (sc, 20, 112, "WaitPort()", MakeRGB15 (24, 24, 24));
      DrawText (sc, 176, 124, "SendSmallMsg()", MakeRGB15 (24, 24, 24));
      DrawText (sc, 124, 124, "<---", MakeRGB15 (31, 31, 0));
    }
  if (step >= 5)
    {
      if (msg)
        sprintf (line, "Received %ld, %ld", msg->msg_DataPtr,
                 msg->msg_DataSize);
      else
        sprintf (line, "Received child msg");
      DrawText (sc, 20, 140, line, MakeRGB15 (24, 24, 24));
    }
  if (step >= 6)
    {
      DrawText (sc, 20, 156, "ReplySmallMsg()", MakeRGB15 (24, 24, 24));
      DrawText (sc, 124, 156, "--->", MakeRGB15 (31, 31, 0));
      DrawText (sc, 176, 156, "Waiting for reply", MakeRGB15 (24, 24, 24));
    }
  if (step >= 7)
    {
      DrawText (sc, 20, 180, "Done", MakeRGB15 (24, 24, 24));
      DrawText (sc, 176, 180, "Reply received", MakeRGB15 (24, 24, 24));
      DrawText (sc, 84, 214, "Press X to exit", MakeRGB15 (18, 18, 18));
    }

  Present (sc);
}

static void
WaitForExit (void)
{
  ControlPadEventData cped;

  do
    {
      GetControlPad (1, TRUE, &cped);
    }
  while (!(cped.cped_ButtonBits & ControlX));
}

/*****************************************************************************/

static void
ThreadFunction (void)
{
  Item childPortItem;
  Item childMsgItem;
  Item parentPortItem;
  Err err;
  Message *msg;

  printf ("Child thread is running\n");

  childPortItem = CreateMsgPort ("ChildPort", 0, 0);
  if (childPortItem >= 0)
    {
      childMsgItem = CreateSmallMsg ("ChildMsg", 0, childPortItem);
      if (childMsgItem >= 0)
        {
          parentPortItem = FindMsgPort ("ParentPort");
          if (parentPortItem >= 0)
            {
              /* tell the paren't we're done initializing */
              SendSignal (CURRENTTASK->t_ThreadTask->t.n_Item, parentSig);

              err = SendSmallMsg (parentPortItem, childMsgItem, 12, 34);
              if (err >= 0)
                {
                  err = WaitPort (childPortItem, childMsgItem);
                  if (err >= 0)
                    {
                      msg = (Message *)LookupItem (childMsgItem);
                      printf ("Child received reply from parent: ");
                      printf (
                          "msg_Result %d, msg_DataPtr %d, msg_DataSize %d\n",
                          msg->msg_Result, msg->msg_DataPtr,
                          msg->msg_DataSize);
                    }
                  else
                    {
                      printf ("WaitPort() failed: ");
                      PrintfSysErr (err);
                    }
                }
              else
                {
                  printf ("SendSmallMsg() failed: ");
                  PrintfSysErr (err);
                }

              SendSignal (CURRENTTASK->t_ThreadTask->t.n_Item, parentSig);
            }
          else
            {
              printf ("Could not find parent message port: ");
              PrintfSysErr (parentPortItem);
            }
          DeleteMsg (childMsgItem);
        }
      else
        {
          printf ("CreateSmallMsg() failed: ");
          PrintfSysErr (childMsgItem);
        }
      DeleteMsgPort (childPortItem);
    }
  else
    {
      printf ("CreateMsgPort() failed: ");
      PrintfSysErr (childPortItem);
    }
}

/*****************************************************************************/

int
main (int32 argc, char **argv)
{
  ScreenContext sc;
  Item portItem;
  Item threadItem;
  Item msgItem;
  Message *msg;
  Err err;

  (void)argc;
  (void)argv;

  err = InitEventUtility (1, 0, LC_ISFOCUSED);
  if (err < 0)
    {
      printf ("InitEventUtility failed: ");
      PrintfSysErr (err);
      return (int)err;
    }

  err = CreateBasicDisplay (&sc, DI_TYPE_NTSC, 2);
  if (err < 0)
    {
      printf ("CreateBasicDisplay failed: ");
      PrintfSysErr (err);
      KillEventUtility ();
      return (int)err;
    }
  sc.sc_curScreen = 0;
  DrawStep (&sc, 0, NULL);

  parentSig = AllocSignal (0);
  if (parentSig > 0)
    {
      portItem = CreateMsgPort ("ParentPort", 0, 0);
      if (portItem >= 0)
        {
          DrawStep (&sc, 1, NULL);
          threadItem = CreateThread ("Child", 10, ThreadFunction, 2048);
          if (threadItem >= 0)
            {
              DrawStep (&sc, 2, NULL);

              /* wait for the child to be ready */
              WaitSignal (parentSig);
              DrawStep (&sc, 3, NULL);

              /* confirm that the child initialized correctly */
              if (FindMsgPort ("ChildPort") >= 0)
                {
                  printf ("Parent waiting for message from child\n");
                  DrawStep (&sc, 4, NULL);

                  msgItem = WaitPort (portItem, 0);
                  if (msgItem >= 0)
                    {
                      msg = (Message *)LookupItem (msgItem);
                      DrawStep (&sc, 5, msg);
                      printf ("Parent got child's message: ");
                      printf ("msg_DataPtr %d, msg_DataSize %d\n",
                              msg->msg_DataPtr, msg->msg_DataSize);
                      ReplySmallMsg (msgItem, 56, 78, 90);
                      DrawStep (&sc, 6, msg);
                    }
                  else
                    {
                      printf ("WaitPort() failed: ");
                      PrintfSysErr (msgItem);
                    }
                }

              /* wait for the thread to tell us it's done before we zap it */
              WaitSignal (parentSig);
              DrawStep (&sc, 7, NULL);
              WaitForExit ();

              DeleteThread (threadItem);
            }
          else
            {
              printf ("CreateThread() failed: ");
              PrintfSysErr (threadItem);
            }
          DeleteMsgPort (portItem);
        }
      else
        {
          printf ("CreateMsgPort() failed: ");
          PrintfSysErr (portItem);
        }
      FreeSignal (parentSig);
    }
  else
    {
      printf ("AllocSignal() failed");
    }

  DeleteBasicDisplay (&sc);
  KillEventUtility ();
  return 0;
}
