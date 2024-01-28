/*
        File:		TaskDemo.c

        Contains:	Task Demo (Thread)

        Written by:	Steve Hayes

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <3>	 3/18/93	JAY		changing Wait to
   WaitSignal and Signal to SendSignal. Wait and Signal where changed <2>
   3/18/93	JAY		chaging lists.h to list.h for Cardinal build
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

/* Globals for communication between thread and it's parent */
static ulong parentSignal, threadSignal;
static int threadMod = 0;

void
Thread (void)
{
  Item myParent = THREAD_PARENT;
  /* Signals must be allocated in the task that is to wait on them.*/
  threadSignal = AllocSignal (0);
  while (1)
    {
      ++threadMod;
      SendSignal (myParent,
                  parentSignal); /* Tell parent task that data is now ready */
      WaitSignal (threadSignal); /* Wait until data is consumed by parent */
    }
}

int
main (int argc, char **argv)
{
  long ix;
  Item it;
  kprintf ("Spawning thread\n");
  parentSignal = AllocSignal (0);
  it = SpawnThread (Thread, 128, CURRENT_TASK_PRIORITY, "Steve's Thread");
  kprintf ("Spawned thread item %x\n", it);
  for (ix = 0; ix < 100; ix++)
    {
      int my;
      WaitSignal (parentSignal); /* Thread has something for me */
      my = threadMod; /* This is the only safe place to read 'threadMod' */
      SendSignal (it, threadSignal); /* Thread's turn to do something */
      if ((ix % 10) == 0)
        kprintf ("\n");
      kprintf (" %d",
               my); /* 'my' could differ from 'threadMod' at this point */
    }
  FreeSignal (parentSignal);
  kprintf (" Bye!\n");
  return 0; /* Cleanup is largely left to the system */
}
