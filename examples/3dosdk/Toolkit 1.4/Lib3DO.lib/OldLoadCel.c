/*
	File:		OldLoadCel.c

	Contains:	old version of LoadCel

	Written by:	Ian

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 8/20/93	JAY		first checked in

	To Do:
*/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*	LoadCel - for Single Cel form of 3DO file format
 *	
 *	Loads the entire file identified by name and points pCel
 *	to the first Cel Control Block in the file. If a Cel does
 *	not point to any Cel data or PLUT, then search for the first
 *	occurance of Cel data (and PLUT if needed) following the Cel
 *	header chunk and the CCB to the data.
 *	
 *	Errors:	returns NULL, or number of Cels in file
 *
 *	7/1/93		NJC		Commented out the zeroing of ccb_XPos and ccb_YPos
 *						to accomodate CelWriter 1.5's ability to create cels
 *						with non-zero x and y pos.
 *
 */
CCB* OldLoadCel ( char *name, long **buffer )
{
int			result, buffSize;
long		tempSize, *theBuffer;
ulong		chunk_ID;
char		*tempBuf, *pChunk, *lastPLUT;
CCB			*lastCCB;
	
	/* determine file size, alloc memory, and read the file into memory
	*/
	if ((buffSize = GetFileSize(name)) <= 0)
		return NULL;
		
	if(buffer == NULL || (buffer != NULL && *buffer == NULL)) {			
		if ((theBuffer = (long *)ALLOCMEM(buffSize,MEMTYPE_CEL)) == NULL)
			return NULL;
	}
	else {
#ifdef VERBOSE
#define VERBOSE 1
		kprintf("Warrning: using preexisting buffer for LoadCel of %s, hope its big enough\n", name);
#endif /* VERBOSE */
		theBuffer = *buffer;
	}
	if ((result = ReadFile(name, buffSize, theBuffer, 0)) < 0)
		return NULL;
		
	
	
	/*	Init the vars for our chunk hunting while loop
	 */
	lastPLUT	= NULL;
	lastCCB		= NULL;
	tempBuf		= (char *)theBuffer;
	tempSize	= buffSize;
	

	/*	Iterate through each chunk and set the CCB.
	 */
	while ( (pChunk = GetChunk( &chunk_ID, &tempBuf, &tempSize )) != NULL)
	{
	  switch (chunk_ID)
	  {
		case CHUNK_CCB:
			lastCCB	= (CCB *) &(((CCC *)pChunk)->ccb_Flags);
			
			lastCCB->ccb_HDX	= 0x00100000;	/* this is 1 in 12.20 fixed pt */
			lastCCB->ccb_VDY	= 0x00010000;	/* this is 1 in 16.16 fixed pt */
			// lastCCB->ccb_XPos	=
			// lastCCB->ccb_YPos	= 0;
			lastCCB->ccb_Flags	|= CCB_SPABS
								|  CCB_PPABS
								|  CCB_NPABS
								|  CCB_YOXY
								|  CCB_LAST;	/* V32 anims might not have these set */
		  break;
			
		case CHUNK_PLUT:
			lastPLUT = (char *)&(((PLUTChunk *)pChunk)->PLUT[0]);
			lastCCB->ccb_PLUTPtr = lastPLUT;
		  break;
	
		case CHUNK_PDAT:
			lastCCB->ccb_SourcePtr = (CelData *)&(((PixelChunk *)pChunk)->pixels[0]);
		  break;
			
		default:
			kprintf("Chunk %.4s was parsed but not used.\n", &chunk_ID);
		  break;
	  }
	}
	
	if (buffer != NULL)
		*buffer = theBuffer;

	/*	return the last ccb found (there should only be one for this call)
	 */
	return lastCCB;
}
