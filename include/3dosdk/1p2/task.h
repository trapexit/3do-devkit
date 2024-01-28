#ifndef __TASK_H
#define __TASK_H

/*****

$Id: task.h,v 1.35 1994/01/21 18:27:45 sdas Exp $

$Log: task.h,v $
 * Revision 1.35  1994/01/21  18:27:45  sdas
 * TasksLinkNode appended to struct Task
 *
 * Revision 1.35  1994/01/20  02:13:47  sdas
 * C++ compatibility - updated
 *
 * Revision 1.34  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.33  1993/12/23  23:59:53  sdas
 * Added tag CREATETASK_TAG_ALLCDTHREADSP and t_Flag TASK_ALLOCATED_SP
 *
 * Revision 1.32  1993/12/10  01:47:49  dale
 * cleanup of Signal function return values
 *
 * Revision 1.31  1993/11/05  03:27:45  limes
 * get autodocs out of include files, for now.
 *
 * Revision 1.30  1993/11/05  03:25:09  limes
 * Autodoc Merge
 *
 * Revision 1.29  1993/11/05  03:07:25  limes
 * *** empty log message ***
 *
 * Revision 1.28  1993/08/13  02:20:55  dale
 * added FindTask macro
 *
 * Revision 1.27  1993/08/03  17:53:08  dale
 * changed t_MaxUsec to t_MaxUSecs, like the 3dobin header
 *
 * Revision 1.26  1993/07/29  09:13:42  dale
 * comments
 *
 * Revision 1.25  1993/07/29  08:53:07  dale
 * TASK_DATADISK_OK, and now have new t_Flags field.
 *
 * Revision 1.24  1993/06/29  07:21:48  andy
 * added new tag for createtask
 *
 * Revision 1.23  1993/06/18  20:58:51  dale
 * remove comment about allocating stack before calling CreateThread
 *
 * Revision 1.22  1993/06/09  00:43:15  andy
 * Added List for stacks associated with this task
 *
 * Revision 1.21  1993/05/28  00:54:28  andy
 * needed ssl var for superstack limit storage
 *
 * Revision 1.1  1993/05/27  17:46:54  andy
 * Initial revision
 *
 * Revision 1.20  1993/05/08  05:09:15  dale
 * c3
 *
 * Revision 1.19  1993/04/01  04:43:50  dale
 * signals being screwed down
 *
 * Revision 1.18  1993/03/16  06:48:43  dale
 * api
 *
 * Revision 1.17  1993/03/08  22:47:10  dale
 * renamed ThisTasksSignals to GetCurrentSignals.
 *
 * Revision 1.16  1993/03/05  03:58:54  dale
 * new macro ThisTasksSignals()
 *
 * Revision 1.15  1993/03/05  03:49:56  dale
 * new Signal return value
 *
 * Revision 1.14  1993/02/17  05:22:57  dale
 * new CreateThread routine
 *
 * Revision 1.13  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.12  1992/11/18  00:06:31  dale
 * RSA stuff
 *
 * Revision 1.11  1992/10/24  01:46:09  dale
 * rcsa
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
#include "item.h"
#include "list.h"
#include "time.h"

/* t.n_Flags */
#define TASK_READY   1	/* task in readyq waiting for cpu */
#define TASK_WAITING 2	/* task in waitq waiting for a signal */
#define TASK_RUNNING 4	/* task has cpu */
#define TASK_SUPER   8	/* task is privileged */

/* t_Flags */
#define TASK_DATADISCOK	1	/* This task tolerates data disks */
#define TASK_ALLOCATED_SP	2	/* This threadtask's sp was created */
					/* by convenience routine */

/* Some bits in the SigMask are predefined */
/* All functions must be able to handle an error return */
/* from Wait() */

/* Task Priorities go from 1 to 254 */
/* 0 and 255 are reserved for the systems use */

#define TASK_MAX_PRIORITY 254
#define TASK_MIN_PRIORITY 1

typedef struct Task
{
	ItemNode t;
	struct Task	*t_ThreadTask;	/* Am I a thread of what task? */
	Item	*t_ResourceTable;	/* list of Items we need to clean up */
	int32	t_ResourceCnt;	/* maxumum number of slots in ResourceTable */
	uint32	t_WaitBits;	/* What will wake us up */
	uint32	t_SigBits;
	uint32	t_AllocatedSigs;
	uint32	*t_StackBase;	/* ptr to Base of stack */
	int32	t_StackSize;
	uint32	t_regs[13];	/* all current mode registers */
	uint32	*t_sp;	/* r13 */
	uint32	t_lk;	/* r14 */
	uint32	t_pc;	/* r15 */
	uint32	t_psr;		/* program status register */
	uint32	t_Userpsr;	/* " */
	uint32	*t_ssp;		/* ptr to supervisor stack */
	uint32	*t_Usersp;	/* in case we are in super mode */
	uint32	t_Userlk;	/* " */
	uint32	t_Reserved;
	int32	t_SuperStackSize;
	uint32	*t_SuperStackBase;
	uint32	*t_ObsoleteMemProtectBits;	/* mem we can write to */
			/* above moved to MemList */
	List	*t_FreeMemoryLists;	/* task free memory pool */
	void	**t_FolioData;	/* preallocated ptrs for */
				/* the first 4 folios */

	uint32	t_FolioContext;	/* 32 bits */
	struct timeval t_ElapsedTime;

	uint32	t_MaxTicks;	/* max tics b4 task switch */
	uint32	t_MaxUSecs;	/* Equivalent in usecs */
	uint32	*t_PageTable;	/* Arm600 page table for this task */
	uint32	*t_ssl;		/* ptr to supervisor stack */
	List	*t_UserStackList; /* List of available user stacks for this process */
	uint32	t_Flags;	/* more task specific flags */
	MinNode	t_TasksLinkNode;/* Link to the list of all tasks */
} Task, *TaskP;

/* The resource table has some bits packed in the upper */
/* bits of the Item, since Items will max out at 4096 */
#define ITEM_WAS_OPENED	0x4000
#define ITEM_NOT_AN_ITEM	0x80000000

/* predefined signals */

#define SIGF_MEMLOW	2	/* first warning of low memory */
#define SIGF_MEMGONE	1	/* major memory exhaustion */

#define SIGF_ABORT	4	/* abort current operation */
/* By default, this signal is orred into all Wait requests */
/* so all callers of wait must be ready to deal with an */
/* abnormal return */

#define SIGF_IODONE	8	/* Polled IO signal */

#define SIGF_DEADTASK	0x10	/* signal sent to parent of dead task */

/* Tag Args for CreateTask */
enum task_tags
{
	CREATETASK_TAG_PC = TAG_ITEM_LAST+1,
	CREATETASK_TAG_MAXQ,
	CREATETASK_TAG_STACKSIZE,
	CREATETASK_TAG_ARGC,	/* initial r0 */
	CREATETASK_TAG_ARGP,	/* initial r1 */
	CREATETASK_TAG_SP,	/* Makes task a thread */
	CREATETASK_TAG_BASE,	/* initial r9 */
	CREATETASK_TAG_IMAGE,	/* OBSOLETE TAG was: load image starts at */
	CREATETASK_TAG_IMAGESZ,	/* size of initial load image */
	CREATETASK_TAG_AIF,	/* points to AIF header */
	CREATETASK_TAG_CMDSTR,	/* ptr to command string to stack */
	CREATETASK_TAG_SUPER,	/* make task privileged */
	CREATETASK_TAG_RSA,	/* force RSA/MD4 processing */
	CREATETASK_TAG_USERONLY,	/* Create a task with no super stack */
	CREATETASK_TAG_ALLOCDTHREADSP	/* thread stack was allocated */
};
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

extern Item CreateThread(char *name, uint8 pri, void (*code)(),int32 stacksize);
extern Err DeleteThread(Item x);

extern int32 __swi(KERNELSWI+1) WaitSignal(uint32 sigMask);
extern Err __swi(KERNELSWI+2)   SendSignal(Item task,uint32 sigMask);
extern void __swi(KERNELSWI+9)   Yield(void);
extern int32 __swi(KERNELSWI+21)	AllocSignal(uint32 sigMask);
extern Err __swi(KERNELSWI+22)	FreeSignal(uint32 sigMask);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#define GetCurrentSignals()	(KernelBase->kb_CurrentTask->t_SigBits)
#define FindTask(n)	FindNamedItem(MKNODEID(KERNELNODE,TASKNODE),(n))

#endif /* __TASK_H */
