
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bs_mouse.c,v 1.2 1994/11/22 00:56:02 vertex Exp $
**
******************************************************************************/

#include "broker_shell.h"

#ifdef __BS_MOUSE_H

/* simple absolute value macro */
#define ABS(x) ((x)<0?-(x):(x))

/* debounce buttons and filter */
/* mouse movement (fine control and limitation to screen dimensions) */
/* returns 1 if there has been any change in device state */
/* since the last time and 0 if not */
int
BS_ProcessMouseData(BS_MouseData *aMouse,uint32 deBounceBits)
{
  int changeF=0,wrapF;
  short x,y,dx,dy;
  register uint32 lastB,thisB,outBits=0;

  	/* preserve our history */
	/* before we change the current values */
 	BS_SetMouseThisLast(aMouse);

	/* get the raw cartesian info   */
  	dx=(short)(BS_GetMouseRawX(aMouse)-BS_GetMouseLastRawX(aMouse));
	dy=(short)(BS_GetMouseRawY(aMouse)-BS_GetMouseLastRawY(aMouse));

	/* if they made a small move, fine control filter it */
	if(ABS(dx)<BS_GetMouseXThresh(aMouse))
	 	/* divide by 4 */
		dx>>=2;
	else
		/* divide by 2 */
		dx>>=1;

	if(ABS(dy)<BS_GetMouseYThresh(aMouse))
		dy>>=2;
	else
		dy>>=1;

	if(dx||dy)
	{
		wrapF=BS_GetMouseWrapF(aMouse);

		/* current position = last plus delta */
		x=(short)BS_GetMouseLastX(aMouse)+dx;
		y=(short)BS_GetMouseLastY(aMouse)+dy;

		/* limit to defined boundaries	 */
		if(x>BS_GetMouseMaxX(aMouse))
			x=(wrapF?BS_GetMouseMinX(aMouse):BS_GetMouseMaxX(aMouse));
		else if(x<BS_GetMouseMinX(aMouse))
			x=(wrapF?BS_GetMouseMaxX(aMouse):BS_GetMouseMinX(aMouse));

		if(y>BS_GetMouseMaxY(aMouse))
			y=(wrapF?BS_GetMouseMinY(aMouse):BS_GetMouseMaxY(aMouse));
		else if(y<BS_GetMouseMinY(aMouse))
			y=(wrapF?BS_GetMouseMaxY(aMouse):BS_GetMouseMinY(aMouse));

		/* set the filtered x and y values   */
		BS_SetMouseX(aMouse,x);
		BS_SetMouseY(aMouse,y);

		changeF=1;
	}

	/* no debouncing, so just cough up the button data */
	/* and leave */
	if(!deBounceBits)
	{
		BS_SetMouseButtons(aMouse,BS_GetMouseRawButtons(aMouse));

		if(BS_GetMouseButtons(aMouse)!=BS_GetMouseLastButtons(aMouse)||
		   BS_GetMouseX(aMouse)!=BS_GetMouseLastX(aMouse)||
		   BS_GetMouseY(aMouse)!=BS_GetMouseLastY(aMouse))
			changeF=1;

		BS_SetMouseThisLastRaw(aMouse);

		return changeF;
	}

	/* get the current and last raw button presses */
	/* for debouncing purposes */
	thisB=BS_GetMouseRawButtons(aMouse);
	lastB=BS_GetMouseLastRawButtons(aMouse);

	/* unroll the debouncing checks */
	/* if there is a button that you will never debounce, */
	/* #if 0/#endif the check out to save the time */

	if(thisB&MouseLeft)
	{
		if(deBounceBits&MouseLeft)
		{
			if(!(lastB&MouseLeft))
			{
				outBits|=MouseLeft;
			}
		}
		else
			outBits|=MouseLeft;
	}

	if(thisB&MouseMiddle)
	{
		if(deBounceBits&MouseMiddle)
		{
			if(!(lastB&MouseMiddle))
			{
				outBits|=MouseMiddle;
			}
		}
		else
			outBits|=MouseMiddle;
	}

	if(thisB&MouseRight)
	{
		if(deBounceBits&MouseRight)
		{
			if(!(lastB&MouseRight))
			{
				outBits|=MouseRight;
			}
		}
		else
			outBits|=MouseRight;
	}

	if(thisB&MouseShift)
	{
		if(deBounceBits&MouseShift)
		{
			if(!(lastB&MouseShift))
			{
				outBits|=MouseShift;
			}
		}
		else
			outBits|=MouseShift;
	}

	BS_SetMouseButtons(aMouse,outBits);

	if(BS_GetMouseButtons(aMouse)!=BS_GetMouseLastButtons(aMouse)||
		   BS_GetMouseX(aMouse)!=BS_GetMouseLastX(aMouse)||
		   BS_GetMouseY(aMouse)!=BS_GetMouseLastY(aMouse))
		changeF=1;

	BS_SetMouseThisLastRaw(aMouse);
	return changeF;
}

#endif /* __BS_MOUSE_H */
