#pragma include_only_once

/*******************************************************************************************
 *	File:			MemPool.h
 *
 *	Contains:		definitions for MemPool.c
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	5/9/93		jb		Added ForEachFreePoolMember() convenience routine
 *	4/20/93		jb		Added field 'numFreeInPool' to MemPool for instrumentation
 *	3/30/93		jb		New today
 *
 *******************************************************************************************/

#include "extern_c.h"

#include "mem.h"

/******************************************************************/
/* Data structures for managing a pool of preallocated structures */
/******************************************************************/
struct MemPoolEntry {
  struct MemPoolEntry* next;	/* pointer to next in the list */
  char		       data[4];	/* start of user data */
};
typedef struct MemPoolEntry MemPoolEntry, *MemPoolEntryPtr;

typedef struct MemPool {
  long		  numItemsInPool; /* total number allocated */
  long		  numFreeInPool; /* current number of free entries */
  MemPoolEntryPtr availList;	/* ptr to first available entry */
  char		  data[4];	/* start of user data blocks */
} MemPool, *MemPoolPtr;


/******************************************/
/* Fixed pool memory management functions */
/******************************************/

EXTERN_C_BEGIN

typedef boolean (*ForEachFreePoolMemberFuncPtr)( void* argValue, void* poolEntry );

MemPoolPtr CreateMemPool( long numToPreallocate, long sizeOfEntry );
void       DeleteMemPool( MemPoolPtr memPool );
void      *AllocPoolMem( MemPoolPtr memPool );
void       ReturnPoolMem( MemPoolPtr memPool, void* poolEntry );
boolean    ForEachFreePoolMember(MemPoolPtr                   memPool,
                                 ForEachFreePoolMemberFuncPtr forEachFunc,
                                 void*                        argValue );

EXTERN_C_END
