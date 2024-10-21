
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: stmem.c,v 1.2 1994/11/18 18:04:47 vertex Exp $
**
******************************************************************************/

#include "stmem.h"
#include "mem.h"
#include "types.h"

/*****************************************************************************/

List *memLists;

/*****************************************************************************/

void
STInitMem (List *lists)
{
  if (lists)
    memLists = lists;
  else
    memLists = CURRENTTASK->t_FreeMemoryLists;
}

/*****************************************************************************/

void *
STAllocMem (uint32 size, uint32 flags)
{
  return AllocMemFromMemLists (memLists, size, flags | MEMTYPE_TRACKSIZE);
}

/*****************************************************************************/

void
STFreeMem (void *p)
{
  FreeMemToMemLists (memLists, p, -1);
}
