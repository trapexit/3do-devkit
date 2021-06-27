#pragma include_only_once
#ifndef _handy_tools_h
#define _handy_tools_h


/*
** Handy Tools Includes
** By:  Phil Burk
**
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
