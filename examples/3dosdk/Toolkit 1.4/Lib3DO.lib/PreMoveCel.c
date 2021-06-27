#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	PreMoveCel	- Calculate the increments for each corner of a Cel in order to move it from
 *				  the quadrilateral defined by beginQuad to the quadrilateral defined by endQuad.
 *				  The numberOfFrames determines how many iterations will be needed to complete
 *				  the movement.
 *				  
 *	PARAMETERS
 *	
 *		ccb				- this CCB's position fields will be updated
 *		beginQuad		- the four Coords that define the Cel's starting position
 *		endQuad			- the four Coords that define the Cel's ending position
 *		numberOfFrames	- the number of iterations to go from beginning to end {1..32767}
 *		pMove			- ptr to the MoveRec to contain increments and current Quad.
 *	
 */
void
PreMoveCel (CCB *ccb, Point *beginQuad, Point *endQuad, int32 numberOfFrames, MoveRec *pMove)
{
register int	i;
frac16			rem;
Point			q[4];

	for (i=0;i<4;i++) {
		pMove->curQuadf16[i].xVector = Convert32_F16 (beginQuad[i].pt_X);
		pMove->curQuadf16[i].yVector = Convert32_F16 (beginQuad[i].pt_Y);
		
		q[i].pt_X = beginQuad[i].pt_X;
		q[i].pt_Y = beginQuad[i].pt_Y;

		pMove->quadIncr[i].xVector
			= DivRemSF16 (&rem, Convert32_F16 (endQuad[i].pt_X - beginQuad[i].pt_X), Convert32_F16 (numberOfFrames) );
		
		pMove->quadIncr[i].yVector
			= DivRemSF16 (&rem, Convert32_F16 (endQuad[i].pt_Y - beginQuad[i].pt_Y), Convert32_F16 (numberOfFrames) );
	}

	/*	Map Cel corners to the starting position.
	 */
	MapCel (ccb , q);

}
