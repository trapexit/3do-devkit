#pragma include_only_once

/*******************************************************************************************
 *	File:			ItemPool.h
 *
 *	Contains:		definitions for ItemPool.c
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	5/18/93		jb		Switch 'unassignedList' to 'inUseList'
 *	3/29/93		jb		New today.
 *
 *******************************************************************************************/

#include "extern_c.h"

#include "types.h"
#include "item.h"

/*************************************************************/
/* Data structures for managing a pool of preallocated items */
/*************************************************************/
typedef struct ItemDesc {
  struct ItemDesc* next;	/* pointer to next in the list */
  Item		   item;	/* item number of a free item */

} ItemDesc, *ItemDescPtr;

typedef struct ItemPool {
  long	      numItemsInPool;	/* (not currently used) number allocated */
  void*	      itemDescBlockPtr;	/* ptr to block of allocated descriptors */
  ItemDescPtr availList;	/* ptr to first available item descriptor */
  ItemDescPtr inUseList;	/* ptr to first in-use item descriptor */
} ItemPool, *ItemPoolPtr;


/********************************************/
/* Routines for managing preallocated items */
/********************************************/
EXTERN_C_BEGIN

typedef Item (*CreateProcPtr)(void* createArg);

ItemPoolPtr CreateItemPool(long numToPreallocate, CreateProcPtr createProc, void *createArg);
void	    DeleteItemPool(ItemPoolPtr itemPool);
Item	    AllocPoolItem(ItemPoolPtr itemPool);
void	    ReturnPoolItem(ItemPoolPtr itemPool, Item item);

EXTERN_C_END
