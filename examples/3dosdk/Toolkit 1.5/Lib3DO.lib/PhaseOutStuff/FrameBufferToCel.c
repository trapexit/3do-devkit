/*****************************************************************************
 *	File:			FrameBufferToCel.c 
 *
 *	Contains:		Routine to copy frame buffer contents to cel data buffer.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This routine uses the ARM chip to actually copy-and-convert the LRForm
 *	data from the screen to the cel's data buffer.  Slow.  You might want 
 *	to consider the CreateLRFormCel() function.
 ****************************************************************************/
 
#include "Utils3DO.h"

#define		pixelAdjust 0x11111111

Boolean FrameBufferToCel(Item iScreen, CCB *cel)
{
	int32		w, wordCount, h, x, y;
	int32		*p, top, left, secondPix;
	int32		*pBitMap, srcPixel;
	bool		bumpX;
	Screen 		*psScreen;
	Bitmap		*bitmap;


	psScreen = (Screen*)LookupItem(iScreen);

	if (psScreen == NULL)
		return false;
	else
		bitmap = psScreen->scr_TempBitmap;
		
	/* Screen is actually 640x120 so we need to adjust the top and height to reflect that */
	top = cel->ccb_YPos >> 17;  						/* 17 so that we divide by 2 */
	h = cel->ccb_Height >> 1;

	left = cel->ccb_XPos >> 16;
	w = cel->ccb_Width;
	wordCount = (w+1) >> 1;   							/* Must take care of odd widths */
	p = (int32 *)cel->ccb_SourcePtr;
	
	pBitMap = (int32 *)bitmap->bm_Buffer;

	pBitMap += top * (bitmap->bm_Width); 			/* We need to advance by top scan lines */ 

	y = top;
	x = left;

	while (y <top+h)
	{
	  bumpX = false;
	  while (x<left+w)
	  {
		if ((x+1) < left+w)
		  secondPix = x+1;
		else {
		  secondPix = (bitmap->bm_Width);
		  bumpX = true;
		}
				
		srcPixel = (*(pBitMap + x) & 0xFFFF0000) | ((*(pBitMap + secondPix) & 0xFFFF0000) >> 16);	

		*p = srcPixel + pixelAdjust;									
		
		srcPixel = ((*(pBitMap + x) & 0x0000FFFF) << 16) |
		     (*(pBitMap + secondPix) & 0x0000FFFF);  	
		
		*(p + wordCount) = srcPixel + pixelAdjust;					

		p++;											
		x+=2;
	  }
	  pBitMap += (bitmap->bm_Width);
	  y++;
	  if (bumpX)
	    x = left+1;
	  else x = left;
	  p+=wordCount;
	}
	return true;
}
