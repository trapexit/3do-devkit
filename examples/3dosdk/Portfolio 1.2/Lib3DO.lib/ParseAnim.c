/*
	File:		ParseAnim.c

	Contains:	Routine to parse an anim file previously loaded into
 				memory, creating an ANIM structure with attached AnimFrames.

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

/*****************************************************************************
 *
 *	Implementation notes:
 *
 *	Iterate through each chunk and fill the AnimFrames with pointers
 *	to CCBs, PLUTs, and Pixel Data for each frame. If an Anim has one
 *	CCB and many PDATs the CCB pointer is used repeatedly for each frame
 *	in the animFrame. We end up with an array of AnimEntries that we can
 *	index into at any point and display a valid frame from each entry.
 *
 *	If we run across an ANIM chunk before we've allocated any AnimFrames,
 *	we gain a bit of efficiency by allocating all the AnimFrames we'll
 *	need at once.  If there is no ANIM chunk in the file, we'll expand
 *	the array of AnimFrames on the fly as needed.
 *
 *	We handle both types of ANIM files which are currently fashionable:
 *	3DO animator format, where PLUTs come before PDATs, and "glom-them-
 *	together-with-MPW-cat-command" where PDATs come before PLUTs. In an
 *	ideal world, there'd be a tool to create Animator-like ANIM files
 *	from a collection of cels, and everyone would promptly convert all
 *	their existing ANIMs.  ::sigh::
 ****************************************************************************/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "BlockFile.h"
#include "UMemory.h"

#include "Debug3DO.h"

/*----------------------------------------------------------------------------
 * ReallocFrames
 *	Expand the array of AnimFrames attached to an ANIM to the given size.
 *--------------------------------------------------------------------------*/

static AnimFrame * ReallocFrames(ANIM *pAnim, int32 newFrameCount, uint32 memTypeBits)
{
	AnimFrame * newFrames;
	int32		oldFrameCount;
	int32		memNeeded;

	oldFrameCount = pAnim->num_Alloced_Frames;
	memNeeded = newFrameCount*sizeof(AnimFrame);

	newFrames = (AnimFrame *)Malloc(memNeeded, (memTypeBits|MEMTYPE_FILL|0x00));

	if (newFrames == NULL) {
		DIAGNOSE(("No memory for AnimFrames\n"));
		return NULL;
	}

	if (pAnim->pentries != NULL) {
		memcpy(newFrames, pAnim->pentries, oldFrameCount*sizeof(AnimFrame));
		Free(pAnim->pentries);
	}

	pAnim->pentries 		  = newFrames;
	pAnim->num_Alloced_Frames = newFrameCount;

	return &pAnim->pentries[oldFrameCount];
}

/*----------------------------------------------------------------------------
 * ParseAnim
 *	Turn a buffer full of anim-ish data into an ANIM and attached AnimFrames.
 *	Use UnloadAnim() to release the resources acquired in this process.
 *	(UnloadAnim works both after LoadAnim and ParseAnim, with the only
 *	difference being that with ParseAnim you allocated the input buffer,
 *	so you have to free it yourself.)
 *--------------------------------------------------------------------------*/

ANIM * ParseAnim(void *inBuf, long inBufSize, uint32 memTypeBits)
{
	ANIM *		pAnim;
	AnimFrame * aFrame;
	int32		allocedFrames;
	int32		curFrame;
	long		tempSize;
	char *		tempBuf;
	char *		pChunk;
	char *		lastPLUT;
	CCB *		lastCCB;
	ulong		chunk_ID;
	Boolean 	PLUTsFirst;

	/*------------------------------------------------------------------------
	 * sanity-check the parameters
	 * allocate the ANIM
	 *----------------------------------------------------------------------*/

#if DEBUG
	if (inBuf == NULL) {
		DIAGNOSE(("NULL buffer pointer\n"));
		return NULL;
	}

	if (inBufSize < sizeof(CCB)) {
		DIAGNOSE(("Buffer size is less than sizeof(CCB); can't be an Anim file\n"));
		return NULL;
	}
#endif

	if (NULL == (pAnim = (ANIM *)Malloc(sizeof(*pAnim), (memTypeBits|MEMTYPE_FILL|0x00)))) {
		DIAGNOSE(("No memory for ANIM\n"));
		goto ERROR_EXIT;
	}

	/*------------------------------------------------------------------------
	 * Loop through the chunks in the buffer, building the AnimFrames.
	 *----------------------------------------------------------------------*/

	curFrame		= 0;
	allocedFrames	= 0;
	PLUTsFirst		= FALSE;
	lastCCB 		= NULL;
	lastPLUT		= NULL;
	aFrame			= NULL;
	tempSize		= inBufSize;
	tempBuf 		= (char *)inBuf;

	while ( (pChunk = GetChunk( &chunk_ID, &tempBuf, &tempSize )) != NULL) {
	  switch (chunk_ID) {

		case CHUNK_ANIM:
			if (allocedFrames == 0) {
				allocedFrames = ((AnimChunk *)pChunk)->numFrames;
				aFrame = ReallocFrames(pAnim, allocedFrames, memTypeBits);
				if (aFrame == NULL) {
					/* error reported by ReallocFrames() */
					goto ERROR_EXIT;
				}
			}
			break;

		case CHUNK_CCB:
			if (allocedFrames == 0) {
				allocedFrames = N_FRAMES_PER_CHUNK;
				aFrame = ReallocFrames(pAnim, allocedFrames, memTypeBits);
				if (aFrame == NULL) {
					/* error reported by ReallocFrames() */
					goto ERROR_EXIT;
				}
			}
			lastCCB = (CCB *) &(((CCC *)pChunk)->ccb_Flags);

			lastCCB->ccb_Flags	|= CCB_SPABS
								|  CCB_PPABS
								|  CCB_NPABS
								|  CCB_YOXY
								|  CCB_LAST;	/* V32 anims might not have these set */
			break;

		case CHUNK_PLUT:
			if (lastCCB == NULL) {
				DIAGNOSE(("Found PLUT before first CCB\n"));
				goto ERROR_EXIT;
			}
			lastPLUT = (char *)&(((PLUTChunk *)pChunk)->PLUT[0]);
			if (aFrame->af_pix == NULL) {
				PLUTsFirst = TRUE;
			}
			break;

		case CHUNK_PDAT:
			if (lastCCB == NULL) {
				DIAGNOSE(("Found PDAT before first CCB\n"));
				goto ERROR_EXIT;
			}

			if (!PLUTsFirst) {
				aFrame->af_PLUT = lastPLUT;
			}

			if (aFrame->af_pix != NULL) {
				++curFrame;
				if (curFrame < allocedFrames) {
					++aFrame;
				} else {
					allocedFrames += N_FRAMES_PER_CHUNK;
					aFrame = ReallocFrames(pAnim, allocedFrames, memTypeBits);
					if (aFrame == NULL) {
						/* error reported by ReallocFrames() */
						goto ERROR_EXIT;
					}
				}
			}

			aFrame->af_CCB	= lastCCB;
			aFrame->af_pix	= &(((PixelChunk *)pChunk)->pixels[0]);
			if (PLUTsFirst) {
				aFrame->af_PLUT = lastPLUT;
			}
			break;

		case CHUNK_CPYR:
		case CHUNK_DESC:
		case CHUNK_KWRD:
		case CHUNK_CRDT:
		case CHUNK_XTRA:
			break;

		default:
		  DIAGNOSE(("Chunk %.4s was unexpected.\n", &chunk_ID));
		  break;
		}
	}

	/*------------------------------------------------------------------------
	 * if we didn't get even one frame, whine about it.
	 * if PLUTs were coming after PDATs, attach the last PLUT to the last
	 * frame we built.
	 * convert the frame count to 1-based.
	 *----------------------------------------------------------------------*/

	if (aFrame == NULL || aFrame->af_pix == NULL) {
		DIAGNOSE(("No cels found in buffer\n"));
		goto ERROR_EXIT;
	}

	if (!PLUTsFirst) {
		aFrame->af_PLUT = lastPLUT;
	}

	pAnim->num_Frames	= curFrame + 1;
	pAnim->cur_Frame	= 0;

	return pAnim;

ERROR_EXIT:

	UnloadAnim(pAnim); // cleanly undoes anything we've (even partially) done.

	return NULL;
}

