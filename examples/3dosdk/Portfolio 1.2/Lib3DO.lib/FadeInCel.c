#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

bool
FadeInCel (CCB *ccb, CCB *maskccb, Int32 *stepValue)
{

	/* Fade the cel in one step at a time */
	if (*stepValue < MAX_SCALE) {
		SetCelScale (ccb, maskccb, (*stepValue)++);
		if (*stepValue == MAX_SCALE) {
			SET_TO_NORMAL (ccb);
			return false;
		}
		else
			return true;
	}
	
	return false;
}
