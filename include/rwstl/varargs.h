#pragma force_top_level
#pragma include_only_once

/* varargs.h: PCC 'C' library header - support for variadic function
 * Copyright (C) Advanced Risc Machines Ltd., 1995
 */

/*
 * RCS $Revision: 1.1 $
 * Checkin $Date: 1995/03/07 10:25:33 $
 * Revising $Author: enevill $
 */

/* Defines a set of macros for handling variable length argument list
 * in PCC style C. These macros rely on the compiler being able to
 * accept the ... denotation.
 */

typedef char *va_list;
#define va_alist int __va_alist, ...
#define va_dcl
#define va_start(list) list = (char *)&__va_alist
#define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]
#define va_end(list)

/* end of varargs.h */
