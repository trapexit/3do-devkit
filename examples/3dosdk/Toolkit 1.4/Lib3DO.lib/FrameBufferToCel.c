#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/* FrameBufferToCel
	INPUTS: A screen item and a CCB ptr
	OUTPUTS: TRUE, if all went well
			 FALSE, otherwise
			 
	DESC:	Converts the L,R formatted image in the frame buffer where
			the CCB is current located and copies it to the CCB
*/

bool
FrameBufferToCel(Item iScreen,CCB *cel)
{
	#define		pixelAdjust 0x11111111
	int32		w, wlong, h, x, y;
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
	wlong = (w+1) >> 1;   							/* Must take care of odd widths */
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
		
		*(p + wlong) = srcPixel + pixelAdjust;					

		p++;											
		x+=2;
	  }
	  pBitMap += (bitmap->bm_Width);
	  y++;
	  if (bumpX)
	    x = left+1;
	  else x = left;
	  p+=wlong;
	}
	return true;
}
