/*
	File:		JSIntMusicThread.c

	Contains:	Variation of JSInteractiveMusic which uses a thread
				instead of a child task to play the background music

	Files used:
		
		$boot/JSData/Art/sky.img				-	the background image
		$boot/JSData/Art/JSUFO.cel				-	the UFO cel
		$boot/JSData/Art/Cockpit.cel			-	the cockpit cel
		$boot/JSData/Art/Crosshairs.cel			-	the crosshairs "cursor" cel
		$boot/JSData/Art/Laser.cel				-	the laser beam cel
		$boot/JSData/Art/Blip.cel				-	the radar blip cel
		$boot/JSData/Art/Options.cel			-	the options buttons ccb
		$boot/JSData/Art/PleaseWait.cel			-	the "Please Wait" box cel
		$boot/JSData/Sound/whirr.aiff			-	the whirring sound effect
		$boot/JSData/Sound/zap.aiff				-	the zap sound effect
		$boot/JSData/Sound/JSPlayBgndMusic.task	-	the sound handler for music-playing
		
	Written by:	Steve Beeman
				( Freely adapted for JumpStart by Charlie Eckhaus )

	Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 1/4/94		CDE		Derived from JSInteractiveMusic.c and JSPlayBgndMusic.c;
		 							Inserted code for JSPlayBgndMusic and changed
									task management to thread management
*/

#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"

/* Include headers used by the thread */
#include "music.h"

#define		CURRENT_TASK_PRIORITY	KernelBase->kb_CurrentTask->t.n_Priority

#define LASER_COUNT 15

ScreenContext gScreenContext;

int32 nLaserCount = 0;
bool bHoverMode = FALSE;
int32 nBoomCount = 0;
signed long iDistance = 8000;
int32 iVelocity = 50;

int32 iScore = 0;
int32 iMisses = 0;

ubyte* gBackgroundImage = NULL;
CCB* gUFO_CCB = NULL;
CCB* gCockpitCCB = NULL;
CCB* gCrosshairsCCB = NULL;
CCB* gLaserCCB = NULL;
CCB* gBlipCCB = NULL;
CCB* gOptionsCCB = NULL;
CCB* gPleaseWaitCCB = NULL;
ANIM* gExplosionAnim = NULL;
CCB* gExplosionCCB = NULL;

Item iWhirr = -1;
Item iExplosion = -1;
Item iZap = -1;
Item iSoundHandler = -1;

Item gVBLIOReq;				// I/O request for VBL timing calls

/***** This section contains the thread which plays the background music *****/

#define NUMBLOCKS (16)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS*BLOCKSIZE) 

void JSPlayBgndMusic(void)
{
	
	SoundFilePlayer* psfpBackgroundMusic;
	int32				cueIn;
	int32				cueNeeded;

	OpenAudioFolio();	// Threads must open the folios they use
	
	psfpBackgroundMusic = OpenSoundFile("$boot/JSData/Sound/BackgroundMusic.aiff", 2, BUFSIZE);

	while (1)
	{
		StartSoundFile(psfpBackgroundMusic, 0x1000);

		cueIn = 0;
		cueNeeded = 0;

		do {
			if (cueNeeded)
				cueIn = WaitSignal(cueNeeded);

			ServiceSoundFile(psfpBackgroundMusic, cueIn, &cueNeeded);
		} while (cueNeeded);
			
		StopSoundFile(psfpBackgroundMusic);
		RewindSoundFile(psfpBackgroundMusic);
	}
}

/***** ( End of thread section ) *****/


uint32 randbits(uint32 shift)
	{
	shift = 31 - shift;
	return (rand() >> shift);
	}

bool UpdateDashboard()
	{
	static int32 sine[16] =
		{
		0,
		2753,
		5502,
		8241,
		10965,
		13670,
		16351,
		19003,
		21621,
		24201,
		26739,
		29229,
		31668,
		34051,
		36373,
		38632
		};
	static int32 cosine[16] =
		{
		65536,
		65478,
		65304,
		65015,
		64612,
		64094,
		63463,
		62720,
		61866,
		60903,
		59832,
		58656,
		57376,
		55995,
		54515,
		52938
		};

	int32 i, dist;
	signed long x, y;
		
	i = (gUFO_CCB->ccb_Width << 8) / iDistance;
	i = (i >> 1) + (gUFO_CCB->ccb_XPos >> 16);
	i = ((i - 160) << 1) / 21;
	
	dist = iDistance / 307;
	
	if (i < 0)
		{
		x = -1 * dist * sine[i * -1];
		y = dist * cosine[i * -1];
		}
	else
		{
		x = dist * sine[i];
		y = dist * cosine[i];
		}
		
	gBlipCCB->ccb_XPos = (246 << 16) + x + gCockpitCCB->ccb_XPos;
	gBlipCCB->ccb_YPos = (205 << 16) - y + gCockpitCCB->ccb_YPos;
	
	if (nLaserCount == LASER_COUNT)
		DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gLaserCCB);

	DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gCrosshairsCCB);
	DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gCockpitCCB);
	DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gBlipCCB);

	if (nLaserCount)
		{
		--nLaserCount;
		}
		
	return TRUE;
	/* Later, will update score and lives, and will return FALSE when all lives are gone. */
	}

bool Zap(CCB* aTargetCCB, Coord X, Coord Y)
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
	
	(void)StartInstrument(iZap, taStartArgs);

	X -= (aTargetCCB->ccb_XPos >> 16);
	Y -= (aTargetCCB->ccb_YPos >> 16);
	if (	X >= 0
		&&	Y >= 0
		&&	(Y < ((aTargetCCB->ccb_Height<< 8) / iDistance))
		&&	(X < ((aTargetCCB->ccb_Width << 8) / iDistance))	)
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

bool DoOptions(void)
{
	int32 xUFO, yUFO, hdxUFO, vdyUFO;
	TagArg taStartArgs[] =
	{
		{AF_TAG_FREQUENCY, (void*)0x8000},
		{AF_TAG_AMPLITUDE, (void*)0x3000},
		{0, 0}
	};
			
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
	
	StopInstrument(iWhirr, NULL);
		
	while (1)
	{
		joyBits = JSGetControlPad ( ControlA | ControlB | ControlC );
	
		if ( joyBits & ControlStart )
		{
			gUFO_CCB->ccb_XPos = xUFO;
			gUFO_CCB->ccb_YPos = yUFO;
			gUFO_CCB->ccb_HDX = hdxUFO;
			gUFO_CCB->ccb_VDY = vdyUFO;

			StartInstrument(iWhirr, taStartArgs);

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

bool DoControlPad(void)
	{
	static TagArg taStartArgs[] =
		{
		{AF_TAG_FREQUENCY, (void*)0x4000},
		{AF_TAG_AMPLITUDE, (void*)0x7FFF},
		{0, 0}
		}; 
	
	long lJoyMove = 3 << 16;
	uint32 	joyBits;
	
	int32 Balance;
	int32 Volume;
	
	joyBits = JSGetControlPad ( kContinuousMask );
	
	if ( joyBits & ControlStart )
	{
		return DoOptions();
	}
	else
		{
		if ( joyBits & ControlA && !nLaserCount)
			{
			nLaserCount = LASER_COUNT;
			if (Zap(gUFO_CCB, (gCrosshairsCCB->ccb_XPos>>16)+(gCrosshairsCCB->ccb_Width>>1), (gCrosshairsCCB->ccb_YPos>>16)+(gCrosshairsCCB->ccb_Height>>1)))
				{
				Balance = ((gCrosshairsCCB->ccb_XPos + (gCrosshairsCCB->ccb_Width >> 1)) >> 2) / 160;
				Volume = ((8000 - iDistance) << 15) / 3000;
				SetMixer(1, Volume, Balance);
				StartInstrument(iExplosion, taStartArgs);

				nBoomCount = gExplosionAnim->num_Frames;
				gExplosionCCB->ccb_XPos = gCrosshairsCCB->ccb_XPos + (gCrosshairsCCB->ccb_Width<<15) - (gExplosionCCB->ccb_Width * gUFO_CCB->ccb_VDY >> 1);
				gExplosionCCB->ccb_YPos = gCrosshairsCCB->ccb_YPos + (gCrosshairsCCB->ccb_Height<<15) - (gExplosionCCB->ccb_Height * gUFO_CCB->ccb_VDY >> 1);
				gExplosionCCB->ccb_HDX = gUFO_CCB->ccb_HDX;
				gExplosionCCB->ccb_VDY = gUFO_CCB->ccb_VDY;
				
				iScore += iDistance;
				iDistance = 8000;
				if (iVelocity > 0)
					iVelocity *= -1;
							
				gUFO_CCB->ccb_XPos = (20 + randbits(8)) << 16;
				gUFO_CCB->ccb_YPos = (20 + randbits(9)) << 16;
				}
			}
		
		if ( joyBits & ControlB )
			{
			bHoverMode = (bHoverMode == FALSE);
			}
		
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

void TargetAction(CCB* aTargetCCB)
	{
	static signed long iDeltaX = 1 << 16;
	static signed long iDeltaY = 3 << 15;
	
	signed long iTest;
	
	int32 Balance;
	int32 Volume;
	
	if (iDistance > 8000)
		{
		iDistance = 8000;
		iVelocity *= -1;
		iMisses++;
		
		aTargetCCB->ccb_XPos = (20 + randbits(8)) << 16;
		aTargetCCB->ccb_YPos = (20 + randbits(9)) << 16;
		}
	else if (iDistance < 256)
		{
		iDistance = 256;
		iVelocity *= -1;
		}
			
	if (iDistance < 500 && bHoverMode)
		{
		iDistance += (iVelocity >> 5);
		aTargetCCB->ccb_YPos += iDeltaY >> 1;
		aTargetCCB->ccb_XPos += iDeltaX >> 1;
		}
	else
		{
		iDistance += iVelocity;
		iTest = (256 << 8) / iDistance;
		aTargetCCB->ccb_YPos += (iDeltaY >> 7) * iTest;
		aTargetCCB->ccb_XPos += (iDeltaX >> 7) * iTest;
		
		if (!randbits(8))
			iDeltaX *= -1;
			
		if (!randbits(8))
			iDeltaY *= -1;
		}

	aTargetCCB->ccb_HDX = (1<<28) / iDistance;
	aTargetCCB->ccb_VDY = (1<<24) / iDistance;

	iTest = (aTargetCCB->ccb_YPos >> 16) + ((aTargetCCB->ccb_Height << 8)/ iDistance);
	if (aTargetCCB->ccb_YPos <= (20 << 16))
		{
		aTargetCCB->ccb_YPos = 20 << 16;
		iDeltaY *= -1;
		}
	else if (iTest >= 149)
		{
		aTargetCCB->ccb_YPos -= (iTest - 149) << 16;
		iDeltaY *= -1;
		}

	iTest = (aTargetCCB->ccb_XPos >> 16) + ((aTargetCCB->ccb_Width << 8) / iDistance);
	if (aTargetCCB->ccb_XPos <= (20 << 16))
		{
		aTargetCCB->ccb_XPos = 20 << 16;
		iDeltaX *= -1;
		}
	else if (iTest >= 299)
		{
		aTargetCCB->ccb_XPos -= (iTest - 299) << 16;
		iDeltaX *= -1;
		}
	
	iTest -= (aTargetCCB->ccb_XPos >> 16);
	Balance = ((aTargetCCB->ccb_XPos + (iTest << 15)) >> 2) / 160;
	Volume = ((8000 - iDistance) << 15) / 8000;
	
	SetMixer(0, Volume, Balance);

	DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gUFO_CCB);

	if (nBoomCount)
		{
		nBoomCount--;
		if (nBoomCount)
			{
			(void)GetAnimCel(gExplosionAnim, 1<<16);
			DrawCels(gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen], gExplosionCCB);
			}
		else
			{
			gExplosionAnim->cur_Frame = 0;
			}
		}
		
	return;
	}
	
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

bool InitGame(void)
{
	gScreenContext.sc_nScreens = 2;
	
	gVBLIOReq = GetVBLIOReq();				// Create the I/O request we'll use for all of
											// our VBL timing calls
	
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
	
	if ((gOptionsCCB = LoadCel("$boot/JSData/Art/Options.cel", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the options cel");
		return FALSE;
		}
	
	if ((gPleaseWaitCCB = LoadCel("$boot/JSData/Art/PleaseWait.cel", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the PleaseWait cel");
		return FALSE;
		}
	
	if ((iSoundHandler = CreateThread("JSPlayBgndMusic", CURRENT_TASK_PRIORITY + 1, JSPlayBgndMusic, 4000)) < 0)
		{
		DIAGNOSTIC("Can't spawn thread for SoundHandler");
		return FALSE;
		}

	gUFO_CCB->ccb_XPos = (176<<16);
	gUFO_CCB->ccb_YPos = (67<<16);
	gUFO_CCB->ccb_HDX = 1<<19;
	gUFO_CCB->ccb_VDY = 1<<15;
	DrawImage(gScreenContext.sc_Screens[1], gBackgroundImage, &gScreenContext);
	DrawCels(gScreenContext.sc_BitmapItems[1], gCockpitCCB);
	DrawCels(gScreenContext.sc_BitmapItems[1], gOptionsCCB);
	DrawCels(gScreenContext.sc_BitmapItems[1], gUFO_CCB);
	DrawCels(gScreenContext.sc_BitmapItems[1], gPleaseWaitCCB);
	DisplayScreen(gScreenContext.sc_Screens[1], 0);

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
		
	if ((gBlipCCB = LoadCel("$boot/JSData/Art/Blip.CEL", MEMTYPE_CEL)) == NULL)
		{
		DIAGNOSTIC("Can't load the radar blip cel");
		return FALSE;
		}
		
	if ( (gExplosionAnim = LoadAnim("$boot/JSData/Art/Boom.ANIM", MEMTYPE_CEL) ) == 0)
		{
		DIAGNOSTIC("Can't load the animation");
		return FALSE;
		}
	UnifyAnimation(gExplosionAnim);
	gExplosionCCB = GetAnimCel(gExplosionAnim, 0);

	if ((iWhirr = LoadSoundEffect("$boot/JSData/Sound/Whirr.aiff", 1)) < 0)
		{
		DIAGNOSTIC("Can't load the whirr sound effect");
		return FALSE;
		}
	
	if ((iExplosion = LoadSoundEffect("$boot/JSData/Sound/Explosion.aiff", 1)) < 0)
		{
		DIAGNOSTIC("Can't load the explosion sound effect");
		return FALSE;
		}
		
	if ((iZap = LoadSoundEffect("$boot/JSData/Sound/Zap.aiff", 1)) < 0)
		{
		DIAGNOSTIC("Can't load the zap sound effect");
		return FALSE;
		}

	SetChannel(iWhirr, 0);
	SetChannel(iExplosion, 1);
	SetChannel(iZap, 2);
		
	MakeLastCCB( gUFO_CCB );
	MakeLastCCB( gExplosionCCB );
	MakeLastCCB( gLaserCCB );
	MakeLastCCB( gCrosshairsCCB );
	MakeLastCCB( gCockpitCCB );
	MakeLastCCB( gBlipCCB );

	gCrosshairsCCB->ccb_XPos = (long)((160-gCrosshairsCCB->ccb_Width)<<16);
	gCrosshairsCCB->ccb_YPos = (long)((120-gCrosshairsCCB->ccb_Height)<<16);

	gScreenContext.sc_curScreen = 0;
	DrawImage(gScreenContext.sc_Screens[0], gBackgroundImage, &gScreenContext);
	DrawCels(gScreenContext.sc_BitmapItems[0], gCockpitCCB);

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
	UnloadCel(gBlipCCB);
	UnloadCel(gOptionsCCB);
	UnloadCel(gPleaseWaitCCB);

	UnloadAnim(gExplosionAnim);
	
	if (iSoundHandler >= 0)
		DeleteThread(iSoundHandler);

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
			DrawImage(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], gBackgroundImage, &gScreenContext);
			TargetAction(gUFO_CCB);
			if (!UpdateDashboard())
				break;
			DisplayScreen(gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 0);
		}
		
		if ( (iScore + iMisses) > 0 )
			printf("You scored %ld points and let only %d get away.\n", iScore, iMisses);
			
		(void)DeleteItem(iSoundHandler);	
	}

	FadeToBlack(&gScreenContext, 60);	
	printf("Thank you for playing JSIntMusicThread!\n");

	Cleanup();
}

