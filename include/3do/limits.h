#ifndef __LIMITS_H
#define __LIMITS_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: limits.h,v 1.3 1994/09/10 01:22:35 peabody Exp $
**
******************************************************************************/


#define CHAR_BIT    8
#define SCHAR_MIN  -0x80
#define SCHAR_MAX   0x7f
#define UCHAR_MAX   0xff
#define CHAR_MIN    0x00      /* Portfolio assumes unsigned chars */
#define CHAR_MAX    0xff
#define SHRT_MIN   -0x8000
#define SHRT_MAX    0x7fff
#define USHRT_MAX   0xffff
#define INT_MIN    -0x80000000
#define INT_MAX     0x7fffffff
#define UINT_MAX    0xffffffff
#define LONG_MIN   -0x80000000
#define LONG_MAX    0x7fffffff
#define ULONG_MAX   0xffffffff
#define MB_LEN_MAX  2


/*****************************************************************************/


#endif /* __LIMITS_H */
