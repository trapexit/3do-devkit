/*
	File:		SpawnThread.h

	Contains:	header file for Task Demo

	Written by:	Steve Hayes

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 3/18/93	JAY		first checked in

	To Do:
*/

#define CURRENT_TASK_PRIORITY (KernelBase->kb_CurrentTask->t.n_Priority)
#define CURRENT_TASK ((Item)KernelBase->kb_CurrentTask->t.n_Item)
#define THREAD_PARENT ((Item)KernelBase->kb_CurrentTask->t_ThreadTask->t.n_Item)

typedef int32 Signal;

typedef void (*ThreadProc)();
extern Item SpawnThread(ThreadProc proc, int stacksz, int pri, char * name);
