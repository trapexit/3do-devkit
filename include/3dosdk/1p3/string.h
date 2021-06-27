#ifndef __STRING_H
#define __STRING_H

/*****

$Id: string.h,v 1.20 1994/04/06 16:45:21 sdas Exp $

$Log: string.h,v $
 * Revision 1.20  1994/04/06  16:45:21  sdas
 * This check-in is really log-message addition to the last check-in
 * CR 2152 : Going towards ANSI; strtol() and strtoul() moved to stdlib.h
 *           stdlib.h is being included temporarily for compatibility with
 *           existing source
 *
 * Revision 1.19  1994/04/06  16:42:15  sdas
 * CR-2153 : Going towards ANSI; str*cmp() now have first two params declared
 *           as const char *
 *
 * Revision 1.18  1994/04/04  19:01:18  limes
 * first arg for strcmp is const
 *
 * Revision 1.17  1994/01/28  23:20:24  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.16  1994/01/21  02:29:22  limes
 * recover from rcs bobble
 *
 * Revision 1.17  1994/01/20  02:12:12  sdas
 * C++ compatibility - updated
 *
 * Revision 1.16  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.15  1993/12/16  20:44:09  dale
 * corrent strncasecmp!
 *
 * Revision 1.14  1993/12/16  19:49:10  dale
 * added #define for strncasecmp to strcasecmp.
 *
 * Revision 1.13  1993/12/15  00:32:53  sdas
 * Added macro for SetMem(). Previously a function in lib/vectors.c
 *
 * Revision 1.12  1993/12/08  02:37:44  limes
 * Add decl for FindLSB and some comments.
 *
 * Revision 1.11  1993/09/27  22:55:31  dale
 * added FindMSB, ffs, CountBits, popc.
 *
 * Revision 1.10  1993/08/13  03:55:32  dale
 * Added macro for CopyMem
 *
 * Revision 1.9  1993/06/18  01:42:58  andy
 * string ifdef
 *
 * Revision 1.8  1993/06/15  03:54:24  andy
 * change to strcasecmp
 *
 * Revision 1.7  1993/01/13  04:13:06  dale
 * atoi moved to stdlib.h
 *
 * Revision 1.6  1993/01/02  04:23:23  dale
 * added atoi()
 *
 * Revision 1.5  1992/12/01  02:03:44  dale
 * added strncasecmp
 *
 * Revision 1.4  1992/11/22  21:28:44  dale
 * fix warnings
 *
 * Revision 1.3  1992/10/24  01:43:54  dale
 * rcs
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
#include "stdlib.h"	/* temporarily left for application src compatibility */
			/* strtol(),strtoul() in stdlib.h used to be here */
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

extern void *memcpy(void * /* s1 */, const void * /*s2*/, size_t /*n*/ );
	/* copies n chars from s2 to s1 */
extern void *memmove(void * /* s1 */, const void * /*s2*/, size_t /*n*/ );
	/* copies n chars from s2 to s1 */

extern char *strcpy(char * /*s1*/, const char * /*s2*/);
	/* copies string from s2 to s1 */
extern char *strncpy(char * /*s1*/, const char * /*s2*/, size_t /*n*/);
	/* copies string from s2 to s1, at most n chars copied */

extern char *strcat(char * /*s1*/, const char * /*s2*/);
	/* appends string s2 to s1 */
extern char *strncat(char * /*s1*/, const char * /*s2*/, size_t /*n*/);
	/* appends string s2 to s1, at most n chars copied */

extern int strcmp(const char * /*s1*/, const char * /*s2*/);
	/* compares string s2 to s1, returns 0 if equal */
extern int strcasecmp(const char * /*s1*/, const char * /*s2*/);
	/* compares string s2 to s1, returns 0 if equal , case insensitive */
extern int strncmp(const char * /*s1*/, const char * /*s2*/, size_t n);
	/* compares string s2 to s1, returns 0 if equal */
	/* at most n chars compared */
extern int strncasecmp(const char * /*s1*/, const char * /*s2*/, size_t n);
	/* compares string s2 to s1, returns 0 if equal , case insensitive */
	/* at most n chars compared */

extern size_t strlen(const char * /*s1 */);
	/* returns length string s1 */

extern char *strchr(const char * /*s*/, int c /*c*/);
	/* locate first occurance of (char)c in string s */

extern void *memset(void * /*s*/, int /*c*/, size_t /*n*/);
	/* set n chars starting at s to (char)c */

extern void bzero(void *s, int len);
extern void bcopy(void *s,void *d,int len);

#define CopyMem(a,b,c) memcpy(a,b,c)
#define SetMem(a,b,c) memset(a,b,c)

/*
 * In standard usage, "ffs" returns the bit index of the *least*
 * significant bit; but it has recently been returning the bit index
 * of the most significant bit. In general, use FindMSB or FindLSB so
 * it is clear what you are expecting. Some code using ffs was expecting
 * to get the LSBit ...
 *
 * All three of these functions return zero if the parameter is zero,
 * or a bit index (of the most or least significant bit) as
 * appropriate, where the bit index of the least significant bit in
 * the word is 1.
 */ 
extern int ffs(unsigned int mask); 	/* find msb */
extern int FindMSB(uint32 mask); 	/* find MSBit */
extern int FindLSB(uint32 mask); 	/* find LSBit */

/*
 * The normal name for population counting is CountBits, and the
 * common name "popc" is provided for convenience.
 */
extern int CountBits(uint32 mask);	/* count 1s in mask */
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#define popc(mask)	CountBits(mask)

#endif	/* __STRING_H */

