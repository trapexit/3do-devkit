/*
        File:		ft_test.c

        Contains:	This module demonstrates how to call a user folio at
   runtime

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

#include "ft.h"

int
main ()
{
  int32 nRet;

  // open the folio
  nRet = ftOpenFolio ();

  // call the first dynamic link, which takes no parameters
  ftTestFolio ();

  // call the second function, which takes two parameters
  ftTestFolioParms (4, "This is the second parameter");

  // now close the folio
  ftCloseFolio ();

  return (int)nRet;
}
