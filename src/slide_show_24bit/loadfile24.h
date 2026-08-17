#pragma once

/******************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
 *reserved.
 **  This material contains confidential information that is the property of The
 *3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: loadfile24.h,v 1.3 1994/11/15 17:48:18 vertex Exp $
 **
 **  Load an image file and return its image control chunk.
 **
 ******************************************************************************/

#include "types.h"
#include "form3do.h"

s32 loadfile24(char *name,
                 void *buffer,
                 u32 buffersize,
                 u32 memtype,
                 VdlChunk **rawVDLPtr,
                 ImageCC *image,
                 s32 width,
                 s32 height);
