#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	MakeNewDupCCB	- Allocate a new CCB and duplicate the old ccb's data.
 *				  	
 *	PARAMETERS
 *		cel			- the cel to duplicate.
 *		
 */
CCB *
MakeNewDupCCB( CCB *ccb )
{
CCB	*newCCB;

	if ((newCCB = (CCB *)ALLOCMEM(sizeof(CCB),MEMTYPE_CEL)) == NULL)
		return NULL;
	
	memcpy(newCCB, ccb, sizeof(CCB));
	
	return newCCB;
}
