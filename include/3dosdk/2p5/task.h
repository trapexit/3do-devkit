#ifndef __TASK_H
#define __TASK_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: task.h,v 1.61 1995/02/02 22:23:05 limes Exp $
**
**  Kernel task management definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __NODES_H
#include "nodes.h"
#endif

#ifndef __KERNELNODES_H
#include "kernelnodes.h"
#endif

#ifndef __ITEM_H
#include "item.h"
#endif

#ifndef __LIST_H
#include "list.h"
#endif

#ifndef __TIME_H
#include "time.h"
#endif


/* t.n_Flags */
#define TASK_READY   1	/* task in readyq waiting for cpu */
#define TASK_WAITING 2	/* task in waitq waiting for a signal */
#define TASK_RUNNING 4	/* task has cpu */
#define TASK_SUPER   8	/* task is privileged */

/* t_Flags */
#define TASK_DATADISCOK	1	/* This task tolerates data disks */
#define TASK_ALLOCATED_SP	2	/* This threadtask's sp was created */
					/* by convenience routine */
#define	TASK_EXITING		4	/* The task is now exiting */

/* Some bits in the SigMask are predefined */
/* All functions must be able to handle an error return */
/* from Wait() */

/* priorities go from 1 to 254 for privileged tasks */
#define PRIVTASK_MIN_PRIORITY 1
#define PRIVTASK_MAX_PRIORITY 254

/* priorities go from 10 to 199 for non-privileged tasks */
#define TASK_MIN_PRIORITY 10
#define TASK_MAX_PRIORITY 199

typedef struct Task
{
	ItemNode t;
	struct Task	*t_ThreadTask;	/* Am I a thread of what task? */
	uint32   t_Private0[2];
	uint32	t_WaitBits;	/* What will wake us up */
	uint32	t_SigBits;
	uint32	t_AllocatedSigs;
	uint32	*t_StackBase;	/* ptr to Base of stack */
	int32	t_StackSize;
	uint32  t_Private1[22];
	int32	t_SuperStackSize;
	uint32	*t_SuperStackBase;
	Item     t_WaitItem;            /* Item this task is waiting on */
	List	*t_FreeMemoryLists;	/* task free memory pool */
	uint32   t_Private2[2];
	struct timeval t_ElapsedTime;

	uint32	t_MaxTicks;	/* max tics b4 task switch */
	uint32	t_MaxUSecs;	/* Equivalent in usecs */
	uint32  t_Private3[2];
	uint32  t_NumTaskLaunch; /* # of times this task has been launched */
	uint32	t_Flags;	/* more task specific flags */
	MinNode	t_TasksLinkNode;/* Link to the list of all tasks */
} Task, *TaskP;


/* predefined signals */

#define SIGF_MEMLOW	2	/* first warning of low memory */
#define SIGF_MEMGONE	1	/* major memory exhaustion */

#define SIGF_ABORT	4	/* abort current operation */
/* By default, this signal is orred into all Wait requests */
/* so all callers of wait must be ready to deal with an */
/* abnormal return */

#define SIGF_IODONE	8	/* Polled IO signal */

#define SIGF_DEADTASK	0x10	/* signal sent to parent of dead task */
#define SIGF_ONESHOT	0x20	/* signal used for internal communication */

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
	CREATETASK_TAG_UNUSED,
	CREATETASK_TAG_IMAGESZ,	/* size of initial load image */
	CREATETASK_TAG_AIF,	/* points to AIF header */
	CREATETASK_TAG_CMDSTR,	/* ptr to command string to stack */
	CREATETASK_TAG_PRIVATE0,
	CREATETASK_TAG_RSA,	/* force RSA/MD4 processing */
	CREATETASK_TAG_USERONLY,	/* Create a task with no super stack */
	CREATETASK_TAG_ALLOCDTHREADSP,	/* thread stack was allocated */
	CREATETASK_TAG_MSGFROMCHILD	/* send status msg to parent on exit */
};

/* get the address of a Task from the embedded t_TasksLinkNode address */
/* internal use only */
#define Task_Addr(a)	(Task *)((uint32)(a) - Offset(Task *, t_TasksLinkNode))

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern Item CreateThread(const char *name, uint8 pri, void (*code)(),int32 stacksize);
extern Err DeleteThread(Item x);

extern int32 __swi(KERNELSWI+1) WaitSignal(uint32 sigMask);
extern Err __swi(KERNELSWI+2)   SendSignal(Item task,uint32 sigMask);
extern void __swi(KERNELSWI+9)   Yield(void);
extern int32 __swi(KERNELSWI+21)	AllocSignal(uint32 sigMask);
extern Err __swi(KERNELSWI+22)	FreeSignal(uint32 sigMask);
extern Err __swi(KERNELSWI+39)	SetExitStatus(int32 status);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#define GetTaskSignals(t)     ((t)->t_SigBits)
#define FindTask(n)           FindNamedItem(MKNODEID(KERNELNODE,TASKNODE),(n))


#endif /* __TASK_H */
