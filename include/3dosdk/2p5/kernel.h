#ifndef __KERNEL_H
#define __KERNEL_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: kernel.h,v 1.57 1994/12/08 18:59:54 markn Exp $
**
**  Kernel folio structure definition
**
******************************************************************************/


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

        uint32 kb_Private0[3];
        uint32 kb_CPUFlags;
        uint32 kb_Private0b[13];


	List	*kb_ExtendedErrors;	/* list of extended err tables */
	uint8  	kb_MadamRev;
	uint8  	kb_ClioRev;

        uint16  kb_Private1;
        uint32  kb_Private2[2];

	uint32	kb_NumTaskSwitches;     /* total # of switch since bootup  */

        uint32  kb_Private3[4];

	char	*kb_BootVolumeName;
	List	*kb_Tasks;

};

extern struct KernelBase *KernelBase;

#define CURRENTTASK	(KernelBase->kb_CurrentTask)
#define GetCurrentSignals()	GetTaskSignals(CURRENTTASK)
#define ClearCurrentSignals(s) ((GetCurrentSignals() & (s)) ? WaitSignal(GetCurrentSignals() & (s)) : 0)

/* kb_CPUFlags */
#define KB_NODBGR	0x8000	/* debugger is gone */
#define KB_SERIALPORT   0x00100000	/* diagnostic serial port */

/*****************************************************************************/


#endif /* __KERNEL_H */
