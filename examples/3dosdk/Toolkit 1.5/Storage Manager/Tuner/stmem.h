/* $Id: */

#ifndef __STMEM_H
#define __STMEM_H


/****************************************************************************/


/* Copyright (C) 1993-1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __MEM_H
#include "mem.h"
#endif


/*****************************************************************************/


void STInitMem(List *memLists);

void *STAllocMem(uint32 size, uint32 flags);
void STFreeMem(void *p);


/*****************************************************************************/


#endif /* __STMEM_H */
