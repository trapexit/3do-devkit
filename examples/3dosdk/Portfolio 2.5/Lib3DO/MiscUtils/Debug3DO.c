/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**
**  Lib3DO routines to report errors.
**
**	It's important that anything in this module be written as 100% reentrant
**	code.  Every Lib3DO routine can potentially end up in here, so the 
**	reentrancy of the whole library is related to this module.
**
**	The Debug3DOBanner() routine exists here for basically two reasons:
**	- It eliminates the need to #include "kernel.h" (for the CURRENTTASK
**	  macro) in Debug3DO.h (since that results in pulling in tons of 3DO
**	  header files into every module that #includes Debug3DO.h).
**	- It allows an application to supply a replacement banner if desired.
**
**	A replacement banner routine can conceivably do other things besides 
**	just printing a banner; just remember to keep it reentrant!
******************************************************************************/

#include "debug3do.h"
#include "kernel.h"

/*****************************************************************************
 * Debug3DOBanner()
 *	Print a banner that precedes a DIAGNOSE or VERBOSE message.
 ****************************************************************************/

void Debug3DOBanner(char *title, char *module, int line)
{
	printf("%s File %s; Line %d # Task %s: ", title, module, line, CURRENTTASK->t.n_Name);	
}
