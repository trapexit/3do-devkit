/*
        File:		ListTest.cp

        Contains:	Test list management classes.

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CList.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "stdio.h"

#include "CPlusSwiHack.h"

#define kNumListEntries 100

int
main (void)
{
  CList *lst = new CList ();

  if (!lst)
    {
      printf ("Creating new list failed!\n");
      return (0);
    }

  printf ("Appending %d entries to list...\n", kNumListEntries);
  for (long i = 0; i < kNumListEntries; i++)
    {
      char *tmp = new char[255];

      if (!tmp)
        {
          printf ("Creating new entry failed!\n");
          continue;
        }

      sprintf (tmp, "List entry #%ld", i);
      lst->Append (tmp);
    }

  printf ("The list has %ld entries\n", lst->Count ());

  lst->Remove (3);

  printf ("The list has %ld entries\n", lst->Count ());

  long lstCnt = lst->Count ();

  for (long j = 0; j < lstCnt; j++)
    {
      char *pEntry = (char *)lst->GetNth (j);

      printf ("%s\n", pEntry);

      delete pEntry;
    }

  delete lst;
}