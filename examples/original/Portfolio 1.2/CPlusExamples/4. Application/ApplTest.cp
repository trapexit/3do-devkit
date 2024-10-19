/*
        File:		ApplTest.cp

        Contains:	Test application class.

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <2>	  1/19/94	cde		Converted to event broker;
                                                        replaced kprintf's.
        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CApplication.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "CPlusSwiHack.h"

class CMyApplication : public CApplication
{
protected:
  void
  OnStart (void)
  {
    printf ("Start pressed\n");
  }
  void
  OnX (void)
  {
    printf ("X pressed\n");
  }

  void
  OnUp (void)
  {
    printf ("Up pressed\n");
  }
  void
  OnDown (void)
  {
    printf ("Down pressed\n");
  }
  void
  OnLeft (void)
  {
    printf ("Left pressed\n");
  }
  void
  OnRight (void)
  {
    printf ("Right pressed\n");
  }

  void
  OnA (void)
  {
    printf ("A pressed\n");
  }
  void
  OnB (void)
  {
    printf ("B pressed\n");
  }
  void
  OnC (void)
  {
    printf ("C pressed -- exiting\n");
    Stop ();
  }
};

int
main (void)
{
  CMyApplication myAppl;

  myAppl.Run ();
}