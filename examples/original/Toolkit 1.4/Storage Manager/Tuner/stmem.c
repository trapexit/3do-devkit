/* $Id: $ */

/****************************************************************************/

/* Copyright (C) 1993-1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */

/****************************************************************************/

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
  uint32 *p;

  size += 4;

  p = (uint32 *)AllocMemFromMemLists (memLists, size, flags);

  if (p)
    {
      *p = size;
      p++;
    }

  return (p);
}

/*****************************************************************************/

void
STFreeMem (void *p)
{
  uint32 *s;

  if (p)
    {
      s = (uint32 *)p;
      s--;
      FreeMemToMemLists (memLists, s, *s);
    }
}

/*****************************************************************************/

/* Override these silly Lib3DO functions. This saves over a K in executable
 * size
 */

void *
Malloc (uint32 size, uint32 flags)
{
  return STAllocMem (size, flags);
}

void
Free (void *p)
{
  STFreeMem (p);
}

uint32
MemBlockSize (void *p)
{
  uint32 *s;

  if (p)
    {
      s = (uint32 *)p;
      s--;

      return (*s - 4);
    }

  return 0;
}
