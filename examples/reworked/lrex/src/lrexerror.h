#ifndef __LREXERROR_H
#define __LREXERROR_H

#pragma force_top_level
#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: lrexerror.h,v 1.3 1994/10/27 21:05:22 vertex Exp $
**
******************************************************************************/

#include "operror.h"

#define ER_LREX MakeErrId ('L', 'x')

#define LREXLOADIMAGE_ERR MAKETERR (ER_LREX, ER_SEVERE, ER_C_NSTND, 1)
#define LREXGRAPHICS_ERR MAKETERR (ER_LREX, ER_SEVERE, ER_C_NSTND, 2)
#define LREXALLOC_ERR MAKETERR (ER_LREX, ER_SEVERE, ER_C_NSTND, 3)

/*
   The error text to print out with PrintfSysErr()
*/
static char *LREXErrors[] = {
  "no error",

  /* LREXLOADIMAGE_ERR */
  "Error in loading background image",

  /* LREXGRAPHICS_ERR */
  "Unable to open graphics",

  /* LREXALLOC_ERR */
  "Unable to allocate necessary memory",

};

#define LREXMAX_ERROR_LEN 40

/*
   Tag args for the error text item
*/
static TagArg LREXErrorTags[]
    = { TAG_ITEM_NAME,
        (void *)"lrex",
        ERRTEXT_TAG_OBJID,
        (void *)((ER_TASK << ERR_IDSIZE) | ER_LREX),
        ERRTEXT_TAG_MAXERR,
        (void *)(sizeof (LREXErrors) / sizeof (char *)),
        ERRTEXT_TAG_TABLE,
        (void *)LREXErrors,
        ERRTEXT_TAG_MAXSTR,
        (void *)LREXMAX_ERROR_LEN,
        TAG_END,
        0 };

#endif /* __LREXERROR_H */
