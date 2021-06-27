/*******************************************************************************************
 *	File:			MemPool.h
 *
 *	Contains:		definitions for MemPool.c
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	5/9/93		jb		Added ForEachFreePoolMember() convenience routine
 *	4/20/93		jb		Added field 'numFreeInPool' to MemPool for instrumentation
 *	3/30/93		jb		New today
 *
 *******************************************************************************************/

#ifndef	__MEMPOOL_H__
#define	__MEMPOOL_H__

#ifndef _MEM_H
#include "mem.h"
#endif

/******************************************************************/
/* Data structures for managing a pool of preallocated structures */
/******************************************************************/
struct MemPoolEntry {
	struct MemPoolEntry*	next;		/* pointer to next in the list */
	char					data[4];	/* start of user data */
	};
typedef struct MemPoolEntry MemPoolEntry, *MemPoolEntryPtr;

typedef struct MemPool {
	long			numItemsInPool;		/* total number allocated */
	long			numFreeInPool;		/* current number of free entries */
	MemPoolEntryPtr	availList;			/* ptr to first available entry */
	char			data[4];			/* start of user data blocks */
	} MemPool, *MemPoolPtr;


/******************************************/
/* Fixed pool memory management functions */
/******************************************/
typedef Boolean (*ForEachFreePoolMemberFuncPtr)( void* argValue, void* poolEntry );

MemPoolPtr	CreateMemPool( long numToPreallocate, long sizeOfEntry );
void		DeleteMemPool( MemPoolPtr memPool );
void*		AllocPoolMem( MemPoolPtr memPool );
void		ReturnPoolMem( MemPoolPtr memPool, void* poolEntry );
Boolean		ForEachFreePoolMember( MemPoolPtr memPool, 
						ForEachFreePoolMemberFuncPtr forEachFunc, void* argValue );

#endif	/* __MEMPOOL_H__ */
