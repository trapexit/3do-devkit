#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*	setCCBPLUTPtr
 *
 *	Set the most recently loaded Cel to use this PLUT if
 *	1) the Cel expects a PLUT (CCB_LDPIP)
 *	2) there is a PLUT to use (lastPLUT)
 *	3) Cel expects PLUT address to be absolute (CCB_PPABS)
 */
void setCCBPLUTPtr(CCB *lastCCB, char *lastPLUT)
{
	if ((lastCCB->ccb_Flags & CCB_LDPLUT) && (lastPLUT != NULL))
		if (lastCCB->ccb_Flags & CCB_PPABS)
			lastCCB->ccb_PLUTPtr = &(((PLUTChunk *)lastPLUT)->PLUT[0]);
}
