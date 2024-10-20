/*
        File:		HelloWorld.cp

        Contains:	Display a message to the console

        Written by:	Paul A. Bauersfeld

        Copyright:	� 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "stdio.h"
#include "types.h"

#include "CPlusSwiHack.h"

class CConsole
{
public:
  CConsole (void);
  ~CConsole (void);

  virtual void Output (char *msg);
};

CConsole::CConsole (void)
{
  // nothing to do!
}

CConsole::~CConsole (void)
{
  // nothing to do!
}

void
CConsole::Output (char *msg)
{
  // display a message
  printf ("%s\n", msg);
}

int
main (void)
{
  CConsole aConsole;

  aConsole.Output ("Hello World!\n");
}