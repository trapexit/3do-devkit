/*
        File:		ft_folio.c

        Contains:	This is the main folio module, responsible for
   registering the folio.  Currently the folio is run like any other
   application. In the future, this may change, so bear this in mind when
                                implementing the main() function.

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
#include "kernel.h"
#include "kernelnodes.h"

// our function type
typedef void *(*V_FUNC) ();

// add your functions here.  Note the negative offset, which grows down.  Use
// this offset to resolve the reference to function at runtime (as demonstrated
// in ft_lib.c

V_FUNC ftUserFuncs[] = {
  (V_FUNC)ftTestFolio,     // -2
  (V_FUNC)ftTestFolioParms // -1
};

// the size of our virtual table
#define NUM_FTUSERFUNCS (sizeof (ftUserFuncs) / sizeof (void *))

// goodies that makeup our folio
TagArg ftFolioTags[] = {
  { TAG_ITEM_NAME, (void *)LIB3DOFOLIONAME }, /* name of folio */
  { CREATEFOLIO_TAG_NUSERVECS,
    (void *)NUM_FTUSERFUNCS }, /* number of user functions */
  { CREATEFOLIO_TAG_USERFUNCS,
    (void *)ftUserFuncs }, /* list of user functions */
  { TAG_END, (void *)0 }   /* end of tag list */
};

// a folio is an application
int
main ()
{
  Item i;

  if ((i = CreateItem (MKNODEID (KERNELNODE, FOLIONODE), ftFolioTags)) >= 0)
    kprintf ("\nft_folio has been loaded and is ready for use");

  // wait for opera reset
  WaitSignal (0x1);

  return (int)i;
}
