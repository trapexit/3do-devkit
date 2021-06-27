/************************************************************************************
 *	File:		FadeToBlack.c 
 *
 *	Contains:	Routine to fade the screen down to all-black.  Assumes the CLUTs are
 *				are in standard system full-intensity state to start with.  (If they're 
 *				not, you probably won't get exactly the visual effect you're after, but 
 *				it won't crash.  If you had the CLUT entries at greater than standard full 
 *				intensity (> 0xF8) everything will be fine.  If the CLUTs were at less
 *				than full intensity, the screen will just get brighter for a vbl tick 
 *				or two at the start of the fadeout.  We could fix this to fade from the
 *				current values to black if only the ReadScreenColor() (aka ReadCLUTColor())
 *				OS routine worked correctly.)
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
 *						This prevented using this routine as a set-to-black operation
 *						by specifying 0 or 1 as the frame count. Now a count of 0 or 
 *						1 (or even a negative number) results in an instant (1-step)
 *						set-to-black.  This is especially useful for slamming the CLUTs on
 *						an empty screen to black in preparation for loading an image and
 *						fading it up.  Also, the loop logic was changed; instead of running
 *						from frameCount down thru 0, we now run from frameCount thru 1, and 
 *						after the looping is all done, we do the final fade-down step that 
 *						sets all the CLUT entries to zero.
 *						A serendipitous side effect of the frameCount thru 1 change is that 
 *						the starting state isn't counted as a step in the fading.  
 *						Since full intensity is presumably the starting state of the CLUTs 
 *						anyway, it didn't make sense to have the first loop iteration (which 
 *						was just setting everything to full intensity) counted as a fade 
 *						step.  So now, a request to 'fade' in two steps really gives you
 *						two steps: the first step is half intensity and the second step
 *						is all-black.  (Under the old logic, the first step was full
 *						intensity and the second step was all-black.)
 *						And finally, a little performance enhancement: instead of calling
 *						SetScreenColor() 32 times in the inner loop to set all the CLUT
 *						entries for each fade step, we build an array of 32 entries and
 *						use the SetScreenColors() call to load all 32 at once.  This helps
 *						remove the occasional ripple/banding effect you'd sometimes see if
 *						this routine was time-slicing against some higher-priority threads
 *						and the frameCount value was fairly low (under 20).
 *
 *	  07/28/93	JAY		modify WaitVBL for Dragon Tail Release
 *
 ************************************************************************************/

#include "Portfolio.h"
#include "Utils3DO.h"

#define STD_FULL_INTENSITY	0x00F8	// full intensity R,G,B in standard system CLUT entry

static void do_fade_step(ScreenContext *sc, int32 k)
{
	int32	j;
	ubyte	color;
	int32	index;
	uint32	colorEntries[32];
	
	for ( index = 0; index <= 31; index++ ) {
		color = (ubyte)((k * index) / 31);
		colorEntries[index] = MakeCLUTColorEntry(index, color, color, color);
	}

	for ( j = 0; j <= sc->sc_nScreens; j++) {
		SetScreenColors( sc->sc_Screens[j], colorEntries, 32 );
	}

}

void FadeToBlack(ScreenContext *sc, int32 frameCount)
{
	int32 	i;
	int32	j;
	int32	k;
	Item	VBLIOReq;
	
	VBLIOReq = GetVBLIOReq();

	// first blast the special background register to zeroes.  this only
	// needs to be done once (assuming it needs to be done at all, something
	// I'm not really sure about, but the old code did it.)

	for ( j = 0; j <= sc->sc_nScreens; j++ ) {
		SetScreenColor( sc->sc_Screens[j], MakeCLUTColorEntry(32, 0, 0, 0) );
	}
	
	// now run the fade from the first step thru the last fade step
	// before full color.

	for ( i = frameCount-1; i > 0; i-- ) {
		WaitVBL( VBLIOReq, 1 );
		k = (i * STD_FULL_INTENSITY) / frameCount;
		do_fade_step(sc, k);
	}

	// now do the last step which zeroes all the CLUTs.  this is done outside
	// the loop in case the frame count is less than 2 (causing us to skip the
	// loop altogether)

	do_fade_step(sc, 0);
	
	DeleteItem( VBLIOReq );
	
	
	SetScreenColor( 0, MakeCLUTColorEntry(32, 0, 0, 0) ); // what the hell is this???
}
