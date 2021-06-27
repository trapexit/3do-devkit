
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bs_cpad.c,v 1.2 1994/11/22 00:56:02 vertex Exp $
**
******************************************************************************/

#include "broker_shell.h"

#ifdef __BS_CPAD_H

int
BS_ProcessCPadData(BS_CPadData *aPad,uint32 deBounceBits,int axisEmulF)
{
  int changeF=0,wrapF;
  register uint32 lastBits,theseBits,outBits=0;
  short tx=0,ty=0;

  	theseBits=BS_GetCPadRawButtons(aPad);

	BS_SetCPadLastButtons(aPad,BS_GetCPadButtons(aPad));

	if(!deBounceBits)
	{
		if(axisEmulF)
		{
			if(theseBits&ControlDown)
				ty=BS_GetCPadDY(aPad);
			else if(theseBits&ControlUp)
				ty=-BS_GetCPadDY(aPad);

			if(theseBits&ControlRight)
				tx=BS_GetCPadDX(aPad);
			else if(theseBits&ControlLeft)
				tx=-BS_GetCPadDX(aPad);

			if(tx||ty)
			{
				BS_SetCPadLastX(aPad,BS_GetCPadX(aPad));
				BS_SetCPadLastY(aPad,BS_GetCPadY(aPad));

			  	wrapF=BS_GetCPadWrapF(aPad);

				tx+=BS_GetCPadLastX(aPad);
				ty+=BS_GetCPadLastY(aPad);

				if(tx>BS_GetCPadMaxX(aPad))
					tx=(wrapF?BS_GetCPadMinX(aPad):BS_GetCPadMaxX(aPad));
				else if(tx<BS_GetCPadMinX(aPad))
					tx=(wrapF?BS_GetCPadMaxX(aPad):BS_GetCPadMinX(aPad));

				if(ty>BS_GetCPadMaxY(aPad))
					ty=(wrapF?BS_GetCPadMinY(aPad):BS_GetCPadMaxY(aPad));
				else if(ty<BS_GetCPadMinY(aPad))
					ty=(wrapF?BS_GetCPadMaxY(aPad):BS_GetCPadMinY(aPad));

				BS_SetCPadX(aPad,tx);
				BS_SetCPadY(aPad,ty);

				changeF=1;
			}
		}

		BS_SetCPadButtons(aPad,theseBits);
		BS_SetCPadLastRawButtons(aPad,theseBits);

		if(theseBits!=BS_GetCPadLastButtons(aPad))
			changeF=1;

		return changeF;
	}

	lastBits=BS_GetCPadLastRawButtons(aPad);

	if(theseBits&ControlRightShift)
	{
		if(deBounceBits&ControlRightShift)
		{
			if(!(lastBits&ControlRightShift))
				outBits|=ControlRightShift;
		}
		else
			outBits|=ControlRightShift;
	}

	if(theseBits&ControlLeftShift)
	{
		if(deBounceBits&ControlLeftShift)
		{
			if(!(lastBits&ControlLeftShift))
				outBits|=ControlLeftShift;
		}
		else
			outBits|=ControlLeftShift;
	}

	if(theseBits&ControlA)
	{
		if(deBounceBits&ControlA)
		{
			if(!(lastBits&ControlA))
				outBits|=ControlA;
		}
		else
			outBits|=ControlA;
	}

	if(theseBits&ControlB)
	{
		if(deBounceBits&ControlB)
		{
			if(!(lastBits&ControlB))
				outBits|=ControlB;
		}
		else
			outBits|=ControlB;
	}

	if(theseBits&ControlC)
	{
		if(deBounceBits&ControlC)
		{
			if(!(lastBits&ControlC))
				outBits|=ControlC;
		}
		else
			outBits|=ControlC;
	}

	if(theseBits&ControlStart)
	{
		if(deBounceBits&ControlStart)
		{
			if(!(lastBits&ControlStart))
				outBits|=ControlStart;
		}
		else
			outBits|=ControlStart;
	}

	if(theseBits&ControlX)
	{
		if(deBounceBits&ControlX)
		{
			if(!(lastBits&ControlX))
				outBits|=ControlX;
		}
		else
			outBits|=ControlX;
	}

	if(theseBits&ControlUp)
	{
		if(deBounceBits&ControlUp)
		{
			if(!(lastBits&ControlDown))
				outBits|=ControlUp;
		}
		else
			outBits|=ControlUp;
	}

	if(theseBits&ControlDown)
	{
		if(deBounceBits&ControlDown)
		{
			if(!(lastBits&ControlDown))
				outBits|=ControlDown;
		}
		else
			outBits|=ControlDown;
	}

	if(theseBits&ControlLeft)
	{
		if(deBounceBits&ControlLeft)
		{
			if(!(lastBits&ControlLeft))
				outBits|=ControlLeft;
		}
		else
			outBits|=ControlLeft;
	}

	if(theseBits&ControlRight)
	{
		if(deBounceBits&ControlRight)
		{
			if(!(lastBits&ControlRight))
				outBits|=ControlRight;
		}
		else
			outBits|=ControlRight;
	}

	if(axisEmulF)
	{
		if(outBits&ControlDown)
			ty=BS_GetCPadDY(aPad);
		else if(outBits&ControlUp)
			ty=-BS_GetCPadDY(aPad);

		if(outBits&ControlRight)
			tx=BS_GetCPadDX(aPad);
		else if(outBits&ControlLeft)
			tx=-BS_GetCPadDX(aPad);

		if(tx||ty)
		{
			wrapF=BS_GetCPadWrapF(aPad);

			BS_SetCPadLastX(aPad,BS_GetCPadX(aPad));
			BS_SetCPadLastY(aPad,BS_GetCPadY(aPad));

			tx+=BS_GetCPadLastX(aPad);
			ty+=BS_GetCPadLastY(aPad);

			if(tx>BS_GetCPadMaxX(aPad))
				tx=(wrapF?BS_GetCPadMinX(aPad):BS_GetCPadMaxX(aPad));
			else if(tx<BS_GetCPadMinX(aPad))
				tx=(wrapF?BS_GetCPadMaxX(aPad):BS_GetCPadMinX(aPad));

			if(ty>BS_GetCPadMaxY(aPad))
				ty=(wrapF?BS_GetCPadMinY(aPad):BS_GetCPadMaxY(aPad));
			else if(ty<BS_GetCPadMinY(aPad))
				ty=(wrapF?BS_GetCPadMaxY(aPad):BS_GetCPadMinY(aPad));

			BS_SetCPadX(aPad,tx);
			BS_SetCPadY(aPad,ty);

			changeF=1;
		}
	}

	if(outBits!=BS_GetCPadLastButtons(aPad))
	{
		changeF=1;
		BS_SetCPadButtons(aPad,outBits);
	}

	BS_SetCPadLastRawButtons(aPad,theseBits);

	return changeF;
}

#endif /* __BS_CPAD_H */
