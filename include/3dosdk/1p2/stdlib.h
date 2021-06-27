#ifndef _STDLIB_H
#define _STDLIB_H


/*****

$Id: stdlib.h,v 1.6 1994/01/21 01:13:43 limes Exp $

$Log: stdlib.h,v $
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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void exit(int status );

extern int32 rand(void);
extern void srand(int32);
extern uint32 urand(void);

extern int _ANSI_rand(void);
extern int _ANSI_srand(unsigned int seed);

extern int32 atoi(const char *nptr);
extern long int atol(const char *nptr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
