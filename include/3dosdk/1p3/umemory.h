/*
    File:       UMemory.h

    Contains:   Header file for new memory unit that adds minimal intelligence
                        to the memory allocation scheme available on 3DO.  Basic
                        operation is handled by storing the size of the block in the
                        word preceding the actual pointer returned to the client.
                        This allows fast deletion of the memory without having to
                        remember the size.  This unit is occasionally more useful
                        that Mem3DO as there is no additional overhead in list or
                        tag processing for the memory blocks.

    Written by: Anup K Murarka

    Copyright:  © 1993 by The 3DO Company. All rights reserved.
                This material constitutes confidential and proprietary
                information of the 3DO Company and shall not be used by
                any Person or for any purpose except as expressly
                authorized in writing by the 3DO Company.

    Change History (most recent first):

				 9/9/93		ian		Added DebugMemoryUsage().  It is the same as ReportMemoryUsage()
				 					except that it disappears when DEBUG compilation flag is set
									to 0.
         <6>     8/2/93     ian     Added handling of page-aligned VRAM allocations.
                                    Added new standardized names for functions, (Malloc/Free)
                                    with macros to support the old names (NewPtr/FreePtr).
        <5+>     5/17/93    akm     Adding rudimentary memory leak detection
         <5>     5/13/93    akm     Split shared memory code into seperate file Added Phil's
                                    AvailMemory code
         <4>     5/11/93    akm     Changed shared memory structures to work around possible
                                    compiler bug
         <3>      5/7/93    akm     Added new implementations for shared memory routines
         <2>      5/3/93    akm     Adding new headers for NewPtr & FreePtr.  Added stubs for shared
                                    memory utilities.  Moved definition of nil & kZeroedMemType.
                                    Now include mem.h automatically.
         <1>      5/3/93    akm     First Writing

    To Do:
*/

#ifndef UMEMORY_H
#define UMEMORY_H

#pragma force_top_level
#pragma include_only_once

#include "mem.h"

#define nil NULL
#define kZeroedMemType  (MEMTYPE_ANY | MEMTYPE_FILL | 0x00)

#ifdef __cplusplus 
extern "C" {
#endif


// Primary allocation routines

void *	Malloc(uint32 size, uint32 typebits);
void *	Free(void * ptr);
uint32	MemBlockSize(void * ptr);

int32	InitMalloc(uint32 numVRAMTableEntries);

// Alternate names for allocation routines
// (These are for backwards compatibility, don't use them in new code)

void *	NewPtr(uint32 size, uint32 typebits);
void *	FreePtr(void * ptr);
uint32	getptrsize(void * ptr);

void ReportMemoryUsage(void);

#if DEBUG
  void DebugMemoryUsage(void);
#else
  #define DebugMemoryUsage()
#endif

#ifdef __cplusplus
}
#endif

#endif /* UMEMORY_H */
