/*
 *	JoyPad.c
 *
 *	Routines to read the joypad settings.
 *
 *	Copyright © 1993 The 3DO Company
 *
 *	All rights reserved. This material constitutes confidential and proprietary
 *	information of the 3DO Company and shall not be used by any Person or for any
 *	purpose except as expressly authorized in writing by the 3DO Company.
 *
 * HISTORY
 *	Date		Author				Description
 *	--------	-------------	-------------------------------------------------
 *	93.06.21		ec			Updated for eventp broker (finally...)  Remove dependency
 *								 on Utils3DO.
 *	93.06.03		ec			Added GetJoyPadContinuous, changed SetJoyPadContinuous
 *								 to return the old setting.
 *	93.02.26		ec			first writing
 *
 */

#include "portfolio.h"

#include "event.h"
#include "Utils3DO.h"
#include "JoyPad.h"

#ifndef noErr
#	define noErr	0
#endif

// private static variables
		Boolean				gPadInitialized = false;
static	ControlPadEventData	gCPadData;
static	Boolean				gLeftHandedPad[kDefaultCtrlPadCount] = {0,0,0,0};
static	int32				gJoyContinuousBtns[kDefaultCtrlPadCount] = {0,0,0,0};
static	uint32				gLastJoySettings[kDefaultCtrlPadCount] = {0,0,0,0};


/*
 * _InitJoypad
 *
 *  Initialize the event manager's simple joypad interface for our use.
 */
int32
_InitJoypad(int32 numControlPads )
{
	int32	errorCode;
	
	if ( gPadInitialized )
		return 0;
		
	errorCode = InitEventUtility(numControlPads, 0, LC_FocusListener);
	if ( errorCode < 0 )
	{
#if	DEBUG
		kprintf(("_InitJoypad: error in InitEventUtility\n"));
		PrintfSysErr(errorCode);
#endif
	}
	
	gPadInitialized = true;
	return errorCode;
}


/*
 * _ReadJoyPad
 *
 *  Read the specified pad, return the current button state.
 */
int32
_ReadJoyPad(uint32 *buttons, int32 padNum)
{
	int32	errorCode;
	errorCode = GetControlPad (padNum, FALSE, &gCPadData);
	if ( errorCode < 0) 
	{
#if	DEBUG
		kprintf(("_ReadJoyPad: error in GetControlPad\n"));
		PrintfSysErr(errorCode);
#endif
	}
	*buttons = gCPadData.cped_ButtonBits;
	return errorCode;
}


/*
 * KillJoypad
 *
 *  Kill the event manager's simple joypad interface.
 */
int32
KillJoypad( void )
{ 
	int32	errorCode;
	
	errorCode = KillEventUtility();
	if ( errorCode < 0 )
	{
#if	DEBUG
		kprintf(("TermJoypad: error in KillEventUtility\n"));
		PrintfSysErr(errorCode);
#endif
	}
	return errorCode;
}


/*
 * SetJoyPadContinuous
 *
 * set which joypad buttons may be held down continuously.  return the old setting
 *  in case the user wants to restore it later.
 */
int32 
SetJoyPadContinuous(int32 continuousBtnFlags, int32 padNum)
{
	int32	oldJoyBits;
	
	oldJoyBits = gJoyContinuousBtns[padNum - 1];
	gJoyContinuousBtns[padNum - 1] = continuousBtnFlags;
	return oldJoyBits;
}


/*
 * GetJoyPadContinuous
 *
 * return which joypad buttons may be held down continuously.  
 */
int32 
GetJoyPadContinuous(int32 padNum)
{
	return gJoyContinuousBtns[padNum - 1];
}


/*
 * SetJoyPadLeftHanded
 *
 * allow the user to play with the standard pad upsidedown, or in a left
 *  handed orientation.  
 */
void 
SetJoyPadLeftHanded(Boolean playLeftHanded, int32 padNum)
{
	gLeftHandedPad[padNum - 1] = playLeftHanded;
}


/*
 * _GetJoyPadState
 *
 * Return the state of the joypad switches that have made a transition
 * from off to on since the last call.
 */
static 
uint32 _GetJoyPadState(int32 padNum)
{
	uint32	currentJoy;				// current joystick settings
	uint32	resultJoy;				// settings returned to caller

	// read the current joypad buttons settings
	if ( _ReadJoyPad(&currentJoy, padNum) < 0 )
		currentJoy = 0;
	resultJoy = currentJoy ^ gLastJoySettings[padNum - 1];
	resultJoy &= currentJoy;

	// mask out any buttons which aren't allowed to be held down continuously,
	//  remember what we're returning
	resultJoy |= (currentJoy & gJoyContinuousBtns[padNum - 1]);
	gLastJoySettings[padNum - 1] = currentJoy;

	return resultJoy;
}


/*
 * GetJoyPad
 *
 * read the joypad and report the state of the buttons.  continuously
 *  pressed buttons are only reported if their bits are set in the 
 *  global ÔgJoyContinuousBtnsÕ.
 * if the user has requested lefthanded play, map the switches.
 *
 * Returns true if the START button is depressed.
 */
Boolean 
GetJoyPad(JoyPadStatePtr joyStatePtr, int32 padNum)
{
	uint32	joyBits;
	
	if ( gPadInitialized == false )
	{
		_InitJoypad(kDefaultCtrlPadCount);
	}
		
#if	DEBUG
	if ( (padNum < 1) || (padNum > kDefaultCtrlPadCount) )
	{
		kprintf("ERROR: %d is an invalid pad number.  Must be between 1 and %d\n", 
					padNum, kDefaultCtrlPadCount);
		return false;
	}
#endif

	joyBits = _GetJoyPadState(padNum);
	
	//!!!!! is this useful???
	// in the 'left handed' position, the pad is upsidedown so we need
	//  to remap the arrows, and the A & C buttons, the start and X buttons
	//  and the left/right shift buttons
	//!!!!! is this useful???
	if ( gLeftHandedPad[padNum - 1] )
	{
		LONGWORD(*joyStatePtr) = 0L;
		joyStatePtr->leftShift	= (joyBits & ControlRightShift)	? 1 : 0;
		joyStatePtr->rightShift	= (joyBits & ControlLeftShift)	? 1 : 0;
	
		joyStatePtr->xBtn		= (joyBits & ControlStart)		? 1 : 0;
		joyStatePtr->startBtn	= (joyBits & ControlX)			? 1 : 0;
		
		joyStatePtr->aBtn		= (joyBits & ControlC)			? 1 : 0;
		joyStatePtr->bBtn		= (joyBits & ControlB)			? 1 : 0;
		joyStatePtr->cBtn		= (joyBits & ControlA)			? 1 : 0;
		joyStatePtr->upArrow	= (joyBits & ControlDown)		? 1 : 0;
		joyStatePtr->downArrow	= (joyBits & ControlUp)			? 1 : 0;
		joyStatePtr->leftArrow	= (joyBits & ControlRight)		? 1 : 0;
		joyStatePtr->rightArrow	= (joyBits & ControlLeft)		? 1 : 0;
	}
	else
	{
		LONGWORD(*joyStatePtr) = joyBits;

#define	LONG_HAND	0
#if	LONG_HAND
		joyStatePtr->leftShift	= (joyBits & ControlLeftShift)	? 1 : 0;
		joyStatePtr->rightShift	= (joyBits & ControlRightShift)	? 1 : 0;
	
		joyStatePtr->xBtn		= (joyBits & ControlX)			? 1 : 0;
		joyStatePtr->startBtn	= (joyBits & ControlStart)		? 1 : 0;
		
		joyStatePtr->aBtn		= (joyBits & ControlA)			? 1 : 0;
		joyStatePtr->bBtn		= (joyBits & ControlB)			? 1 : 0;
		joyStatePtr->cBtn		= (joyBits & ControlC)			? 1 : 0;
		joyStatePtr->upArrow	= (joyBits & ControlUp)			? 1 : 0;
		joyStatePtr->downArrow	= (joyBits & ControlDown)		? 1 : 0;
		joyStatePtr->leftArrow	= (joyBits & ControlLeft)		? 1 : 0;
		joyStatePtr->rightArrow	= (joyBits & ControlRight)		? 1 : 0;
#endif
	}
	
	return ( joyStatePtr->startBtn );
}

