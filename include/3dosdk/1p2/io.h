#ifndef __IO_H
#define __IO_H

/*****

$Id: io.h,v 1.36 1994/01/21 02:14:21 limes Exp $

$Log: io.h,v $
 * Revision 1.36  1994/01/21  02:14:21  limes
 * +  * Revision 1.37  1994/01/20  01:59:26  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.36  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 *
 * Revision 1.37  1994/01/20  01:59:26  sdas
 * C++ compatibility - updated
 *
 * Revision 1.36  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.35  1994/01/05  18:27:15  dale
 * Added CompleteIO as a privileged swi function
 *
 * Revision 1.34  1993/11/24  05:24:52  limes
 * We might be typedefing IOReq in driver.h ... see driver.h for more info.
 *
 * Revision 1.33  1993/07/25  05:30:30  andy
 * changed pad to flags2
 *
 * Revision 1.33  1993/07/25  03:11:05  andy
 * changed field name
 *
 * Revision 1.32  1993/06/24  22:09:15  dale
 * needed comment
 *
 * Revision 1.31  1993/06/09  00:43:15  andy
 * Changed io_Reserved field into io_SigItem field.
 *
 * Revision 1.30  1993/05/21  18:49:11  andy
 * Added new field to allow kill code to abort messages
 *
 * Revision 1.29  1993/05/15  22:21:05  dale
 * removed io_DevItem, unused.
 *
 * Revision 1.28  1993/04/14  23:32:32  dale
 * TIMERCMD_DELAYUNTIL
 *
 * Revision 1.27  1993/04/01  04:52:48  dale
 * TIMER_UNIT, cleanup.
 *
 * Revision 1.26  1993/03/26  18:26:39  dale
 * LOMAC_DEBUGTRIGGER
 *
 * Revision 1.25  1993/03/16  06:51:17  dale
 * api
 *
 * Revision 1.24  1993/02/17  04:43:21  dale
 * added simple library routines for creating Items
 *
 * Revision 1.23  1993/02/11  04:58:58  dale
 * name update for M6
 *
 * Revision 1.22  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.21  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.20  1993/02/10  00:33:10  dale
 * added new MACCMD_READCDDELAY
 *
 * Revision 1.19  1993/02/09  07:46:40  dale
 * removed required Message, now optional.
 *
 * Revision 1.18  1992/12/23  01:02:49  dale
 * New CommandSyncStat support
 *
 * Revision 1.17  1992/12/22  07:33:48  dale
 * JOYCMDS
 *
 * Revision 1.16  1992/12/19  05:06:05  dale
 * added XBUSCMD_DrainDataFifo
 *
 * Revision 1.15  1992/12/08  10:01:38  dale
 * XBUS
 *
 * Revision 1.14  1992/12/02  21:58:35  dale
 * new field in IOInfo structure
 *
 * Revision 1.13  1992/11/25  00:01:25  dale
 * new msgs
 *
 * Revision 1.12  1992/11/22  21:28:44  dale
 * fix warnings
 *
 * Revision 1.11  1992/10/29  01:14:11  dale
 * Flags changed to unsigned
 *
 * Revision 1.10  1992/10/24  01:37:21  dale
 * fix id
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
#include "msgport.h"

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

/****** begin: stuff for timer device */
#define TIMER_UNIT_VBLANK	0
#define TIMER_UNIT_USEC	1

/* Timer device commands */
#define TIMERCMD_DELAY		3		/* delay for */
#define TIMERCMD_DELAYUNTIL	4		/* delay until */

/****** end: stuff for timer device */

/****** begin: cdrom commands (for host emulation only) */
#define CDROMCMD_SETUNITNAME	3		/* for cdrom device */
/****** end: cdrom commands */

/* commands for low level debug packet interface */
/* private for 3DO only */
#define LOMAC_DEBUGTRIGGER	3	/* trigger host interrupt */

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

/* ExpansionBus Commands */
#define XBUSCMD_Command	3	/* send command, request status */
#define XBUSCMD_DrainDataFifo	4 /* read recv.len bytes of data, discard */
#define XBUSCMD_CommandSyncStat	5 /* reselect each status fifo byte */

#define XBUS_IO_SyncStatus	0x10000	/* syncronous status read */

/* Joystick commands */
#define JOYCMD_WAIT	3	/* Wait for joystick down event */


#define Offset(p_type,field) ((unsigned int)&(((p_type)NULL)->field))

/* get the address of an IOReq from the embedded io_Link address */
/* internal use only */
#define IOReq_Addr(a)	(IOReq *)((long)((int)a - (int)Offset(struct IOReq *, io_Link)))

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
	Item	io_SigItem;	/* designated reciever of the Signal */
};

/* io_Flags 0x0000ffff reserved for system */
/* io_Flags 0xffff0000 reserved for device */
#define IO_DONE		1
#define IO_QUICK	2	/* also an ioi_Flag */
#define IO_INTERNAL	4	/* used by CompleteIOR */

enum ioreq_tags
{
    CREATEIOREQ_TAG_REPLYPORT = TAG_ITEM_LAST+1,    /* optional */
    CREATEIOREQ_TAG_DEVICE			    /* required */
};
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

extern Item CreateIOReq(char *name, uint8 pri, Item dev, Item mp);/* mp can be 0 */
#define DeleteIOReq(x)	DeleteItem(x)

/* if the IO_QUICK bit is not set, the device must send this req */
/* via the normal ReplyMsg route since the application is waiting */
/* for it to appear in its ReplyPort or is waiting for the CalBack */

Err __swi(KERNELSWI+24)	SendIO(Item ior, IOInfo *ioiP); /* async*/
void __swi(KERNELSWI+25)	AbortIO(Item ior);
void __swi(KERNELSWI+34)	CompleteIO(IOReq *ior); /* privileged use only! */

int32 CheckIO(Item ior);		   /* Is io done? */
Err WaitIO(Item ior);		   /* wait for io completion */
Err DoIO(Item ior, IOInfo *ioiP);   /* sync */
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#endif	/* __IO_H */
