/*
        File:		CLine.cp

        Contains:	Line shape subclass.

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	� 1994 by The 3DO Company. All rights reserved.
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

CLine::CLine (ScreenContext *pSC, Coord sx, Coord sy, Coord ex, Coord ey,
              Color c)
    : CShape (pSC)
{
  this->SetLine (sx, sy, ex, ey);
  this->SetColor (c);
}

void
CLine::Draw (void)
{
  DrawTo (fScreenContext->sc_BitmapItems[0], &fGrafCon, fEndPtX, fEndPtY);
}

void
CLine::SetLine (Coord sx, Coord sy, Coord ex, Coord ey)
{
  Rect r;

  fStartPtX = sx;
  fStartPtY = sy;
  fEndPtX = ex;
  fEndPtY = ey;

  if (fStartPtX > fEndPtX)
    {
      r.rect_XRight = fStartPtX;
      r.rect_XLeft = fEndPtX;
    }
  else
    {
      r.rect_XRight = fEndPtX;
      r.rect_XLeft = fStartPtX;
    }

  if (fStartPtY > fEndPtY)
    {
      r.rect_YBottom = fStartPtY;
      r.rect_YTop = fEndPtY;
    }
  else
    {
      r.rect_YBottom = fEndPtY;
      r.rect_YTop = fStartPtY;
    }

  this->SetBBox (r);

  this->MoveTo (fStartPtX, fStartPtY);
}
