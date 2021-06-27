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
 *	09/14/93	ian		Added VERBOSE macro.  Added wrapper braces around
 *						expansions of multi-statement macros, both to enforce the
 *						"must be used in a statement context" rule, and to provide
 *						proper operation in situations such as
 *							if (some_error) DIAGNOSE(("some message"));
 *	08/09/93	JAY		merged Debug3DO.h and LibDebug.h
 *	08/05/93	ian 	Added DIAGNOSE and DIAGNOSE_SYSERR
 *	07/09/93	jb		Fix type mismatch in printf 
 *	06/02/93	jb		Make conditional based on global DEBUG compile switch which
 *						is typically set in your make file. Add conditional includes of
 *						other header files to simplify use of debugging functions.
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
 *	The CHECKRESULT() macro should be used with extreme care.  It calls exit()
 *	when an error is detected.  From the main-application level this can be 
 *	useful for early development and debugging, but from a thread it can be
 *	deadly, because the exit() causes the thread to shut down but it doesn't
 *	stop the main application.  In fact, the main application gets no notification
 *	that the thread has quit, and the next attempt to communicate with the thread
 *	or rely on its services will lead to an error far removed from the original 
 *	cause of the problem, making debugging just that much more difficult.  For 
 *	this reason, CHECKRESULT() should not be used at all in library code.
 *
 *	Applications can also use this header, with the following limitations:
 *	 -	Only the macros listed in this comment block are guaranteed to keep
 *		working the way they do now.
 *	 -	The actual implementation of the macros may change, don't rely on
 *		the precise expansion of the macro, rely on its documented behavior.
 *
 *	Things you can count on:
 *
 *	 When DEBUG is defined to non-zero, these macros expand to code that
 *	 issues formatted error messages.  When DEBUG is undefined or defined
 *	 to zero, these macros expand to no code at all.
 *
 *	 DIAGNOSE(x)
 *
 *		 Report the current source filename, followed by your message.
 *		 The exact format of the source filename display may change.
 *		 The macro must be used in a statement (not expression)
 *		 context.  The argument to the macro is anything that can
 *		 appear in a call to printf(), including the parens.  EG,
 *		 DIAGNOSE(("mymsg %d\n", value));
 *
 *	 DIAGNOSE_SYSERR(i,x)
 *
 *		 Report the current source filename, followed by your message,
 *		 followed by a system-provided message.  This is similar to
 *		 DIAGNOSE, except that it takes an additional parm (the return
 *		 code from a folio call that failed).  The limitations, caveats,
 *		 and usage of the 'x' parameter are the same as for DIAGNOSE.
 *
 *	 VERBOSE(x)
 *
 *		This is identical to DIAGNOSE() in usage and limitations; it 
 *		differs only in that the word 'error' doesn't appear in the pre-
 *		formatted part of the output message.  This is useful for sprinkling
 *		"I made it to here," or "I'm about to do this" messages in your code.
 *
 *
 *******************************************************************************************/
#ifndef __DEBUG3DO_H__
#define __DEBUG3DO_H__

#ifndef DEBUG
  #define DEBUG 0
#endif

#if DEBUG
	#ifndef _OPERROR_H
	#include "operror.h"		/* for PrintfSysErr() */
	#endif

	#ifndef _STDLIB_H
	#include "stdlib.h"			/* for exit() */
	#endif

	#ifndef _STDIO_H
	#include "stdio.h"			/* for printf() */
	#endif

	/********************************/
	/* Macros for diagnostic output */
	/********************************/
	#define	PRT(x)	{ printf x; }
	#define	ERR(x)	PRT(x)
	#define CHECKRESULT(name, val) 							\
		if ( (int32)val < 0 )								\
			{ 												\
			printf("Failure in %s: $%lx\n", name, val);		\
			PrintfSysErr((Item)val); 						\
			exit( 0 );										\
			}
			
    #define DIAGNOSE(x)				{									\
									printf("Error ("__FILE__"): ");		\
									printf x;							\
									}

    #define DIAGNOSE_SYSERR(i,x)	{									\
									printf("Error ("__FILE__"): ");		\
									printf x;							\
									PrintfSysErr((Item)(i));			\
									}

    #define VERBOSE(x)				{									\
									printf(__FILE__ ": ");				\
									printf x;							\
									}



#else
	/***************************************************************/
	/* Turn off diagnostic messages if DEBUG compile flag is FALSE */
	/***************************************************************/
	#define	PRT(x)
	#define	ERR(x)
	#define CHECKRESULT(name, val)
	
    #define DIAGNOSE(x)
    #define DIAGNOSE_SYSERR(i,x)
	#define VERBOSE(x)
#endif

#endif	/* __DEBUG3DO_H__ */

