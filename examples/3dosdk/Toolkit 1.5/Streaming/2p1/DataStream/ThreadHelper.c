/*******************************************************************************************
 *	File:			ThreadHelper.c
 *
 *	Contains:		wrapper routines for kernel thread related
 *services
 *
 *	Copyright © 1992-3 The 3DO Company. All Rights Reserved.
 *
 *	History:
 * 	  7/29/94	dtc		Version 2.0.1d2
 *						Fixed NewThread to make the thread name
 *provided, unique. 10/19/93	jb		Version 1.5 Switch to using
 *UMemory.c routines for memory allocation 8/9/93		jb
 *Version 1.4 Changed DEFAULT_MAXQ to 0x4000 for Dragontail3 release as
 *required. 7/10/93		jb		Version 1.3 Switch to using
 *MakeName() routine and get rid of non-reentrant hack that we had before for
 *generating unique names. Now, when we create a thread with a unique name, we
 *use the thread's stack memory address as a postfix to the string "Thread" to
 *name it. 6/1/93		jb		Version 1.2.3 Remove all
 *variable initializers in local variable declarations to avoid register
 *bashing by the ARM C compiler.
 *	4/18/93		jb		Now that thread names do not appear to
 *be optional, create an unspeakable hack to generate thread names so the
 *system won't have to strain itself doing this for us...
 *	4/18/93		jb		Make thread name optional (omit from
 *tag argument list) if specified name string pointer is NULL.
 *	4/16/93		jb		Switch to using NewThread() &
 *DisposeThread() to avoid collisions with new system wrapper functions in 3B1.
 *	3/30/93		jb		New today, taken from SHayes 8/10/92
 *example code
 *
 *******************************************************************************************/

#include "ThreadHelper.h"
#include "Portfolio.h"
#include "UMemory.h"

#define DEFAULT_MAXQ 0x4000 /* for CREATETASK_TAG_MAXQ tag arg */

/*******************************************************************************************
 * Routine to create a new thread of execution given the proc pointer, stack
 *size, thread priority, and thread name. Returns the thread's Item or error.
 *******************************************************************************************/
Item
NewThread (void *threadProcPtr, int32 stackSize, int32 threadPriority,
           char *threadName, void **stackMemoryPtr, int32 argc, void *argp)
{
  Item thread;
  char *stackMemory;
  long alignedStackSize;
  TagArg tags[9];
  char nameBuf[32];

  /* Align stack memory block size to QUADBYTE boundary
   */
  alignedStackSize = (stackSize + 3) & ~3;

  /* Allocate the stack space for the thread. First, set the returned
   * stack block pointer to NULL in case the thread creation fails so that
   * the caller can determine whether or not to free the space.
   */
  *stackMemoryPtr = 0;
  stackMemory = (char *)NewPtr (alignedStackSize,
                                MEMTYPE_ANY); /* allocate stack space */
  if (stackMemory == NULL)
    return (Item)0;

  *stackMemoryPtr = stackMemory;

  /* Fill out the arguments and create the item
   */
  tags[0].ta_Tag = TAG_ITEM_PRI; /* Priority */
  tags[0].ta_Arg = (void *)threadPriority;

  tags[1].ta_Tag = CREATETASK_TAG_PC; /* Initial PC */
  tags[1].ta_Arg = (void *)threadProcPtr;

  tags[2].ta_Tag = CREATETASK_TAG_STACKSIZE; /* Stack size */
  tags[2].ta_Arg = (void *)alignedStackSize;

  tags[3].ta_Tag = CREATETASK_TAG_SP; /* initial stack pointer */
  tags[3].ta_Arg = stackMemory + alignedStackSize;

  tags[4].ta_Tag = CREATETASK_TAG_ARGC; /* initial args count */
  tags[4].ta_Arg = (void *)argc;

  tags[5].ta_Tag = CREATETASK_TAG_ARGP; /* initial args pointer */
  tags[5].ta_Arg = (void *)argp;

  tags[6].ta_Tag = CREATETASK_TAG_MAXQ; /* ??? what does this do ???? */
  tags[6].ta_Arg = (void *)DEFAULT_MAXQ;

  tags[7].ta_Tag = TAG_ITEM_NAME; /* thread name */
  if (threadName == NULL)
    /* Generate a unique name using the thread's stack
     * memory address to guarantee uniqueness
     */
    tags[7].ta_Arg = (void *)MakeName (nameBuf, sizeof (nameBuf), "Thread",
                                       (int32)stackMemory);
  else
    tags[7].ta_Arg = (void *)MakeName (nameBuf, sizeof (nameBuf), threadName,
                                       (int32)stackMemory);
  //		tags[7].ta_Arg = (void *) threadName;

  tags[8].ta_Tag = TAG_END;

  thread = CreateItem (MKNODEID (KERNELNODE, TASKNODE), tags);
  if (thread < 0)
    {
      FreePtr (stackMemory);
      *stackMemoryPtr = 0;
    }

  return thread;
}

/*******************************************************************************************
 * Routine to delete a previously created thread.
 *******************************************************************************************/
int32
DisposeThread (Item threadItem)
{
  return DeleteItem (threadItem);
}
