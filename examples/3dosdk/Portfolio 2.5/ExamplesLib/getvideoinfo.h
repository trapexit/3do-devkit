#ifndef _GETVIDEOINFO_H
#define _GETVIDEOINFO_H

#pragma force_top_level
#pragma include_only_once

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: getvideoinfo.h,v 1.2 1994/11/11 05:13:03 ceckhaus Exp $
**
******************************************************************************/

#include "graphics.h"
#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define NTSC_SCREEN_WIDTH 320
#define NTSC_SCREEN_HEIGHT 240
#define NTSC_DISPLAY(_displayType) (_displayType == DI_TYPE_NTSC)

#define PAL1_SCREEN_WIDTH 320
#define PAL2_SCREEN_WIDTH 384
#define PAL1_SCREEN_HEIGHT 240
#define PAL2_SCREEN_HEIGHT 288
#define PAL_DISPLAY(_displayType)                                             \
  ((_displayType == DI_TYPE_PAL1) || (_displayType == DI_TYPE_PAL2))

  int32 GetDisplayType (void);
  int32 GetScreenWidth (int32 displayType);
  int32 GetScreenHeight (int32 displayType);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
