#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

/*****

$Id: semaphore.h,v 1.14 1994/01/21 02:28:17 limes Exp $

$Log: semaphore.h,v $
 * Revision 1.14  1994/01/21  02:28:17  limes
 * recover from rcs bobble
 *
 * Revision 1.15  1994/01/20  02:10:14  sdas
 * C++ compatibility - updated
 *
 * Revision 1.14  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.13  1994/01/14  05:06:26  dale
 * added DeleteSemaphore
 *
 * Revision 1.12  1993/08/13  03:36:53  dale
 * added FindSemaphore
 *
 * Revision 1.11  1993/06/09  00:43:15  andy
 * Padded semaphore wait to 16 byte multiple
 *
 * Revision 1.10  1993/03/16  06:45:35  dale
 * api
 *
 * Revision 1.9  1993/02/17  04:43:21  dale
 * added simple library routines for creating Items
 *
 * Revision 1.8  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.7  1993/01/28  17:04:46  dale
 * UnlockSemaphore used wrong swi numer!
 *
 * Revision 1.6  1992/10/24  01:43:54  dale
 * rcs
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
#include "list.h"
#include "item.h"

typedef struct SemaphoreWaitNode
{
	NamelessNode swn;
	uint32	swn_sig;	/* Signal to send to task waiting */
#ifdef MULTIT
	Task * swn_Task;
#else
	Item swn_Task;
#endif
	uint32 swn_Pad[2];	/* pad to 32 bits */
} SemaphoreWaitNode, *SemaphoreWaitNodeP;

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

  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

int32 __swi(KERNELSWI+7)	LockSemaphore(Item s,uint32 flags);
Err __swi(KERNELSWI+6)	UnlockSemaphore(Item s);

#define FindSemaphore(n)     FindNamedItem(MKNODEID(KERNELNODE,SEMAPHORENODE),(n))

#define braindeadcompiler

#ifndef braindeadcompiler
enum semaphore_tags
{			/* only standard item tags used right now */
};
#endif
extern Item CreateSemaphore(char *name, uint8 pri);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#define DeleteSemaphore(semaphore)	DeleteItem(Semaphore)

#endif /* __SEMAPHORE_H */
