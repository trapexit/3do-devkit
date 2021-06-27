#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	CenterRectf16	- Center the rect within the frame.
 *				  	
 *	PARAMETERS
 *		q		- the quad returned by Center.
 *		rect	- the rect to be centered.
 *		frame	- the frame within which to center the rect.
 *		
 */
void CenterRectf16( Point *q, Rectf16 *rect, Rectf16 *frame)
{
	frac16	xmid, ymid, ydiff, xdiff;
	frac16	rem;
	
	xmid = DivRemSF16(&rem, frame->rectf16_XRight, Convert32_F16(2));
	ymid = DivRemSF16(&rem, frame->rectf16_YBottom, Convert32_F16(2));

	ydiff = rect->rectf16_YBottom >> 1 ;
	xdiff = rect->rectf16_XRight>> 1;
	
	q[0].pt_X = ((xmid -  xdiff)+0x8000) >>16;
	q[0].pt_Y = ((ymid -  ydiff) +0x8000)>>16;
	q[1].pt_X = ((xmid +  xdiff)+0x8000) >>16;
	q[1].pt_Y = ((ymid -  ydiff) +0x8000)>>16;
	q[2].pt_X = ((xmid +  xdiff)+0x8000) >>16;
	q[2].pt_Y = ((ymid +  ydiff)+0x8000) >>16;
	q[3].pt_X = ((xmid -  xdiff) +0x8000)>>16;
	q[3].pt_Y = ((ymid +  ydiff) +0x8000)>>16;
}
