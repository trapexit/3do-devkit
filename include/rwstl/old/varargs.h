/*
 * Copyright (c) 1991 Advanced RISC Machines Ltd., Cambridge, England
 */

#pragma force_top_level
#pragma include_only_once

#ifndef __varargs_h
#define __varargs_h

typedef  char *va_list[1];    /* Not conventional - for compatibility with */
                              /* our stdarg.h. Usually just char *va_list; */
#define  va_dcl               int va_alist;
#define  va_start(list)       list[0] = (char *)&va_alist
#define  va_end(list)
#define  va_arg(list,mode)    (((mode *)(list[0] += sizeof(mode)))[-1])

#endif
