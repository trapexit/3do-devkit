/*
        File:		Sprite.h

        Contains:	xxx put contents here xxx

        Written by:	David Maynard

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	  4/3/93	JAY		first checked in

        To Do:
*/

#include "debug.h"
#include "folio.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
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
