#ifndef __KERNEL_H
#define __KERNEL_H

/*****

$Id: kernel.h,v 1.44 1994/03/24 21:15:29 dplatt Exp $

$Log: kernel.h,v $
 * Revision 1.44  1994/03/24  21:15:29  dplatt
 * Move ReportEvent out of kernelbase itself, make it a kernel-folio
 * vector.
 *
 * Revision 1.43  1994/03/15  04:33:43  dale
 * diagnostic serial port
 *
 * Revision 1.42  1994/03/11  21:03:53  dplatt
 * Add kernelbase hook for async-event-reporting subroutine.
 *
 * Revision 1.41  1994/03/01  18:31:47  sdas
 * GetCurrentSignals() redefined using GetTaskSignals()
 *
 * Revision 1.40  1994/02/28  21:15:36  sdas
 * GetCurrentSignals() moved from task.h for include consistency
 *
 * Revision 1.39  1994/01/28  23:20:18  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.38  1994/01/21  18:24:19  sdas
 * Tasks list appended to KernelBase
 *
 * Revision 1.38  1994/01/20  02:01:03  sdas
 * C++ compatibility - updated
 *
 * Revision 1.37  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.36  1993/09/09  04:15:00  dale
 * Removed Reboot swi
 *
 * Revision 1.35  1993/07/30  23:58:55  dale
 * removed kb_MSysBits, changed to kb_Obsolete
 *
 * Revision 1.34  1993/06/23  23:58:59  dale
 * BootVolumeName passed in from dipir
 *
 * Revision 1.33  1993/06/12  10:51:48  dale
 * secret ptrs for unused VRAM
 *
 * Revision 1.32  1993/06/11  20:08:59  dale
 * added VRAM0 so sport device can share the memory
 *
 * Revision 1.31  1993/06/09  00:43:15  andy
 * Added new KernelBase fields, and padded to 16 byte multiple.
 *
 * Revision 1.30  1993/05/31  00:47:11  dale
 * new item table architecture
 *
 * Revision 1.29  1993/05/17  00:49:15  andy
 * added defines for red and green revs
 *
 * Revision 1.28  1993/05/17  00:44:18  andy
 * switched to Clio and Madam version numbers
 *
 * Revision 1.27  1993/05/16  23:56:54  andy
 * changed bits to split madam/clio revs
 *
 * Revision 1.26  1993/05/08  05:09:29  dale
 * c3
 *
 * Revision 1.25  1993/03/28  19:47:19  dale
 * Brooktree support
 * Reboot support
 * checked in by Stan
 *
 * Revision 1.24  1993/03/26  18:26:15  dale
 * Reboot swi
 *
 * Revision 1.23  1993/03/17  00:09:45  dale
 * added BROOKTREE
 *
 * Revision 1.22  1993/03/16  06:50:10  dale
 * api
 *
 * Revision 1.21  1993/03/02  03:18:44  dale
 * green
 *
 * Revision 1.20  1993/02/19  02:12:07  dale
 * new kernel routines, slack timer, TagProcessor
 *
 * Revision 1.19  1993/02/09  00:32:55  dale
 * KB_REDWW
 *
 * Revision 1.18  1993/01/28  17:12:49  dale
 * better tuning for timer ticks.
 *
 * Revision 1.17  1992/12/22  11:08:58  dale
 * KB_CONTROLPORT
 *
 * Revision 1.16  1992/12/16  21:55:50  dale
 * printf and syserr
 *
 * Revision 1.15  1992/12/16  04:56:56  dale
 * changes for modified memory allocator
 *
 * Revision 1.14  1992/12/13  23:41:02  dale
 * support for QuietAbort handling
 *
 * Revision 1.13  1992/12/09  21:21:22  dale
 * ces roms stuff
 *
 * Revision 1.12  1992/12/07  09:20:10  dale
 * CES
 *
 * Revision 1.11  1992/12/04  22:00:59  dale
 * KB_RED
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

#include "types.h"
#include "item.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "setjmp.h"

struct KernelBase
{
	Folio kb;
	List *kb_RomTags;
	List *kb_MemFreeLists;
	List *kb_MemHdrList;
	List *kb_FolioList;	/* Libraries */
	List *kb_Drivers;
	List *kb_Devices;
	List *kb_TaskWaitQ;	/* Tasks waiting for some event */
	List *kb_TaskReadyQ;/* Tasks waiting for CPU time */
	List *kb_MsgPorts;	/* will we be message based? */
	List *kb_Semaphores;	/* will we be message based? */
	Task *kb_CurrentTask;	/* Currently executing Task */
	Node **kb_InterruptHandlers;
	uint32 kb_TimerBits;	/* allocated timers/ctrs */
	uint32 kb_ElapsedQuanta;	/* timerticks for current task */
	uint32 *kb_VRAMHack;
	ItemEntry **kb_ItemTable;	/* table of ptrs to ItemEntries */
	int32   kb_MaxItem;
	uint32 kb_CPUFlags;	/* various flags for operation */

	uint8 kb_MaxInterrupts;
	uint8 kb_Forbid;	/* software lockout for task swapping */

	uint8 kb_FolioTableSize;
	uint8 kb_PleaseReschedule;
	uint32 *kb_MacPkt;
	uint32 kb_Flags;
	uint32 kb_Reserved;
	uint32 kb_numticks;	/* convert secs to ticks numerator */
	uint32 kb_denomticks;	/* convert secs to ticks denominator */
	uint32 kb_Obsolete;	/* shadow copy of Madam->Msysbits */
	uint8  kb_FolioTaskDataCnt;	/* lwords */
	uint8  kb_FolioTaskDataSize;	/* lwords */
	uint8  kb_DRAMSetSize;
	uint8  kb_VRAMSetSize;
	struct Folio **kb_DataFolios;
	jmp_buf	*kb_CatchDataAborts;	/* setjmp buf */
	uint32 kb_QuietAborts;		/* no messages for these bits */
	uint32 *kb_RamDiskAddr;		/* kernel needs to help RamDevice */
	int32	kb_RamDiskSize;
	List	*kb_ExtendedErrors;	/* list of extended err tables */
	uint8  	kb_MadamRev;
	uint8  	kb_ClioRev;
	uint8	kb_Resbyte0;
	uint8	kb_Resbyte1;
	Item	kb_DevSemaphore;	/* Device List Semaphore */
	List	*kb_SystemStackList;	/* List of System stacks available */
	uint32	kb_Pad;
	uint32	*kb_VRAM0;		/* memory reserved by kernel */
	uint32	kb_VRAM0Size;
	uint32	*kb_VRAM1;
	uint32	kb_VRAM1Size;
	char	*kb_BootVolumeName;
	List	*kb_Tasks;		/* All Tasks in the system */
};

#ifndef KERNEL
extern struct KernelBase *KernelBase;
#endif

#define CURRENTTASK	(KernelBase->kb_CurrentTask)
#define GetCurrentSignals()	GetTaskSignals(CURRENTTASK)

#define MAXFOLIOS	16
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

extern void *AllocateKernelNode(int);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

/* kb_CPUFlags */

#define MADAM_GREENWW   0x01020001

#define KB_BIGENDIAN	1	/* we are Big Endian, 0=Little Endian */
#define KB_32BITMODE	2	/* 32 Bit Address Operation, 0=26 bit */
#define KB_ARM600	4	/* this is an ARM600 */
#define KB_SHERRY	8	/* new hardware? */
#define KB_SHERRIE	KB_SHERRY

#define KB_BLUE		0x10
#define KB_RED		0x20
#define KB_REDWW	0x40	/* Red Wire Wrap, not same as silicon */

#define KB_GREEN	0x80	/* temporary compatibility alias */

#define KB_WIREWRAP	0x100	/* Green (or above) wirewrap flag */

#define KB_NODBGR	0x8000	/* debugger is gone */
#define KB_NODBGRPOOF	0x4000	/* debugger is gone , just kidding */
#define KB_CONTROLPORT	0x1000	/* control port found (MEI) */

#define KB_BROOKTREE	0x00010000	/* Brooktree (not philips) */
#define KB_SERIALPORT   0x00100000	/* diagnostic serial port */
#define KB_REBOOTED	0x01000000

/* other kernelbase flags */
#define KB_TASK_DBG	1	/* debug out for createtask */

#define REDMHZx1000	50000	/* 50mhz * 1000 red silicon */
#define MHZx1000	49091	/* 49.0908mhz * 1000 */
#define REDWWMHZx1000	26820	/* 26.82mhz */

#define CHIP_RED_REV 0
#define CHIP_GREEN_REV 1

#endif /* __KERNEL_H */
