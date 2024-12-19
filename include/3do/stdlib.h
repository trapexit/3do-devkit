#pragma once

#include "extern_c.h"

#include "types.h"

EXTERN_C_BEGIN

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

void *malloc(long size);
void free(void *);
void *calloc(size_t nelem, size_t elsize);
void *realloc(void *oldBlock, size_t newSize);
#define mallocDebug(s,f,l)    malloc(s)
#define callocDebug(n,s,f,l)  calloc(n,s)
#define freeDebug(p,f,l)      free(p)
#define reallocDebug(p,s,f,l) realloc(p,s)

#endif

extern void exit(int status);

/* rand() is just urand() & 0x7FFFFFFF */
extern void srand(unsigned int);
extern int rand(void);
extern unsigned int urand(void);

extern int _ANSI_rand(void);
extern int _ANSI_srand(unsigned int seed);

extern int  atoi(const char *nptr);
extern long atol(const char *nptr);

extern long          strtol(const char *nptr, char **endptr, int base);
extern unsigned long strtoul(const char *nptr, char **endptr, int base);

extern void qsort(void *base, size_t nmemb, size_t size,
                  int (*compar)(const void *, const void *));

extern void *bsearch(const void *key, const void *base,
                     size_t nmemb, size_t size,
                     int (*compar)(const void *, const void *));

/*
  The system() function currently does not return the exit value produced
  by the program being run. It will return an error code if the program
  couldn't be started, and will otherwise always return 0.
*/
extern int system(const char *cmd);

EXTERN_C_END
