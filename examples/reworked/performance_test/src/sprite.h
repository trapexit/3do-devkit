
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: sprite.h,v 1.3 1994/12/05 20:20:19 vertex Exp $
**
******************************************************************************/

#include "debug.h"
#include "debug3do.h"
#include "folio.h"
#include "form3do.h"
#include "graphics.h"
#include "hardware.h"
#include "init3do.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "parse3do.h"
#include "semaphore.h"
#include "stdlib.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "utils3do.h"

typedef struct Sprite
{
  struct Sprite *nxtSprite;
  long xcenter;
  long ycenter;
  long width;
  long height;
  ANIM *anim;
  frac16 xpos;
  frac16 ypos;
  frac16 xvel;
  frac16 yvel;
  frac16 zpos;
  frac16 zvel;
  frac16 tpos;
  frac16 tvel;
  Point corner[4];
} Sprite;

#if DEBUG
#define DBUG(x)                                                               \
  {                                                                           \
    kprintf x;                                                                \
  }
#else
#define DBUG(x)
#endif
extern Sprite *LoadSprite (char *name, frac16 xvel, frac16 yvel, frac16 zvel,
                           frac16 tvel);
extern int zscaling;
extern int rotating;
extern void DrawSprites (Item bitmapItem);
extern void MoveSprites (void);
extern Sprite *SpriteList;
