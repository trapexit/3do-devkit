/*
	File:		JoyPad.h

	Contains:	Routines to read the joypad settings.

	Written by:	Eric Carlson

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <1>	 6/28/93	JAY		first checked in

	To Do:
*/

/*
 *
 * HISTORY
 *	Date		Author				Description
 *	--------	--------------	-------------------------------------------------
 *	93.06.21	ec				Updated for eventp broker (finally...)  Add defines for
 *								 shift and X buttons.
 *	93.06.03	ec				Added GetJoyPadContinuous, changed SetJoyPadContinuous
 *								 to return the old setting.
 *	93.2.26		ec				first writing
 *
 */


#ifndef __JPIJOYPAD__
#define __JPIJOYPAD__

#ifndef EVENT_H
#	include "event.h"
#endif

// as written, we allow 4 control pads
#define	kDefaultCtrlPadCount	4

// a couple of handy defines for setting pad continuous flag
#define		PADARROWS	(ControlDown | ControlUp | ControlRight | ControlLeft)
#define		PADBUTTONS	(ControlA | ControlB | ControlC | ControlStart | ControlX)
#define		PADSHIFT	(ControlRightShift | ControlLeftShift)
#define		PADALL		(PADARROWS | PADBUTTONS | PADSHIFT)

// bitfield structure for the control pad buttons
typedef struct 
{
	unsigned	int	junk			: 21;	// [11-31] filler, leave set to 0
	unsigned	int	leftShift		: 1;	// [  10]
	unsigned	int	rightShift		: 1;	// [   9]
	unsigned	int	xBtn			: 1;	// [   8]
	unsigned	int	startBtn		: 1;	// [   7]
	unsigned	int cBtn			: 1;	// [   6]
	unsigned	int aBtn			: 1;	// [   5]
	unsigned	int bBtn			: 1;	// [   4]
	unsigned	int downArrow		: 1;	// [   3]
	unsigned	int upArrow			: 1;	// [   2]
	unsigned	int rightArrow		: 1;	// [   1]
	unsigned	int leftArrow		: 1;	// [   0]
} JoyPadState;
typedef JoyPadState *JoyPadStatePtr;


int32	KillJoypad( void );
int32	GetJoyPadContinuous(int32 padNum);
int32	SetJoyPadContinuous(int32 continuousBtnFlags, int32 padNum);
void	SetJoyPadLeftHanded(Boolean playLeftHanded, int32 padNum);
Boolean	GetJoyPad(JoyPadState* joyState, int32 padNum);


#endif
