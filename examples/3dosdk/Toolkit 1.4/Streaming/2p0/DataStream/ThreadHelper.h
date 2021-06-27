/*******************************************************************************************
 *	File:			ThreadHelper.h
 *
 *	Contains:		definitions for ThreadHelper.c
 *
 *	Copyright © 1992-93 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/10/93		jb		Include MakeName.h because we now use it for thread name generation.
 *	4/16/93		jb		Switch to using NewThread() & DisposeThread() to avoid
 *						collisions with new system wrapper functions in 3B1.
 *	4/16/93		jb		Switch to using "Item.h" from "Items.h" in 3B1
 *	3/30/93		jb		New today, taken from SHayes 8/10/92 example code
 *
 *******************************************************************************************/

#ifndef __THREADHELPER_H__
#define __THREADHELPER_H__

#ifndef _TYPES_H
#include "Types.h"
#endif

#ifndef _ITEMS_H
#include "Item.h"
#endif

#ifndef _KERNEL_H
#include "Kernel.h"
#endif

#ifndef __MAKENAME_H__
#include "MakeName.h"
#endif

/* The following are useful for calling these helper routines and in writing
 * thread code. The current task priority is useful for creating new threads in 
 * that you usually want to specify the new thread's priority as a value relative
 * to the creator task (higher or lower than the current task). The thread parent
 * is useful to threads that wish to communicate via with the parent task via:
 * AllocSignal(), Signal(), Wait(), & FreeSignal().
 */
#define CURRENT_TASK_PRIORITY	(KernelBase->kb_CurrentTask->t.n_Priority)
#define THREAD_PARENT			((Item)KernelBase->kb_CurrentTask->t_ThreadTask->t.n_Item)

/***********************************/
/* Prototypes for thread functions */
/***********************************/

Item	NewThread( void* threadProcPtr, int32 stackSize, int32 threadPriority, 
						char* threadName, void** stackMemoryPtr, int32 argc, void* argp );

int32	DisposeThread( Item threadItem );

#endif

