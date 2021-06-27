 IF :DEF:|_TIMER_I|
 ELSE
	GBLL	|_TIMER_I|

;*****

; $Id: timer.i,v 1.2 1993/03/16 09:25:27 dale Exp $

; $Log: timer.i,v $
;Revision 1.2  1993/03/16  09:25:27  dale
;fix comment
;
;Revision 1.1  1993/03/16  06:42:46  dale
;Initial revision
;
;Revision 1.3  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.2  1992/10/29  02:42:57  dale
;added Id and Log
;

;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;
	INCLUDE	structs.i
	INCLUDE	nodes.i

	BEGINSTRUCT	Timer
		STRUCT	ItemNode,tm_tm
		UINT8	tm_ID
		UINT8	tm_Size
		UINT8	tm_IntNum
		UINT8	tm_r	; reserved
		PTR	tm_Control
		PTR	tm_Load
		PTR	tm_Read
	ENDSTRUCT

TIMER_MUST_INTERRUPT    EQU 1

CREATETIMER_TAG_NUM   EQU 1+TAG_ITEM_LAST ;* How many contiguous timers needed?
CREATETIMER_TAG_FLAGS EQU 1+CREATETIMER_TAG_NUM ;MST needs to generate an intrpt

TIMER_DECREMENT EQU 1
TIMER_RELOAD    EQU 2
TIMER_CASCADE   EQU 4
TIMER_FLABLODE  EQU 8
TIMER_ALLBITS   EQU 0xf

TIMER_INTEN     EQU 0x10    ;* enable interrupts */
TIMER_INTREQ    EQU 0x20    ;* request interrupts */



 ENDIF	; |_TIMERS_I|

	END
