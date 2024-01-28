 IF	:DEF:|_KERNELNODES_I|
 ELSE
	GBLL	|_KERNELNODES_I|

;*****

; $Id: kernelnodes.i,v 1.3 1993/02/11 04:58:58 dale Exp $

; $Log: kernelnodes.i,v $
;Revision 1.3  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.2  1992/10/29  01:28:19  dale
;added Id and Log
;

;*****/

;/*
;    Copyright (C) 1992, New Technologies Group, Inc.
;    All Rights Reserved
;    Confidential and Proprietary
;*/

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

 ENDIF ; |_KERNELNODES_I|

	END
