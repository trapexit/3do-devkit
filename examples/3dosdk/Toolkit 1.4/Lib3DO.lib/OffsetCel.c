#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	OffsetCel	- offset Cel X and Y position by xOffset and yOffset.
 *				  	
 *	PARAMETERS
 *		cel			- the cel to duplicate.
 *		
 */
void
OffsetCel (CCB *ccb, int32 xOffset, int32 yOffset)
{
	ccb->ccb_XPos += Convert32_F16(xOffset);
	ccb->ccb_YPos += Convert32_F16(yOffset);
}
