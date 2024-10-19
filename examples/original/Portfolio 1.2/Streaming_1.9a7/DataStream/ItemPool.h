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

#ifndef __ITEMPOOL_H__
#define __ITEMPOOL_H__

#ifndef _TYPES_H
#include "Types.h"
#endif

#ifndef _ITEM_H
#include "Item.h"
#endif

/*************************************************************/
/* Data structures for managing a pool of preallocated items */
/*************************************************************/
typedef struct ItemDesc
{
  struct ItemDesc *next; /* pointer to next in the list */
  Item item;             /* item number of a free item */

} ItemDesc, *ItemDescPtr;

typedef struct ItemPool
{
  long numItemsInPool;    /* (not currently used) number allocated */
  void *itemDescBlockPtr; /* ptr to block of allocated descriptors */
  ItemDescPtr availList;  /* ptr to first available item descriptor */
  ItemDescPtr inUseList;  /* ptr to first in-use item descriptor */

} ItemPool, *ItemPoolPtr;

/********************************************/
/* Routines for managing preallocated items */
/********************************************/
#ifdef __cplusplus
extern "C"
{
#endif

  typedef Item (*CreateProcPtr) (void *createArg);

  ItemPoolPtr CreateItemPool (long numToPreallocate, CreateProcPtr createProc,
                              void *createArg);
  void DeleteItemPool (ItemPoolPtr itemPool);
  Item AllocPoolItem (ItemPoolPtr itemPool);
  void ReturnPoolItem (ItemPoolPtr itemPool, Item item);

#ifdef __cplusplus
}
#endif

#endif /* __ITEMPOOL_H__ */
