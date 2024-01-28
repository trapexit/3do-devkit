#ifndef __MSGPORT_H
#define __MSGPORT_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: msgport.h,v 1.18 1994/09/30 18:40:12 vertex Exp $
**
**  Kernel messaging system management
**
******************************************************************************/


#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
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
	uint32  msg_Waiters;    /* number of tasks waiting on this msg */
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

extern Item CreateMsgPort(const char *name, uint8 pri, uint32 signal);
extern Item CreateUniqueMsgPort(const char *name, uint8 pri, uint32 signal);

extern Item CreateMsg(const char *name, uint8 pri, Item mp);
extern Item CreateSmallMsg(const char *name, uint8 pri, Item mp);
extern Item CreateBufferedMsg(const char *name, uint8 pri, Item mp, uint32 buffsize);

extern Err __swi(KERNELSWI+16)  SendMsg(Item mp,Item msg,
					 const void *dataptr, int32 datasize);
extern Err __swi(KERNELSWI+16)  SendSmallMsg(Item mp,Item msg,
					 uint32 val1, uint32 val2);
extern Item __swi(KERNELSWI+19)	GetMsg(Item mp);
extern Item __swi(KERNELSWI+15)	GetThisMsg(Item msg);
extern Item __swi(KERNELSWI+40) WaitPort(Item mp,Item msg);
extern Err __swi(KERNELSWI+18)  ReplyMsg(Item msg, int32 result,
					  const void *dataptr, int32 datasize);
extern Err __swi(KERNELSWI+18)  ReplySmallMsg(Item msg, int32 result,
					  uint32 val1, uint32 val2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define DeleteMsgPort(x) DeleteItem(x)
#define DeleteMsg(x)	 DeleteItem(x)
#define FindMsgPort(n)   FindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),(n))

#endif	/* __MSGPORT_H */
