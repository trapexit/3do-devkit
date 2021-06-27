#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	CenterCelOnScreen	- assumes screen sized frame for now.
 *				  	
 *	PARAMETERS
 *		cel			- the cel to center.
 *		
 */
void
CenterCelOnScreen (CCB *ccb)
{
Rectf16	celRect, frameRect;
Point	q[4];

	SetRectf16 (&frameRect, 0, 0, 320, 240);
	SetRectf16 (&celRect, 0, 0, ccb->ccb_Width, ccb->ccb_Height);
	CenterRectf16 (q, &celRect, &frameRect);
	MapCel (ccb, q);
}
