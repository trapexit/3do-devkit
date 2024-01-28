#ifndef __MSGPORT_H
#define __MSGPORT_H

/*****

$Id: msgport.h,v 1.10 1994/01/21 02:23:38 limes Exp $

$Log: msgport.h,v $
 * Revision 1.10  1994/01/21  02:23:38  limes
 * recover from rcs bobble
 *
 * Revision 1.11  1994/01/20  02:06:10  sdas
 * C++ compatibility - updated
 *
 * Revision 1.10  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.9  1993/12/17  20:18:57  dale
 * added CreateSmallMsg CreateBufferedMsg
 *
 * Revision 1.8  1993/08/13  03:42:25  dale
 * Find convenience macro
 *
 * Revision 1.7  1993/06/10  18:14:38  andy
 * changed MSG_TAG_DATA_AREA to MSG_TAG_DATA_SIZE
 *
 * Revision 1.6  1993/06/09  22:30:20  andy
 * *** empty log message ***
 *
 * Revision 1.5  1993/06/09  22:18:14  andy
 * put new tag in msg, as opposed to msgport, where it didn't belong.
 *
 * Revision 1.4  1993/06/09  00:43:15  andy
 * Added UserData pointer and Reserved to MsgPort, and defined new
 * tag to set UserData pointer.  Changed msg_Reserved of Message to
 * a msg_DataPtrSize, for the new seperately allocated pass_by_valye
 * area, and defined a tag to set that up.
 *
 * Revision 1.3  1993/05/21  18:49:11  andy
 * Added new field to allow kill code to abort messages
 *
 * Revision 1.2  1993/04/01  04:45:14  dale
 * added CREATEMSG_TAG_MSG_IS_SMALL
 *
 * Revision 1.1  1993/03/16  06:43:22  dale
 * Initial revision
 *
 * Revision 1.11  1993/02/17  04:43:21  dale
 * added simple library routines for creating Items
 *
 * Revision 1.10  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.9  1992/11/25  00:01:25  dale
 * new msgs
 *
 * Revision 1.8  1992/10/29  23:25:28  dale
 * removed msg_LastSent
 *
 * Revision 1.7  1992/10/24  01:41:07  dale
 * rcs
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

#include "nodes.h"
#include "list.h"
#include "item.h"

typedef struct MsgPort
{
	ItemNode	mp;
	uint32	mp_Signal;	/* what Owner needs to wake up */
	List	mp_Msgs;	/* Messages waiting for Owner */
	void   *mp_UserData;	/* User data pointer */
	uint32	mp_Reserved;	/* Kernel use only */
} MsgPort;

/* MsgPort flags */
#define MSGPORT_SIGNAL_ALLOCATED	1

enum msgport_tags
{
	CREATEPORT_TAG_SIGNAL = TAG_ITEM_LAST+1,/* use this signal */
	CREATEPORT_TAG_USERDATA		/* set MsgPort UserData pointer */
};
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

extern Item CreateMsgPort(char *name, uint8 pri, uint32 signal);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#define DeleteMsgPort(x)	DeleteItem(x)

typedef struct Message
{
	ItemNode	msg;
	Item 	msg_ReplyPort;
	uint32	msg_Result;	/* result from ReplyMsg */
	void	*msg_DataPtr;	/* ptr to beginning of data */
	int32	msg_DataSize;	/* size of data field */
	Item	msg_MsgPort;	/* MsgPort currently queued on */
	uint32	msg_DataPtrSize;/* size of allocated data area */
	Item	msg_SigItem;	/* Designated Signal Receiver */
} Message;

#define Msg Message

/* specify a different size in the CreateItem call to get */
/* pass by value message */
#define MESSAGE_SENT		0x1	/* msg sent and not replied */
#define MESSAGE_REPLIED		0x2	/* msg replied and not removed */
#define MESSAGE_SMALL		0x4	/* this is really a small value msg */
#define MESSAGE_PASS_BY_VALUE	0x8	/* copy data to msg buffer */

enum message_tags
{
	CREATEMSG_TAG_REPLYPORT	= TAG_ITEM_LAST+1,
	CREATEMSG_TAG_MSG_IS_SMALL,
	CREATEMSG_TAG_DATA_SIZE			/* data area for pass by value */
};
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

extern Item CreateMsg(char *name, uint8 pri, Item mp);
extern Item CreateSmallMsg(char *name, uint8 pri, Item mp);
extern Item CreateBufferedMsg(char *name, uint8 pri, Item mp, uint32 buffsize);
#define DeleteMsg(x)	DeleteItem(x)

extern Err __swi(KERNELSWI+16)  SendMsg(Item mp,Item msg,
					 void *dataptr, int32 datasize);
extern Err __swi(KERNELSWI+16)  SendSmallMsg(Item mp,Item msg,
					 uint32 val1, uint32 val2);
extern Item __swi(KERNELSWI+19)	  GetMsg(Item mp);
extern Item __swi(KERNELSWI+15)	  GetThisMsg(Item msg);
extern Item WaitPort(Item mp,Item msg);
extern Err __swi(KERNELSWI+18)  ReplyMsg(Item msg, int32 result,
					  void *dataptr, int32 datasize);
extern Err __swi(KERNELSWI+18)  ReplySmallMsg(Item msg, int32 result,
					  uint32 val1, uint32 val2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define FindMsgPort(n)     FindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),(n))

#endif	/* __MSGPORT_H */
