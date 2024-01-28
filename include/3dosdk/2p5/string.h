#ifndef __STRING_H
#define __STRING_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: string.h,v 1.26 1994/09/10 01:35:19 vertex Exp $
**
**  Standard C string and memory management definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __STDLIB_H
#include "stdlib.h"	/* temporarily left for application src compatibility */
#endif
			/* strtol(),strtoul() in stdlib.h used to be here */

/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* string manipulation routines */

extern char *strcpy(char *dest, const char *source);
extern char *strncpy(char *dest, const char *source, size_t maxChars);
extern char *strcat(char *str, const char *appendStr);
extern char *strncat(char *str, const char *appendStr, size_t maxChars);
extern int strcmp(const char *str1, const char *str2);
extern int strncmp(const char *str1, const char *str2, size_t maxChars);
extern int strcasecmp(const char *str1, const char *str2);
extern int strncasecmp(const char *str1, const char *str2, size_t maxChars);
extern size_t strlen(const char *str);
extern char *strchr(const char *str, int searchChar);
extern char *strrchr(const char *str, int searchChar);
extern char *strtok(char *str, const char *seps);
extern char *strpbrk(const char *str, const char *breaks);
extern char *strstr(const char *str, const char *subString);
extern size_t strcspn(const char *str, const char *set);
extern size_t strspn(const char *str, const char *set);
extern char *strerror(int errorCode);


/* memory manipulation routines */

extern void *memcpy(void *dest, const void *source, size_t numBytes);
extern void *memmove(void *dest, const void *source, size_t numBytes);
extern void *memset(void *mem, int c, size_t numBytes);
extern void *memchr(const void *mem, int searchChar, size_t numBytes);
extern int memcmp(const void *mem1, const void *mem2, size_t numBytes);
extern void bzero(void *mem, int numBytes);
extern void bcopy(const void *source, void *dest, int numBytes);


/* bit manipulation routines
 *
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
extern int CountBits(uint32 mask);	/* count 1s in mask */


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/* synonyms */
#define CopyMem(a,b,c) memcpy(a,b,c)
#define SetMem(a,b,c)  memset(a,b,c)
#define popc(mask)     CountBits(mask)


/*****************************************************************************/


#endif /* __STRING_H */
