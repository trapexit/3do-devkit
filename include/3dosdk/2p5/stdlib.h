#ifndef __STDLIB_H
#define __STDLIB_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: stdlib.h,v 1.16 1994/09/10 01:22:35 peabody Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*****************************************************************************/


#ifdef MEMDEBUG

#define malloc(s)    mallocDebug(s,__FILE__,__LINE__)
#define calloc(n,s)  callocDebug(n,s,__FILE__,__LINE__)
#define free(p)      freeDebug(p,__FILE__,__LINE__)
#define realloc(p,s) reallocDebug(p,s,__FILE__,__LINE__)
void *mallocDebug(int32, const char *sourceFile, uint32 sourceLine);
void freeDebug(void *, const char *sourceFile, uint32 sourceLine);
void *callocDebug(size_t nelem, size_t elsize, const char *sourceFile, uint32 sourceLine);
void *reallocDebug(void *oldBlock, size_t newSize, const char *sourceFile, uint32 sourceLine);

#else

void *malloc(int32);	/* not changed to size_t for src compatibility */
void free(void *);
void *calloc(size_t nelem, size_t elsize);
void *realloc(void *oldBlock, size_t newSize);
#define mallocDebug(s,f,l)    malloc(s)
#define callocDebug(n,s,f,l)  calloc(n,s)
#define freeDebug(p,f,l)      free(p)
#define reallocDebug(p,s,f,l) realloc(p,s)

#endif


/*****************************************************************************/


extern void exit(int status);

extern int32 rand(void);
extern void srand(int32);
extern uint32 urand(void);

extern int _ANSI_rand(void);
extern int _ANSI_srand(unsigned int seed);

extern int32 atoi(const char *nptr);
extern long int atol(const char *nptr);

extern ulong strtoul(const char *nsptr, char **endptr, int base);
extern long strtol(const char *nsptr, char **endptr, int base);

extern void qsort(void *base, size_t nmemb, size_t size,
                  int (*compar)(const void *, const void *));

extern void *bsearch(const void *key, const void *base,
                     size_t nmemb, size_t size,
                     int (*compar)(const void *, const void *));

extern int system(const char *cmdString);
/* The system() function currently does not return the exit value produced
 * by the program being run. It will return an error code if the program
 * couldn't be started, and will otherwise always return 0.
 */


/*****************************************************************************/


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __STDLIB_H */
