/*
 *	JoyPad.c
 *
 *	Routines to read the joypad settings.
 *
 *	Copyright © 1993 The 3DO Company
 *
 *	All rights reserved. This material constitutes confidential and
 *proprietary information of the 3DO Company and shall not be used by any
 *Person or for any purpose except as expressly authorized in writing by the
 *3DO Company.
 *
 * HISTORY
 *	Date		Author				Description
 *	--------	--------------
 *------------------------------------------------- 93.06.21	ec
 *Updated for eventp broker (finally...)  Add defines for shift and X buttons.
 *	93.06.03	ec				Added GetJoyPadContinuous,
 *changed SetJoyPadContinuous to return the old setting. 93.2.26
 *ec				first writing
 *
 */

#ifndef __JPIJOYPAD__
#define __JPIJOYPAD__

#ifndef EVENT_H
#include "event.h"
#endif

// as written, we allow 4 control pads
#define kDefaultCtrlPadCount 4

#define LONGWORD(aStruct) (*((long *)&(aStruct)))

// a couple of handy defines for setting pad continuous flag
#define PADARROWS (ControlDown | ControlUp | ControlRight | ControlLeft)
#define PADBUTTONS (ControlA | ControlB | ControlC | ControlStart | ControlX)
#define PADSHIFT (ControlRightShift | ControlLeftShift)
#define PADALL (PADARROWS | PADBUTTONS | PADSHIFT)

// bitfield structure for the control pad buttons
typedef struct
{
  unsigned int downArrow : 1;  // [  31]
  unsigned int upArrow : 1;    // [  30]
  unsigned int rightArrow : 1; // [  29]
  unsigned int leftArrow : 1;  // [  28]
  unsigned int aBtn : 1;       // [  27]
  unsigned int bBtn : 1;       // [  26]
  unsigned int cBtn : 1;       // [  25]
  unsigned int startBtn : 1;   // [  24]
  unsigned int xBtn : 1;       // [  23]
  unsigned int rightShift : 1; // [  22]
  unsigned int leftShift : 1;  // [  21]
  unsigned int : 21;           // [0-20] filler, leave set to 0
} JoyPadState;
typedef JoyPadState *JoyPadStatePtr;

int32 KillJoypad (void);
int32 GetJoyPadContinuous (int32 padNum);
int32 SetJoyPadContinuous (int32 continuousBtnFlags, int32 padNum);
void SetJoyPadLeftHanded (Boolean playLeftHanded, int32 padNum);
Boolean GetJoyPad (JoyPadState *joyState, int32 padNum);

extern Boolean gPadInitialized;
#endif
