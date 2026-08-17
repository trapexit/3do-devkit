#pragma once

/****************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: handy_tools.h,v 1.15 1994/09/10 00:17:48 peabody Exp $
 **
 **  Handy Tools
 **
 **  By: Phil Burk
 **
 ****************************************************************************/

#include "extern_c.h"

#include "types.h"
#include "list.h"       /* to support SumAvailMem() prototype */

/* Structures */
typedef struct TableAllocator
{
  s32  tall_Size;
  s32  tall_Offset;
  uchar *tall_Table;
  s32  tall_Many;   /* How many have been allocated. */
} TableAllocator;

EXTERN_C_BEGIN

/* Prototypes */
s32 AllocThings(TableAllocator *tall, s32 Many, u32 *Allocated);
s32 Choose(s32 range);
s32 ClearThings(TableAllocator *tall);
s32 EZMemSize(void *ptr);
s32 FreeThings(TableAllocator *tall, s32 Start, s32 Many);
s32 MarkThings(TableAllocator *tall, s32 StartIndex, s32 Many, s32 Val);
s32 PrintThings(TableAllocator *tall);
s32 EZMemSetCustomVectors(void *(*AllocVector)(s32 Size, u32 Type),
                            void (*FreeVector)(void *p, s32 Size));
s32 SumAvailMem(List *l, u32 Type);
void  DumpMemory(void *addr, s32 cnt);
void  EZMemFree(void *ptr);
void *EZMemAlloc(s32 size, s32 type);
void *zalloc(s32 NumBytes);
void *UserMemAlloc(s32 size, s32 type);
void UserMemFree(void *p, s32 size);

EXTERN_C_END
