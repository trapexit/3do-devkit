#ifndef __DEBUG3DO_H
#define __DEBUG3DO_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**
**  Lib3DO debugging and diagnostic output macros
**
**	The macros defined here provide a standard method for Lib3DO to report
**	errors discovered during the various sanity checks that get included
**	in the debugging version of the library.  
**
**	Applications can also use this header, with the following limitations:
**	 -	Only the macros and functions listed as "public interface" in this 
**		header file are guaranteed to keep working the way they do now.
**	 -	The actual implementation of the macros may change, don't rely on
**		the precise expansion of the macro, rely on its documented behavior.
**
**	The CHECKRESULT() macro should be used with extreme care.  It calls exit()
**	when an error is detected.  From the main-application level this can be 
**	useful for early development and debugging, but from a thread it can be
**	deadly, because the exit() causes the thread to shut down but it doesn't
**	stop the main application.  The next attempt to communicate with the thread
**	or rely on its services will lead to an error far removed from the original 
**	cause of the problem, making debugging just that much more difficult.  For 
**	this reason, CHECKRESULT() should not be used at all in library code.
**
**	Things you can count on:
**
**	 When DEBUG is defined to non-zero, these macros expand to code that
**	 issues formatted messages.  When DEBUG is undefined or defined to
**	 zero, these macros expand to no code at all.
**
**	 DIAGNOSE(x)
**
**		 Report the current task/module/file, followed by your message.
**		 The exact format of the task/module/file display may change.
**		 The macro must be used in a statement (not expression)
**		 context.  The argument to the macro is anything that can
**		 appear in a call to printf(), including the parens.  EG,
**		 DIAGNOSE(("mymsg %d\n", value));
**
**	 DIAGNOSE_SYSERR(i,x)
**
**		 Report the current task/module/file, followed by your message,
**		 followed by a system-provided message.  This is similar to
**		 DIAGNOSE, except that it takes an additional parm (the return
**		 code from a folio call that failed).  The limitations, caveats,
**		 and usage of the 'x' parameter are the same as for DIAGNOSE.
**
**	 VERBOSE(x)
**
**		This is a verbose message to report progress during debugging.
**
******************************************************************************/


#include "operror.h"
#include "stdlib.h"
#include "stdio.h"

#ifndef DEBUG
  #define DEBUG 0
#endif

#ifdef __cplusplus
  extern "C" {
#endif

void Debug3DOBanner(char *title, char *module, int line);

#ifdef __cplusplus
  }
#endif

/*----------------------------------------------------------------------------
 * if DEBUG is TRUE, define public-interface reporting macros.
 *--------------------------------------------------------------------------*/

#if DEBUG

	#define	PRT(x)	{ printf x; }
	#define	ERR(x)	PRT(x)
	#define CHECKRESULT(name, val) 									\
		if ( (int32)val < 0 )										\
			{ 														\
			printf("Failure in %s: $%lx\n", name, val);				\
			PrintfSysErr((Item)val); 								\
			exit( 0 );												\
			}

	#define DEBUG3DO_BANNER(title)	Debug3DOBanner(title, __FILE__, __LINE__);	

	#define VERBOSE(x)				{								\
									DEBUG3DO_BANNER("VERBOSE");		\
									printf x;						\
									}

	#define DIAGNOSE(x)				{								\
									DEBUG3DO_BANNER("ERROR");		\
									printf x;						\
									}
									
    #define DIAGNOSE_SYSERR(i,x)	{								\
									DEBUG3DO_BANNER("ERROR");		\
									printf x;						\
									PrintfSysErr(i);				\
									}


/*----------------------------------------------------------------------------
 * if DEBUG is false, all reporting macros become no-ops.
 *--------------------------------------------------------------------------*/

#else

	#define	PRT(x)
	#define	ERR(x)
	#define CHECKRESULT(name, val)
	
	#define DEBUG3DO_BANNER(title)
	#define DIAGNOSE_SYSERR(i,x)
    #define DIAGNOSE(x)
	#define VERBOSE(x)
	
#endif


#endif /* __DEBUG3DO_H */
