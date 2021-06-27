; Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
; This material contains confidential information that is the property of The 3DO Company.
; Any unauthorized duplication, disclosure or use is prohibited.
; $Id: registers.i,v 1.2 1994/02/09 02:04:35 limes Exp $
; +++++++++
;
;	Registers.i
;	
;	Copyright (C) 1990-1991 Apple Computer, Incorporated.  All rights reserved.
;
;	Register definitions for the ARM processor.
;	
; +++++

; General registers

R0			RN		0
R1			RN		1
R2			RN		2
R3			RN		3
R4			RN		4
R5			RN		5
R6			RN		6
R7			RN		7
R8			RN		8
R9			RN		9
R10			RN		10
R11			RN		11
R12			RN		12
R13			RN		13
R14			RN		14
R15			RN		15


; Synonyms for registers with specific usage.

SP			RN		R13
LK			RN		R14
PC			RN		R15

; Fans of lowercase will enjoy these

r0			RN		0
r1			RN		1
r2			RN		2
r3			RN		3
r4			RN		4
r5			RN		5
r6			RN		6
r7			RN		7
r8			RN		8
r9			RN		9
r10			RN		10
r11			RN		11
r12			RN		12
r13			RN		13
r14			RN		14
r15			RN		15

; C afficionados use different names

a1			RN		0
a2			RN		1
a3			RN		2
a4			RN		3
v1 			RN		4
v2 			RN		5
v3 			RN		6
v4 			RN		7
v5 			RN		8
sb 			RN		9
sl			RN		10
fp			RN		11
ip			RN		12
sp			RN		13
lk			RN		14
lr			RN		14
pc			RN		15


;;; ARM3 control registers are accesed thru this coprocessor number.

ARM3Cntl	CP		15


;; This coprocessor supports 6 control registers.

; Register 1 flushes the cache.

CntlReg1	CN		1

; Register 2 turns the cache on and off.

CntlReg2	CN		2

; Register 3 turns on / off cacheing of 32 2 megabyte regions.

CntlReg3	CN		3

; Register 4 turns on / off updateability of 32 2 megabyte regions.

CntlReg4	CN		4

CntlReg5	CN		5

			END
