/*****************************************************************************
 *	File:			GetAnimCel.c 
 *
 *	Contains:		Routine to seek to a frame within an ANIM.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 ****************************************************************************/
 
#include "AnimUtils.h"

CCB * GetAnimCel(ANIM *pAnim, frac16 frameIncrement)
{
	CCB		*curCCB;
	int32    framenum;
	AnimFrame	*aFrame;

	/*	Get a ptr to the current frameEntry and the CCB for that entry.
	 */
	 
	framenum = ConvertF16_32(pAnim->cur_Frame);
	if (framenum >=  pAnim->num_Frames) framenum = 0;
	if (framenum < 0) framenum = 0;
	aFrame	= &(pAnim->pentries[ framenum ]);
	curCCB	= aFrame->af_CCB;
	
	/*	Point the CCB to its pixels and PLUT for this frame.
	 */
	 
	aFrame->af_CCB->ccb_SourcePtr = (CelData *)aFrame->af_pix;
	if (aFrame->af_PLUT != NULL)

		aFrame->af_CCB->ccb_PLUTPtr = aFrame->af_PLUT;

	/*	Increment the curFrame index and wrap it (pos or neg) if needed.
	 *	Note that we preserve the fractional component of the curFrame.
	 */
	 
	pAnim->cur_Frame += frameIncrement;
	if (ConvertF16_32(pAnim->cur_Frame) < 0) {
		pAnim->cur_Frame = Convert32_F16(pAnim->num_Frames) + pAnim->cur_Frame;
	}
	else if (ConvertF16_32(pAnim->cur_Frame) >= pAnim->num_Frames) {
		pAnim->cur_Frame = pAnim->cur_Frame - Convert32_F16(pAnim->num_Frames);
	}

	return curCCB;
}
