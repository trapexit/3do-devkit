#pragma include_only_once

/******************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: semaphore.h,v 1.23 1994/10/07 20:15:44 vertex Exp $
 **
 **  Kernel semaphore management definitions
 **
 ******************************************************************************/

#include "extern_c.h"

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "item.h"
#include "task.h"

typedef struct Semaphore
{
  ItemNode s;
  uint32	sem_bit;	/* use swap on this word */
#ifdef MULTIT
  Task	*sem_Owner;	/* Present owner */
#else
  Item	sem_Owner;	/* Present owner */
#endif
  int32	sem_NestCnt;	/* cnt of times this task has locked */
  List sem_TaskWaitingList;
} Semaphore, *SemaphoreP;


EXTERN_C_BEGIN

int32 __swi(KERNELSWI+7) LockSemaphore(Item s,uint32 flags);
Err   __swi(KERNELSWI+6) UnlockSemaphore(Item s);

extern Item CreateSemaphore(const char *name, uint8 pri);
extern Item CreateUniqueSemaphore(const char *name, uint8 pri);

EXTERN_C_END

#define FindSemaphore(n)           FindNamedItem(MKNODEID(KERNELNODE,SEMAPHORENODE),(n))
#define DeleteSemaphore(semaphore) DeleteItem(semaphore)
