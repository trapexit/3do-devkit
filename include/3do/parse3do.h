#ifndef __PARSE3DO_H
#define __PARSE3DO_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**
**  Lib3DO datatypes and prototypes for handling Cel, Image, Anim, and
**  Sound data stored in standard 3DO file formats.
**
**  This header is all but obsolete.  It includes the current proper headers
**  (CelUtils, AnimUtils, etc).  It exists for compatibility for old code.
**
******************************************************************************/

#include "init3do.h"
#include "animutils.h"
#include "celutils.h"

/*----------------------------------------------------------------------------
 * prototypes
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

int32 GetFileSize(char *fn);
int32 ReadFile(char *filename, int32 size, int32 *buffer, int32 offset);

#ifdef __cplusplus
  }
#endif

#endif /* __PARSE3DO_H */
