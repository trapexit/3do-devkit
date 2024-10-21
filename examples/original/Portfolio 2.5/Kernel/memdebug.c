
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: memdebug.c,v 1.5 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/memdebug
|||	memdebug - Demonstrates the memory debugging subsystem.
|||
|||	  Synopsis
|||
|||	    memdebug
|||
|||	  Description
|||
|||	    This program demonstrates the features of the memory debugging
subsystem.
|||
|||	    The memory debugging subsystem helps you make sure that your
application
|||	    is freeing all of its memory, not stomping on innocent memory
areas, and
|||	    generally doing illegal things to the Portfolio memory manager. As
it
|||	    detects errors and illegal operations, it displays information in
the
|||	    Debugger Terminal window.
|||
|||	    For more information about the memory debugging subsystem, refer to
the
|||	    documentation for the CreateMemDebug() function.
|||
|||	  Caveats
|||
|||	    This program intentionally does a some illegal things. Don't do
this in
|||	    your programs!
|||
|||	  Associated Files
|||
|||	    memdebug.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include "operror.h"
#include "stdio.h"
#include "types.h"

#define MEMDEBUG
#include "mem.h"

/*****************************************************************************/

int
main (int32 argc, char **argv)
{
  void *mem;
  uint32 *ptr;
  Err err;

  /* initialize the memory debugging subsystem */
  err = CreateMemDebug (MEMDEBUGF_PAD_COOKIES | MEMDEBUGF_ALLOC_PATTERNS
                            | MEMDEBUGF_FREE_PATTERNS
                            | MEMDEBUGF_CHECK_ALLOC_FAILURES
                            | MEMDEBUGF_KEEP_TASK_DATA,
                        NULL);
  if (err >= 0)
    {
      /* test what happens when we allocate nothing */
      AllocMem (0, 0);

      /* test what happens when we ask for too much */
      AllocMem (123456789, 0);

      /* test a NULL pointer */
      FreeMem (NULL, 0);

      /* should say there's nothing to say... */
      DumpMemDebug (NULL);

      /* test a bogus pointer */
      FreeMem ((void *)1, 0);

      /* check a free operation with the wrong size */
      mem = AllocMem (123, MEMTYPE_ANY);
      FreeMem (mem, 234);

      /* check if DumpMemDebug() will list these correctly */
      AllocMem (123, MEMTYPE_ANY);
      AllocMem (234, MEMTYPE_ANY);
      AllocMem (345, MEMTYPE_ANY);
      AllocMem (456, MEMTYPE_ANY);

      /* trash the cookie before the allocation and see if it is noticed */
      ptr = (uint32 *)AllocMem (333, MEMTYPE_ANY);
      ptr--;
      *ptr = 0;
      ptr++;

      /* see if the trashed cookie is detected */
      SanityCheckMemDebug (NULL);

      /* the trashed cookie should be reported once again by this call */
      FreeMem (ptr, 333);

      /* try out MEMTYPE_TRACKSIZE handling */
      ptr = (uint32 *)AllocMem (373, MEMTYPE_TRACKSIZE);
      FreeMem (ptr, -1);
      ptr = (uint32 *)AllocMem (373, MEMTYPE_TRACKSIZE);
      FreeMem (ptr, 373);

      /* output everything still allocated */
      DumpMemDebug (NULL);

      DeleteMemDebug ();
    }
  else
    {
      printf ("CreateMemDebug() failed: ");
      PrintfSysErr (err);
    }
}
