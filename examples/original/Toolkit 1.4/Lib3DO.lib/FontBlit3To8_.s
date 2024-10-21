;------------------------------------------------------------------------------
;	File:		FontBlit3To8_.s
;
;	Contains:	Private library routine to unpack font pixels.
;
;	Written by: Ian Lepore
;
;	Copyright:	© 1994 by The 3DO Company.  All rights reserved.
;
;	Change History (most recent first):
;	05/15/94  Ian	Changed output logic so that transparent pixels (those with
;					the low-order 3 bits set to zero) are not written to the
;					destination buffer.  This lets characters kern together, 
;					and gives predictible results when other non-text pixels
;					already exist in the destination buffer.
;	01/26/94  Ian	New today.
; 
;	Implementation notes:
;
;	This routine should never be invoked directly from client code.  Clients
;	call BlitFontChar() and control ends up in here for 3bpp fonts.
;------------------------------------------------------------------------------
 
        AREA |C$$code|, CODE, READONLY
|x$codeseg|

	EXPORT  FontBlit3To8_

;------------------------------------------------------------------------------
; 
; 16-bit packets are encoded as:
;
; 	1ppppppppppppppp - Output 5 pixels, then check XCount for EOL.
; 	01ppppppssssssss - Output 2 pixels, then do special stuff.
; 	0010xpppssssssss - Output 1 pixel, then do special stuff.
; 	00010000ssssssss - Just do special stuff (ssssssss==0 means skip to EOL). 
;	00011000cccccccc - Skip cccccccc blank lines.
; 	0000pppppppppppp - Output up to 4 pixels, checking XCount for EOL after each.
; 	0011xxxxxxxxxxxx - Unassigned; new encodings could be added here.
;
; Special stuff maps out as:
;
; 	0ccccppp - 	Output a run of cccc pixels of value ppp, then check XCount for
; 			 	EOL. A special form of this, 00000000, means early-end-of-line.
; 	1cccccnn - 	Go up nn+1 lines and copy ccccc pixels from there, then check
; 			 	XCount for EOL. 
;
; Parameters are passed as:
;
;	typedef struct FontCharBlitParms_ {	
;		void *srcPtr;
;		int32 srcWidth;
;		int32 srcHeight;
;		void *dstPtr;
;		int32 dstBPR;
;		int32 dstX;
;		int32 dstY;
;	} FontCharBlitParms_;
;
; Register assignments:
;
; 	r12	= output work
; 	r11 = constant 7 (for masking pixels)
; 	r10 = current input packets
; 	r9  = copy/run work counter
;	r8	= copy work pointer
;	r7	= dstColorIndex, then Color/AMV constant for EOR 
; 	r6	= dstY, then XCount (inner loop X/width counter)
;	r5	= dstX, then deltaBPR (distance from end of dst row to start of next dst row)
; 	r4	= dstBPR
; 	r3	= dstPtr
; 	r2	= srcHeight
; 	r1	= srcWidth
; 	r0	= srcPtr
;
;------------------------------------------------------------------------------

FontBlit3To8_


	stmfd	sp!,{r4-r11,lr}	; save regs
	ldmia	r1,{r3-r7}				; load FontBlitDstParms into regs.
	ldmia	r0,{r0-r2}				; load FontBlitSrcParms into regs.
	
	mla		r12,r6,r4,r5			; temp = (dstY*dstBPR)+dstX
	add		r3,r3,r12				; dstPtr += temp
	
	mov		r11,#7					; r11 = constant 00000111 for AND
	
	and		r7,r7,#&03				; mask color index to two bits.
	mov		r7,r7,LSL #3			; shift color index up to its home,
	orr		r7,r7,#&E0				; lay in constant for AMV EOR operations.
	
	sub		r5,r4,r1				; deltaBPR = dstBPR - srcWidth
	mov		r6,r1					; XCount = srcWidth
	b		input1					; go get things started	

blankrows							; skip some all-blank rows...

	mov		r6,r10,LSR #16			; move skipcount to low-order.
	and		r6,r6,#&000000FF		; mask off high-order cruft.
	mul		r12,r6,r4				; skipoffset = skipcount * dstBPR
	b		skiplines				; go skip 'em.

nextrow								; EOL detected, move to next row...

	add		r12,r5,r6				; skipoffset = deltaBPR + XCount
	mov		r6,#1					; skipcount  = 1
skiplines
	subs	r2,r2,r6				; if ((srcHeight-skipcount) == 0)
	ldmeqfd sp!,{r4-r11,pc}			; 	return
	add		r3,r3,r12				; dstPtr += skipoffset
	mov		r6,r1					; XCount  = srcWidth
	mov		pc,lr					; resume at appropriate input packet handler

input1								; read pair of packets, handle high-order...

	lea		lr,input2				; after this packet, input2 handles next packet
	ldr		r10,[r0],#4				; load a pair of 16-bit packets
	msr		CPSR_flg,r10			; NZCV = 4 high bits of input
	bmi		write5
	beq		special2
	bcs		special1
	bvs		special0
	b		write4
	
input2								; handle the low-order packet read earlier...

	lea		lr,input1				; after this packet, input1 handles next packet
 	mov		r10,r10,LSL #16			; move other packet up to high-order
	msr		CPSR_flg,r10			; NZCV = 4 high bits of input
	bmi		write5
	beq		special2
	bcs		special1
	bvs		special0
	b		write4

write4								; write 1-4 pixels, checking EOL after each...

	ands	r12,r11,r10,LSR #25
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	subs	r6,r6,#1
	beq		nextrow
	
	ands	r12,r11,r10,LSR #22
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	subs	r6,r6,#1
	beq		nextrow
	
	ands	r12,r11,r10,LSR #19
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	subs	r6,r6,#1
	beq		nextrow
		
	ands	r12,r11,r10,LSR #16
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	subs	r6,r6,#1
	beq		nextrow

	mov		pc,lr
	
special2 							; write 2 pixels then do special stuff...

	ands	r12,r11,r10,LSR #27
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	sub		r6,r6,#1
	
special1							; write 1 pixel then do special stuff...

	ands	r12,r11,r10,LSR #24
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	sub		r6,r6,#1
	
	bic		r10,r10,#&08000000		; clean pixels out of skip-many-lines flag.
	
special0							; do special stuff (runs, copies, skip lines)...

	tst		r10,#&08000000			; check skip-many-lines flag,
	bne		blankrows				; if set, go skip some lines.

	movs	r12,r10,LSL #9			; move codes to high-order; puts flag bit
	bcs		docopy					; in carry.  branch to copy if flag was on.
	tst		r12,#&FE000000
	beq		nextrow					; flag off and codes=0 skip to EOL.
	
	mov		r9,r12,LSR #28			; load run count into count register. 
	sub		r6,r6,r9				; subtract run count from XCount.
	ands	r12,r11,r12,LSR #25		; load pixel to repeat,
	addeq	r3,r3,r9
	beq		skiprun
	orr		r12,r12,r12,LSL #5		; transform pixel into PLUTindex + AMV.
	eor		r12,r12,r7
litrun
	strb	r12,[r3],#1				; store the pixel
	subs	r9,r9,#1				; until the counter
	bne		litrun					; hits zero.
skiprun
	teq		r6,#0					; if XCount is now zero,
	beq		nextrow					; go set up for next row,
	mov		pc,lr					; else go do next packet.

docopy

	rsb		r9,r4,#0				; work1  = -dstBPR
	mov		r8,r12,LSR #25			; work2  = codes >> 25
	and		r8,r8,#3				; work2 &= 3 (nn isolated in work2)
	add		r8,r8,#1				; work2++
	mla		r8,r9,r8,r3				; workPtr = work1 * work2 + dstPtr
	mov		r9,r12,LSR #27			; workCount = ccccc
	sub		r6,r6,r9				; XCount -= copy count
copyrun
	ldrb	r12,[r8],#1				; load byte from *workPtr++ (an earlier line)
	tst		r12,r11					; make sure it isn't a transparent pixel,
	strneb	r12,[r3],#1				; if not transparent, store it to *dstPtr++
	addeq	r3,r3,#1				; if it was transprent, just incr dstPtr
	subs	r9,r9,#1				; feels so good, keep doing it
	bne		copyrun					; for a while.
	
	teq		r6,#0					; if XCount is now zero,
	beq		nextrow					; go set up for next row,
	mov		pc,lr					; else go do next packet.

write5								; write 5 pixels then check for EOL...

	ands	r12,r11,r10,LSR #28		
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	
	ands	r12,r11,r10,LSR #25
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	
	ands	r12,r11,r10,LSR #22
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	
	ands	r12,r11,r10,LSR #19
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
		
	ands	r12,r11,r10,LSR #16
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1

	subs	r6,r6,#5
	beq		nextrow
	mov		pc,lr

	END
