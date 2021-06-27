#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

#define	SCALER_MASK		(PPMPC_MF_MASK + PPMPC_SF_MASK + (PPMPC_MF_MASK<<16) + (PPMPC_SF_MASK<<16))

/* ================== TABLES ================== */

/*	This array contains the Mult and Div values to OR into the PPMP
	control word after masking off the old values. These values are
	organized to provide the smoothest ramping possible
*/
ulong	ScalarValues[NUM_FADE_STEPS] =
			  { (0<<10) + (0<<8),		/* mul=1, div=16, scale=.0625 */
				(0<<10) + (3<<8),		/* mul=1, div=8, scale=.125 */
				(2<<10) + (0<<8),		/* mul=3, div=16, scale=.1875 */
				(0<<10) + (2<<8),		/* mul=1, div=4, scale=.25 */
				(4<<10) + (0<<8),		/* mul=5, div=16, scale=.3125 */
				(2<<10) + (3<<8),		/* mul=3, div=8, scale=.375 */
				(6<<10) + (0<<8),		/* mul=7, div=16, scale=.4375 */
				(0<<10) + (1<<8),		/* mul=1, div=2, scale=.5 */
				(4<<10) + (3<<8),		/* mul=5, div=8, scale=.625 */
				(2<<10) + (2<<8),		/* mul=3, div=4, scale=.75 */
				(6<<10) + (3<<8),		/* mul=7, div=8, scale=.875 */
				(7<<10) + (3<<8),		/* mul=8, div=8, scale=1.0 */
				(4<<10) + (2<<8),		/* mul=5, div=4, scale=1.25 */
				(5<<10) + (2<<8),		/* mul=6, div=4, scale=1.5 */
				(6<<10) + (2<<8),		/* mul=7, div=4, scale=1.75 */
				(3<<10) + (1<<8),		/* mul=4, div=2, scale=2.0 */
				(4<<10) + (1<<8),		/* mul=5, div=2, scale=2.5 */
				(5<<10) + (1<<8),		/* mul=6, div=2, scale=3.0 */
				(6<<10) + (1<<8),		/* mul=7, div=2, scale=3.5 */
				(7<<10) + (1<<8) };		/* mul=8, div=2, scale=4.0 */

ulong	ScalarValues8[] =
			  { (0<<10) + (3<<8),		/* mul=1, div=8, scale=.125 */
				(0<<10) + (2<<8),		/* mul=1, div=4, scale=.25 */
				(2<<10) + (3<<8),		/* mul=3, div=8, scale=.375 */
				(0<<10) + (1<<8),		/* mul=1, div=2, scale=.5 */
				(4<<10) + (3<<8),		/* mul=5, div=8, scale=.625 */
				(2<<10) + (2<<8),		/* mul=3, div=4, scale=.75 */
				(6<<10) + (3<<8),		/* mul=7, div=8, scale=.875 */
				(7<<10) + (3<<8)		/* mul=8, div=8, scale=1.0 */
			   };		/* mul=8, div=2, scale=4.0 */

Int16	ScaleIndex[25] = { -1,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11 };

/*
 *	SetCelScale	- Set the multiplier and divider for the 1st Source
 *				  to the level determined by the step field. The cel
 *				  is assumed to have a checkerboard or random dither
 *				  P-Mode mask. The High and low words of the PPMPC are
 *				  updated alternatly. If the Cel does not have a per
 *				  pixel P-Mode mask it will fade in on even steps only.
 *				  
 *	PARAMETERS
 *	
 *		ccb				- this CCB's 1S mult and 1S div will be updated.
 *		step			- {0..22} determines which ScalarValues entry to apply to the cel
 *	
 */
void
SetCelScale (CCB *ccb, CCB *maskccb, Int32 step)
{
Int16	sIndexLo, sIndexHi;


	sIndexLo = ScaleIndex[step];
	sIndexHi = ScaleIndex[step-1];
	
	/* When the Cel is entirely faded out, the mask must also
	 * be skipped in order to keep animating cels from showing
	 * up against the background. We don't know why this is so...
	 */
	if (sIndexLo == -1) {
		SKIP_CEL(ccb);
		SKIP_CEL(maskccb);
		return;
	}
	else {
		/* Clear the mult and div bits for both halves of PPMPC
		 * and set the new mult and div bits.
		 */
		UNSKIP_CEL(ccb);
		ccb->ccb_PIXC &= ~SCALER_MASK;
		ccb->ccb_PIXC |= ScalarValues[sIndexLo] + (ScalarValues[sIndexHi] << 16);
	}

	sIndexLo = ScaleIndex[(MAX_SCALE-1) - step];
	sIndexHi = ScaleIndex[(MAX_SCALE-1) - step - 1];
	
	if (sIndexLo == -1)
		SKIP_CEL(maskccb);
	else {
		/* Clear the mult and div bits for both halves of PPMPC
		 * and set the new mult and div bits.
		 */
		UNSKIP_CEL(maskccb);
		maskccb->ccb_PIXC &= ~SCALER_MASK;
		maskccb->ccb_PIXC |= ScalarValues[sIndexLo] + (ScalarValues[sIndexHi] << 16);
	}
}
