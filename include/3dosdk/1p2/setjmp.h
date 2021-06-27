#ifndef __SETJMP_H
#define __SETJMP_H

/*****

$Id: setjmp.h,v 1.4 1994/01/21 01:12:35 limes Exp $

$Log: setjmp.h,v $
 * Revision 1.4  1994/01/21  01:12:35  limes
 * RCS files were lost. This reestablishes the changes
 * evident from the old log data:
 *
 * +  * Revision 1.5  1994/01/20  05:19:39  phil
 * +  * fixed C++ mods
 * +  *
 * +  * Revision 1.4  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 * +  *
 *
 * Revision 1.5  1994/01/20  05:19:39  phil
 * fixed C++ mods
 *
 * Revision 1.4  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
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

typedef int jmp_buf[22];	/* size suitable for the arm */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int setjmp(jmp_buf /*env*/);
   /* Saves its calling environment in its jmp_buf argument, for later use
    * by the longjmp function.
    * Returns: If the return is from a direct invocation, the setjmp function
    *	       returns the value zero. If the return from a call to the longjmp
    *	       function, the setjmp function returns a non zero value.
    */

extern void longjmp(jmp_buf /*env*/, int /*val*/);
   /* Restores the environment saved by the most recent call to setjmp in the
    * same invocation of the program, with the corresponding jmp_buf argument.
    * If there has been no such call, or if the function containing the call
    * to setjmp has terminated execution (eg. with a return statement) in the
    * interim, the behaviour is undefined.
    * All accessible objects have values as of the time longjmp was called,
    * except that the values of objects of automatic storage duration that do
    * not have volatile type and have been changed between the setjmp and
    * longjmp calls are indeterminate.
    * As it bypasses the usual function call and return mechanism, the longjmp
    * function shall execute correctly in contexts of interrupts, signals and
    * any of their associated functions.
    * Returns: After longjmp is completed, program execution continues as if
    *	       the corresponding call to setjmp had just returned the value
    *	       specified by val. The longjmp function cannot cause setjmp to
    *	       return the value 0; if val is 0, setjmp returns the value 1.
    */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __SETJMP_H */
