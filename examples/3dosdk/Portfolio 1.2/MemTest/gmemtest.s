;
;	File:		gmemtest.s
;
;	Contains:	Diagnostic routines for the memory hardware
;
;	Written by:	Stephen H. Landrum
;
;	Copyright:	© 1993 by The 3DO Company. All rights reserved.
;				This material constitutes confidential and proprietary
;				information of the 3DO Company and shall not be used by
;				any Person or for any purpose except as expressly
;				authorized in writing by the 3DO Company.
;
;	Change History (most recent first):
;
;
;	To Do:
;


		INCLUDE registers.i
		INCLUDE macros.i
		INCLUDE structs.i
		INCLUDE hardware.i
;		INCLUDE inthard.i


;--------------------------------------------------

		GBLL	BURST
		GBLL	SLOWTEST

BURST		SETL	{TRUE}
SLOWTEST	SETL	{FALSE}

;--------------------------------------------------

		AREA	ASMCODE2, CODE

;--------------------------------------------------
;
; memtest (ulong seed, void *lomem, void *himem, ulong mask);
;
; r0 <= seed pattern
; r1 <= memory address to start checking
; r2 <= memory address at end of range to check
; r3 <= mask

memtest		PROC

		STMFD	SP!, {r4-r12, lk}

		AND	r4, r3, r0
		AND	r5, r3, r0
		EOR	r5, r3, r5
		AND	r6, r3, r0, ROR #16
		AND	r7, r3, r0, ROR #16
		EOR	r7, r3, r7
		MOV	r8, r4
		MOV	r9, r5
		MOV	r10, r6
		MOV	r11, r7
		MOV	r12, r4

		LDR	r0, ERRORCOUNT
		MOV	r3, r1
	IF BURST
00		  STMIA	r3!, {r4-r11}
	ELSE
00		  STR	r4, [r3], #4
		  STR	r5, [r3], #4
		  STR	r6, [r3], #4
		  STR	r7, [r3], #4
		  STR	r8, [r3], #4
		  STR	r9, [r3], #4
		  STR	r10, [r3], #4
		  STR	r11, [r3], #4
	ENDIF
		  CMP	r3, r2
		  BLT	%00

	IF SLOWTEST
		BL	_longdelay
	ENDIF

	IF BURST
01		  LDMIA	r1!, {r8-r11}
	ELSE
01		  LDR	r8, [r1],#4
		  LDR	r9, [r1],#4
		  LDR	r10, [r1],#4
		  LDR	r11, [r1],#4
	ENDIF
		  CMP	r8, r4
		  CMPEQ	r9, r5
		  CMPEQ r10, r6
		  CMPEQ	r11, r7
		  BNE	%02
03		  CMP	r1, r2
		  BLT	%01

;		LDMIA	r1!, {r8}
;		CMP	r8, r4
;		BEQ	%99
;		CMP	r8, r8			; set Z

		B	%99

02		LDR	r3, [r0]
		ADD	r3, r3, #1
		STR	r3, [r0]
		STR	r1, [r0, #4]
		STR	r4, [r0, #&10]
		STR	r5, [r0, #&14]
		STR	r6, [r0, #&18]
		STR	r7, [r0, #&1c]
		STR	r8, [r0, #&20]
		STR	r9, [r0, #&24]
		STR	r10, [r0, #&28]
		STR	r11, [r0, #&2c]
;		BNE	%03
;		B	%99
		B	%03

99		LDMFD	SP!, {r4-r12, pc}	; RTS


;--------------------------------------------------
;
; memtest2 (ulong seed, void *lomem, void *himem, ulong mask);
;
; r0 <= seed pattern
; r1 <= memory address to start checking
; r2 <= memory address at end of range to check
; r3 <= mask

memtest2	PROC

		STMFD	SP!, {r4-r12, lk}

		MOV	lk, r1

00		  ADD	r9, lk, r0
		  AND	r4, r3, r9
		  AND	r5, r3, r9
		  EOR	r5, r3, r5
		  AND	r6, r3, r9, ROR #16
		  AND	r7, r3, r9, ROR #16
		  EOR	r7, r3, r7

	IF BURST
		  STMIA	lk!, {r4-r7}
	ELSE
		  STR	r4, [lk], #4
		  STR	r5, [lk], #4
		  STR	r6, [lk], #4
		  STR	r7, [lk], #4
	ENDIF
		  CMP	lk, r2
		 BLT	%00

	IF SLOWTEST
		BL	_longdelay
	ENDIF

01		  ADD	r9, r1, r0
		  AND	r4, r3, r9
		  AND	r5, r3, r9
		  EOR	r5, r3, r5
		  AND	r6, r3, r9, ROR #16
		  AND	r7, r3, r9, ROR #16
		  EOR	r7, r3, r7

	IF BURST
		  LDMIA	r1!, {r8-r11}
	ELSE
		  LDR	r8, [r1],#4
		  LDR	r9, [r1],#4
		  LDR	r10, [r1],#4
		  LDR	r11, [r1],#4
	ENDIF
		  CMP	r8, r4
		  CMPEQ	r9, r5
		  CMPEQ r10, r6
		  CMPEQ	r11, r7
		  BNE	%02
03		  CMP	r1, r2
		 BLT	%01

;		LDMIA	r1!, {r8}
;		CMP	r8, r4
;		BEQ	%99
;		CMP	r8, r8			; set Z

		B	%99

02		LDR	r12, ERRORCOUNT
		STR	r1, [r12, #4]
		STR	r4, [r12, #&10]
		STR	r5, [r12, #&14]
		STR	r6, [r12, #&18]
		STR	r7, [r12, #&1c]
		STR	r8, [r12, #&20]
		STR	r9, [r12, #&24]
		STR	r10, [r12, #&28]
		STR	r11, [r12, #&2c]
		LDR	r4, [r12]
		ADD	r4, r4, #1
		STR	r4, [r12]
;		BNE	%03
;		B	%99
		B	%03

99		LDMFD	SP!, {r4-r12, pc}	; RTS


;--------------------------------------------------

	IF SLOWTEST

_longdelay	PROC

		STMFD	sp!, {r0, lk}

		MOV	r0, #42<<20
00		  SUBS	r0, r0, #1
		  BNE	%00

		LDMFD	sp!, {r0, pc}

	ENDIF
		
;--------------------------------------------------

		IMPORT	errorcount

ERRORCOUNT	DCD	errorcount


;--------------------------------------------------


		END


