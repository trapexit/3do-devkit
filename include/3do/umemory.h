#pragma include_only_once

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

#include "extern_c.h"

#include "mem.h"

EXTERN_C_BEGIN

void *Malloc(uint32 size, uint32 memtype);
void *Free(void *ptr);

EXTERN_C_END

#define NewPtr	Malloc
#define FreePtr	Free

#define getptrsize   GetMemTrackSize
#define MemBlockSize GetMemTrackSize

#define InitMalloc(x) 0

#define ReportMemoryUsage()
#define DebugMemUsage()
