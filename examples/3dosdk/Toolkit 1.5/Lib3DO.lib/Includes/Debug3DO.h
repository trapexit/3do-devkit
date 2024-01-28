/*******************************************************************************************
 *	File:			Debug3DO.h
 *
 *	Contains:		debugging and diagnostic output macros
 *
 *	Written by:		Neil Cormia, Joe Buczek and Ian Lepore
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	03/20/94	ian		Moved #includes of stddef.h and stdio.h
 *from conditional section to top of file so they always get included.  If
 *						folks come to rely on this file including
 *stdio.h when debugging is on, it's rude to have their code suddenly break
 *when they compile without debugging. 01/12/94	ian		Added #include
 *"types.h". 12/19/93	ian		Added conditional verbose facility.
 *						Removed direct calls to printf and added
 *calls to Diagnose1_()/Diagnose2_(), to allow error handling to be vectored
 *through application-specified hook routine.
 *	09/14/93	ian		Added VERBOSE macro.  Added wrapper braces
 *around expansions of multi-statement macros, both to enforce the "must be
 *used in a statement context" rule, and to provide proper operation in
 *situations such as if (some_error) DIAGNOSE(("some message")); 08/09/93
 *JAY		merged Debug3DO.h and LibDebug.h 08/05/93	ian Added
 *DIAGNOSE and DIAGNOSE_SYSERR 07/09/93	jb		Fix type mismatch in
 *printf 06/02/93	jb		Make conditional based on global DEBUG
 *compile switch which is typically set in your make file. Add conditional
 *includes of other header files to simplify use of debugging functions.
 *	5/14/93		njc		New today.
 *
 *	Implementation notes:
 *
 *	The macros defined here provide a standard method for Lib3DO to report
 *	errors discovered during the various sanity checks that get included
 *	in the debugging version of the library.  In an ideal world, all
 *	library modules would include this header and use these macros.
 *	Maybe someday this will be so.
 *
 *	Applications can also use this header, with the following limitations:
 *	 -	Only the macros and functions listed as "public interface" in
 *this header file are guaranteed to keep working the way they do now.
 *	 -	The actual implementation of the macros may change, don't rely
 *on the precise expansion of the macro, rely on its documented behavior. -
 *When passing printf-style format and args to one of the documented macros, a
 *maximum of 10 args are allowed.
 *
 *	Conditional verbose messages are now implemented.  These are primarily
 *	for application (rather than Lib3DO) usage.  There are 31 categories
 *	of verbose messages; each category can have a different application-
 *	assigned meaning.  Applications can code CVERBOSE() macros which
 *include a category number, and then turn all verbose messages for a category
 *on or off using SetVerboseMask().
 *
 *	The output invoked by the diagnostic and verbose macros is now routed
 *	through a pair of routines which assemble and print the messages using
 *	semaphore protection.  This means that if multiple threads are issuing
 *	messages, the output won't be interspersed (as long as all output is
 *	via these macros).
 *
 *	Applications can intercept the diagnostic and verbose output process by
 *	supplying a intercept routine via SetDiagnoseHook().  The intercept
 *routine is supplied with all the information that will be reported; it can do
 *a variety of things including alternate output handling, suppression of
 *output, and custom system error messages.  See the Debug3DO.c source code for
 *more info on writing output intercept routines.
 *
 *	The CHECKRESULT() macro should be used with extreme care.  It calls
 *exit() when an error is detected.  From the main-application level this can
 *be useful for early development and debugging, but from a thread it can be
 *	deadly, because the exit() causes the thread to shut down but it
 *doesn't stop the main application.  The next attempt to communicate with the
 *thread or rely on its services will lead to an error far removed from the
 *original cause of the problem, making debugging just that much more
 *difficult.  For this reason, CHECKRESULT() should not be used at all in
 *library code.
 *
 *	Things you can count on:
 *
 *	 When DEBUG is defined to non-zero, these macros expand to code that
 *	 issues formatted messages.  When DEBUG is undefined or defined to
 *	 zero, these macros expand to no code at all.
 *
 *	 DIAGNOSE(x)
 *
 *		 Report the current task/module/file, followed by your message.
 *		 The exact format of the display may change.
 *		 The macro must be used in a statement (not expression)
 *		 context.  The argument to the macro is anything that can
 *		 appear in a call to printf(), including the parens.  EG,
 *		 DIAGNOSE(("mymsg %d\n", value));
 *
 *	 DIAGNOSE_SYSERR(i,x)
 *
 *		 Report the current task/module/file, followed by your message,
 *		 followed by a system-provided message.  This is similar to
 *		 DIAGNOSE, except that it takes an additional parm (the return
 *		 code from a folio call that failed).  The limitations,
 *caveats, and usage of the 'x' parameter are the same as for DIAGNOSE.
 *
 *	 CVERBOSE(i,x)
 *
 *		Conditional verbose message: report the current
 *task/module/file, followed by your message, only if the 'i' category of
 *verbose messages has been enabled by setting its bit on in a call to
 *SetVerboseMask(). (Initially, all 31 categories of verbose messages are
 *enabled.) The limitations, caveats, and usage of the 'x' parameter are the
 *		same as for DIAGNOSE.
 *
 *	 VERBOSE(x)
 *
 *		This is a verbose message with the default category of 0.
 *
 *******************************************************************************************/

#ifndef DEBUG3DO_H
#define DEBUG3DO_H

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"

#ifndef DEBUG
#define DEBUG 0
#endif

/*----------------------------------------------------------------------------
 * public interface.
 *	these things (along with the documented macros) are the public
 *	interface designed to be called/used from the application code.
 *--------------------------------------------------------------------------*/

#define DIAGNOSE_SYSMSGBUFSIZE 128
#define VERBOSE_CATEGORY2MASK(x) (1UL << (uint32)(x))

typedef char *(DiagnoseHookFunc)(int32 err, char *module, int32 line,
                                 char *sysErrMsg, char *fmt, va_list args);

uint32 GetVerboseMask (void);
uint32 SetVerboseMask (uint32 maskBits);

void SetDiagnoseHook (DiagnoseHookFunc *userHookFunc);
void UnsetDiagnoseHook (void);

/*----------------------------------------------------------------------------
 * private interface.
 *	these things are not to be called/used directly; they help support the
 *	documented macros and functions, and their interface could change.
 *--------------------------------------------------------------------------*/

#define DIAGNOSE_NOSYSMSG                                                     \
  ((int32)0x80000000) // magic value means error not associated with system
                      // message

extern void Diagnose1_ (int32 err, char *moduleName, int32 moduleLine);
extern void Diagnose2_ (char *fmt, ...);

/*----------------------------------------------------------------------------
 * if DEBUG is TRUE, define public-interface reporting macros.
 *--------------------------------------------------------------------------*/

#if DEBUG

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define CHECKRESULT(name, val)                                                \
  if ((int32)val < 0)                                                         \
    {                                                                         \
      printf ("Failure in %s: $%lx\n", name, val);                            \
      PrintfSysErr ((Item)val);                                               \
      exit (0);                                                               \
    }

#define DIAGNOSE_SYSERR(i, x)                                                 \
  {                                                                           \
    Diagnose1_ ((i), __FILE__, __LINE__);                                     \
    Diagnose2_ x;                                                             \
  }

#define DIAGNOSE(x) DIAGNOSE_SYSERR (DIAGNOSE_NOSYSMSG, x)
#define CVERBOSE(i, x) DIAGNOSE_SYSERR (i, x)
#define VERBOSE(x) CVERBOSE (0, x)

/*----------------------------------------------------------------------------
 * if DEBUG is false, all reporting macros become no-ops.
 *--------------------------------------------------------------------------*/

#else

#define PRT(x)
#define ERR(x)
#define CHECKRESULT(name, val)

#define DIAGNOSE_SYSERR(i, x)
#define DIAGNOSE(x)
#define CVERBOSE(i, x)
#define VERBOSE(x)

#endif

#endif
