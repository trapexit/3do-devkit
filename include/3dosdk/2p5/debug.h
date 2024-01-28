#ifndef __DEBUG_H
#define __DEBUG_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: debug.h,v 1.18 1994/10/07 19:53:57 vertex Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define DEBUGGERNODE (0)
#define DEBUGGERSWI ((DEBUGGERNODE << 16) + 0x0100)

extern void __swi(0x1000e) kprintf(const char *fmt, ... );
extern int __swi(0x10000+30) MayGetChar(void);	/* get a char from diagport */
extern void __swi(0x101) Debug(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#define MACNAMEBUFBYTES 128

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
	char    namebuf[MACNAMEBUFBYTES];
} debugio;


/*****************************************************************************/


#endif /* __DEBUG_H */
