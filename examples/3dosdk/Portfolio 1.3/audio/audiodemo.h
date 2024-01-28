/* $Id: audiodemo.h,v 1.4 1994/03/01 01:16:39 peabody Exp $ */
/****************************************************************
**
**  Phil's Demo Library Includes
**
**  By:  Phil Burk
**
**  Copyright (C) 1993, 3DO Company.
**  All Rights Reserved
**  Confidential and Proprietary
**
*****************************************************************
**  940228 WJB  Added standard comment block.  Added some of my macros for
*later publication.
**  940228 WJB  Moved graphic externs from audiodemo.h to graphic_tools.h.
****************************************************************/

#pragma include_only_once
#ifndef __AUDIODEMO_H
#define __AUDIODEMO_H

#include "faders.h"
#include "graphic_tools.h"
#include "joytools.h"

/* -------------------- macros (!!! publish) */

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(x) ((x < 0) ? (-(x)) : (x))
#define CLIPRANGE(n, a, b)                                                    \
  ((n) < (a) ? (a) : (n) > (b) ? (b) : (n)) /* range clipping */

#endif /* __AUDIODEMO_H */
