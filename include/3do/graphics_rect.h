#ifndef TDO_GRAPHICS_RECT_H_INCLUDED
#define TDO_GRAPHICS_RECT_H_INCLUDED

#include "graphics_coord.h"

typedef struct Rect Rect;
struct Rect
{
  Coord rect_XLeft;
  Coord rect_YTop;
  Coord rect_XRight;
  Coord rect_YBottom;
};

#endif
