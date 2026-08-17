#pragma once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: debug.h,v 1.18 1994/10/07 19:53:57 vertex Exp $
**
******************************************************************************/

#include "extern_c.h"

#include "types.h"

EXTERN_C_BEGIN

#define DEBUGGERNODE (0)
#define DEBUGGERSWI ((DEBUGGERNODE << 16) + 0x0100)

extern void __swi(0x1000e)    kprintf(const char *fmt, ... );
extern int  __swi(0x10000+30) MayGetChar(void); /* get a char from diagport */
extern void __swi(0x101)      Debug(void);

EXTERN_C_END

#define MACNAMEBUFBYTES 128

typedef struct dbghdr
{
  u32 dbgLock;
  u32 dbgReady;
} dbghdr;

typedef struct debugio
{
  u32  reqOwner;             /* unused */
  u32  reqCallerID;          /* unused */
  u32  reqCommand;
  s32  *reqStatusPtr;
  u32  ptrs[4];              /* misc other args */
  char    namebuf[MACNAMEBUFBYTES];
} debugio;

/*****************************************************************************/
