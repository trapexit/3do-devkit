/*
        File:		CRect.cp

        Contains:	Rectangle shape subclass.

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

CRect::CRect (ScreenContext *pSC, Rect &r, Color c) : CShape (pSC, r, c)
{
  fFrameFill = 1; // default rectangle drawing is done with filling
}

void
CRect::Draw (void)
{
  if (fFrameFill)
    {
      FillRect (fScreenContext->sc_BitmapItems[0], &fGrafCon, &fBBox);
    }
  else
    {
      CLine line (fScreenContext, 0, 0, 0, 0, GetColor ());

      line.SetLine (fBBox.rect_XLeft, fBBox.rect_YTop, fBBox.rect_XRight,
                    fBBox.rect_YTop);
      line.Draw ();
      line.SetLine (fBBox.rect_XRight, fBBox.rect_YTop, fBBox.rect_XRight,
                    fBBox.rect_YBottom);
      line.Draw ();
      line.SetLine (fBBox.rect_XRight, fBBox.rect_YBottom, fBBox.rect_XLeft,
                    fBBox.rect_YBottom);
      line.Draw ();
      line.SetLine (fBBox.rect_XLeft, fBBox.rect_YBottom, fBBox.rect_XLeft,
                    fBBox.rect_YTop);
      line.Draw ();
    }
}

void
CRect::SetFrameFill (Boolean frameFill)
{
  fFrameFill = frameFill;
}
