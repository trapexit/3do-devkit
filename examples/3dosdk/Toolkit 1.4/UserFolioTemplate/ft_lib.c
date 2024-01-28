/*
        File:		ft_lib.c

        Contains:	This module provides the dynamic links to the folio,
   and should be linked with all applicatons that will be accessing functions
   in the folio.

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
#include "item.h"
#include "kernel.h"
#include "kernelnodes.h"

static Item ftFolioItem;
static void *ftBase;

// these offsets are into the virtual function array in ft_folio.c.  They
// provide the dynamic link that gets resolved at runtime.  If you add a
// function to the folio, it will need an offset value (see ft_folio.c)

#define FTTESTFOLIO (-2)
#define FTTESTFOLIOPARMS (-1)

// this function gets called when an application wishes to dynamically link to
// the folio the folio needs to already have been loaded
int32
ftOpenFolio (void)
{
  int32 nRet;

  ftFolioItem
      = FindNamedItem (MKNODEID (KERNELNODE, FOLIONODE), LIB3DOFOLIONAME);

  if (ftFolioItem < 0)
    return (ftFolioItem);

  if ((nRet = OpenItem (ftFolioItem, 0)) < 0)
    return (nRet);

  ftBase = LookupItem (ftFolioItem);

  return 0;
}

// this call will remove the dynamic link to the folio
int32
ftCloseFolio (void)
{
  int32 nRet = -1;

  if (ftFolioItem != 0)
    {
      nRet = CloseItem (ftFolioItem);
      ftFolioItem = 0;
    }

  return (nRet);
}

// this is one of two dynmaic links (to folio.c) demonstrated.  This call has
// no parameters.  Note that this call returns an integer
int32
ftTestFolio (void)
{
  int32 nRet;
  CALLFOLIORET (ftBase, FTTESTFOLIO, (), nRet, (int32));
  return nRet;
}

// this is the second dynamic link.  This call takes two parameters (an int and
// a char *) note that this call returns a void pointer
void *
ftTestFolioParms (int n, char *p)
{
  void *pRet;
  CALLFOLIORET (ftBase, FTTESTFOLIOPARMS, (n, p), pRet, (void *));
  return pRet;
}
