/************************************************************************************
 *	File:		FadeFromBlack.c 
 *
 *	Contains:	Routine to fade the screen up from all-black.  Assumes the CLUTs are
 *				are in an all-black state on entry.  (If they're not, you probably
 *				won't get the visual effect you're after, but it won't crash.)
 *
 *	Written by:	Jay Moreland
 *
 *	Copyright:	© 1993 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any Person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company. 
 *
 *	Change History (most recent first):
 *
 * 	  09/14/93	Ian		Reworked the logic in several ways.  The old logic would
 *	  					give unpredictable results when the frame count was less 
 *						than 2, due to divide-by-zero errors in the calculations.
 *						This prevented using this routine as a set-to-normal operation
 *						by specifying 0 or 1 as the frame count. Now a count of 0 or 
 *						1 (or even a negative number) results in an instant (1-step)
 *						set-to-full-intensity. Also, the old logic would leave the CLUTs in
 *						a somewhat indeterminate state in terms of the low-order 3 bits of
 *						each CLUT color component entry.  The standard fixed CLUTs always 
 *						have zeroes in the low-order 3 bits, and this (according to 
 *						the docs) is desirable if you want predictable math behavior 
 *						out of the cel engine.  It is also desirable in that the 
 *						brightness of any displayed image is not dependant on how 
 *						many frames you specified on the last fade-up call.  To fix
 *						this, the loop logic was changed so that instead of running
 *						from 0 thru frameCount we now run from 1 thru frameCount, and after
 *						the looping is all done, we do the final fade-up step by calling
 *						ResetScreenColors(), which installs the standardized system CLUTs
 *						that have the 3 low-order bits of each color component zeroed.
 *						A serendipitous side effect of the 1 thru frameCount change is that 
 *						the all-black starting state isn't counted as a step in the fading.  
 *						Since all-black is presumably the starting state of the CLUTs 
 *						anyway, it didn't make sense to have the first loop iteration
 *						(which was just setting everything to black) counted as a fade 
 *						step.  So now, a request to 'fade' in two steps really gives you
 *						two steps: the first step is half intensity and the second step
 *						is full intensity.  (Under the old logic, the first step was
 *						all-black and the second step was full intensity.)
 *						And finally, a little performance enhancement: instead of calling
 *						SetScreenColor() 32 times in the inner loop to set all the CLUT
 *						entries for each fade step, we build an array of 32 entries and
 *						use the SetScreenColors() call to load all 32 at once.  This helps
 *						remove the occasional ripple/banding effect you'd sometimes see if
 *						this routine was time-slicing against some higher-priority threads
 *						and the frameCount value was fairly low (under 20).
 *	  07/28/93	JAY		modify WaitVBL for Dragon Tail Release
 *
 *
 ************************************************************************************/

#include "Portfolio.h"
#include "Utils3DO.h"

#define STD_FULL_INTENSITY	0x00F8	// full intensity R,G,B in standard system CLUT entry

void FadeFromBlack(ScreenContext *sc, int32 frameCount)
{
	int32		i;
	int32		j;
	int32		k;
	ubyte		color;
	int32		index;
	uint32		colorEntries[32];
	Item		VBLIOReq;
	
	VBLIOReq = GetVBLIOReq();

	// first blast the special background register to zeroes.  this only
	// needs to be done once (assuming it needs to be done at all, something
	// I'm not really sure about, but the old code did it.)

	for ( j = 0; j <= sc->sc_nScreens; j++) {
		SetScreenColor( sc->sc_Screens[j], MakeCLUTColorEntry(32, 0, 0, 0) );
	}
	
	// now run the fade from the first step thru the last fade step
	// before full color.
	
	for ( i = 1; i < frameCount; i++ ) {
		WaitVBL( VBLIOReq, 1 );
		k = (i * STD_FULL_INTENSITY) / frameCount;
		for ( index = 0; index <= 31; index++ ) {
			color = (ubyte)(k * index / 31);
			colorEntries[index] = MakeCLUTColorEntry(index, color, color, color);
		}			
		for ( j = 0; j <= sc->sc_nScreens; j++) {
			SetScreenColors( sc->sc_Screens[j], colorEntries, 32 );
		}

	}
	
	// now use the ResetScreenColors() call to do the last fade-up step.
	
	for ( j = 0; j <= sc->sc_nScreens; j++) {
		ResetScreenColors( sc->sc_Screens[j] );
	}
	

	
	DeleteItem( VBLIOReq );
}
