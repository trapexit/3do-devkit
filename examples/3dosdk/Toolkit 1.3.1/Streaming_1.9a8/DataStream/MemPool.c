/*******************************************************************************************
 *	File:			MemPool.c
 *
 *	Contains:		Pool oriented memory manager. Manages preallocated
 *pools of fixed sized objects. Free objects are singly linked together into a
 *free list using the "-4" offset of the objects. Objects are reused in LIFO
 *order (add and remove from list head only) for maximum performance.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/19/93	jb		Version 1.3
 *						Switch to using UMemory.c routines for
 *memory allocation 6/1/93		jb		Version 1.2.3 Remove
 *all variable initializers in local variable declarations to avoid register
 *bashing by the ARM C compiler. 5/9/93		jb		Added
 *ForEachFreePoolMember() convenience routine
 *	4/20/93		jb		Added field 'numFreeInPool' to MemPool
 *for instrumentation 4/18/93		jb		Fix cast in
 *CreateMemPool() 3/30/93		jb		New today
 *
 *******************************************************************************************/

#include "MemPool.h"
#include "Portfolio.h"
#include "UMemory.h"

#define ROUND_TO_LONG(x) ((x + 3) & ~3)

/*******************************************************************************************
 * Routine to create a new allocation pool. Allocates a single block of size
 *				numToPreallocate * (sizeOfEntry + 4) +
 *sizeof(MemPool) and links them together using the extra space. Note that
 *entry data size is rounded UP to a multiple of 4 BYTES.
 *******************************************************************************************/
MemPoolPtr
CreateMemPool (long numToPreallocate, long sizeOfEntry)
{
  MemPoolPtr pool;
  MemPoolEntryPtr entryPtr;
  MemPoolEntryPtr lastEntryPtr;
  long poolBlockSize;
  long actualEntrySize;

  /* Entry sizes must be a multiple of long words to insure alignment
   * for linkage fields.
   */
  actualEntrySize = ROUND_TO_LONG (sizeOfEntry) + sizeof (MemPoolEntryPtr);

  /* Allocate the block of memory that will hold the header and all
   * of the entry blocks.
   */
  poolBlockSize = sizeof (MemPool) + (numToPreallocate * actualEntrySize);
  pool = (MemPoolPtr)NewPtr (poolBlockSize, MEMTYPE_ANY);

  if (pool != NULL)
    {
      pool->numItemsInPool = numToPreallocate;
      pool->numFreeInPool = numToPreallocate;

      /* Make a linked list of entries
       */
      entryPtr = (MemPoolEntryPtr)&pool->data;
      lastEntryPtr = NULL;
      while (numToPreallocate-- > 0)
        {
          entryPtr->next = lastEntryPtr;
          lastEntryPtr = entryPtr;

          entryPtr = (MemPoolEntryPtr)(((char *)entryPtr) + actualEntrySize);
        }

      /* Point to the first entry in the free list */
      pool->availList = lastEntryPtr;
    }

  return pool;
}

/*******************************************************************************************
 * Routine to delete an allocation pool.
 *******************************************************************************************/
void
DeleteMemPool (MemPoolPtr memPool)
{
  FreePtr (memPool);
}

/*******************************************************************************************
 * Routine to allocate an entry from an allocation pool. Returns a pointer to
 *the data portion of the entry.
 *******************************************************************************************/
void *
AllocPoolMem (MemPoolPtr memPool)
{
  MemPoolEntryPtr entry;

  /* Pull the first available entry from the free list */
  entry = memPool->availList;

  /* If we actually found one, update the free list head
   * to point to the next available entry.
   */
  if (entry != NULL)
    {
      memPool->availList = entry->next;
      entry = (MemPoolEntryPtr)&entry->data;

      memPool->numFreeInPool--;
    }

  return entry;
}

/*******************************************************************************************
 * Routine to return an entry  previously allocated with AllocPoolMem() to an
 * allocation pool.
 *******************************************************************************************/
void
ReturnPoolMem (MemPoolPtr memPool, void *poolEntry)
{
  MemPoolEntryPtr entry;

  /* Wicked cast to back up the pointer we are handed to the REAL
   * beginning of the type of structure we manage.
   */
  entry = (MemPoolEntryPtr)(((char *)poolEntry)
                            - ((char *)&(((MemPoolEntryPtr)0)->data)));

  /* Make the returning entry point to the first entry
   * in the free list.
   */
  entry->next = memPool->availList;

  /* Now point the free list at the returning entry
   */
  memPool->availList = entry;

  memPool->numFreeInPool++;
}

/*******************************************************************************************
 * Routine to iterate through each _free_ member in a memory pool and call a
 *user specified function. The user function is called with a pointer to the
 *data portion of the pool entry. Handy for initialization of all members in a
 *pool, or for whatever makes sense in terms of operating on only the _free_
 *members in the pool.
 *******************************************************************************************/
Boolean
ForEachFreePoolMember (MemPoolPtr memPool,
                       ForEachFreePoolMemberFuncPtr forEachFunc,
                       void *argValue)
{
  MemPoolEntryPtr entry;

  for (entry = memPool->availList; entry != NULL; entry = entry->next)
    if (false == (*forEachFunc) (argValue, (void *)&entry->data))
      return false;

  return true;
}
