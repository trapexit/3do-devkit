#pragma once
#include "graphics_celdata.h"
#include "graphics_coord.h"
#include "types_ints.h"

typedef struct CCB CCB;
struct CCB
{
  u32 ccb_Flags;

  CCB     *ccb_NextPtr;
  CelData *ccb_SourcePtr;
  void    *ccb_PLUTPtr;

  Coord  ccb_XPos;
  Coord  ccb_YPos;
  s32  ccb_HDX;
  s32  ccb_HDY;
  s32  ccb_VDX;
  s32  ccb_VDY;
  s32  ccb_HDDX;
  s32  ccb_HDDY;
  u32 ccb_PIXC;
  u32 ccb_PRE0;
  u32 ccb_PRE1;

  /* These are special fields, tacked on to support some of the
   * rendering functions.
   */
  s32 ccb_Width;
  s32 ccb_Height;
};
