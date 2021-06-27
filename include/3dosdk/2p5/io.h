#ifndef __IO_H
#define __IO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: io.h,v 1.56 1994/11/19 01:48:49 sdas Exp $
**
**  Kernel device IO definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __NODES_H
#include "nodes.h"
#endif

#ifndef __MSGPORT_H
#include "msgport.h"
#endif


typedef struct IOBuf
{
	void	*iob_Buffer;	/* ptr to users buffer */
	int32	iob_Len;	/* len of this buffer, or transfer size */
} IOBuf;

/* User portion of IOReq data structure supplied by user*/
typedef struct IOInfo
{
	uint8	ioi_Command;	/* Command to be executed */
	uint8	ioi_Flags;	/* misc flags */
	uint8	ioi_Unit;	/* unit of this device */
	uint8	ioi_Flags2;	/* more flags, should be set to zero */
	uint32	ioi_CmdOptions;	/* device dependent options */
	uint32	ioi_User;	/* back ptr for user use */
	int32 	ioi_Offset;	/* offset into device for transfer to begin */
	IOBuf	ioi_Send;	/* copy out information */
	IOBuf	ioi_Recv;	/* copy in info, (address validated) */
} IOInfo;

/* common commands */
#define CMD_WRITE	0
#define CMD_READ	1
#define CMD_STATUS	2


/*****************************************************************************/



/* Mac device commands */
/* private for 3DO only */
#define MACCMD_PRINT	3	/* print string to note window */
#define MACCMD_ASK	4	/* print string wait for response */
#define MACCMD_APPEND	5	/* append data to existing file */
#define MACCMD_FILELEN	6	/* return filelength bufsize=4, FileInfo */
#define MACCMD_FILEINFO MACCMD_FILELEN
#define MACCMD_WRITECR	7	/* create file, write at offset 0, truncate */
#define MACCMD_READDIR	9	/* get an entry from mac directory */
				/* offset is interpreted as an index */
#define MACCMD_READCDDELAY 10	/* standard read with simulated CD delay */


/*****************************************************************************/


/* kernel printf special commands */
#define KPRINTF_STOP	1	/* stop kprintf */
#define KPRINTF_START	2	/* start kprintf */
#define KPRINTF_DISABLE	4	/* disable kprintf forever */


/*****************************************************************************/

/* get the address of an IOReq from the embedded io_Link address */
/* internal use only */
#define IOReq_Addr(a)	(IOReq *)((uint32)(a) - Offset(IOReq *, io_Link))

/* System Portion of IoReq */
#ifndef	IOReq_typedef
#define	IOReq_typedef
typedef struct IOReq IOReq;
#endif

struct IOReq {
	ItemNode	io;
	MinNode	io_Link;
	struct Device	*io_Dev;
	struct IOReq	*(*io_CallBack)(struct IOReq *iorP); /* call, donot ReplyMsg */
	IOInfo	io_Info;
	int32	io_Actual;	/* actual size of request completed */
	uint32	io_Flags;	/* internal to device driver */
	int32	io_Error;	/* any errors from request? */
	int32	io_Extension[2];/* extra space if needed */
	Item	io_MsgItem;
	uint32	io_Private0;
};

/* io_Flags 0x0000ffff reserved for system */
/* io_Flags 0xffff0000 reserved for device */
#define IO_DONE		1
#define IO_QUICK	2	/* also an ioi_Flag */
#define IO_INTERNAL	4	/* used by CompleteIOR */
#define IO_WAITING      8       /* owner waiting for this IO */

enum ioreq_tags
{
    CREATEIOREQ_TAG_REPLYPORT = TAG_ITEM_LAST+1,    /* optional */
    CREATEIOREQ_TAG_DEVICE			    /* required */
};


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


Err __swi(KERNELSWI+24)	SendIO(Item ior, const IOInfo *ioiP);       /* async  */
Err __swi(KERNELSWI+37) DoIO(Item ior, const IOInfo *ioiP);   /* sync   */
Err __swi(KERNELSWI+25) AbortIO(Item ior);      /* abort an io          */
Err __swi(KERNELSWI+41) WaitIO(Item ior);              /* wait for io completion */

int32 CheckIO(Item ior);           /* Is io done? */
Item CreateIOReq(const char *name, uint8 pri, Item dev, Item mp);/* mp can be 0 */


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


#define DeleteIOReq(x)	DeleteItem(x)


/*****************************************************************************/


#endif	/* __IO_H */
