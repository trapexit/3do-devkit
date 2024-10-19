#ifndef __ANIMERROR_H
#define __ANIMERROR_H

#pragma force_top_level
#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: animerror.h,v 1.3 1994/10/27 21:30:32 vertex Exp $
**
******************************************************************************/

#include "operror.h"

#define ER_ANIM MakeErrId ('A', 'n')

#define ANIMLOADIMAGE_ERR MAKETERR (ER_ANIM, ER_SEVERE, ER_C_NSTND, 1)
#define ANIMLOADANIM_ERR MAKETERR (ER_ANIM, ER_SEVERE, ER_C_NSTND, 2)
#define ANIMNUMFRAMES_ERR MAKETERR (ER_ANIM, ER_SEVERE, ER_C_NSTND, 3)
#define ANIMGRAPHICS_ERR MAKETERR (ER_ANIM, ER_SEVERE, ER_C_NSTND, 4)
#define ANIMALLOC_ERR MAKETERR (ER_ANIM, ER_SEVERE, ER_C_NSTND, 5)

/*
   The error text to print out with PrintfSysErr()
*/
static char *AnimErrors[] = {
  "no error",

  /* ANIMLOADIMAGE_ERR */
  "Error in loading background image",

  /* ANIMLOADANIM_ERR */
  "Error in loading anim or cel",

  /* ANIMNUMFRAMES_ERR */
  "Loaded anim contains no frames",

  /* ANIMGRAPHICS_ERR */
  "Unable to open graphics",

  /* ANIMALLOC_ERR */
  "Unable to allocate appropriate memory",

};

#define ANIMMAX_ERROR_LEN 40

/*
   Tag args for the error text item
*/
static TagArg AnimErrorTags[]
    = { TAG_ITEM_NAME,
        (void *)"animsample",
        ERRTEXT_TAG_OBJID,
        (void *)((ER_TASK << ERR_IDSIZE) | ER_ANIM),
        ERRTEXT_TAG_MAXERR,
        (void *)(sizeof (AnimErrors) / sizeof (char *)),
        ERRTEXT_TAG_TABLE,
        (void *)AnimErrors,
        ERRTEXT_TAG_MAXSTR,
        (void *)ANIMMAX_ERROR_LEN,
        TAG_END,
        0 };

#endif /* __ANIMERROR_H */
