/*****************************************************************************
 *	File:			UMemory.c 
 *
 *	Contains:		Malloc() and Free() functions.
 *
 *	Copyright:	(c) 1993-1995 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	02/14/95  Ian 	Complete rewrite.  The OS's new MEMTYPE_TRACKSIZE 
 *					allocation flag	now provides 99% of the functionality
 *					this module used to implement.  This module provides
 *					the remaining (nearly trivial) 1%.
 *					
 *	Implementation notes:
 *
 *	These functions now just call through to the OS's memory routines.  They
 *	do still have to exist as functions, because:
 *
 *	- They implement the ANSI-defined behaviors of memory management
 *	  (Malloc(0) returns NULL and Free(NULL) is valid).
 *
 *	- As an extension to the ANSI requirements, Free() returns NULL, and
 *	  existing code relies on this documented behavior of our Free().
 *
 *	- Existing code references these functions using the address-of operator,
 *	  and &Malloc and &Free just don't work when Malloc/Free are macros.
 ****************************************************************************/
 
#include "umemory.h"

void * Malloc(uint32 size, uint32 memtype)
{
	if (size == 0) {
		return NULL;
	} else {
		return AllocMem(size, memtype|MEMTYPE_TRACKSIZE);
	}
}
  
void * Free(void *ptr)
{
	if (ptr != NULL) {
		FreeMem(ptr,-1);
	}
	
	return NULL;
}
  
