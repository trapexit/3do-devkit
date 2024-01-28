
#ifndef __ORBITERROR_H
#define __ORBITERROR_H

#pragma force_top_level
#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: orbiterror.h,v 1.2 1994/10/18 19:57:48 gyld Exp $
**
******************************************************************************/

#include "operror.h"

#define ER_ORBIT MakeErrId ('O', 'r')

#define ORBITLOADIMAGE_ERR MAKETERR (ER_ORBIT, ER_SEVERE, ER_C_NSTND, 1)
#define ORBITLOADANIM_ERR MAKETERR (ER_ORBIT, ER_SEVERE, ER_C_NSTND, 2)
#define ORBITNUMFRAMES_ERR MAKETERR (ER_ORBIT, ER_SEVERE, ER_C_NSTND, 3)
#define ORBITGRAPHICS_ERR MAKETERR (ER_ORBIT, ER_SEVERE, ER_C_NSTND, 4)
#define ORBITALLOC_ERR MAKETERR (ER_ORBIT, ER_SEVERE, ER_C_NSTND, 5)
#define ORBITLOADSOUND_ERR MAKETERR (ER_ORBIT, ER_SEVERE, ER_C_NSTND, 6)
#define ORBITAUDIOCTX_ERR MAKETERR (ER_ORBIT, ER_SEVERE, ER_C_NSTND, 7)

/*
   The error text to print out with PrintfSysErr()
*/
static char *OrbitErrors[] = {
  "no error",

  /* ORBITLOADIMAGE_ERR */
  "Error in loading background image",

  /* ORBITLOADANIM_ERR */
  "Error in loading anim or cel",

  /* ORBITNUMFRAMES_ERR */
  "Loaded anim contains no frames",

  /* ORBITGRAPHICS_ERR */
  "Unable to open graphics",

  /* ORBITALLOC_ERR */
  "Unable to allocate appropriate memory",

  /* ORBITLOADSOUND_ERR */
  "Unable to load sound effect",

  /* ORBITAUDIOCTX_ERR */
  "Unable to allocate audio context",

};

#define ORBITMAX_ERROR_LEN 40

/*
   Tag args for the error text item
*/
static TagArg OrbitErrorTags[]
    = { TAG_ITEM_NAME,
        (void *)"3doorbit",
        ERRTEXT_TAG_OBJID,
        (void *)((ER_TASK << ERR_IDSIZE) | ER_ORBIT),
        ERRTEXT_TAG_MAXERR,
        (void *)(sizeof (OrbitErrors) / sizeof (char *)),
        ERRTEXT_TAG_TABLE,
        (void *)OrbitErrors,
        ERRTEXT_TAG_MAXSTR,
        (void *)ORBITMAX_ERROR_LEN,
        TAG_END,
        0 };

#endif /* _ORBITERROR_H */
