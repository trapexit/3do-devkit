#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

bool
FadeOutCel (CCB *ccb, CCB *maskccb, Int32 *stepValue)
{
	if (*stepValue == (MAX_SCALE-1))
		SET_TO_AVERAGE (ccb);

	/* Fade the cel out one step at a time */
	SetCelScale (ccb, maskccb, --(*stepValue));
	
	if ( *stepValue > 0)
		return true;

	return false;
}
