 IF	:DEF:|_KERNELNODES_I|
 ELSE
	GBLL	|_KERNELNODES_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: kernelnodes.i,v 1.5 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel node and item types
;*
;*****************************************************************************

KERNELNODE	EQU	1

MEMFREENODE	EQU	1
LISTNODE	EQU	2
MEMHDRNODE	EQU	3
FOLIONODE	EQU	4	;* Here so I can do quick compares */
TASKNODE	EQU	5
FIRQNODE	EQU	6
SEMA4NODE	EQU	7
SEMA4WAIT	EQU	8

MESSAGENODE	EQU	9
MSGPORTNODE	EQU	10

MEMLISTNODE	EQU	11
ROMTAGNODE	EQU	12

DRIVERNODE	EQU	13
IOREQNODE	EQU	14
DEVICENODE	EQU	15
TIMERNODE	EQU	16
ERRORTEXTNODE	EQU	17

HLINTNODE	EQU	18

 ENDIF ; |_KERNELNODES_I|

	END
