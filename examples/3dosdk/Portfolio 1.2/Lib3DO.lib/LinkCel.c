#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	LinkCel	- link ccb in front of nextCCB.
 *				  	
 *	PARAMETERS
 *		cel			- the cel to duplicate.
 *		
 */
void
LinkCel( CCB *ccb, CCB *nextCCB )
{
	ccb->ccb_NextPtr	 = nextCCB;
	ccb->ccb_Flags		|= CCB_NPABS;
	ccb->ccb_Flags		&= ~CCB_LAST;
}
