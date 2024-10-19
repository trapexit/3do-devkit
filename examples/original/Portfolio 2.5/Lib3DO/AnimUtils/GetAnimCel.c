
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: GetAnimCel.c,v 1.6 1994/10/05 18:21:43 vertex Exp $
**
**  Lib3DO routine to seek to a frame within an ANIM.
**
******************************************************************************/

#include "animutils.h"

CCB *
GetAnimCel (ANIM *pAnim, frac16 frameIncrement)
{
  CCB *curCCB;
  int32 framenum;
  AnimFrame *aFrame;

  /*	Get a ptr to the current frameEntry and the CCB for that entry.
   */

  framenum = ConvertF16_32 (pAnim->cur_Frame);
  if (framenum >= pAnim->num_Frames)
    framenum = 0;
  if (framenum < 0)
    framenum = 0;
  aFrame = &(pAnim->pentries[framenum]);
  curCCB = aFrame->af_CCB;

  /*	Point the CCB to its pixels and PLUT for this frame.
   */

  aFrame->af_CCB->ccb_SourcePtr = (CelData *)aFrame->af_pix;
  if (aFrame->af_PLUT != NULL)

    aFrame->af_CCB->ccb_PLUTPtr = aFrame->af_PLUT;

  /*	Increment the curFrame index and wrap it (pos or neg) if needed.
   *	Note that we preserve the fractional component of the curFrame.
   */

  pAnim->cur_Frame += frameIncrement;
  if (ConvertF16_32 (pAnim->cur_Frame) < 0)
    {
      pAnim->cur_Frame = Convert32_F16 (pAnim->num_Frames) + pAnim->cur_Frame;
    }
  else if (ConvertF16_32 (pAnim->cur_Frame) >= pAnim->num_Frames)
    {
      pAnim->cur_Frame = pAnim->cur_Frame - Convert32_F16 (pAnim->num_Frames);
    }

  return curCCB;
}
