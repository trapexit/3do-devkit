#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	SetQuad		- Set an array of four points to conform to the rectangle
 *				  defined by left, top, right, and bottom.
 *				  	
 *	PARAMETERS
 *	
 *		r				- a ptr to an array of 4 Points.
 *		left,top, ...	- for coordinates that define a rectangle.
 *	
 */
void SetQuad( Point *r, Coord left, Coord top, Coord right, Coord bottom )
{
	r[0].pt_X =   left;
	r[0].pt_Y =   top;
	r[1].pt_X =   right;
	r[1].pt_Y =   top;
	r[2].pt_X =   right;
	r[2].pt_Y =   bottom;
	r[3].pt_X =   left;
	r[3].pt_Y =   bottom;
}
