/******************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
 *reserved.
 **  This material contains confidential information that is the property of The
 *3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: sprite.c,v 1.5 1994/10/19 23:28:03 vertex Exp $
 **
 ******************************************************************************/

#include "sprite.h"
#include "graphics.h"
#include "list.h"
#include "nodes.h"
#include "parse3do.h"
#include "string.h"
#include "types.h"

/*****************************************************************************/

typedef struct Sprite
{
  MinNode link;
  int32 width;
  int32 height;
  ANIM *anim;
  CCB *ccb;
  frac16 xpos;
  frac16 ypos;
  frac16 xvel;
  frac16 yvel;
  frac16 zpos;
  frac16 zvel;
  frac16 tpos;
  frac16 tvel;
  Point corners[4];
} Sprite;

/*****************************************************************************/

bool zscaling = FALSE;
uint8 rotating = 0;

static int32 symorder;
static int32 xmax;
static int32 ymax;
static List spriteList;

#define TWOPI Convert32_F16 (256)
#define XMIN 0
#define YMIN 0
#define ZMIN 0x00001000
#define ZMAX 0x00018000
#define TMIN Convert32_F16 (0)
#define TMAX Convert32_F16 (256)
#define MAXSYM 32

static frac16 symsin[MAXSYM];
static frac16 symcos[MAXSYM];
static frac16 xcenter;
static frac16 ycenter;

/*****************************************************************************/

int32
SetSymOrder (int32 nsym)
{
  int32 i;
  frac16 theta, rem, tinc;

  symorder = nsym;
  if (nsym <= 1)
    symorder = 1;
  else if (nsym >= MAXSYM)
    symorder = 1;

  tinc = DivRemSF16 (&rem, TWOPI, Convert32_F16 (symorder));
  theta = 0;
  xcenter = Convert32_F16 (xmax >> 1);
  ycenter = Convert32_F16 (ymax >> 1);

  for (i = 0; i < symorder; i++)
    {
      symsin[i] = SinF16 (theta);
      symcos[i] = CosF16 (theta);
      theta += tinc;
    }

  return symorder;
}

/*****************************************************************************/

static void
Bounce (Sprite *s, int32 x, int32 y)
{
  if ((x < XMIN) && (s->xvel < 0))
    s->xvel = 0 - s->xvel;

  if ((x > xmax) && (s->xvel > 0))
    s->xvel = 0 - s->xvel;

  if ((y < YMIN) && (s->yvel < 0))
    s->yvel = 0 - s->yvel;

  if ((y > ymax) && (s->yvel > 0))
    s->yvel = 0 - s->yvel;
}

/*****************************************************************************/

static void
BounceZ (Sprite *s, frac16 z)
{
  if ((z < ZMIN) && (s->zvel < 0))
    s->zvel = 0 - s->zvel;

  if ((z > ZMAX) && (s->zvel > 0))
    s->zvel = 0 - s->zvel;
}

/*****************************************************************************/

static void
Rotate (Sprite *s, int32 xc, int32 yc, frac16 fw, frac16 fh)
{
  frac16 theta;
  frac16 wover2, hover2;
  frac16 ftop, fleft, fbot, fright;
  frac16 fx, fy;
  frac16 CosTheta, SinTheta;
  int32 x, y;

  theta = s->tpos + s->tvel;
  if (theta >= TMAX)
    theta = theta - TMAX;

  if (theta < TMIN)
    theta = theta + TMAX;

  s->tpos = theta;
  wover2 = fw >> 1;
  hover2 = fh >> 1;
  fleft = -wover2;
  ftop = hover2;
  fright = wover2;
  fbot = -hover2;
  CosTheta = CosF16 (theta);
  SinTheta = SinF16 (theta);

  fx = MulSF16 (fleft, CosTheta) - MulSF16 (ftop, SinTheta);
  fy = MulSF16 (fleft, SinTheta) + MulSF16 (ftop, CosTheta);
  x = ConvertF16_32 (fx) + xc;
  y = ConvertF16_32 (fy) + yc;
  s->corners[0].pt_X = x;
  s->corners[0].pt_Y = y;
  Bounce (s, x, y);

  fx = MulSF16 (fright, CosTheta) - MulSF16 (ftop, SinTheta);
  fy = MulSF16 (fright, SinTheta) + MulSF16 (ftop, CosTheta);
  x = ConvertF16_32 (fx) + xc;
  y = ConvertF16_32 (fy) + yc;
  s->corners[1].pt_X = x;
  s->corners[1].pt_Y = y;
  Bounce (s, x, y);

  fx = MulSF16 (fright, CosTheta) - MulSF16 (fbot, SinTheta);
  fy = MulSF16 (fright, SinTheta) + MulSF16 (fbot, CosTheta);
  x = ConvertF16_32 (fx) + xc;
  y = ConvertF16_32 (fy) + yc;
  s->corners[2].pt_X = x;
  s->corners[2].pt_Y = y;
  Bounce (s, x, y);

  if (rotating == 2)
    {
      s->corners[2].pt_X = (s->corners[1].pt_X + s->corners[1].pt_X) >> 2;
      s->corners[2].pt_Y = (s->corners[1].pt_Y + s->corners[1].pt_Y) >> 2;
      Bounce (s, x, y);
    }

  fx = MulSF16 (fleft, CosTheta) - MulSF16 (fbot, SinTheta);
  fy = MulSF16 (fleft, SinTheta) + MulSF16 (fbot, CosTheta);
  x = ConvertF16_32 (fx) + xc;
  y = ConvertF16_32 (fy) + yc;
  s->corners[3].pt_X = x;
  s->corners[3].pt_Y = y;
  Bounce (s, x, y);
}

/*****************************************************************************/

static void
CopyCorner (Point *from, Point *to)
{
  int32 i;

  for (i = 0; i < 4; i++)
    {
      to[i].pt_X = from[i].pt_X;
      to[i].pt_Y = from[i].pt_Y;
    }
}

/*****************************************************************************/

static void
XFormCorners (Point *from, int32 symindex)
{
  int32 i;
  frac16 fx, fy, tx, ty;
  frac16 CosTheta, SinTheta;

  CosTheta = symcos[symindex];
  SinTheta = symsin[symindex];
  for (i = 0; i < 4; i++)
    {
      fx = Convert32_F16 (from[i].pt_X) - xcenter;
      fy = Convert32_F16 (from[i].pt_Y) - ycenter;
      tx = MulSF16 (fx, CosTheta) - MulSF16 (fy, SinTheta);
      ty = MulSF16 (fx, SinTheta) + MulSF16 (fy, CosTheta);
      from[i].pt_X = ConvertF16_32 (tx + xcenter);
      from[i].pt_Y = ConvertF16_32 (ty + ycenter);
    }
}

/*****************************************************************************/

/* Prepare this module for use */
void
InitSprites (uint32 bitmapWidth, uint32 bitmapHeight)
{
  xmax = bitmapWidth;
  ymax = bitmapHeight;
  symorder = 1;

  InitList (&spriteList, "Sprite List");
}

/*****************************************************************************/

/* Load an anim, build a Sprite structure for it, and link it into the
 * sprite list
 */
Err
LoadSprite (const char *name, frac16 xvel, frac16 yvel, frac16 zvel,
            frac16 tvel)
{
  CCB *ccb;
  Sprite *s;
  Err result;

  s = (Sprite *)AllocMem (sizeof (Sprite), MEMTYPE_ANY);
  if (s)
    {
      s->anim = LoadAnim ((char *)name, MEMTYPE_CEL);
      if (s->anim)
        {
          ccb = GetAnimCel (s->anim, 0);
          ccb->ccb_Flags |= CCB_ACE;
          s->ccb = ccb;
          s->width = ccb->ccb_Width;
          s->height = ccb->ccb_Height;
          s->xpos = Convert32_F16 (0);
          s->ypos = Convert32_F16 (0);
          s->zpos = Convert32_F16 (1);
          s->xvel = xvel;
          s->yvel = yvel;
          s->zvel = zvel;
          s->tvel = tvel;
          s->corners[0].pt_X = 0;
          s->corners[0].pt_Y = 0;
          s->corners[1].pt_X = ccb->ccb_Width - 1;
          s->corners[1].pt_Y = 0;
          s->corners[2].pt_X = ccb->ccb_Width - 1;
          s->corners[2].pt_Y = ccb->ccb_Height - 1;
          s->corners[3].pt_X = 0;
          s->corners[3].pt_Y = ccb->ccb_Height - 1;

          AddHead (&spriteList, (Node *)s);

          return 0; /* success */
        }
      else
        {
          /* Couldn't load anim */
          result = -1; /* generic failure code */
        }
      FreeMem (s, sizeof (Sprite));
    }
  else
    {
      /* not enough memory for sprite structure */
      result = NOMEM;
    }

  return result;
}

/*****************************************************************************/

/* Unload all previously loaded sprites, freeing the associated resources */
void
UnloadSprites (void)
{
  Sprite *s;

  while (TRUE)
    {
      s = (Sprite *)RemHead (&spriteList);
      if (!s)
        break;

      UnloadAnim (s->anim);
      FreeMem (s, sizeof (Sprite));
    }
}

/*****************************************************************************/

/* Jiggle the sprites around a bit */
void
MoveSprites (void)
{
  int32 xc, yc, top, bot, left, right;
  int32 w, h, wover2, hover2;
  frac16 fw, fh;
  Sprite *s;

  ScanList (&spriteList, s, Sprite)
    {
      if (zscaling)
        {
          s->zpos += s->zvel;
          BounceZ (s, s->zpos);
          fw = MulSF16 (Convert32_F16 (s->width), s->zpos);
          fh = MulSF16 (Convert32_F16 (s->height), s->zpos);
          w = ConvertF16_32 (fw);
          h = ConvertF16_32 (fh);
        }
      else
        {
          w = s->width;
          h = s->height;
          s->zpos = Convert32_F16 (1);
          fw = Convert32_F16 (s->width);
          fh = Convert32_F16 (s->height);
        }
      wover2 = w >> 1;
      hover2 = h >> 1;
      s->xpos += s->xvel;
      s->ypos += s->yvel;
      xc = ConvertF16_32 (s->xpos);
      yc = ConvertF16_32 (s->ypos);
      top = yc - hover2;
      bot = top + h - 1;
      left = xc - wover2;
      right = left + w - 1;
      if (rotating == 0)
        {
          s->corners[0].pt_X = left;
          s->corners[0].pt_Y = top;
          Bounce (s, left, top);

          s->corners[1].pt_X = right;
          s->corners[1].pt_Y = top;
          Bounce (s, right, top);

          s->corners[2].pt_X = right;
          s->corners[2].pt_Y = bot;
          Bounce (s, right, bot);

          s->corners[3].pt_X = left;
          s->corners[3].pt_Y = bot;
          Bounce (s, left, bot);
        }
      else
        {
          Rotate (s, xc, yc, fw, fh);
        }
    }
}

/*****************************************************************************/

/* Draw all the sprites to the specified bitmap */
void
DrawSprites (Item bitmapItem)
{
  Sprite *s;
  Point box[4];
  int32 i;

  if (symorder == 1)
    {
      ScanList (&spriteList, s, Sprite)
        {
          MapCel (s->ccb, s->corners);
          DrawCels (bitmapItem, s->ccb);
        }
    }
  else
    {
      ScanList (&spriteList, s, Sprite)
        {
          CopyCorner (s->corners, box);
          for (i = 0; i < symorder; i++)
            {
              CopyCorner (box, s->corners);
              XFormCorners (s->corners, i);
              MapCel (s->ccb, s->corners);
              DrawCels (bitmapItem, s->ccb);
            }
        }
    }
}
