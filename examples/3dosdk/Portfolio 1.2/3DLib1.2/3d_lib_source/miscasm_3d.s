;-----------------------------------------------------
;
;  misc_3d.s : Miscellaneous assembly routines
;
;  Written by Dan Duncalf
;
;  Copyright 1993, The 3DO Company
;  All Rights Reserved
;  This document is proprietary and confidential
;
;  Err SetObjectMatrix (WorldPort *world,Obj3D Obj,mat3f2 matrix);
;  void CopyMatrix(mat3f2 matrix, mat3f2 matrix);
;
;-----------------------------------------------------
DEBUG	EQU	1
	INCLUDE macros.i
	INCLUDE structs.i
	INCLUDE registers.i
	INCLUDE stack.i
	INCLUDE operamath.i

	AREA	CODE_AREA,CODE

	INCLUDE userdefs3D.i
	INCLUDE 3dlib.i

;UserHandles	EQU	36*4	;GOT AN ERROR.  VERIFY THIS NUMBER IS CORRECT.




;--------------------------------------------------------------------------
;
; VectorTranslate (vec3f16 *vectarray,vec3f16 *const,int32 count);
;
;    An optimized vector translation routine where
;	vectarray is an array of count vectors that are added to the const
;       vector.   This is used in DoRotateMath in render_3d.c
;
;--------------------------------------------------------------------------
	EXPORT	VectorTranslate
VectorTranslate
	ENTER	r4-r10

	ldmia	r1,{r3-r5}
	mov	r1,r2		
	mov	r2,r2,lsr #2		;count/4

	ands	r1,r1,#3		;count % 4
	beq	next4c			;if no remainder, start 4 wide loop.
	
next1
	ldmia	r0,{r6-r8}
	add	r6,r6,r3
	add	r7,r7,r4
	add	r8,r8,r5
	stmia	r0!,{r6-r8}
	subs	r1,r1,#1
	bne	next1
next4c
	cmp	r2,#0
	beq	none	
next4
	ldmia	r0,{r1,r6-r10}
	add	r1,r1,r3
	add	r6,r6,r4
	add	r7,r7,r5
	add	r8,r8,r3
	add	r9,r9,r4
	add	r10,r10,r5
	stmia	r0!,{r1,r6-r10}
	
	ldmia	r0,{r1,r6-r10}
	add	r1,r1,r3
	add	r6,r6,r4
	add	r7,r7,r5
	add	r8,r8,r3
	add	r9,r9,r4
	add	r10,r10,r5
	stmia	r0!,{r1,r6-r10}

	subs	r2,r2,#1
	bne	next4
none	
	EXIT	r4-r10

	EXPORT	GetLocalPositions
	IMPORT	printmatrix
;---------------------------------------------------------
;
; GetLocalPositions(WorldPort *world, Obj3D camera_id,WorldObject *cam)
;
; batch routine to generate the local positions of all objects 
;     	visible to the supplied camera. This is used by
;	BuildCameraList().
;---------------------------------------------------------
GetLocalPositions	
	ENTER	r4-r9
	
	add	r2,r2,#wo_WorldPosition
	ldmia	r2,{r3-r5}			;Get cameras x,y,z position
	stmfd	sp!,{r0,r2,r10}
	ldr	r2,[r0,#wp_TotalSortedObjects]
	
	cmp	r2,#0
	beq	NoCount
						;r2=ObjectCount
	mov	r6,#1				;r0->WorldPointer	
	mov	r1,r6,LSL r1			;r1=Visible mask
						;r2=ObjectCount
	ldr	ip,[r0,#wp_PointHeapPtr]	;r3-r5 cameras local position
						;ip = Heap area.
	add	r6,r0,#wp_UserObjects		;Point to the array of pointers.
	ldr	r7,[r0,#wp_MaxObjects]
	add	r6,r6,r7,lsl #2
	mov	r0,#0
NextObject
	ldr	r7,[r6],#4			;r7 -> WorldObject
	ldr	r8,[r7,#wo_CameraVisibility]
	tst	r8,r1				;Is camera mask here
	beq	NotVisible1			;No, then skip this object.
	add	r0,r0,#1			;Count++
	add	r7,r7,#wo_WorldPosition		;Point to the points
	ldmia	r7,{r8-r10}			;Load them
	sub	r8,r8,r3
	sub	r9,r9,r4			;Subtract the cameras x,y,z
	sub	r10,r10,r5
	stmia	ip!,{r8-r10}			;Store the result in the heap.
NotVisible1
	subs	r2,r2,#1
	bne	NextObject
	mov	r3,r0				;Get the count
	ldmfd	sp!,{r0,r2,r10}			;Get the world and camera pointer
	mov	r4,r0				;r4 is the world pointer
	mov	r5,r1				;r5 is the camera mask
	add	r2,r2,#(wo_ObjectMatrix-wo_WorldPosition)
	
	ldr	r0,[r0,#wp_PointHeapPtr]
	mov	r6,r0				;r6 is the PointHeapPtr
	mov	r1,r0
	cmp	r3,#0
	beq	NoCount	
	SWI	MULMANYVEC3MAT33_F16

	ldr	r0,[r4,#wp_TotalSortedObjects]	
	add	r1,r4,#wp_UserObjects
	ldr	r2,[r4,#wp_MaxObjects]
	add	r1,r1,r2,lsl #2
NextObjectToCopy
	ldr	r2,[r1],#4
	ldr	r7,[r2,#wo_CameraVisibility]
	tst	r7,r5
	beq	NotVisible2
	
	add	r2,r2,#wo_LocalPosition
	ldmia	r6!,{r7-r9}
	stmia	r2,{r7-r9}
NotVisible2
	subs	r0,r0,#1
	bne	NextObjectToCopy				
NoCount		
	EXIT	r4-r9
	
	
;---------------------------------------------------------
;
; MapCelQuick (CCB *ccb, Point *quad)
;
; Written by Dan Duncalf, adapted from MapCell.  
;   If the cel is 2x2 (Typical monochome polygon), performs an optimized 
;   MapCel without doing any divides  (VERY VERY COSTLY).
;
; The overhead using MapCelQuick is quicker only if more than 5% of the cels'
; are 2x2.  Otherwise it adds a 10 cycle per face penalty to mapcel.
;
;---------------------------------------------------------
	EXPORT	MapCelQuick
	IMPORT	MapCel
MapCelQuick
	add	r0,r0,#&3c		;ccb_Width
	ldmia	r0,{r2-r3}		; Get Width in r2, length in r3.
	sub	r0,r0,#&3c
	cmp	r2,#2
	BNE	MapCel			;if width not two, leave.
	cmp	r3,#2
	BNE	MapCel			;If height not two, leave.
	
	ENTER	r4-r10
	ldmia	r1,{r3-r10}

	MOV     r1,#&8000
        ADD     r2,r1,r3,LSL #16	;((quad[0].pt_X<<16))+0x8000;
        STR     r2,[a1,#&10]
        ADD     r1,r1,r4,LSL #16	;((quad[0].pt_X<<16))+0x8000;
        STR     r1,[a1,#&14]
        SUB     r1,r5,r3		
        MOV     r1,r1,LSL #19
        STR     r1,[a1,#&18]		;((quad[1].pt_X-quad[0].pt_X)<<20) >>1
        SUB     r1,r6,r4
        MOV     r1,r1,LSL #19
        STR     r1,[a1,#&1c]		;((quad[1].pt_Y-quad[0].pt_Y)<<20) >>1;
        SUB     r1,r9,r3
        MOV     r1,r1,LSL #15
        STR     r1,[a1,#&20]		;((quad[3].pt_X-quad[0].pt_X)<<16) >>1;
        SUB     r1,r10,r4
        MOV     r1,r1,LSL #15
        STR     r1,[a1,#&24]		;((quad[3].pt_Y-quad[0].pt_Y)<<16) >>1;
        SUB     r1,r7,r9
        SUB     r1,r1,r5
        add	r1,r3,r1
        MOV     r1,r1,LSL #18
        STR     r1,[a1,#&28]		;((quad[2].pt_X-quad[3].pt_X-quad[1].pt_X+quad[0].pt_X)<<20) >>2;
        SUB     r1,r8,r10
	sub	r1,r1,r6
	add	r1,r1,r4
        MOV     r1,r1,LSL #18
        STR     r1,[a1,#&2c]!		;((quad[2].pt_Y-quad[3].pt_Y-quad[1].pt_Y+quad[0].pt_Y)<<20) >>2;

        EXIT	r4-r10
        
        END
        
;---------------------------------------------------------------
;
;Err SetObjectMatrix(WorldPort *world,Obj3D Obj,mat3f2 matrix)
;
;---------------------------------------------------------------

SetObjectMatrix	PROC
		ENTER	r4-r9
		add	r1,r1,#(wp_UserObjects/4)
		ldr	ip,[r0,r1,lsl #2]		;De-reference the object
		ldmia	ip,{r0,r1,r3-r9}		;Load the matrix
		stmia	r2,{r0,r1,r3-r9}		;Save the matrix
		EXIT	r4-r9

;---------------------------------------------------
;
;void CopyMatrix(mat3f2 matrix,mat3f2 matrix)
;
;---------------------------------------------------
CopyMatrix	PROC
		ENTER	r4-r9
		ldmia	r1,{r1-r9}
		stmia	r0,{r1-r9}
		EXIT	r4-r9

	END

