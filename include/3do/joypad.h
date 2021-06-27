#ifndef __JPIJOYPAD__
#define __JPIJOYPAD__

#include "event.h"

// as written, we allow 4 control pads
#define	kDefaultCtrlPadCount	4

#define LONGWORD(aStruct)		( *((long*) &(aStruct)) )

// a couple of handy defines for setting pad continuous flag
#define		PADARROWS	(ControlDown | ControlUp | ControlRight | ControlLeft)
#define		PADBUTTONS	(ControlA | ControlB | ControlC | ControlStart | ControlX)
#define		PADSHIFT	(ControlRightShift | ControlLeftShift)
#define		PADALL		(PADARROWS | PADBUTTONS | PADSHIFT)

// bitfield structure for the control pad buttons
typedef struct
{
  unsigned	int downArrow	: 1; // [  31]
  unsigned	int upArrow	: 1; // [  30]
  unsigned	int rightArrow	: 1; // [  29]
  unsigned	int leftArrow	: 1; // [  28]
  unsigned	int aBtn	: 1; // [  27]
  unsigned	int bBtn	: 1; // [  26]
  unsigned	int cBtn	: 1; // [  25]
  unsigned	int startBtn	: 1; // [  24]
  unsigned	int xBtn	: 1; // [  23]
  unsigned	int rightShift	: 1; // [  22]
  unsigned	int leftShift   : 1; // [  21]
  unsigned	int             : 21; // [0-20] filler, leave set to 0
} JoyPadState;
typedef JoyPadState *JoyPadStatePtr;


int32	KillJoypad( void );
int32	GetJoyPadContinuous(int32 padNum);
int32	SetJoyPadContinuous(int32 continuousBtnFlags, int32 padNum);
void	SetJoyPadLeftHanded(boolean playLeftHanded, int32 padNum);
boolean	GetJoyPad(JoyPadState* joyState, int32 padNum);

extern	boolean gPadInitialized;
#endif
