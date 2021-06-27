#ifndef __UMEMORY_H
#define __UMEMORY_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: umemory.h,v 1.12 1995/02/21 00:04:11 stan Exp $
**
**  Lib3DO compatibility header. Do not use in new code.
**
******************************************************************************/

#include "mem.h"

#ifdef __cplusplus
  extern "C" {
#endif

void *Malloc(uint32 size, uint32 memtype);
void *Free(void *ptr);

#ifdef __cplusplus
  }
#endif

#define NewPtr	Malloc
#define FreePtr	Free

#define getptrsize   GetMemTrackSize
#define MemBlockSize GetMemTrackSize

#define InitMalloc(x) 0

#define ReportMemoryUsage()
#define DebugMemUsage()

/*****************************************************************************/


#endif /* __UMEMORY_H */
