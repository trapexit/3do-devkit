#pragma include_only_once

#include "extern_c.h"

#include "types.h"

EXTERN_C_BEGIN

extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memmove(void *dst, const void *src, size_t n);
extern void *memchr(void *src, int c,size_t n);
extern int   memcmp(const void *s1, const void *s2, size_t n);
extern void *memset(void *s, int c, size_t n);

extern void bzero(void *s, size_t n);
extern void bcopy(const void *src, void *dst, size_t n);

extern char   *strcat(char *dst, const char *src);
extern char   *strchr(const char *s, int c);
extern char   *strcpy(char *dst, const char *src);
extern char   *strerror(int errnum);
extern char   *strncat(char *dst, const char *src, size_t n);
extern char   *strncpy(char *dst, const char *src, size_t n);
extern char   *strpbrk(const char *s, const char *accept);
extern char   *strrchr(const char *s, int c);
extern char   *strstr(const char *haystack, const char *needle);
extern char   *strtok(char *str, const char *delim);
extern int     strcmp(const char *s1, const char *s2);
extern int     strncmp(const char *s1, const char *s2, size_t n);
extern size_t  strcspn(const char *s, const char *reject);
extern size_t  strlen(const char *s);
extern size_t  strspn(const char *s, const char *accept);

extern int ffs(int i);

EXTERN_C_END
