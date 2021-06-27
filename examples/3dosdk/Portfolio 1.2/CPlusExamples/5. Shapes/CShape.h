/*
	File:		CShape.h

	Contains:	Graphic shape class (header).

	Written by:	Paul A. Bauersfeld
				Jon A. Weiner

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

	<1>	 10/28/93	pab		New today.

	To Do:
*/
#ifndef _CSHAPE_H_
#define _CSHAPE_H_

#include "Portfolio.h"
#include "Utils3DO.h"

class CShape 
{
	public:
		CShape (ScreenContext *sc);
		CShape (ScreenContext *sc, Rect& r, Color c = 0xffffffff);
		
		virtual void Draw( void ) {}
		virtual void Erase( void );

		virtual void Move(long x, long y);
		virtual void MoveTo(long x, long y);
		
		Color GetColor( void );
		Rect *GetBBox(Rect *pBBox = 0L);
		ScreenContext *GetScreen( void );
		
		void MoveUp(long y);
		void MoveRight(long x);
		void MoveDown(long y);
		void MoveLeft(long x);

		void SetColor(Color c);
		void SetColor(uchar r, uchar g, uchar b);
		
		void SetScreen(ScreenContext *pSC);
		void SetBBox(Rect& r);
		
	protected:
		Rect			fBBox;
		GrafCon			fGrafCon;
		ScreenContext	*fScreenContext;
};

class CRect : public CShape
{
	public:
		CRect(ScreenContext *pSC, Rect &r, Color c  = 0xffffffff);
		 
		void Draw( void );
		void SetFrameFill(Boolean frameFill);
	
	private:
		Boolean fFrameFill;
};

class CFont;

class CText : public CShape
{
	public:
		CText(ScreenContext *pSC, char *pText);
		CText(ScreenContext *pSC, char *pText, CFont *pFont);
		 
		void Draw( void );
		
		long GetCharWidth (long lChar);
		long GetStringWidth (char *pString);

		void SetFont(CFont *pFont);
		void SetText(char *pText);
		
	private:
		char *fText;
		CFont *fFont;
};

class CLine : public CShape
{
	public:
		CLine(ScreenContext *pSC, Coord sx, Coord sy, Coord ex, Coord ey, Color c = 0xffffffff);
		 
		void Draw( void );
		
		void SetLine(Coord sx, Coord sy, Coord ex, Coord ey);
		
	private:
		Coord fStartPtX;
		Coord fStartPtY;
		Coord fEndPtX;
		Coord fEndPtY;
		long fSlope;
};

#endif