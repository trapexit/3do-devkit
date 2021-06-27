#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

void
SetFadeInCel (CCB *ccb, CCB *maskccb, Int32 *stepValue)
{

	/* Copy the CCB data to the maskCCB and set the maskCCB
	 * to use the data to generate a shadow of the asteroid.
	 * Set the asteroid CCB to blend with the shadow and set
	 * the starting scale for both CCB's.
	 */
	memcpy(maskccb, ccb, sizeof(CCB));
	SET_TO_SHADOW (maskccb);
	SET_TO_AVERAGE (ccb);
	*stepValue = 0;

	/* Link the cel in front of it's shadow mask. The intensity
	 * of the shadow will be the inverse of the intensity of the
	 * cel. Since the cel will blend with the shadow, the
	 * effect will be to fade the cel in from the background.
	 */
	LinkCel (maskccb, ccb);
}
