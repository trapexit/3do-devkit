#ifndef __VARARGS_H
#define __VARARGS_H

/*****

$Id: varargs.h,v 1.5 1994/01/21 02:33:07 limes Exp $

$Log: varargs.h,v $
 * Revision 1.5  1994/01/21  02:33:07  limes
 * recover from rcs bobble
 *
 * Revision 1.6  1994/01/20  02:16:36  sdas
 * C++ compatibility - updated
 *
 * Revision 1.5  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.4  1992/10/24  01:46:09  dale
 * rcsa
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

typedef char *va_list;
#define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]
/*#define va_start(list,va_list) list = (char *)&va_list*/
#define va_start(list,va_list) list = (char *)&(va_list) + sizeof(va_list)
#define va_end(list)

#endif /* __VARARGS_H */
