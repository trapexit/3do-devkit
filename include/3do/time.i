 IF :DEF:|_TIME_I|
 ELSE
	GBLL	|_TIME_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: time.i,v 1.4 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel time management definitions
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE device.i
	INCLUDE item.i

	BEGINSTRUCT	TimerDevice
		STRUCT	Device,timerdev_dev
		UINT32  timerdev_VBlankCountOverFlow
		UINT32  timerdev_VBlankCount
	ENDSTRUCT

TIMER_UNIT_VBLANK	EQU 0
TIMER_UNIT_USEC		EQU 1

TIMERCMD_DELAY		EQU 3
TIMERCMD_DELAYUNTIL	EQU 4
TIMERCMD_METRONOME	EQU 5

	BEGINSTRUCT	TIMEVAL
		INT32	tv_sec
		INT32	tv_usec
	ENDSTRUCT

	BEGINSTRUCT	TimeVal
		INT32	tv_Seconds
		INT32	tv_Microseconds
	ENDSTRUCT

	BEGINSTRUCT	VBlankTimeVal
		UINT32	vbltv_VBlankHi32
		UINT32	vbltv_VBlankLo32
	ENDSTRUCT

	MACRO
	SampleSystemTime
	swi	KERNELSWI+38
	MEND

 ENDIF	; |_TIME_I|

	END
