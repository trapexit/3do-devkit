/*
        File:		SpawnThread.c

        Contains:	Task Demo

        Written by:	Jay Moreland,lynn ackler,philippe cassereau

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <3>	  8/6/93	JAY		changing
   CREATETASK_TAG_MAXQ initial value to 0x4000. Before Dragon Tail 3 kernel use
   to see 2 and change it to 0x4000. DT3 just issues an error (BAD_QUANTA) <2>
   3/18/93	JAY		changing lists.h to list.h for Cardinal build
                 <1>	 3/18/93	JAY		first checked in
                                 8/10/92	SH		My First Task
   (Thread) Demo

        To Do:
*/

#include "SpawnThread.h"
#include "debug.h" /* for kprintf() */
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "task.h"
#include "types.h"
#define CURRENT_TASK_PRIORITY (KernelBase->kb_CurrentTask->t.n_Priority)
#define THREAD_PARENT                                                         \
  ((Item)KernelBase->kb_CurrentTask->t_ThreadTask->t.n_Item)

/* symbolic tag indeces for the TagArgs array */
static enum TASKTAGIX {
  TID_PRI,
  TID_MAXQ,
  TID_PC,
  TID_NAME,
  TID_STACKSIZE,
  TID_SP
};

Item
SpawnThread (ThreadProc proc, int stacksz, int pri, char *name)
{
  Item myThread;
  char *sb;
  TagArg tags[] = { TAG_ITEM_PRI,
                    (void *)0,
                    CREATETASK_TAG_MAXQ,
                    (void *)0x4000,
                    CREATETASK_TAG_PC,
                    (void *)0,
                    TAG_ITEM_NAME,
                    0,
                    CREATETASK_TAG_STACKSIZE,
                    (void *)0,
                    CREATETASK_TAG_SP,
                    (void *)0,
                    TAG_END,
                    0 };
  sb = (char *)ALLOCMEM (stacksz,
                         MEMTYPE_ANY); /* need to make our own stack */
  if (!sb)
    {
      kprintf ("Could not allocate stack memory for %s\n", name);
      return (Item)0;
    }
  tags[TID_PRI].ta_Arg = (void *)pri;           /* Set thread's priority*/
  tags[TID_PC].ta_Arg = (void *)proc;           /* Set thread's initial pc*/
  tags[TID_NAME].ta_Arg = (void *)name;         /* Name is optional	*/
  tags[TID_STACKSIZE].ta_Arg = (void *)stacksz; /* Stack size		*/
  tags[TID_SP].ta_Arg = sb + stacksz;           /* stacks grown down 	*/
  /* The programmer interface is really done via Items. */
  myThread = CreateItem (MKNODEID (KERNELNODE, TASKNODE), tags);
  if (myThread < 0)
    {
      kprintf ("Launch of '%n' failed\n", name);
      FREEMEM (sb, stacksz);
    }
  return myThread;
}
