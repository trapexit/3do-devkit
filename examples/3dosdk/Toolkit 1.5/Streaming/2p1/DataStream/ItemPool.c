/*******************************************************************************************
 *	File:			ItemPool.c
 *
 *	Contains:		Routines for managing a pool of preallocated
 *items
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	  7/1/94	dtc		Version 2.0.1d1
 *						Bug# 2181: Fixed CreateItemPool to handle
 *a null allocation of block of item descriptors. 10/19/93	jb
 *Version 1.3 Switch to using UMemory.c routines for memory allocation 6/1/93
 *jb		Version 1.2.3 Remove all variable initializers in local
 *variable declarations to avoid register bashing by the ARM C compiler.
 *	5/18/93		jb		Switch 'unassignedList' to 'inUseList'.
 *Add tracking of "owned" items by keeping an "in-use" list and verifying the
 *existence of items being returned to the pool. 3/29/93		jb
 *New today.
 *
 *******************************************************************************************/

#include "ItemPool.h"
#include "Portfolio.h"
#include "UMemory.h"

/*******************************************************************************************
 * Routine to create a pool of preallocated Items. These are managed by a
 *couple of other routines to allocate and free these items, once created. This
 *is a way to dynamically allocate/free items without incurring any substantial
 *overhead.
 *******************************************************************************************/
ItemPoolPtr
CreateItemPool (long numToPreallocate, CreateProcPtr createProc,
                void *createArg)
{
  ItemPoolPtr itemPool;
  ItemDescPtr idp;

  /* Allocate the pool description record */

  itemPool = (ItemPoolPtr)NewPtr (sizeof (ItemPool), MEMTYPE_ANY);
  if (itemPool != NULL)
    {
      /* Initialize a few fields, and allocate the block of
       * item descriptors that we'll build our allocation
       * list from.
       */
      itemPool->numItemsInPool = numToPreallocate; /* (not used) */
      itemPool->inUseList = NULL;
      itemPool->availList = NULL;

      itemPool->itemDescBlockPtr
          = NewPtr (numToPreallocate * sizeof (ItemDesc), MEMTYPE_ANY);

      /* Bug# 2181:
       * Check to see if allocation of block of item descriptors was
       * successful
       */
      if (itemPool->itemDescBlockPtr == NULL)
        return NULL;

      /* Allocate the specified number of Items by calling the supplied
       * creation function, and store each in a item descriptor. Link
       * these into an allocation list.
       */
      idp = (ItemDescPtr)(itemPool->itemDescBlockPtr);
      while (numToPreallocate-- > 0)
        {
          /* Create a new pool Item by calling the user supplied
           * function to do so. The functions creates whatever type of
           * item is being pooled.
           */
          idp->item = (*createProc) (createArg);
          if (idp->item <= 0)
            break;

          /* Stuff the new descriptor onto the front of
           * the available list. This is where the allocator
           * looks for available Item descriptors.
           */
          idp->next = itemPool->availList;
          itemPool->availList = idp;

          /* Advance the pointer to the next slot in the
           * table of descriptors we allocated above.
           */
          idp++;
        }
    }

  return itemPool;
}

/*******************************************************************************************
 * Routine to delete all resources associated with an item pool.
 * NOTE: we do not track Items that are currently not in the pool.
 *******************************************************************************************/
void
DeleteItemPool (ItemPoolPtr itemPool)
{
  ItemDescPtr idp;

  /* Delete all known Items */
  for (idp = itemPool->availList; idp != NULL; idp = idp->next)
    DeleteItem (idp->item);

  FreePtr (itemPool->itemDescBlockPtr);
  FreePtr (itemPool);
}

/*******************************************************************************************
 * Routine to get a free Item from a pool of preallocated Items. The items are
 *kept in a singly linked list of descriptors. As items are allocated, the
 *descriptors are linked into a list of unassigned descriptors. When an Item is
 *returned, an unassigned descriptor is allocated, the Item is stored in it,
 *and the desriptor is linked back into the list of available item descriptors
 *for subsequent use.
 *******************************************************************************************/
Item
AllocPoolItem (ItemPoolPtr itemPool)
{
  ItemDescPtr idp;
  Item item;

  item = 0;

  /* Allocate an available item from the pool
   */
  if ((idp = itemPool->availList) != NULL)
    {
      /* Get the item number out of this item descriptor as the
       * one we'll return to the caller as an available Item.
       * Delete the descriptor from the front of the available
       * descriptor list.
       */
      item = idp->item;
      itemPool->availList = idp->next;

      /* Place the descriptor onto the front of the
       * in-use list.
       */
      idp->next = itemPool->inUseList;
      itemPool->inUseList = idp;
    }

  return item;
}

/*******************************************************************************************
 * Routine to return an item pool Item back to the pool's list of available
 *Items.
 *
 * NOTE:	WE IGNORE ANY ATTEMPT TO RETURN AN ITEM THAT WE DON'T OWN !!!!
 *
 *******************************************************************************************/
void
ReturnPoolItem (ItemPoolPtr itemPool, Item item)
{
  ItemDescPtr idp;
  ItemDescPtr lastPtr;

  lastPtr = NULL;
  idp = itemPool->inUseList;
  while (idp != NULL)
    {
      /* If we find the entry we're looking for, delete it from
       * the "in-use" list and add it back to the "available" list.
       */
      if (item == idp->item)
        {
          if (lastPtr == NULL)
            {
              /* Remove in-use item from the head of
               * the "in-use" queue.
               */
              itemPool->inUseList = idp->next;
            }
          else
            {
              /* Remove in-use item from the middle of
               * the list someplace.
               */
              lastPtr->next = idp->next;
            }

          /* Add the item descriptor to the front of the
           * available descriptor list.
           */
          idp->next = itemPool->availList;
          itemPool->availList = idp;
          break;
        }

      /* Advance to next entry in the list
       */
      lastPtr = idp;
      idp = idp->next;
    }
}
