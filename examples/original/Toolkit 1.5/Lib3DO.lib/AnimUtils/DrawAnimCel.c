/*****************************************************************************
 *	File:			DrawAnimCel.c
 *
 *	Contains:		Routine to position and draw the next frame of an
 *ANIM.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/25/94  Ian	Now does all the calcs it used to, but draws only if
 *					the bitmapItem parm is greater than
 *zero. 07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "AnimUtils.h"
#include "Debug3DO.h"

/*----------------------------------------------------------------------------
 *	DrawAnimCel	- determines the next Cel in this animation to draw and
 *draws it.
 *
 *
 *	Parameters
 *				pAnim			ptr to an ANIM record that was
 *filled by LoadAnim().
 *
 *				bitmapItem		the Cel will be drawn into this
 *bitMap.
 *
 *				xPos			the X pos of the upper-left
 *corner of the animation.
 *
 *				yPos			the Y pos of the upper-left
 *corner of the animation.
 *
 *				frameIncrement	a signed 16.16 fixed point number
 *that is added to the current frame offset to determine the next Cel to draw.
 *If frameIncrement increments the current frame past the end of the animation,
 *cur_Frame will be reset to the beginning. (works backwards too!)
 *
 *				hotSpot			Identifies where the x,y
 *coordinates refer to in the cel
 *
 *--------------------------------------------------------------------------*/

void
DrawAnimCel (ANIM *pAnim, Item bitmapItem, int32 xPos, int32 yPos,
             frac16 frameIncrement, int32 hotSpot)
{
  CCB *curCCB;
  int32 retvalue;
  AnimFrame *aFrame;

  /*	Get a ptr to the current frameEntry and the CCB for that entry.
   */

  aFrame = &(pAnim->pentries[ConvertF16_32 (pAnim->cur_Frame)]);
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

  /*	Set the X,Y pos for this frame.
   */

  switch (hotSpot)
    {
    case CenterHotSpot:
      xPos -= curCCB->ccb_Width / 2;
      yPos -= curCCB->ccb_Height / 2;
      break;
    case LowerLeftHotSpot:
      yPos -= curCCB->ccb_Height;
      break;
    case LowerRightHotSpot:
      yPos -= curCCB->ccb_Height;
    case UpperRightHotSpot:
      xPos -= curCCB->ccb_Width;
    case UpperLeftHotSpot:
      break;
    }

  curCCB->ccb_XPos = xPos << 16;
  curCCB->ccb_YPos = yPos << 16;

  /*	Draw the current frame in this animation.
   */

  if (bitmapItem > 0)
    {
      retvalue = DrawCels (bitmapItem, curCCB);
      if (retvalue < 0)
        {
          DIAGNOSE_SYSERR (retvalue, ("DrawCels() failed\n"));
        }
    }
}
