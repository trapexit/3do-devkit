/*
	File:		Leaks.c

	Contains:	track memory allocation and free, hopefully make it 
				 easier to find memory leaks

	Written by:	Eric Carlson
					based on some code I found on Neil's disk... (oh the wonder 
					 of file sharing)

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 10/12/93	ec		Added ptr size to Leaks_Dump.  Added function
		 							 Leaks_MemBlockSize, was returning incorrect
									 size because it included leaks header fields.
		 <3>	 10/7/93	ec		a few more comments, removed compiler warnings.
		 <2>	 9/29/93	EC		couple of little syntax problems
		 <1>	 9/28/93	EC		first checked in
		 		 9/27/93	ec		first writing

	To Do:
*/
    

#include "types.h"
#include "string.h"			// for memcpy()
#include "stdio.h"			// for printf()
#include "Debug3DO.h"		// for Err()
#include "UMemory.h"		// for New/FreePtr etc

#include "Leaks.h"


#define	kMaxFileNameLen		20		// only keep first 20 chars of file name
typedef struct MemHeader
{
	struct	MemHeader	*link;						// next block in list
			uint32		memSignature;				// mem type of this block
			int32		lineNum;					// line number of source file which allocated it
			char		fileName[kMaxFileNameLen];	// name of source file which allocated it
			void		*address;					//  address of ptr returned to caller
} MemHeader, *MemHeadPtr;


// our globals
MemHeader	*gMemList;
int32		gTrackAllocs = 0;


// change this flag if you want a printf for every NewPtr/FreePtr
#define	VERBOSE_ALLOC_MODE	0

static MemHeader	*Leaks_FindMemHeader(void *address, MemHeader **prevHeader);

/*
 * Leaks_On
 *	Start tracking all memory allocations and frees.
 */
void
Leaks_On()
{
	gTrackAllocs = 1;
}


/*
 * Leaks_Off
 *	Stop tracking memory allocations and frees.
 */
void
Leaks_Off()
{
	gTrackAllocs = 0;
}


/*
 * _Leaks_Dump
 *	Private function to dump the current contents of the leak tracking table.
 *	 Use can 'call' Leaks_Dump and end up here via the ÒLeaks_DumpÓ macro
 */
void
_Leaks_Dump(int32 lineNum, char *fileName)
{
	MemHeader	*temp;
	int32		ptrSize;
	
	// print the header
	printf("\n\tMemtype-----Addr------Size--Line#---File name\n");
	
	temp = gMemList;

	if ( temp == NULL )
	{
		printf("    No blocks!\n\n");
	}
	else
	{
		while ( temp != NULL )
		{
			ptrSize = Leaks_MemBlockSize(temp->address, lineNum, fileName);
			printf("\t0x%08lX\t\t%p\t\t%04ld\t\t%ld\t%s\n", 
									temp->memSignature,
									temp->address,
									ptrSize,
									temp->lineNum,
									temp->fileName);
			temp = temp->link;
		}
	}
}


/*
 * Leaks_Malloc
 *	Allocate a block, add it to our tracking list if we're currently tracking.
 */
void *
Leaks_Malloc(uint32 bytesNeeded, uint32 memType, int32 lineNum, char *fileName)
{
	MemHeader	*memBlock;
	void		*address;
	
	// if we aren't supposed to track allocations, don't bother...
	if ( gTrackAllocs == 0 )
		return InternalNewPtr(bytesNeeded, memType);
		
	if ( (memBlock = (MemHeader *)InternalNewPtr(bytesNeeded + sizeof(MemHeader), memType)) == NULL )
	{
		ERR(("Could not allocate memory, called from %s, line %ld\n", fileName, lineNum));
		return (NULL);
	}
	
	// fill in the fields we use
	address = (void *)(((char *)memBlock) + sizeof(MemHeader));
	memBlock->address = address;
	memBlock->memSignature = memType;
	memBlock->lineNum = lineNum;
	memcpy(memBlock->fileName, fileName, kMaxFileNameLen);
	memBlock->fileName[kMaxFileNameLen - 1] = 0;
	memBlock->link = gMemList;
	
	// it is the new list head
	gMemList = memBlock;
	
	#if VERBOSE_ALLOC_MODE
		printf(stdout, "\n# Allocated block %p", memBlock->address);
	#endif
	
	return(address);
}


/*
 * Leaks_FindMemHeader
 *	Search through our linked list of memory blocks for the specified block.
 */
static MemHeader*
Leaks_FindMemHeader(void *address, MemHeader **prevHeader)	
{
	MemHeader	*current; 
	MemHeader	*previous;
	
	// look through the list for the specified ptr address
	for ( previous = NULL, current = gMemList;
			current;
			previous = current, current = current->link)
		{
			if ( current->address == address )
			{
				if ( prevHeader != NULL ) 
					*prevHeader = previous;
				return(current);
			}
		}
	
	if ( prevHeader != NULL ) 
		*prevHeader = previous;
	return(NULL);
}
	

/*
 * Leaks_MemBlockSize
 *	Return the size of the memory block, minus our header.
 */
uint32
Leaks_MemBlockSize(void *address, int32 lineNum, char *fileName)
{
	MemHeader	*prevHeader;
	MemHeader	*memBlock;
	
	// look for the address in the global list
	if ( (memBlock = Leaks_FindMemHeader(address, &prevHeader)) == NULL )
	{
		// if we aren't supposed to track allocations, don't bother complaining...
		if ( gTrackAllocs != 0 )
		{
			ERR(("Attempt to get size of un-accounted for memory from %s, line %ld\n", fileName, lineNum));
			ERR(("calling MemBlockSize on block, hope it works...\n\n"));
		}
		return InternalMemBlockSize(address);
	}
	else
	{
		// return the size of the block, minus our header as that is the size they
		//  asked for in the first place
		return (InternalMemBlockSize(memBlock) - sizeof(MemHeader));
	}

}


/*
 * Leaks_Free
 *	Remove a block from our linked list and free it.
 */
void 
*Leaks_Free(void *address, int32 lineNum, char *fileName)
{
	MemHeader	*prevHeader;
	MemHeader	*memBlock;
	
		
	// look for the address in the global list
	if ( (memBlock = Leaks_FindMemHeader(address, &prevHeader)) == NULL )
	{
		// if we aren't supposed to track allocations, don't bother complaining...
		if ( gTrackAllocs != 0 )
		{
			ERR(("Attempt to free un-accounted for memory from %s, line %ld\n", fileName, lineNum));
			ERR(("Calling FreePtr on block, hope it works...\n\n"));
		}
		InternalFreePtr(address);
	}
	else
	{
		// link the previous block around the one we're freeing
		if ( prevHeader != NULL ) 
			prevHeader->link = memBlock->link;
		else 
			gMemList = memBlock->link;
			
		#if VERBOSE_ALLOC_MODE
			printf(stdout, "\n# Freed block %p", memBlock->address);
		#endif
		
		InternalFreePtr(memBlock);
	}
	
	return(NULL);
}

