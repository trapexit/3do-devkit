/*****************************************************************************
 *	File:			ParseCel.c
 *
 *	Contains:		Routine to parse a cel previously loaded into a buffer,
 *					creating a valid CCB with PDAT and PLUT attached to it.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/10/93  Ian	When multiple CCBs/PDATs/PLUTs appear in the input 
 *					buffer, we now link the CCBs together (in the order
 *					they were found in the buffer) and return a pointer
 *					to the first CCB found.  (IE, we now return a list
 *					of one or more cels.)  This is useful for handling
 *					anti-aliased cels, among other things, as if they 
 *					were a single cel.
 *	08/05/93  Ian 	Rewrite.
 *
 *	Implementation notes:
 ****************************************************************************/

#include "CelUtils.h"
#include "Parse3DO.h"
#include "BlockFile.h"
#include "UMemory.h"
#include "Debug3DO.h"

/*****************************************************************************
 * CCB * ParseCel(void *inBuf, long inBufSize)
 *
 *	Parses a cel file in a buffer, returns a pointer to the CCB for the cel;
 *	the CCB will contain pointers to the pixels and (optional) PLUT.  If 
 *	multiple CCBs appear in the file, they are strung together through the
 *	ccb_NextPtr fields.  For anti-aliased cels (which we have to detect via
 *	a characteristic signature, since they are just a cel file with two CCBs
 *	in them really) we reverse the order of the CCB links so that the alpha
 *	channel comes first, then the data (since that's the way to draw them).
 *
 ****************************************************************************/

CCB * ParseCel(void *inBuf, long inBufSize)
{
	ulong		chunk_ID;
	long		tempSize;
	int			numCCBs;
	int			numPLUTs;
	char *		tempBuf;
	char *		pChunk;
	CCB *		pCCB;
	CCB *		firstCCB;
	CCB *		lastCCB;
	long *		pPLUT;
	CelData *	pPDAT;

	/*------------------------------------------------------------------------
	 * sanity-check parms.
	 *----------------------------------------------------------------------*/

#if DEBUG
	if (inBuf == NULL) {
		DIAGNOSE(("NULL buffer pointer\n"));
		goto ERROR_EXIT;
	}

	if (inBufSize < sizeof(CCB)) {
		DIAGNOSE(("Buffer size is less than sizeof(CCB); can't be a Cel file\n"));
		goto ERROR_EXIT;
	}
#endif

	/*------------------------------------------------------------------------
	 * loop thru chunks, remembering pointers to the important bits.
	 *----------------------------------------------------------------------*/

	pCCB	 = NULL;
	pPDAT	 = NULL;
	pPLUT	 = NULL;
	firstCCB = NULL;
	lastCCB  = NULL;
	numCCBs	 = 0;
	numPLUTs = 0;
	
	tempBuf  = (char *)inBuf;
	tempSize = inBufSize;

	while ( (pChunk = GetChunk( &chunk_ID, &tempBuf, &tempSize )) != NULL)	{
		switch (chunk_ID) {
			case CHUNK_CCB:
				++numCCBs;
				pCCB = (CCB *)&((CCC *)pChunk)->ccb_Flags;
				pCCB->ccb_NextPtr 	= NULL;
				pCCB->ccb_SourcePtr	= NULL;
				pCCB->ccb_PLUTPtr 	= NULL;
				pCCB->ccb_Flags |= CCB_SPABS
								|  CCB_PPABS
								|  CCB_NPABS
								|  CCB_YOXY
								|  CCB_LAST;  /* V32 anims might not have these set */
				
				if (firstCCB == NULL) {
					firstCCB = pCCB;
				}
				
				if (lastCCB != NULL) {
					if (lastCCB->ccb_SourcePtr == NULL) {
						DIAGNOSE(("Found a new CCB without finding a PDAT for the prior CCB\n"));
						goto ERROR_EXIT;
					}
					lastCCB->ccb_Flags  &= ~CCB_LAST;
					lastCCB->ccb_NextPtr = pCCB;
				}
				lastCCB = pCCB;
				break;

			case CHUNK_PDAT:
				pPDAT = (CelData *)((PixelChunk *)pChunk)->pixels;
				if (lastCCB == NULL) {
					DIAGNOSE(("Found a PDAT before finding any CCB\n"));
					goto ERROR_EXIT;
				} else if (lastCCB->ccb_SourcePtr != NULL) {
					DIAGNOSE(("Found two PDATs without an intervening CCB\n"));
					goto ERROR_EXIT;
				}
				lastCCB->ccb_SourcePtr = pPDAT;
				break;

			case CHUNK_PLUT:
				++numPLUTs;
				pPLUT = (long *)((PLUTChunk *)pChunk)->PLUT;
				if (lastCCB == NULL) {
					DIAGNOSE(("Found a PLUT before finding any CCB\n"));
					goto ERROR_EXIT;
				} else if (lastCCB->ccb_PLUTPtr != NULL) {
					DIAGNOSE(("Found two PLUTs without an intervening CCB\n"));
					goto ERROR_EXIT;
				}
				lastCCB->ccb_PLUTPtr = pPLUT;
				break;

			case CHUNK_CPYR:
			case CHUNK_DESC:
			case CHUNK_KWRD:
			case CHUNK_CRDT:
			case CHUNK_XTRA:
				break;

			default:
				DIAGNOSE(("Unexpected chunk ID %.4s, ignored\n", pChunk));
				break;
		}
	}

	/*------------------------------------------------------------------------
	 * do a couple basic sanity checks
	 *----------------------------------------------------------------------*/

	if (lastCCB == NULL) {
		DIAGNOSE(("No CCB found in buffer\n"));
		goto ERROR_EXIT;
	}

	if (lastCCB->ccb_SourcePtr == NULL) {
		DIAGNOSE(("Found CCB without associated PDAT in buffer\n"));
		goto ERROR_EXIT;
	}
	
	/*------------------------------------------------------------------------
	 * handle anti-aliased cels
	 *	we have to look for a characteristic signature of an anti-aliased
	 *	file since it isn't really any different from two cels in a file:
	 *	  -	there are exactly 2 CCBs
	 *	  -	there is exactly 1 PLUT
	 *	  -	the first PDAT is 16-bit data
	 *	  - the second PDAT is 4-bit data
	 *	  - the CCB fields Width and Height are identical between the two
	 *	  -	the CCB fields XPos and YPos are identical between the two 
	 *	if all this is true, we assume we have an anti-aliased cel file,
	 *	which the brain-dead AACelWriter creates with the CCBs in the 
	 *	opposite order from how they have to be drawn, so we reverse them.
	 *	(note that 99% of the time, the if() is gonna shortcut out on the 
	 *	first or second test.)
	 *----------------------------------------------------------------------*/

	if (numCCBs == 2 
	 && numPLUTs == 1 
	 && firstCCB->ccb_Width  == lastCCB->ccb_Width 
	 && firstCCB->ccb_Height == lastCCB->ccb_Height 
	 && firstCCB->ccb_XPos   == lastCCB->ccb_XPos 
	 && firstCCB->ccb_YPos   == lastCCB->ccb_YPos
	 && (CEL_PRE0WORD(firstCCB) & 0x07) == 6		// check for 16-bit pixels
	 && (CEL_PRE0WORD(lastCCB) & 0x07)  == 3) {		// check for  4-bit pixels
		lastCCB->ccb_NextPtr   = firstCCB;
		lastCCB->ccb_Flags    &= ~CCB_LAST;
		firstCCB->ccb_NextPtr  = NULL;
		firstCCB->ccb_Flags   |= CCB_LAST;
		firstCCB = lastCCB;
	}
	
	return firstCCB;

ERROR_EXIT:

	return NULL;
}

