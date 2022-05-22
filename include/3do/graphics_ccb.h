#pragma include_only_once

#include "graphics_celdata.h"
#include "graphics_coord.h"
#include "types_ints.h"

typedef struct CCB CCB;
struct CCB
{
  uint32 ccb_Flags;

  CCB     *ccb_NextPtr;
  CelData *ccb_SourcePtr;
  void    *ccb_PLUTPtr;

  Coord  ccb_XPos;
  Coord  ccb_YPos;
  int32  ccb_HDX;
  int32  ccb_HDY;
  int32  ccb_VDX;
  int32  ccb_VDY;
  int32  ccb_HDDX;
  int32  ccb_HDDY;
  uint32 ccb_PIXC;
  uint32 ccb_PRE0;
  uint32 ccb_PRE1;

  /* These are special fields, tacked on to support some of the
   * rendering functions.
   */
  int32 ccb_Width;
  int32 ccb_Height;
};
