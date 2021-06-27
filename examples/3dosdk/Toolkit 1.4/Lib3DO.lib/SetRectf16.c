#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	SetRectf16		- Set rectangle to left, top, right, and bottom.
 *				  	
 *	PARAMETERS
 *	
 *		r				- a ptr to Rectangle.
 *		left,top, ...	- for coordinates that define a rectangle.
 *	
 */
void SetRectf16( Rectf16 *r, Coord left, Coord top, Coord right, Coord bottom )
{
	r->rectf16_XLeft	= Convert32_F16 (left);
	r->rectf16_YTop		= Convert32_F16 (top);
	r->rectf16_XRight	= Convert32_F16 (right);
	r->rectf16_YBottom	= Convert32_F16 (bottom);
}
