/*
	File:		UMemoryDebug.c

	Contains:	Routine to display current task and kernal memory usage via debugging 
 				printf(), and routines to support that display.

	Written by:	Anup K Murarka

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 8/20/93	JAY		first checked in
  		 <9>	 7/27/93	ian		Split into separate module for inclusion in Lib3DO.
  		<8+>	 5/17/93	akm		Adding rudimentary memory leak support
  		 <8>	 5/13/93	akm		Changed abort() to Debug();
  		 <7>	 5/13/93	akm		Added eric*s changes from JPIDemo integration
  		 <6>	 5/13/93	akm		Fixed header problems for old mem check routines
  		 <5>	 5/13/93	akm		Split shared memory routines into seperate file.
  		 							Added Phil*s AvailMem code.
   		 <4>	 5/11/93	akm		Changed sharedmemory structures to work around compiler bug
  		 <3>	  5/7/93	akm		Added new implementations for shared memory routines
  		 <2>	  5/3/93	akm		Added new wrappers for ALLOCMEM and FREEMEM.
  		 							Added new getptrsize routine.
  		 <1>	  5/3/93	AKM		First Writing

	To Do:
*/

#include "types.h"
#include "debug.h"
#include "UMemory.h"

#include "Debug3DO.h"

/***************************************************************
** Add up the sizes of the nodes in a linked list.
***************************************************************/
static uint32 SumListNodes ( List *TheList )
{
	uint32 Sum = 0;
	Node *n;

	for (n = FirstNode( TheList ); IsNode( TheList, n ); n = NextNode(n) )
    {
    	Sum += n->n_Size;
	}
	return Sum;
}

/***************************************************************
** Count the number of 1 bits in a long word.
***************************************************************/
/* Stolen from shell.c */
static uint32 OnesCount( uint32 v)
{
	uint32 ret = 0;
	while (v)
	{
		if (v & 1) ret++;
		v >>= 1;
	}
	return ret;
}

/***************************************************************
** Return total memory allocated for a memlist.
***************************************************************/
static uint32 TotalPageMem( MemList *ml )
{
	ulong *p = ml->meml_OwnBits;
	unsigned long ones = 0;
	int32 size;
	int i =  ml->meml_OwnBitsSize;
	
	while (i--)
		ones += OnesCount(*p++);
	size = ones*ml->meml_MemHdr->memh_PageSize;

	return size;
}

/***************************************************************
** Find MemList of a given type.
***************************************************************/
/* Stolen from mem.c */
static void *FindML( List *l, uint32 types)
{
    Node *n;
    MemList *ml;
    uint32 mt;
    types &= MEMTYPE_VRAM|MEMTYPE_DMA|MEMTYPE_CEL;
    for (n = FIRSTNODE(l); ISNODE(l,n); n = NEXTNODE(n))
    {
        ml = (MemList *)n;
//      printf("FindML: test %lx nm=%s\n",ml,ml->meml_n.n_Name);
        mt = ml->meml_Types;
        mt &= types;    /* mask out bits we need */
        if (types == mt) break; /* a match? */
    }
    if (!(ISNODE(l,n)))     n = 0;
    return (MemList *)n;
}

/***************************************************************
** BytesOwned is total of all pages owned  of given type.
** BytesFree is sum of available memory nodes given type.
***************************************************************/
static int32 SumAvailMem( List *l, uint32 Type, uint32 *pBytesOwned, uint32 *pBytesFree )
{
	MemList *ml;
	int32 Result = -1;
	
	ml = (MemList*) FindML(l, Type);
	if (ml)
	{
		*pBytesFree = SumListNodes( ml->meml_l );
		*pBytesOwned = TotalPageMem( ml );
		Result = 0;
	}
	return Result;
}

/***************************************************************/
void (ReportMemoryUsage)( void )
{

	#define REPORTMEM(List,Type,Msg) \
	{ \
		Result = SumAvailMem( List, Type, &BytesOwned, &BytesFree); \
		printf("%s:  %8d  %8d  %8d\n", \
			Msg, BytesOwned, BytesFree, (BytesOwned-BytesFree)); \
	}
	uint32 BytesFree, BytesOwned;
	int32 Result;

	printf("\nMemory Type------Owned------Free----In Use\n");
	REPORTMEM(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_DRAM,"Task's DRAM");
	REPORTMEM(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_VRAM,"Task's VRAM");
	REPORTMEM(KernelBase->kb_MemFreeLists,
		MEMTYPE_DRAM,"Kernel DRAM");
	REPORTMEM(KernelBase->kb_MemFreeLists,
		MEMTYPE_VRAM,"Kernel VRAM");


	return;
}

#if 0 // apparently, these functions are not currently used

uint32 TotalAvailTaskMem(void)
{
	uint32	BytesFree, BytesOwned;
	uint32	TotalBytesFree=0;
	uint32	tempResult;
	
	tempResult = SumAvailMem(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
								MEMTYPE_DRAM, &BytesOwned, &BytesFree);
	TotalBytesFree += BytesFree;
	tempResult = SumAvailMem(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
								MEMTYPE_VRAM, &BytesOwned, &BytesFree);
	TotalBytesFree += BytesFree;

	return TotalBytesFree;
}

uint32 TotalAvailKernalMem(void)
{
	uint32	BytesFree, BytesOwned;
	uint32	TotalBytesFree=0;
	uint32	tempResult;
	
	tempResult = SumAvailMem(KernelBase->kb_MemFreeLists,
								MEMTYPE_DRAM, &BytesOwned, &BytesFree);
	TotalBytesFree += BytesFree;
	tempResult = SumAvailMem(KernelBase->kb_MemFreeLists,
								MEMTYPE_VRAM, &BytesOwned, &BytesFree);
	TotalBytesFree += BytesFree;

	return TotalBytesFree;
}

/***************************************************************
** Count the number of nodes in a linked list.
***************************************************************/
static uint32 CountListNodes ( List *TheList )
{
	uint32 Count = 0;
	Node *n;

	for (n = FirstNode( TheList ); IsNode( TheList, n ); n = NextNode(n) )
    {
    	Count += 1;
	}
	return Count;
}
#endif
