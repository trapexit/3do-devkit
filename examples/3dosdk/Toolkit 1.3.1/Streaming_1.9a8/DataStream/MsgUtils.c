/*******************************************************************************************
 *	File:			MsgUtils.c
 *
 *	Contains:		wrapper routines for kernel message related
 *services
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/1/93		jb		Version 1.2.3
 *						Remove all variable initializers in local
 *variable declarations to avoid register bashing by the ARM C compiler.
 *	5/21/93		jb		Add paranoia check in WaitForMsg(). See
 *the code demarked by the CHECK_FOR_GETTING_UNCLEAN_MESSAGES symbol. 5/9/93
 *jb		Fix possible uninitialized use of 'msgPtr' and make exit code
 *						common to PollForMsg() in
 *WaitForMsg(). 4/18/93		jb		Install WAITPORT_HACK to tiptoe
 *around the fact that WaitPort() always returns an error in 3B1. 4/13/93
 *jb		Add GetMsgPortSignal(), change CreateMsgPort() to NewMsgPort()
 *						to prepare for upgrade to 3B1
 *	4/5/93		jb		Fix PollForMsg() so that it can
 *optionally return any of its result args (not conditionalized on any of them
 *						needing to be set in order to get the
 *others). 4/5/93		jb		Add 'signalMaskPtr' arg to
 *CreateMsgPort()
 *	4/4/93		jb		Return pointer to message data in
 *WaitForMsg() & PollForMsg()
 *	3/30/93		jb		Add specific 'wait for' arg to
 *WaitForMsg() 3/28/93		jb		Added PollForMsg() 3/25/93
 *jb		New today
 *
 *******************************************************************************************/

#include "MsgUtils.h"
#include "Portfolio.h"

#define WAITPORT_HACK 0
#define CHECK_FOR_GETTING_UNCLEAN_MESSAGES 1

#define MSG_SIZE 4 /* bytes */

/*==========================================================================================
  ==========================================================================================
                                                        System related message
  helper functions
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Routine to create a message port item. The result is the port's item number
 *or the usual system error code values. If 'signalMaskPtr' is non-NULL, the
 *signal mask as allocated by the system when the port is created is returned
 *to the caller.
 *******************************************************************************************/
Item
NewMsgPort (uint32 *signalMaskPtr)
{
  Item portItem;

  portItem = CreateItem (MKNODEID (KERNELNODE, MSGPORTNODE), NULL);

  /* When a port is created, a signal is automatically allocated and
   * assigned to it. We optionally return this to the caller if the
   * pointer to the signal mask is non-NULL. Later, this signal can
   * be "waited" upon, in conjunction with other events (like other
   * ports you might be interested in receiving message from).
   */
  if (portItem > 0)
    {
      if (signalMaskPtr != NULL)
        (*signalMaskPtr) = GetMsgPortSignal (portItem);
    }

  return portItem;
}

/*******************************************************************************************
 * Routine to return the signal mask associated with a message port.
 *******************************************************************************************/
uint32
GetMsgPortSignal (Item msgPortItem)
{
  MsgPort *msgPort;

  msgPort = (MsgPort *)LookupItem (msgPortItem);
  if (msgPort == NULL)
    return 0;

  return msgPort->mp_Signal;
}

/*******************************************************************************************
 * Routine to delete a message port item.
 *******************************************************************************************/
void
RemoveMsgPort (Item msgPortItem)
{
  DeleteItem (msgPortItem);
}

/*******************************************************************************************
 * Routine to create a message item. An optional reply port may be specified.
 *If no reply port is intended, 'replyPortItem' should be set == 0. The result
 *is either a message item (result > 0) or an error (result <= 0).
 *******************************************************************************************/
Item
CreateMsgItem (Item replyPortItemOrZero)
{
  Item msgItem;
  TagArg CreateMsgTags[2];

  CreateMsgTags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
  CreateMsgTags[0].ta_Arg = (void *)replyPortItemOrZero;
  CreateMsgTags[1].ta_Tag = TAG_END;

  msgItem = CreateItem (MKNODEID (KERNELNODE, MESSAGENODE), CreateMsgTags);

  return msgItem;
}

/*******************************************************************************************
 * Routine to delete a message item.
 *******************************************************************************************/
void
RemoveMsgItem (Item msgItem)
{
  DeleteItem (msgItem);
}

/*******************************************************************************************
 * Routine to wait for and retrieve a message. On successful return (result ==
 *0), a message item and the 4 byte message value is returned in *msgItemPtr
 *and *msgValuePtr, respectively. The 'waitMsgItem' argument specifies a
 *specific message that must arrive and should be set to zero if the call
 *should wait for the arrival of any message.
 *******************************************************************************************/
int32
WaitForMsg (Item msgPortItem, Item *msgItemPtr, Message **pMsgPtr,
            void **pMsgDataPtr, Item waitMsgItem)
{
  Item msgItem;
  Message *msgPtr;

  /* Wait for the incoming message. Note that a specific message may
   * be what is being waited for here. The 'waitMsgItem' arg should be
   * set to zero if any message will do.
   */
#if WAITPORT_HACK
  /*=================================================================*
   * NOTE!!!!!!!!!!!!!!
   * The following HACK is to try to get around a bug in 3B1 where
   * waiting for a specific message NEVER works. This HACK will only
   * work if there is nothing else expected to arrive on the port in
   * question. If anything other than the desired message arrives,
   * we die immediately.
   *=================================================================*/

  /* Wait for ANYTHING to arrive */
  msgItem = WaitPort (msgPortItem, 0);
  if (msgItem < 0)
    return (int32)msgItem;

  /* If its not the thing we are expecting, die immediately */
  if (msgItem != waitMsgItem)
    {
      kprintf ("WAITPORT_HACK failed! Expecting %d, got %d\n", waitMsgItem,
               msgItem);
      exit (0);
    }
#else
  msgItem = WaitPort (msgPortItem, waitMsgItem);
  if (msgItem < 0)
    return (int32)msgItem;
#endif

  /* Find the address of the message structure so we can
   * get the message value from it.
   */
  msgPtr = (Message *)LookupItem (msgItem);
  if (msgPtr == NULL)
    return -1;

  /* Optionally return:
   *	- the message's item number
   *	- a pointer to the message structure
   *	- the message data  pointer
   */
  if (msgItemPtr != NULL)
    (*msgItemPtr) = msgItem;
  if (pMsgPtr != NULL)
    (*pMsgPtr) = msgPtr;
  if (pMsgDataPtr != NULL)
    (*pMsgDataPtr) = (void *)msgPtr->msg_DataPtr;

#if CHECK_FOR_GETTING_UNCLEAN_MESSAGES
  if (msgPtr->msg.n_Flags & (MESSAGE_SENT | MESSAGE_REPLIED))
    {
      kprintf ("WaitForMsg() - got unclean msg back from WaitPort()\n");
      if (waitMsgItem != 0)
        {
          kprintf ("...while waiting on msg %lx\n", waitMsgItem);
        }
      Debug ();
    }
#endif
  return 0;
}

/*******************************************************************************************
 * Routine to poll for and retrieve a message. On successful return (result ==
 *true), a message item and a pointer to the system Message struct is retuned
 *in 'pMsgPtr'. The message data pointer can be retrieved as follows: msgValue
 *= (*pMsgPtr)->msg_DataPtr;
 *******************************************************************************************/
Boolean
PollForMsg (Item msgPortItem, Item *msgItemPtr, Message **pMsgPtr,
            void **pMsgDataPtr, int32 *status)
{
  Item msgItem;
  Message *msgPtr;

  /* Poll for the next incoming message. If none are waiting, the
   * result is zero, so return false if we either get an error or if
   * the system tells us there's nothing waiting...
   */
  msgItem = GetMsg (msgPortItem);
  if (msgItem <= 0)
    {
      (*status) = msgItem;
      return false;
    }

  /* Find the address of the message structure so we can
   * get the message value from it.
   */
  msgPtr = (Message *)LookupItem (msgItem);
  if (msgPtr == NULL)
    {
      (*status) = -1;
      return false;
    }

  /* Optionally return:
   *	- the message's item number
   *	- a pointer to the message structure
   *	- the message data  pointer
   */
  if (msgItemPtr != NULL)
    (*msgItemPtr) = msgItem;
  if (pMsgPtr != NULL)
    (*pMsgPtr) = msgPtr;
  if (pMsgDataPtr != NULL)
    (*pMsgDataPtr) = (void *)msgPtr->msg_DataPtr;

  /* Assert success */
  (*status) = 0;

#if CHECK_FOR_GETTING_UNCLEAN_MESSAGES
  if (msgPtr->msg.n_Flags & (MESSAGE_SENT | MESSAGE_REPLIED))
    {
      kprintf ("PollForMsg() - got unclean msg back from GetMsg()\n");
      Debug ();
    }
#endif
  return true;
}
