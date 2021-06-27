/*
	File:		OldLoadAnim.c

	Contains:	old version of LoadAnim

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

/****************************************************************************************
 * OldLoadAnim.c 
 *
 *	This exists only to help folks who've become wedded to the old LoadAnim() function.
 *	You should convert to the new LoadAnim()/UnloadAnim() as soon as you can.
 ***************************************************************************************/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

#define NOTFOUND		MAKEKERR(ER_SEVERE,ER_C_STND,ER_NotFound)

/* ReAllocateAnim      */
/* static routine used to dynamically allocate AnimFrames */
/* as an ANIM file is Parsed     */
static AnimFrame * ReAllocateAnim( ANIM *pAnim)
{
AnimFrame	*oldFrame, *newFrame,*xFrame;
long oldn, newn;
long i;
	oldn = pAnim->num_Alloced_Frames;
	oldFrame = pAnim->pentries;
	newn = oldn + N_FRAMES_PER_CHUNK;
	if ((newFrame = (AnimFrame *)ALLOCMEM(newn*sizeof(AnimFrame),MEMTYPE_CEL)) == NULL)
			return NULL;
    for(i=0;i<oldn;i++) newFrame[i] = oldFrame[i];
	/* memcpy( newFrame, oldFrame, oldn*sizeof(AnimFrame)); */
	FREEMEM(oldFrame,oldn*sizeof(AnimFrame));
	pAnim->pentries = newFrame;
	pAnim->num_Alloced_Frames = newn;
    xFrame = newFrame+oldn;
	for (i=0; i<N_FRAMES_PER_CHUNK; ++i) {
		xFrame->af_CCB		= NULL;
		xFrame->af_PLUT		= NULL;
		(xFrame++)->af_pix	= NULL;
	}
	return newFrame+oldn;
}

/*	LoadAnim - for Single CCB and Multi CCB Anim forms of 3DO file format
 *	
 *	Loads the entire file identified by name and points pCel
 *	to each Cel Control Block in the file. If a Cel does
 *	not point to any Cel data or PLUT, then search for the first
 *	occurance of Cel data (and PLUT if needed) following the Cel
 *	header chunk and the CCB to the data.
 *	
 *	Errors:	returns NOTFOUND, NOMEM, or number of Cels in file
 *
 */
int OldLoadAnim( char *name, long **buffer, ANIM *pAnim )
{
int			result, buffSize, i;
long		tempSize, *theBuffer;
ulong		chunk_ID;
char		*tempBuf, *pChunk, *lastPix, *lastPLUT;
CCB			*lastCCB;
Boolean		foundPDAT;
Int32		curFrame,nEmptyFrames;
AnimFrame	*aFrame;

	
	/* determine file size, maybe alloc memory, and read the file into memory
	*/
	if ((buffSize = GetFileSize(name)) <= 0)
			return NOTFOUND;
			
	if(buffer == NULL || (buffer != NULL && *buffer == NULL)) {
		if ((theBuffer = (long *)ALLOCMEM(buffSize,MEMTYPE_CEL)) == NULL)
			return NOMEM;
	}
	else {
#ifdef VERBOSE
		kprintf("Warrning: using preexisting buffer for LoadAnim of %s, hope its big enough\n", name);
#endif /* VERBOSE */
		theBuffer = *buffer;
	}
	if ((result = ReadFile(name, buffSize, theBuffer, 0)) < 0)
		return result;
		
	
	/*	Init the AnimEntry table to NULL
	 */
	 if ((aFrame = (AnimFrame *)ALLOCMEM(N_FRAMES_PER_CHUNK*sizeof(AnimFrame),MEMTYPE_CEL)) == NULL)
			return NOMEM;
	pAnim->pentries = aFrame;
	pAnim->num_Alloced_Frames = N_FRAMES_PER_CHUNK;
	nEmptyFrames = N_FRAMES_PER_CHUNK;
	for (i=0; i<N_FRAMES_PER_CHUNK; ++i) {
		aFrame->af_CCB		= NULL;
		aFrame->af_PLUT		= NULL;
		(aFrame++)->af_pix	= NULL;
	}
	
	/*	Init the vars for our chunk hunting while loop
	 */
	aFrame		= pAnim->pentries;
	curFrame	= 0;
	lastPLUT	= NULL;
	lastPix		= NULL;
	foundPDAT	= FALSE;
	tempBuf		= (char *)theBuffer;
	tempSize	= buffSize;
	

	/*	Iterate through each chunk and fill the AnimEntries with pointers
	 *	to CCBs, PLUTs, and Pixel Data for each frame. If an Anim has one
	 *	CCB and many PDATs the CCB pointer is used repeatedly for each frame
	 *	in the animEntry. We end up with an array of AnimEntries that we can
	 *	index into at any point and display a valid frame from each entry.
	 */
	while ( (pChunk = GetChunk( &chunk_ID, &tempBuf, &tempSize )) != NULL)
	{
	  switch (chunk_ID)
	  {
		case CHUNK_CCB:
			lastCCB	= (CCB *) &(((CCC *)pChunk)->ccb_Flags);
			
			if (foundPDAT) {
				foundPDAT = FALSE;
				if (aFrame->af_PLUT == NULL)	aFrame->af_PLUT	= lastPLUT;
				if (aFrame->af_pix == NULL)		aFrame->af_pix	= lastPix;
				++curFrame;
				++aFrame;
			    if (--nEmptyFrames <= 0) 
					{
					aFrame = ReAllocateAnim(pAnim);
				    if (aFrame == NULL) return NOMEM;
					nEmptyFrames = N_FRAMES_PER_CHUNK;
					}
				}
			
			aFrame->af_CCB = lastCCB;
			
			lastCCB->ccb_HDX	= 0x00100000;	/* this is 1 in 12.20 fixed pt */
			lastCCB->ccb_VDY	= 0x00010000;	/* this is 1 in 16.16 fixed pt */
			lastCCB->ccb_XPos		=
			lastCCB->ccb_YPos		= 0;
			lastCCB->ccb_Flags	|= CCB_SPABS
								|  CCB_PPABS
								|  CCB_NPABS
								|  CCB_YOXY
								|  CCB_LAST;	/* V32 anims might not have these set */
			break;
			
		case CHUNK_PLUT:
		  aFrame->af_PLUT = (char *)&(((PLUTChunk *)pChunk)->PLUT[0]);
		  lastPLUT = aFrame->af_PLUT;
		  break;
	
		case CHUNK_PDAT:
			if (foundPDAT == FALSE) {
				foundPDAT = TRUE;
				aFrame->af_pix = lastPix = &(((PixelChunk *)pChunk)->pixels[0]);
			}
			else {
				if (aFrame->af_PLUT == NULL)	aFrame->af_PLUT	= lastPLUT;
				if (aFrame->af_CCB == NULL)		aFrame->af_CCB	= lastCCB;
				++curFrame;
				++aFrame;
			    if (--nEmptyFrames <= 0) 
					{
					aFrame = ReAllocateAnim(pAnim);
				    if (aFrame == NULL) return NOMEM;
				    nEmptyFrames = N_FRAMES_PER_CHUNK;
					}
				aFrame->af_pix = lastPix = &(((PixelChunk *)pChunk)->pixels[0]);
			}
			break;
			
		case CHUNK_IMAG:
		case CHUNK_ANIM:
		case CHUNK_VDL:
		case CHUNK_CPYR:
		case CHUNK_DESC:
		case CHUNK_KWRD:
		case CHUNK_CRDT:
				break;
	    default:
		  kprintf("Chunk %.4s was unexpected.\n", &chunk_ID);
		  break;
	  }
	}
	
	/*	We must make sure the last frame entry contains ptrs to a CCB.
	 *	Then we increment curFrame to represent 1..N
	 */
	if (foundPDAT) {
		if (aFrame->af_PLUT == NULL)	aFrame->af_PLUT	= lastPLUT;
		if (aFrame->af_CCB == NULL)		aFrame->af_CCB	= lastCCB;
		++curFrame;
	}

	pAnim->num_Frames	= curFrame;
	pAnim->cur_Frame	= 0;

	if (buffer != NULL)
		*buffer = theBuffer;

		
return (curFrame == 0 ? -1:0);
}
