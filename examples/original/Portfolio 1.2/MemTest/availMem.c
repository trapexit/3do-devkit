/*
        File:		availMem.c

        Contains:	Add up memory usage for a task.
                                Watch out for threads changing the list while
   you traverse!

        Written by:	Phil Burk

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 6/23/93	JAY		first checked in

        To Do:
*/

#include "debug.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operror.h"
#include "stdarg.h"
#include "stdio.h"
#include "strings.h"
#include "types.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      ERR (("Failure in %s: $%x\n", name, val));                              \
      PrintfSysErr (val);                                                     \
      goto cleanup;                                                           \
    }

/***************************************************************
** Add up the sizes of the nodes in a linked list.
***************************************************************/
uint32
SumListNodes (List *TheList)
{
  uint32 Sum = 0;
  Node *n;

  for (n = FirstNode (TheList); IsNode (TheList, n); n = NextNode (n))
    {
      Sum += n->n_Size;
    }
  return Sum;
}

/***************************************************************
** Count the number of nodes in a linked list.
***************************************************************/
uint32
CountListNodes (List *TheList)
{
  uint32 Count = 0;
  Node *n;

  for (n = FirstNode (TheList); IsNode (TheList, n); n = NextNode (n))
    {
      Count += 1;
    }
  return Count;
}

/***************************************************************
** Count the number of 1 bits in a long word.
***************************************************************/
/* Stolen from shell.c */
uint32
OnesCount (uint32 v)
{
  uint32 ret = 0;
  while (v)
    {
      if (v & 1)
        ret++;
      v >>= 1;
    }
  return ret;
}

/***************************************************************
** Return total memory allocated for a memlist.
***************************************************************/
uint32
TotalPageMem (MemList *ml)
{
  ulong *p = ml->meml_OwnBits;
  unsigned long ones = 0;
  int32 size;
  int i = ml->meml_OwnBitsSize;

  while (i--)
    ones += OnesCount (*p++);
  size = ones * ml->meml_MemHdr->memh_PageSize;

  return size;
}

/***************************************************************
** Find MemList of a given type.
***************************************************************/
/* Stolen from mem.c */
void *
FindML (List *l, uint32 types)
{
  Node *n;
  MemList *ml;
  uint32 mt;
  types &= MEMTYPE_VRAM | MEMTYPE_DMA | MEMTYPE_CEL;
  for (n = FIRSTNODE (l); ISNODE (l, n); n = NEXTNODE (n))
    {
      ml = (MemList *)n;
      DBUG (("FindML: test %lx nm=%s\n", ml, ml->meml_n.n_Name));
      mt = ml->meml_Types;
      mt &= types; /* mask out bits we need */
      if (types == mt)
        break; /* a match? */
    }
  if (!(ISNODE (l, n)))
    n = 0;
  return (MemList *)n;
}

/***************************************************************
** BytesOwned is total of all pages owned  of given type.
** BytesFree is sum of available memory nodes given type.
***************************************************************/
int32
SumAvailMem (List *l, uint32 Type, uint32 *pBytesOwned, uint32 *pBytesFree)
{
  MemList *ml;
  int32 Result = -1;

  ml = FindML (l, Type);
  if (ml)
    {
      *pBytesFree = SumListNodes (ml->meml_l);
      *pBytesOwned = TotalPageMem (ml);
      Result = 0;
    }
  return Result;
}

#define REPORTMEM(List, Type, Msg)                                            \
  {                                                                           \
    Result = SumAvailMem (List, Type, &BytesOwned, &BytesFree);               \
    PRT (("%s:  %8d  %8d  %8d\n", Msg, BytesOwned, BytesFree,                 \
          (BytesOwned - BytesFree)));                                         \
  }

/***************************************************************/
int32
ReportMemoryUsage (void)
{
  uint32 BytesFree, BytesOwned;
  int32 Result;

  printf ("\nMemory Type------Owned------Free----In Use\n");
  REPORTMEM (KernelBase->kb_CurrentTask->t_FreeMemoryLists, MEMTYPE_DRAM,
             "Task's DRAM");
  REPORTMEM (KernelBase->kb_CurrentTask->t_FreeMemoryLists, MEMTYPE_VRAM,
             "Task's VRAM");
  REPORTMEM (KernelBase->kb_MemFreeLists, MEMTYPE_DRAM, "Kernel DRAM");
  REPORTMEM (KernelBase->kb_MemFreeLists, MEMTYPE_VRAM, "Kernel VRAM");

  return 0;
}

uint32
TotalAvailTaskMem (void)
{
  uint32 BytesFree, BytesOwned;
  uint32 TotalBytesFree = 0;
  uint32 tempResult;

  tempResult = SumAvailMem (KernelBase->kb_CurrentTask->t_FreeMemoryLists,
                            MEMTYPE_DRAM, &BytesOwned, &BytesFree);
  TotalBytesFree += BytesFree;
  tempResult = SumAvailMem (KernelBase->kb_CurrentTask->t_FreeMemoryLists,
                            MEMTYPE_VRAM, &BytesOwned, &BytesFree);
  TotalBytesFree += BytesFree;

  return TotalBytesFree;
}

uint32
TotalAvailKernalMem (void)
{
  uint32 BytesFree, BytesOwned;
  uint32 TotalBytesFree = 0;
  uint32 tempResult;

  tempResult = SumAvailMem (KernelBase->kb_MemFreeLists, MEMTYPE_DRAM,
                            &BytesOwned, &BytesFree);
  TotalBytesFree += BytesFree;
  tempResult = SumAvailMem (KernelBase->kb_MemFreeLists, MEMTYPE_VRAM,
                            &BytesOwned, &BytesFree);
  TotalBytesFree += BytesFree;

  return TotalBytesFree;
}
