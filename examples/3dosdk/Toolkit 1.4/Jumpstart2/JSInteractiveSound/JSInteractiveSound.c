/*
	File:		JSInteractiveSound.c

	Contains:	Base version of UFO game with zap sound effect

	Files used:
		
		$boot/JSData/Art/sky.img		-	background image
		$boot/JSData/Art/JSUFO.cel		-	the UFO cel
		$boot/JSData/Art/Cockpit.cel	-	the cockpit cel
		$boot/JSData/Art/Crosshairs.cel	-	the crosshairs "cursor" cel
		$boot/JSData/Art/Laser.cel		-	the laser beam cel
		$boot/JSData/Art/Options.cel	-	the options buttons ccb
		$boot/JSData/Sound/zap.aiff		-	the zap sound effect
	
	Written by:	Steve Beeman
				( Freely adapted for JumpStart by Charlie Eckhaus )

	Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 1/4/94		CDE		Derived from old Jumpstart's InteractiveUFO.c
*/

#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"

ScreenContext gScreenContext;

signed long iDistance = 8000;
int32 iScore = 0;
int32 iMisses = 0;
Item iZap = -1;

ubyte* gBackgroundImage = NULL;
CCB* gUFO_CCB = NULL;
CCB* gCockpitCCB = NULL;
CCB* gCrosshairsCCB = NULL;
CCB* gLaserCCB = NULL;
CCB* gOptionsCCB = NULL;

Item gVBLIOReq;		// I/O request for VBL timing calls
	
bool Zap(CCB* pcTarget, Coord X, Coord Y)
	{
	static TagArg taStartArgs[] =
		{
		{AF_TAG_FREQUENCY, (void*)0x8000},
		{AF_TAG_AMPLITUDE, (void*)0x4000},
		{0, 0}
		};
		
	static Point Corner[4] =
		{
		{0, 0},
		{0, 0},
		{319, 210},
		{0, 210}
		};
		
	Corner[0].pt_X = Corner[1].pt_X = X;
	Corner[0].pt_Y = Corner[1].pt_Y = Y;
	
	MapCel(gLaserCCB, Corner);
	
	StartInstrument(iZap, taStartArgs);

	X -= (pcTarget->ccb_XPos >> 16);
	Y -= (pcTarget->ccb_YPos >> 16);
	if (	X >= 0
		&&	Y >= 0
		&&	(Y < (pcTarget->ccb_Height / (iDistance>>8)))
		&&	(X < (pcTarget->ccb_Width / (iDistance>>8)))	)
		return TRUE;
	else
		return FALSE;
	}


#define		kContinuousMask		( ControlA | ControlB | ControlC | ControlRight | ControlLeft | ControlDown | ControlUp )


uint32 JSGetControlPad(uint32 continuousMask)
/*
	Get the current control pad bits, masking out undesirable continuous
	button presses.
*/
{
	static uint32			gOldBits = 0;

	int32					aResult;
	ControlPadEventData		aControlPadEvent;
	uint32 					newBits;
	uint32 					maskedBits;
	
	aResult = GetControlPad (1, FALSE, &aControlPadEvent);
	if (aResult < 0)
	{
		printf( "Error: GetControlPad() failed with error code %d\n", aResult );
		PrintfSysErr(aResult);
		return 0;
	}
	newBits = aControlPadEvent.cped_ButtonBits;
	
	maskedBits = newBits ^ gOldBits;

	maskedBits &= newBits;
	maskedBits |= (newBits & continuousMask);

	gOldBits = newBits;
	return maskedBits;
}


Boolean DoOptions(void)
{
	int32 xUFO, yUFO, hdxUFO, vdyUFO;
	uint32 	joyBits;
	
	xUFO = gUFO_CCB->ccb_XPos;
	yUFO = gUFO_CCB->ccb_YPos;
	hdxUFO = gUFO_CCB->ccb_HDX;
	vdyUFO = gUFO_CCB->ccb_VDY;

	gUFO_CCB->ccb_XPos = (176<<16);
	gUFO_CCB->ccb_YPos = (67<<16);
	gUFO_CCB->ccb_HDX = 1<<19;
	gUFO_CCB->ccb_VDY = 1<<15;
	DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gOptionsCCB);
	DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gUFO_CCB);
	DisplayScreen(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 0);
		
	while (1)
	{
		joyBits = JSGetControlPad ( ControlA | ControlB | ControlC );
	
		if ( joyBits & ControlStart )
		{
			gUFO_CCB->ccb_XPos = xUFO;
			gUFO_CCB->ccb_YPos = yUFO;
			gUFO_CCB->ccb_HDX = hdxUFO;
			gUFO_CCB->ccb_VDY = vdyUFO;

			return TRUE;
		}
		else
		{
			if ( joyBits & ControlA )
			{
			}
			
			if ( joyBits & ControlB )
			{
			}
			
			if ( joyBits & ControlC )
			{
				return FALSE;
			}
		}
		
		WaitVBL( gVBLIOReq, 1 );
			
	}

}

Boolean DoControlPad(void)
{
	long lJoyMove;
	uint32 	joyBits;
	
	joyBits = JSGetControlPad ( kContinuousMask );
	
	if ( joyBits & ControlStart )
	{
		return DoOptions();
	}
	else
	{
		if ( joyBits & ControlA )
			{
			gLaserCCB->ccb_Flags &= ~CCB_SKIP;
			if (Zap(gUFO_CCB, (gCrosshairsCCB->ccb_XPos>>16)+(gCrosshairsCCB->ccb_Width>>1), (gCrosshairsCCB->ccb_YPos>>16)+(gCrosshairsCCB->ccb_Width>>1)))
				{
				iDistance = 8000;
				iScore++;
				}
			}
		
		if ( joyBits & ControlB )
			lJoyMove = 6 << 16;
		else
			lJoyMove = 2 << 16;
		
		if ( joyBits & ControlC )
			{
			}
		
		if (joyBits & ControlRight && gCrosshairsCCB->ccb_XPos < ((320-gCrosshairsCCB->ccb_Width)<<16))
			gCrosshairsCCB->ccb_XPos += lJoyMove;
			
		if (joyBits & ControlLeft && gCrosshairsCCB->ccb_XPos > 0)
			gCrosshairsCCB->ccb_XPos -= lJoyMove;
			
		if (joyBits & ControlDown && gCrosshairsCCB->ccb_YPos < ((240-gCrosshairsCCB->ccb_Height)<<16))
			gCrosshairsCCB->ccb_YPos += lJoyMove;
			
		if (joyBits & ControlUp && gCrosshairsCCB->ccb_YPos > 0)
			gCrosshairsCCB->ccb_YPos -= lJoyMove;
	}
		
	return TRUE;
}

void TargetAction(CCB* pcTarget)
	{
	static signed long iDeltaX = 8 << 24;
	static signed long iDeltaY = 12 << 24;
	
	signed long iTestX;
	signed long iTestY;
	
	signed long iTest;
	int32 Balance;
	int32 Volume;

	if (iDistance < 200)
		{
		iDistance = 8000;
		iMisses++;
		}
	else
		iDistance -= 60;
		
	pcTarget->ccb_HDX = (1<<28) / iDistance;
	pcTarget->ccb_VDY = (1<<24) / iDistance;
	
	pcTarget->ccb_XPos += (iDeltaX / iDistance);
	iTestX = pcTarget->ccb_XPos >> 16;
	iTest = iTestX + ((pcTarget->ccb_Width << 8) / iDistance);
	if (iTestX > (310-pcTarget->ccb_Width))
		{
		pcTarget->ccb_XPos = (310-pcTarget->ccb_Width) << 16;
		iDeltaX *= -1;
		}
	else if (iTestX < 10)
		{
		pcTarget->ccb_XPos = 10 << 16;
		iDeltaX *= -1;
		}
		
	iTest -= (pcTarget->ccb_XPos >> 16);
	Balance = ((pcTarget->ccb_XPos + (iTest << 15)) >> 2) / 160;
	Volume = ((8000 - iDistance) << 15) / 8000;
	SetMixer(0, Volume, Balance);

	pcTarget->ccb_YPos += (iDeltaY / iDistance);
	iTestY = pcTarget->ccb_YPos >> 16;
	if (iTestY > (230-pcTarget->ccb_Height))
		{
		pcTarget->ccb_YPos = (230-pcTarget->ccb_Height) << 16;
		iDeltaY *= -1;
		}
	else if (iTestY < 10)
		{
		pcTarget->ccb_YPos = 10 << 16;
		iDeltaY *= -1;
		}
		
	return;
	}
	
void MakeLastCCB( CCB *aCCB )
/*
	Convenience routine to ensure that the cel engine stops after
	aCCB is processed.
	
	Setting ccb_NextPtr to NULL is insufficient; the cel
	engine won't stop until it hits a cel explicitly marked
	as last.  Set the final ccb_NextPtr to NULL anyway, to
	facilitate traversal of the singly-linked list in software.
*/
{
	aCCB->ccb_NextPtr = NULL;
	SetFlag(aCCB->ccb_Flags, CCB_LAST);
}

bool InitGame(void)
	{
	gScreenContext.sc_nScreens = 2;
	
	gVBLIOReq = GetVBLIOReq();		// Create the I/O request we'll use for
									// all of our VBL timing calls
	
	if (!OpenGraphics(&gScreenContext,2))
	{
		DIAGNOSTIC("Can't open the graphics folio!");
		return FALSE;
	}
	
	if (!OpenSPORT())
	{
		DIAGNOSTIC("Can't open SPORT I/O!");
		return FALSE;
	}
	
	gScreenContext.sc_curScreen = 0;
		
	if (!OpenAudio())
	{
		DIAGNOSTIC("Can't open the audio folio!");
		return FALSE;
	}
		
	if ( InitEventUtility(1, 0, LC_Observer) < 0)
	{
		DIAGNOSTIC("Can't initialize the event broker");
		return FALSE;
	}

	if ((gBackgroundImage = (ubyte *) LoadImage("$boot/JSData/Art/Sky.img", NULL, NULL, &gScreenContext)) == NULL)
		{
		DIAGNOSTIC("Can't load the sky image");
		return FALSE;
		}

	if ((gUFO_CCB = LoadCel("$boot/JSData/Art/JSUFO.cel", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the UFO cel");
		return FALSE;
		}
		
	if ((gCockpitCCB = LoadCel("$boot/JSData/Art/Cockpit.cel", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the cockpit cel");
		return FALSE;
		}
		
	if ((gCrosshairsCCB = LoadCel("$boot/JSData/Art/Crosshairs.CEL", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the crosshairs cel");
		return FALSE;
		}
		
	if ((gLaserCCB = LoadCel("$boot/JSData/Art/Laser.CEL", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the laser cel");
		return FALSE;
		}
		
	if ((gOptionsCCB = LoadCel("$boot/JSData/Art/Options.CEL", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the options cel");
		return FALSE;
		}
		
	/* Link the UFO, laser, crosshairs, and cockpit cels: */
	
	LinkCel(gUFO_CCB, gLaserCCB);
	LinkCel(gLaserCCB, gCrosshairsCCB);
	LinkCel(gCrosshairsCCB, gCockpitCCB);
	
	MakeLastCCB(gCockpitCCB);
	
	gCrosshairsCCB->ccb_XPos = (long)((160-gCrosshairsCCB->ccb_Width)<<16);
	gCrosshairsCCB->ccb_YPos = (long)((120-gCrosshairsCCB->ccb_Height)<<16);

	gLaserCCB->ccb_Flags |= CCB_SKIP;	

	if ((iZap = LoadSoundEffect("$boot/JSData/Sound/Zap.aiff", 1)) < 0)
	{
		DIAGNOSTIC("Can't load the zap sound effect");
		return FALSE;
	}

	SetChannel(iZap, 0);

	return DoOptions();
	}


void Cleanup( void )
/*
	Free global data and system resources.
	
	The UnloadÉ calls check for a NULL parameter.
*/
{
	UnloadImage(gBackgroundImage);

	UnloadCel(gUFO_CCB);
	UnloadCel(gCockpitCCB);
	UnloadCel(gCrosshairsCCB);
	UnloadCel(gLaserCCB);
	UnloadCel(gOptionsCCB);
	
	DeleteIOReq( gVBLIOReq );
	KillEventUtility();
	FreeSoundEffects();
	ShutDown();
}


int main(int argc, char** argv)
{	
	if (InitGame())
	{
		while (DoControlPad())
		{
			gScreenContext.sc_curScreen = 1 - gScreenContext.sc_curScreen;
			
			TargetAction(gUFO_CCB);
			
			DrawImage(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], gBackgroundImage, &gScreenContext);
			DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gUFO_CCB);
			DisplayScreen(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 0);
			
			gLaserCCB->ccb_Flags |= CCB_SKIP;
		}
	}

	FadeToBlack(&gScreenContext, 60);	
	printf("Thank you for playing JSInteractiveSound!\n");
	if ( (iScore + iMisses) > 0 )
		printf("You got %ld and let only %ld get away.\n", iScore, iMisses);

	Cleanup();

}

