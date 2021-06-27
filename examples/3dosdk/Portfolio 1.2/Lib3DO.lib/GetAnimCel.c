#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*	GetAnimCel	- returns a CCB ptr that is ready to draw with DrawCels.
 *	
 *
 *	Parameters
 *				pAnim			ptr to an ANIM record that was filled by LoadAnim().
 *
 *				bitmapItem		an Item indicating the current bitMap. (UNUSED)
 *
 *				frameIncrement	a signed 16.16 fixed point number that is added to
 *								the current frame offset to determine the next Cel
 *								to draw. If frameIncrement increments the current
 *								frame past the end of the animation, cur_Frame will
 *								be reset to the beginning. (works backwards too!)
 *
 */
CCB* GetAnimCel(ANIM *pAnim, frac16 frameIncrement)
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
