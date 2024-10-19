/*****************************************************************************
 *	File:		UMemory.h
 *
 *	Contains:   Header file for new memory unit that adds minimal
 *intelligence to the memory allocation scheme available on 3DO.
 *
 *	Written by: Ian Lepore
 *
 *	Copyright:	(c) 1993-1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History (most recent first):
 *	07/11/94  Ian	Standarized protective wrapper macro name to UMEMORY_H.
 *					Standardized comment blocks.
 *	09/09/93  Ian	Added DebugMemoryUsage().  It is the same as
 *ReportMemoryUsage() except that it disappears when DEBUG compilation flag is
 *set to 0. 08/02/93  Ian	Added handling of page-aligned VRAM
 *allocations. Added new standardized names for functions, (Malloc/Free) with
 *macros to support the old names (NewPtr/FreePtr). 05/17/93  akm	Adding
 *rudimentary memory leak detection
 *	05/13/93  akm	Split shared memory code into seperate file Added
 *Phil's AvailMemory code 05/11/93  akm	Changed shared memory structures to
 *work around possible compiler bug
 *	05/07/93  akm	Added new implementations for shared memory routines
 *	05/03/93  akm	Adding new headers for NewPtr & FreePtr.  Added stubs
 *for shared memory utilities.  Moved definition of nil & kZeroedMemType. Now
 *include mem.h automatically. 5/3/93    akm	First Writing
 *
 *	Implementation notes:
 *
 *	Basic operation is handled by storing the size of the block in
 *	the word preceding the actual pointer returned to the client.
 *	This allows fast deletion of the memory without having to
 *	remember the size.
 *
 *	WARNING:
 *		The ReportMemoryUsage() and DebugMemoryUsage() commands are
 *		FOR DEVELOPMENT/DEBUGGING ONLY!  They should NEVER be included
 *		in final release code.  The functions walk OS internal data
 *		structures in relatively unsafe ways.  The OS could modify the
 *		lists during the walk, leading to walking to nowhere, crashing,
 *		etc.  Acceptable risk for debugging, but terribly unsafe for
 *		a final product.
 *
 ****************************************************************************/

#ifndef UMEMORY_H
#define UMEMORY_H

#include "mem.h"

#ifndef nil
#define nil NULL
#endif

#define kZeroedMemType (MEMTYPE_ANY | MEMTYPE_FILL | 0x00)

#ifdef __cplusplus
extern "C"
{
#endif

  // Primary allocation routines

  void *Malloc (uint32 size, uint32 typebits);
  void *Free (void *ptr);
  uint32 MemBlockSize (void *ptr);

  int32 InitMalloc (uint32 numVRAMTableEntries);

  // Alternate names for allocation routines
  // (These are for backwards compatibility, don't use them in new code)

  void *NewPtr (uint32 size, uint32 typebits);
  void *FreePtr (void *ptr);
  uint32 getptrsize (void *ptr);

  void ReportMemoryUsage (void);

#if DEBUG
  void DebugMemoryUsage (void);
#else
#define DebugMemoryUsage()
#endif

#ifdef __cplusplus
}
#endif

#endif
