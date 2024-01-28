;------------------------------------------------------------------------------
;	File:		FontBlit5To8_.s
;
;	Contains:	Private library routine to unpack font pixels.
;
;	Written by: Ian Lepore
;
;	Copyright:	© 1994 by The 3DO Company.  All rights reserved.
;
;	Change History (most recent first):
;	05/15/94  Ian	Changed output logic so that transparent pixels (those with
;					the low-order 5 bits set to zero) are not written to the
;					destination buffer.  This lets shadows kern together 
;					between characters.
;	05/08/94  Ian	New today.
; 
;	Implementation notes:
;
;	This routine should never be invoked directly from client code.  Clients
;	call BlitFontChar() and control ends up in here for 5bpp fonts.
;------------------------------------------------------------------------------
 
        AREA |C$$code|, CODE, READONLY
|x$codeseg|

	EXPORT  FontBlit5To8_

;------------------------------------------------------------------------------
; 16-bit packets are encoded as:
;
; 	1ppppppppppppppp - Output 3 pixels, then check XCount for EOL.
;	0100xxxxcccccccc - Skip cccccccc blank lines.
;	0010xxcccccppppp - Repeat pixel ppppp ccccc times.
;	0001xxxxcccccnnn - Go up nnn+1 lines, copy ccccc pixels.
;	0000xxpppppppppp - Output up to 2 pixels, check XCount for EOL after each
;	0000000000000000 - Early EOL.
;
;	Pixels:
;		00aaa	- FG aa against existing.
;		01aaa	- BG aa against existing.
;		10aaa	- FG aa against BG.
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
; 	r11 = constant for masking pixels
; 	r10 = current input packets
; 	r9  = copy/run work counter
;	r8	= copy work pointer
;	r7	= AMV constant for EOR 
; 	r6	= dstY, then XCount (inner loop X/width counter)
;	r5	= dstX, then deltaBPR (distance from end of dst row to start of next dst row)
; 	r4	= dstBPR
; 	r3	= dstPtr
; 	r2	= srcHeight
; 	r1	= srcWidth
; 	r0	= srcPtr
;
;------------------------------------------------------------------------------

FontBlit5To8_


	stmfd	sp!,{r4-r11,lr}	; save regs
	ldmia	r1,{r3-r7}				; load FontBlitDstParms into regs.
	ldmia	r0,{r0-r2}				; load FontBlitSrcParms into regs.
	
	mla		r12,r6,r4,r5			; temp = (dstY*dstBPR)+dstX
	add		r3,r3,r12				; dstPtr += temp
	
	mov		r11,#&1F				; r11 = constant 00011111 for AND
	mov		r7,#&E0					; r7  = constant 11100000 for AMV EOR operations.
	
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
	bmi		write3
	beq		blankrows
	bcs		litrun
	bvs		copyrun
	b		write2
	
input2								; handle the low-order packet read earlier...
	lea		lr,input1				; after this packet, input1 handles next packet
 	mov		r10,r10,LSL #16			; move other packet up to high-order
	msr		CPSR_flg,r10			; NZCV = 4 high bits of input
	bmi		write3
	beq		blankrows
	bcs		litrun
	bvs		copyrun
	b		write2

litrun
	mov		r9,r10,LSR #21
	and		r9,r9,#&1F
	sub		r6,r6,r9
	mov		r12,r10,LSR #16
	ands	r12,r12,#&1F
	addeq	r3,r3,r9
	beq		skiprun
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
litrunloop
	strb	r12,[r3],#1				; store the pixel
	subs	r9,r9,#1				; until the counter
	bne		litrunloop				; hits zero.
skiprun
	teq		r6,#0					; if XCount is now zero,
	beq		nextrow					; go set up for next row,
	mov		pc,lr					; else go do next packet.

copyrun
	rsb		r9,r4,#0				; work1  = -dstBPR
	mov		r8,r10,LSR #16			; work2  = codes >> 16
	and		r8,r8,#7				; work2 &= 7 (nnn isolated in work2)
	add		r8,r8,#1				; work2++
	mla		r8,r9,r8,r3				; workPtr = work1 * work2 + dstPtr
	mov		r9,r10,LSR #19			; workCount = ccccc
	and		r9,r9,#&1F				; workCount &= 31 (ccccc isolated in workCount)
	sub		r6,r6,r9				; XCount -= copy count
copyrunloop
	ldrb	r12,[r8],#1				; load byte from *workPtr++ (an earlier line)
	tst		r12,r11					; make sure it isn't a transparent pixel,
	strneb	r12,[r3],#1				; if not transparent, store it to *dstPtr++
	addeq	r3,r3,#1
	subs	r9,r9,#1				; feels so good, keep doing it
	bne		copyrunloop				; for a while.
	
	teq		r6,#0					; if XCount is now zero,
	beq		nextrow					; go set up for next row,
	mov		pc,lr					; else go do next packet.

write2								; write 1 or 2 pixels, check EOL after each...
	movs	r12,r10,LSR #16			; if the whole packet is zeroes,
	beq		nextrow					; that means early EOL.
	
	ands	r12,r11,r10,LSR #21
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
	
write3								; write 3 pixels, check EOL after doing them...
	ands	r12,r11,r10,LSR #26
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	
	ands	r12,r11,r10,LSR #21
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	
	ands	r12,r11,r10,LSR #16
	orr		r12,r12,r12,LSL #5
	eor		r12,r12,r7
	strneb	r12,[r3],#1
	addeq	r3,r3,#1
	
	subs	r6,r6,#3
	beq		nextrow	
	mov		pc,lr
	
	END
