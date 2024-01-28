#ifndef _STDLIB_H
#define _STDLIB_H


/*****

$Id: stdlib.h,v 1.10 1994/04/06 16:48:45 sdas Exp $

$Log: stdlib.h,v $
 * Revision 1.10  1994/04/06  16:48:45  sdas
 * CR 2152 : Going towards ANSI; strtol() and strtoul() moved from string.h
 *
 * Revision 1.9  1994/04/04  23:05:41  vertex
 * Added definition for realloc()
 *
 * Revision 1.8  1994/03/23  22:00:51  sdas
 * stdlib.h now includes types.h
 *
 * Revision 1.7  1994/03/18  23:23:55  sdas
 * CR-1974 & CR-1975 Fixes - malloc(), free() and, calloc() now defined in
 * stdlib.h instead of mem.h
 *
 * Revision 1.6  1994/01/21  01:13:43  limes
 * RCS files lost. Recovered this change:
 *
 * +  * Revision 1.6  1994/01/20  05:19:39  phil
 * +  * fixed C++ mods
 * +  *
 *
 * Revision 1.6  1994/01/20  05:19:39  phil
 * fixed C++ mods
 *
 * Revision 1.5  1993/05/31  00:45:33  dale
 * added protos for urand srand, ANSI_rand, ANSI_srand
 *
 * Revision 1.4  1993/01/12  22:20:52  dale
 * added atoi,atol
 *
 * Revision 1.3  1992/10/24  01:43:54  dale
 * rcs
 *

 *****/

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void *malloc(int32);	/* not changd to size_t for src compatibility */
extern void free(void *);
extern void *calloc(size_t, size_t);
extern void *realloc(void *oldBlock, size_t newSize);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
