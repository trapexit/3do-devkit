/*******************************************************************************************
 *	File:			SemHelper.c
 *
 *	Contains:		wrapper routines for kernel semaphore related
 *services
 *
 *	Copyright © 1992-93 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/10/93		jb		New today
 *
 *******************************************************************************************/

#include "SemHelper.h"
#include "ThreadHelper.h" /* for CURRENT_TASK_PRIORITY */
#include "string.h"
#include "types.h"

/*******************************************************************************************
 * Routine to create a uniquely named semaphore given a name preamble string,
 *and some unique 32-bit value. Typically, the uniqueID value is an address of
 *something that is guaranteed to not be ureused until the semaphore is
 *deleted.
 *******************************************************************************************/
Item
NewSemaphore (char *baseNameString, int32 uniqueID)
{
  char nameBuf[32];

  /* Generate a unique name for the semaphore using the base name
   * string "Semaphore", and the unique ID supplied by the caller.
   */
  MakeName (nameBuf, sizeof (nameBuf), "Semaphore", uniqueID);

  return CreateSemaphore (nameBuf, CURRENT_TASK_PRIORITY);
}
