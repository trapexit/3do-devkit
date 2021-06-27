#ifndef __HANDY_TOOLS_H
#define __HANDY_TOOLS_H

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


#include "types.h"
#include "list.h"       /* to support SumAvailMem() prototype */

/* Structures */
typedef struct TableAllocator
{
	int32  tall_Size;
	int32  tall_Offset;
	uchar *tall_Table;
	int32  tall_Many;   /* How many have been allocated. */
} TableAllocator;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Prototypes */
int32 AllocThings( TableAllocator *tall, int32 Many, uint32 *Allocated);
int32 Choose ( int32 range );
int32 ClearThings( TableAllocator *tall);
int32 EZMemSize ( void *ptr );
int32 FreeThings( TableAllocator *tall, int32 Start, int32 Many);
int32 MarkThings( TableAllocator *tall, int32 StartIndex, int32 Many, int32 Val);
int32 PrintThings( TableAllocator *tall);
int32 EZMemSetCustomVectors( void *(*AllocVector)(int32 Size, uint32 Type),
	 void (*FreeVector)(void *p, int32 Size) );
int32 SumAvailMem( List *l, uint32 Type );
void  DumpMemory( void *addr, int32 cnt);
void  EZMemFree ( void *ptr );
void *EZMemAlloc ( int32 size, int32 type );
void *zalloc( int32 NumBytes );
void *UserMemAlloc ( int32 size, int32 type );
void UserMemFree( void *p, int32 size);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __HANDY_TOOLS_H */
