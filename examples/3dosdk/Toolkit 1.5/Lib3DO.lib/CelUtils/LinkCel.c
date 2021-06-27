/*****************************************************************************
 *	File:			LinkCel.c 
 *
 *	Contains:		Routine to link a pair of cels together.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This links nextCCB to ccb.  Works only if 'ccb' is a single cel; if it
 *	is a list of cels, the existing list links are broken.  To preserve
 *	existing links, use the ChainCels() family of functions.
 ****************************************************************************/
 
#include "CelUtils.h"

void LinkCel(CCB *ccb, CCB *nextCCB)
{
	ccb->ccb_NextPtr	 = nextCCB;
	ccb->ccb_Flags		|= CCB_NPABS;
	ccb->ccb_Flags		&= ~CCB_LAST;
}
