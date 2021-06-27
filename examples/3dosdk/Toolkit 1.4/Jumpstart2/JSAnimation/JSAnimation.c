/*
	File:		JSAnimation.c

	Contains:	Play an animation with minimal interaction

	Files used:
		
		$boot/JSData/Art/sky.img	-	background image
		$boot/JSData/Art/JSUFO.cel	-	cel to project against background image
		$boot/JSData/Art/boom.anim	-	animation to play when A button is pressed
	
	Written by:	Steve Beeman
				( Freely adapted for JumpStart by Charlie Eckhaus )

	Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	  1/4/94	CDE		Derived from old Jumpstart's BasicUFO.c;
		 							Added event broker interface for joypad triggering of explosion
*/

#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"

#define DISPLAY_WIDTH	320
#define DISPLAY_HEIGHT	240

ScreenContext gScreenContext;

ubyte* gBackgroundImage = NULL;
CCB* gUFO_CCB = NULL;
ANIM* gExplosionAnim;
CCB* gExplosionCCB = NULL;
int32 nBoomCount = 0;
int32 nPostBoomDelay = 0;
	
void UnifyAnimation(ANIM* pAnim)
{
	int i;
	CCB* theCCB;
	
	theCCB = pAnim->pentries[0].af_CCB;
	for (i = 0; i < pAnim->num_Frames; i++)
	{
		pAnim->pentries[i].af_CCB = theCCB;
	}
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

Boolean InitGame(void)
{
	gScreenContext.sc_nScreens = 2;
	
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
		
	if ( InitEventUtility(1, 0, LC_Observer) < 0)
	{
		DIAGNOSTIC("Can't initialize the event broker");
		return FALSE;
	}

	if ((gBackgroundImage = (ubyte *) LoadImage("$boot/JSData/Art/Sky.img", NULL, NULL, &gScreenContext)) == NULL)
	{
		DIAGNOSTIC("Can't load the background image");
		return FALSE;
	}

	if ((gUFO_CCB = LoadCel("$boot/JSData/Art/JSUFO.cel", MEMTYPE_CEL)) == NULL)
	{
		DIAGNOSTIC("Can't load the foreground cel");
		return FALSE;
	}
	MakeLastCCB(gUFO_CCB);

	if ( ( gExplosionAnim = LoadAnim("$boot/JSData/Art/Boom.anim", MEMTYPE_CEL )) == 0)
	{
		DIAGNOSTIC("Can't load the explosion animation");
		return FALSE;
	}
	UnifyAnimation(gExplosionAnim);
	gExplosionCCB = GetAnimCel(gExplosionAnim, 0);
	MakeLastCCB(gExplosionCCB);

	return TRUE;
}

ulong Random( ulong n )
/* Return a random number from 0 to n-1
 * The return value has 16 bits of significance, and ought to be unsigned.  
 * Is the above true?
 */
{
	ulong i, j, k;

	i = (ulong)rand() << 1;
	j = i & 0x0000FFFF;
	k = i >> 16;
	return ( ( ((j*n) >> 16) + k*n ) >> 16 );
}

Coord CalcCenterCoord(int32 aCoord, int32 aDimension)
{
	return aCoord + aDimension / 2;
}

Point CalcCCBCenterPoint(CCB *aCCB)
{
	Point aPoint;
	
	aPoint.pt_X = CalcCenterCoord(ConvertF16_32(aCCB->ccb_XPos), aCCB->ccb_Width);
	aPoint.pt_Y = CalcCenterCoord(ConvertF16_32(aCCB->ccb_YPos), aCCB->ccb_Height);
	
	return aPoint;
}

void MoveCCB( CCB *aCCB, int32 xPos, int32 yPos )
/*
	Convenience routine to move a cel to the specified int32 coordinates
*/
{
	aCCB->ccb_XPos = Convert32_F16(xPos);
	aCCB->ccb_YPos = Convert32_F16(yPos);
}

void CenterCCB(CCB *aCCB, Coord aXCenter, Coord aYCenter)
{
	int32 aWidth;
	int32 aHeight;
	int32 aXPos;
	int32 aYPos;

	aWidth = aCCB->ccb_Width;
	aHeight = aCCB->ccb_Height;
	aXPos = aXCenter - aWidth / 2;
	aYPos = aYCenter - aHeight / 2;

	MoveCCB(aCCB, aXPos, aYPos);
}

#define		kContinuousMask		( ControlA )

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


Boolean DoControlPad( void )
/* Returns FALSE if user pressed START to quit, else returns TRUE, indicating game still in progress */
{
	uint32 	joyBits;
	Point	aUFOCenter;

	joyBits = JSGetControlPad ( kContinuousMask );

	/* If the user has pressed the START button on the joystick 
	 * game is over
	 */	
	if ( joyBits & ControlStart )
	{
		return FALSE;
	}

	/* Use ControlA to trigger the explosion */
	if ( joyBits & ControlA )
	{
		if (!nBoomCount)	// wait until the explosion is finished
		{
			nBoomCount = gExplosionAnim->num_Frames;

			/* Make the centers of the UFO and explosion coincident */
			aUFOCenter = CalcCCBCenterPoint(gUFO_CCB);
			CenterCCB(gExplosionCCB, aUFOCenter.pt_X, aUFOCenter.pt_Y);
						
			/* Get a random center for the new UFO */
			CenterCCB(gUFO_CCB, (Coord) Random(DISPLAY_WIDTH), (Coord) Random(DISPLAY_HEIGHT));
			
		}
	}
	
	return TRUE;

}


void TargetAction(CCB* pcTarget)
{
	if (nBoomCount)
	{
		nBoomCount--;
		if (nBoomCount)
		{
			GetAnimCel(gExplosionAnim, 1<<16);
			DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gExplosionCCB);
		}
		else
		{
			gExplosionAnim->cur_Frame = 0; 						// reset explosion animation to its first frame
			nPostBoomDelay = gExplosionAnim->num_Frames * 5;	// wait awhile before showing the target
		}
	}
	else
	{
		if (nPostBoomDelay)
			nPostBoomDelay--;
		if (!nPostBoomDelay)
			DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], pcTarget);
	}
}

void Cleanup( void )
/*
	Free global data and system resources.
	
	All of the Unload… calls check for a NULL parameter.
*/
{
	UnloadImage(gBackgroundImage);
	UnloadCel(gUFO_CCB);
	UnloadAnim(gExplosionAnim);

	KillEventUtility();
	ShutDown();
}

int main(int argc, char** argv)
{
	if (InitGame())
	{
		gScreenContext.sc_curScreen = 0;
			
		while (DoControlPad())
		{
			/* Draw the background image */
			DrawImage(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], gBackgroundImage, &gScreenContext);
			
			/* Handle the drawing of the UFO cel and the explosion */
			TargetAction(gUFO_CCB);
	
			/*
				Display the current screen.
				
				DisplayScreen automatically waits for the next vertical blank,
				then sets up the hardware to use the specified screen as the
				display buffer. Once set up properly, the hardware will show 
				this buffer on the TV automatically every frame. You only
				have to call DisplayScreen when you want to change which block
				of memory the hardware is displaying on the TV.
			*/
			DisplayScreen(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 0);
	
			/* Toggle the current screen */
			gScreenContext.sc_curScreen = 1 - gScreenContext.sc_curScreen;
		}
		
	}
	
	FadeToBlack(&gScreenContext, 60);	
	printf("Thank you for playing JSAnimation!\n");

	Cleanup();
	
}

