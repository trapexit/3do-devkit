/*
        File:		Sprite.c

        Contains:	xxx put contents here xxx

        Written by:	David Maynard

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <3>	 8/11/93	JAY		change calling sequence
   to LoadAnim
                 <2>	  4/7/93	JAY		MEMTYPE_SPRITE is now
   MEMTYPE_CEL for Cardinal2
                 <1>	  4/3/93	JAY		first checked in

        To Do:
*/

#include "sprite.h"
frac16 zmin = 0x00001000;
frac16 zmax = 0x00018000;

long xmin = 0;
long xmax = 320;

long ymin = 0;
long ymax = 240;

frac16 tmin = Convert32_F16 (0);
frac16 tmax = Convert32_F16 (256);
frac16 twopi = Convert32_F16 (256);
static char gname[64];
int zscaling = 0;
int rotating = 0;
extern long symorder;
#define MAXSYM 16
frac16 symsin[MAXSYM];
frac16 symcos[MAXSYM];
frac16 XCENTER;
frac16 YCENTER;
extern int RedSi = 0;
extern Sprite *SpriteList = NULL;

extern void SetSymOrder (long nsym);

extern long SCE;
extern long SCEFLAG;

void
SetSymOrder (long nsym)
{
  int i;
  frac16 theta, rem, tinc;
  ;
  symorder = nsym;
  if (nsym <= 1)
    symorder = 1;
  if (nsym >= MAXSYM)
    symorder = 1;
  tinc = DivRemSF16 (&rem, twopi, Convert32_F16 (symorder));
  theta = 0;
  XCENTER = Convert32_F16 (320 >> 1);
  YCENTER = Convert32_F16 (240 >> 1);
  for (i = 0; i < symorder; i++)
    {
      symsin[i] = SinF16 (theta);
      symcos[i] = CosF16 (theta);
      theta += tinc;
    }
}

Sprite *
LoadSprite (char *name, frac16 xvel, frac16 yvel, frac16 zvel, frac16 tvel)
{
  CCB *ccb;
  Sprite *spr, *last, *prev;
  long x, y, w, h;
  ANIM *lanim;
  strcpy (gname, name);
  lanim = LoadAnim (gname, MEMTYPE_CEL);
  if ((lanim == 0))
    {
      DBUG (("Error in LoadAnim call %s \n", gname));
      return NULL;
    }
  if ((spr = (Sprite *)ALLOCMEM (sizeof (Sprite), MEMTYPE_CEL)) == NULL)
    return NULL;
  x = 0;
  y = 0;
  ccb = GetAnimCel (lanim, 0);
  if (RedSi)
    ccb->ccb_Flags = (ccb->ccb_Flags & (~CCB_ACE)) | SCEFLAG;
  w = ccb->ccb_Width;
  h = ccb->ccb_Height;
  spr->nxtSprite = NULL;
  spr->anim = lanim;
  spr->xcenter = w >> 1;
  spr->ycenter = h >> 1;
  spr->width = w;
  spr->height = h;
  spr->xpos = Convert32_F16 (0);
  spr->ypos = Convert32_F16 (0);
  spr->zpos = Convert32_F16 (1);
  spr->xvel = xvel;
  spr->yvel = yvel;
  spr->zvel = zvel;
  spr->tvel = tvel;
  spr->corner[0].pt_X = 0;
  spr->corner[0].pt_Y = 0;
  spr->corner[1].pt_X = w - 1;
  spr->corner[1].pt_Y = 0;
  spr->corner[2].pt_X = w - 1;
  spr->corner[2].pt_Y = h - 1;
  spr->corner[3].pt_X = 0;
  spr->corner[3].pt_Y = h - 1;
  last = SpriteList;
  if (last == NULL)
    {
      SpriteList = spr;
      return spr;
    }
  while (last != NULL)
    {
      prev = last;
      last = last->nxtSprite;
    }
  prev->nxtSprite = spr;
  return spr;
}

int32
AddSprite ()
{
  Sprite *spr;
  Sprite *head;
  long w, h;
  if ((spr = (Sprite *)ALLOCMEM (sizeof (Sprite), MEMTYPE_CEL)) == NULL)
    return -1;
  head = SpriteList;
  memcpy (spr, head, sizeof (Sprite));
  w = spr->width;
  h = spr->height;
  spr->xcenter = w >> 1;
  spr->ycenter = h >> 1;
  spr->width = w;
  spr->height = h;
  spr->xpos = Convert32_F16 (0);
  spr->ypos = Convert32_F16 (0);
  spr->zpos = Convert32_F16 (1);
  spr->corner[0].pt_X = 0;
  spr->corner[0].pt_Y = 0;
  spr->corner[1].pt_X = w - 1;
  spr->corner[1].pt_Y = 0;
  spr->corner[2].pt_X = w - 1;
  spr->corner[2].pt_Y = h - 1;
  spr->corner[3].pt_X = 0;
  spr->corner[3].pt_Y = h - 1;
  spr->nxtSprite = head;
  SpriteList = spr;
  return (1);
}

int32
DeleteSprite ()
{
  Sprite *spr;
  Sprite *head;

  head = SpriteList;
  spr = head->nxtSprite;
  if (spr == NULL)
    return (1); /* keep at least one sprite around */
  FREEMEM (head, sizeof (Sprite));
  SpriteList = spr;
  return (1);
}

void
bounce (Sprite *s, long x, long y)
{
  if (x < xmin)
    if (s->xvel < 0)
      s->xvel = 0 - s->xvel;
  if (x > xmax)
    if (s->xvel > 0)
      s->xvel = 0 - s->xvel;
  if (y < ymin)
    if (s->yvel < 0)
      s->yvel = 0 - s->yvel;
  if (y > ymax)
    if (s->yvel > 0)
      s->yvel = 0 - s->yvel;
}

void
bouncez (Sprite *s, frac16 z)
{
  if (zscaling == 0)
    return;
  if (z < zmin)
    if (s->zvel < 0)
      s->zvel = 0 - s->zvel;
  if (z > zmax)
    if (s->zvel > 0)
      s->zvel = 0 - s->zvel;
}
void
rotate (Sprite *s, long xc, long yc, frac16 fw, frac16 fh)
{
  frac16 theta;
  frac16 wover2, hover2;
  frac16 ftop, fleft, fbot, fright;
  frac16 fx, fy;
  frac16 CosTheta, SinTheta;
  long x, y;
  theta = s->tpos + s->tvel;
  if (theta >= tmax)
    theta = theta - tmax;
  if (theta < tmin)
    theta = theta + tmax;
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
  s->corner[0].pt_X = x;
  s->corner[0].pt_Y = y;
  bounce (s, x, y);

  fx = MulSF16 (fright, CosTheta) - MulSF16 (ftop, SinTheta);
  fy = MulSF16 (fright, SinTheta) + MulSF16 (ftop, CosTheta);
  x = ConvertF16_32 (fx) + xc;
  y = ConvertF16_32 (fy) + yc;
  s->corner[1].pt_X = x;
  s->corner[1].pt_Y = y;
  bounce (s, x, y);

  fx = MulSF16 (fright, CosTheta) - MulSF16 (fbot, SinTheta);
  fy = MulSF16 (fright, SinTheta) + MulSF16 (fbot, CosTheta);
  x = ConvertF16_32 (fx) + xc;
  y = ConvertF16_32 (fy) + yc;
  /* this was a bug but it looked so good I made it a feature */
  s->corner[2].pt_X = x;
  s->corner[2].pt_Y = y;
  bounce (s, x, y);
  if (rotating == 2)
    {
      s->corner[2].pt_X = (s->corner[1].pt_X + s->corner[1].pt_X) >> 2;
      s->corner[2].pt_Y = (s->corner[1].pt_Y + s->corner[1].pt_Y) >> 2;
      bounce (s, x, y);
    }
  fx = MulSF16 (fleft, CosTheta) - MulSF16 (fbot, SinTheta);
  fy = MulSF16 (fleft, SinTheta) + MulSF16 (fbot, CosTheta);
  x = ConvertF16_32 (fx) + xc;
  y = ConvertF16_32 (fy) + yc;
  s->corner[3].pt_X = x;
  s->corner[3].pt_Y = y;
  bounce (s, x, y);
}
void
CopyCorner (Point *from, Point *to)
{
  int i;
  for (i = 0; i < 4; i++)
    {
      to[i].pt_X = from[i].pt_X;
      to[i].pt_Y = from[i].pt_Y;
    }
}

void
XFormCorners (Point *from, int symindex)
{
  int i;
  frac16 fx, fy, tx, ty;
  frac16 CosTheta, SinTheta;
  CosTheta = symcos[symindex];
  SinTheta = symsin[symindex];
  for (i = 0; i < 4; i++)
    {
      fx = Convert32_F16 (from[i].pt_X) - XCENTER;
      fy = Convert32_F16 (from[i].pt_Y) - YCENTER;
      tx = MulSF16 (fx, CosTheta) - MulSF16 (fy, SinTheta);
      ty = MulSF16 (fx, SinTheta) + MulSF16 (fy, CosTheta);
      from[i].pt_X = ConvertF16_32 (tx + XCENTER);
      from[i].pt_Y = ConvertF16_32 (ty + YCENTER);
    }
}

void
MoveSprites ()
{
  long xc, yc, top, bot, left, right;
  long w, h, wover2, hover2;
  frac16 fw, fh;
  Sprite *s;
  s = SpriteList;
  while (s != NULL)
    {
      if (zscaling == 0)
        {
          w = s->width;
          h = s->height;
          s->zpos = Convert32_F16 (1);
          fw = Convert32_F16 (s->width);
          fh = Convert32_F16 (s->height);
        }
      else
        {
          s->zpos += s->zvel;
          bouncez (s, s->zpos);
          fw = MulSF16 (Convert32_F16 (s->width), s->zpos);
          fh = MulSF16 (Convert32_F16 (s->height), s->zpos);
          w = ConvertF16_32 (fw);
          h = ConvertF16_32 (fh);
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
          s->corner[0].pt_X = left;
          s->corner[0].pt_Y = top;
          bounce (s, left, top);
          s->corner[1].pt_X = right;
          s->corner[1].pt_Y = top;
          bounce (s, right, top);
          s->corner[2].pt_X = right;
          s->corner[2].pt_Y = bot;
          bounce (s, right, bot);
          s->corner[3].pt_X = left;
          s->corner[3].pt_Y = bot;
          bounce (s, left, bot);
        }
      else /* rotating */
        {
          rotate (s, xc, yc, fw, fh);
        }
      s = s->nxtSprite;
    }
}
void
DrawSprites (Item bitmapItem)
{
  Sprite *s;
  CCB *ccb;
  Point box[4];
  int32 err;
  int i;
  s = SpriteList;
  if (symorder == 1)
    {
      while (s != NULL)
        {
          ccb = GetAnimCel (s->anim, 0x10000);
          if (RedSi)
            ccb->ccb_Flags = (ccb->ccb_Flags & (~CCB_ACE)) | SCEFLAG;
          MapCel (ccb, &s->corner[0]);
          err = DrawCels (bitmapItem, ccb);
          if (err < 0)
            kprintf ("Error in DrawCels: %d \n", err);
          s = s->nxtSprite;
        }
    }
  else
    {
      while (s != NULL)
        {
          ccb = GetAnimCel (s->anim, 0x10000);
          if (RedSi)
            ccb->ccb_Flags = (ccb->ccb_Flags & (~CCB_ACE)) | SCEFLAG;
          CopyCorner (&s->corner[0], box);
          for (i = 0; i < (symorder); i++)
            {
              CopyCorner (box, &s->corner[0]);
              XFormCorners (&s->corner[0], i);
              MapCel (ccb, &s->corner[0]);
              err = DrawCels (bitmapItem, ccb);
              if (err < 0)
                kprintf ("Error in DrawCels: %d \n", err);
            }
          s = s->nxtSprite;
        }
    }
}
