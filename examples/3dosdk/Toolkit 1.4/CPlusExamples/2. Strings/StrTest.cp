/*
        File:		StrTest.cp

        Contains:	Test CString class.

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
#include "CString.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "stdio.h"

#include "CPlusSwiHack.h"

void
printResult (CString aStr, char *description, CString a2ndStr)
{
  printf ("\"%s\" %s \"%s\"\n", (const char *)aStr, description,
          (const char *)a2ndStr);
}

int
main (void)
{
  CString str1 ("Hello");
  CString str2 (" ");
  CString str3 ("World");
  CString str4;
  CString str5;

  // Change str1 and str3 to test these different conditions
  if (str1 < str3)
    printResult (str1, "is less than", str3);
  else if (str1 > str3)
    printResult (str1, "is greater than", str3);
  else if (str1 <= str3)
    printResult (str1, "is less than or equal to", str3);
  else if (str1 >= str3)
    printResult (str1, "is greater than or equal to", str3);
  else if (str1 == str3)
    printResult (str1, "is equal to", str3);
  else if (str1 != str3)
    printResult (str1, "is not equal to", str3);

  str2 += str3;
  str1 += str2;
  str1 += '!';

  str4 = str1;

  printf ("Length (\"%s\") is %ld\n", (const char *)str4, str4.Length ());

  str4.Delete (5, 6);

  printf ("After Delete(5, 6): \"%s\"\n", (const char *)str4);

  str4.Insert (5, str2);

  printf ("After Insert(5, \"%s\"): \"%s\"\n", (const char *)str2,
          (const char *)str4);

  str4.Insert (str4.Length (), '.');
  str4.Insert (str4.Length (), '.');
  str4.Insert (str4.Length (), '.');

  printf ("After appending an ellipsis: \"%s\"\n", (const char *)str4);

  str4 += str5;
  printf ("After appending NULL CString: \"%s\"\n", (const char *)str4);
}