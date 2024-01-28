/*
        File:		AvailMem.h

        Contains:	Add up memory usage for a task.
                                Watch out for threads changing the list while
   you traverse!

        Written by:	Phil Burk

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 6/23/93	JAY		first checked in

        To Do:
*/

int32 ReportMemoryUsage (void);
uint32 TotalAvailTaskMem (void);
uint32 TotalAvailKernalMem (void);
