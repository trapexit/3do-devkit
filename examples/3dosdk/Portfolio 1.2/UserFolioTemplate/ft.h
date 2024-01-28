/*
        File:		ft.h

        Contains:	user folio header template

        Written by:	Craig Weisenfluh

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	  4/6/93	JAY		first checked in

        To Do:
*/

#include "debug.h"
#include "types.h"

// the name of the folio
#define LIB3DOFOLIONAME "ftFolio"

// prototypes for folio functions that are to be made accessable to other
// applications
extern int32 ftTestFolio (void);
extern void *ftTestFolioParms (int n, char *p);

// folio management functions
extern int32 ftCloseFolio (void);
extern int32 ftOpenFolio (void);
