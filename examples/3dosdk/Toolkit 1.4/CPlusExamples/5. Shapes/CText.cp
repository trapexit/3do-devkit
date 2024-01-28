/*
        File:		CText.cp

        Contains:	Text shape subclass.

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

CText::CText (ScreenContext *pSC, char *pText) : CShape (pSC)
{
  fText = pText;
  this->SetFont (0L);
}

CText::CText (ScreenContext *pSC, char *pText, CFont *pFont) : CShape (pSC)
{
  fText = pText;
  this->SetFont (pFont);
}

void
CText::Draw (void)
{
  DrawText8 (&fGrafCon, fScreenContext->sc_BitmapItems[0], fText);
}

long
CText::GetCharWidth (long lChar)
{
  // UNIMPLEMENTED FOR NOW
  return 0;
}

long
CText::GetStringWidth (char *pString)
{
  long width = 0;
  char *pTmp = pString;

  while (*pTmp)
    {
      width += this->GetCharWidth (*pTmp++);
    }

  return width;
}

void
CText::SetFont (CFont *pFont)
{
  fFont = pFont;
}

void
CText::SetText (char *pText)
{
  fText = pText;
}
