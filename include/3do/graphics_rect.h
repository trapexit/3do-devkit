#pragma once
#include "graphics_coord.h"

typedef struct Rect Rect;
struct Rect
{
  Coord rect_XLeft;
  Coord rect_YTop;
  Coord rect_XRight;
  Coord rect_YBottom;
};
