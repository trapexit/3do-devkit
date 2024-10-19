/*
        File:		CShape.cp

        Contains:	Graphic shape class.

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

        <1>	 10/28/93	pab		New today.

        To Do:
*/
#include "CShape.h"

#include "CPlusSwiHack.h"

CShape::CShape (ScreenContext *pSC)
{
  Rect r = { 0, 0, 0, 0 };

  this->SetScreen (pSC);
  this->SetBBox (r);
  this->SetColor (0xffffffff);
  SetBGPen (&fGrafCon, MakeRGB15 (255, 255, 255)); // always keep bg white
}

CShape::CShape (ScreenContext *pSC, Rect &r, Color c)
{
  this->SetScreen (pSC);
  this->SetBBox (r);
  this->SetColor (c);
  SetBGPen (&fGrafCon, MakeRGB15 (255, 255, 255)); // always keep bg white
}

void
CShape::Erase (void)
{
  GrafCon tmp = fGrafCon;

  tmp.gc_FGPen = tmp.gc_BGPen; // erase with bg color
  FillRect (fScreenContext->sc_BitmapItems[0], &tmp, &fBBox);
}

Rect *
CShape::GetBBox (Rect *pBBox)
{
  if (pBBox)
    {
      *pBBox = fBBox;
      return pBBox;
    }
  else
    return &fBBox;
}

Color
CShape::GetColor (void)
{
  return fGrafCon.gc_FGPen;
}

void
CShape::Move (long x, long y)
{
  fBBox.rect_XLeft += x;
  fBBox.rect_YTop += y;
  fBBox.rect_XRight += x;
  fBBox.rect_YBottom += y;
  ::MoveTo (&fGrafCon, fBBox.rect_XLeft, fBBox.rect_YTop);
}

void
CShape::MoveUp (long y)
{
  this->Move (0, -y);
}

void
CShape::MoveRight (long x)
{
  this->Move (x, 0);
}

void
CShape::MoveDown (long y)
{
  this->Move (0, y);
}

void
CShape::MoveLeft (long x)
{
  this->Move (-x, 0);
}

void
CShape::MoveTo (long x, long y)
{
  fBBox.rect_XRight = fBBox.rect_XRight - fBBox.rect_XLeft + x;
  fBBox.rect_YBottom = fBBox.rect_YBottom - fBBox.rect_YTop + y;
  fBBox.rect_XLeft = x;
  fBBox.rect_YTop = y;
  ::MoveTo (&fGrafCon, x, y);
}

void
CShape::SetBBox (Rect &r)
{
  fBBox = r;
}

void
CShape::SetColor (Color c)
{
  SetFGPen (&fGrafCon, c);
}

void
CShape::SetColor (uchar r, uchar g, uchar b)
{
  this->SetColor (MakeRGB15 (r, g, b));
}

void
CShape::SetScreen (ScreenContext *pSC)
{
  fScreenContext = pSC;
}
