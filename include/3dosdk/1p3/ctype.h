#ifndef __CTYPE_H
#define __CTYPE_H

/*****

$Id: ctype.h,v 1.6 1994/01/21 02:04:04 limes Exp $

$Log: ctype.h,v $
 * Revision 1.6  1994/01/21  02:04:04  limes
 * recover from RCS bobble:
 *
 * +  * Revision 1.7  1994/01/20  01:51:52  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.6  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 * +  *
 *
 * Revision 1.7  1994/01/20  01:51:52  sdas
 * C++ compatibility - updated
 *
 * Revision 1.6  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.5  1992/11/25  06:17:21  dale
 * remove redundant ifdefs __ctype_h
 *
 * Revision 1.4  1992/10/24  01:10:34  dale
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

/* ctype.h: ANSI 'C' (X3J11 Oct 88) library header, section 4.3 */
/* Copyright (C) Codemist Ltd.					*/
/* Copyright (C) Advanced Risc Machines Ltd., 1991		*/
/* version 0.02 */

/*
 * ctype.h declares several functions useful for testing and mapping
 * characters. In all cases the argument is an int, the value of which shall
 * be representable as an unsigned char or shall equal the value of the
 * macro EOF. If the argument has any other value, the behaviour is undefined.
 */

/* N.B. - keep in step with <ctype.c> */

#define __S 1		 /* whitespace		 */
#define __P 2		 /* punctuation 	 */
#define __B 4		 /* blank		 */
#define __L 8		 /* lower case letter	 */
#define __U 16		 /* upper case letter	 */
#define __N 32		 /* (decimal) digit	 */
#define __C 64		 /* control chars	 */
#define __X 128 	 /* A-F and a-f 	 */
extern unsigned const char __ctype[];

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define isalnum(c) (__ctype[c] & (__U+__L+__N))
extern int (isalnum)(int c);
    /* non-0 iff c is alphabetic or numeric */

#define isalpha(c) (__ctype[c] & (__U+__L))
extern int (isalpha)(int c);
    /* non-0 iff c is alphabetic */

#define iscntrl(c) (__ctype[c] & __C)
extern int (iscntrl)(int c);
    /* non-0 iff c is a control character - in the ASCII locale */
    /*	     this means (c < ' ') || (c > '~')			*/

#define isdigit(c) (__ctype[c] & __N)
extern int (isdigit)(int c);
    /* non-0 iff c is a decimal digit */

#define isgraph(c) (__ctype[c] & (__L+__U+__N+__P))
extern int (isgraph)(int c);
    /* non-0 iff c is any printing character other than ' ' */

#define islower(c) (__ctype[c] & __L)
extern int (islower)(int c);
    /* non-0 iff c is a lower-case letter */

#define isprint(c) (__ctype[c] & (__L+__U+__N+__P+__B))
extern int (isprint)(int c);
    /* non-0 iff c is a printing character - in the ASCII locale */
    /*	     this means 0x20 (space) -> 0x7E (tilde) */

#define ispunct(c) (__ctype[c] & __P)
extern int (ispunct)(int c);
    /* non-0 iff c is a non-space, non-alpha-numeric, printing character */

#define isspace(c) (__ctype[c] & __S)
extern int (isspace)(int c);
    /* non-0 iff c is a white-space char: ' ', '\f', '\n', '\r', '\t', '\v'. */

#define isupper(c) (__ctype[c] & __U)
extern int (isupper)(int c);
    /* non-0 iff c is an upper-case letter */

#define isxdigit(c) (__ctype[c] & (__N+__X))
extern int (isxdigit)(int c);
    /* non-0 iff c is a digit, in 'a'..'f', or in 'A'..'F' */

extern int tolower(int c);
    /* if c is an upper-case letter then return the corresponding */
    /* lower-case letter, otherwise return c.			  */

extern int toupper(int c);
    /* if c is an lower-case letter then return the corresponding */
    /* upper-case letter, otherwise return c.			  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ctype.h */
