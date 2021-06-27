#ifndef __DEBUG_H
#define __DEBUG_H

/*****

$Id: debug.h,v 1.9 1994/01/21 01:07:39 limes Exp $

$Log: debug.h,v $
 * Revision 1.9  1994/01/21  01:07:39  limes
 * RCS was blown away, this brings us back to snuff.
 *
 * Old log information:
 * +  * Revision 1.10  1994/01/20  05:19:39  phil
 * +  * fixed C++ mods
 * +  *
 * +  * Revision 1.9  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 * +  *
 *
 * Revision 1.10  1994/01/20  05:19:39  phil
 * fixed C++ mods
 *
 * Revision 1.9  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.8  1993/06/10  03:27:46  dale
 * make kprintf an inline swu
 *
 * Revision 1.7  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.6  1992/12/24  00:20:29  dale
 * new debug swi call
 *
 * Revision 1.5  1992/10/24  01:10:34  dale
 * rcs again
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once
#include "types.h"

/* This pragma was removed because it is probably no longer needed
** and it apparently causes a problem for C++ */
// #pragma -v0

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*extern kprintf(const char *fmt, ... );*/
extern void __swi(0x1000e) kprintf(const char *fmt, ... );

extern void __swi(0x101) Debug(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

typedef struct dbghdr
{
	uint32	dbgLock;
	uint32	dbgReady;
} dbghdr;

typedef struct debugio
{
	uint32	reqOwner;	/* unused */
	uint32	reqCallerID;	/* unused */
	uint32	reqCommand;
	int32	*reqStatusPtr;
	uint32	ptrs[4];	/* misc other args */
} debugio;

typedef struct macFileIOWrite
{
	int32	Length;
	void*	Buffer;
} macFileIOWrite;


typedef struct macFileIORead
{
	int32	Length;
	int32	Offset;
	void*	Buffer;
} macFileIORead;

#define PCKT_ADDR	0x3ff80		/* or something */

/* streaming I/O data structures */

typedef struct MacStreamIO
{
	volatile uint8 *frame1Ptr;
	volatile uint8 *frame2Ptr;
	int32	top,left;
	int32	width,height;
	int32	modulo;	/* offset to next row */
	uint32	flags;
	
} MacStreamIO;

#define DMAFLAG_CONTINUOUS	1
#define DMAFLAG_LIVEVIDEO	2
#define DMAFLAG_STANDARD	4	/* row0,row1,row2,row3.... */

/* sent to debuger at user program abort */
#define ILLINS_ABORT	0x04
#define DATA_ABORT	0x10
#define PREFETCH_ABORT	0x0c

#endif /* __DEBUG_H */

