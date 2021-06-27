;---------------------------------------
;-----------------------------------------------------
;
;  math.s : performs basic 3d math and vector operations
;           and rotations
;
;  Written by Dan Duncalf
;
;  Copyright 1993, The 3DO Company
;  All Rights Reserved
;  This document is proprietary and confidential
;
;-----------------------------------------------------
;
;  void NormalizeVector(vecf16 *vector};
;  uint VectorLength(vecf16 *vector};
;
;  void BuildRotateX (int Angle,mat3 *matrix};
;  void BuildRotateY (int Angle,mat3 *matrix};
;  void BuildRotateZ (int Angle,mat3 *matrix};
;
;  void RotateOnX (int Angle,mat3 *matrix};
;  void RotateOnY (int Angle,mat3 *matrix};
;  void RotateOnZ (int Angle,mat3 *matrix};
; 
;  void RotatePoint (int x,int y,int angle};
;
;  /*  Routine removed s30_2 Mulsf30_30 (s30_2 mult1,s30_2 mult2};  */
;
;  frac16 VectorDot  (vector3 *Vect1,vector3 *Vect2};
;  void VectorCross (vector3 *ResultVect,vector3 *Vect1,vector3 *Vect2};
;  	/* matrix1 = matrix1 * matrix2 */
;  void MatMulMatf2 (mat3 *matrix1,mat3 *matrix2};	
;  void VectMulMatf2 (vector3 *vector,mat3 *matrix,vector3 *result};
;
;---------------------------------------

	EXPORT	NormalizeVectorXX
	EXPORT	VectorLength
	EXPORT	BuildRotateX
	EXPORT	BuildRotateY
	EXPORT	BuildRotateZ
	EXPORT	RotateOnX
	EXPORT	RotateOnY
	EXPORT	RotateOnZ
	EXPORT	VectorDot
	EXPORT	VectorCross

bits32		EQU	1

DebugCount0_2	DCB	"r0= %lx, r1= %lx,r2 =%lx"
		DCB	&0a
		DCD	0

;print out registers R0-R2

	MACRO
$label	DBG0_2
$label	mov	r0,r0
	PUSH	r0-r4
	mov	r3,r2
	mov	r2,r1
	mov	r1,r0
	ADRL	r0,DebugCount0_2
	bl	kprintf
	POP	r0-r4
	MEND
	IMPORT	kprintf

;print out registers R3-R5

DebugCount3_5	DCB	"r3= %lx, r4= %lx,r5 =%lx"
		DCB	&0a
		DCD	0

	MACRO
$label	DBG3_5
$label	mov	r0,r0
	PUSH	r0-r5
	mov	r1,r3
	mov	r3,r5
	mov	r2,r4
	ADRL	r0,DebugCount3_5
	bl	kprintf
	POP	r0-r5
	MEND	

;print out registers R6-R8

DebugCount6_8	DCB	"r6= %lx, r7= %lx,r8 =%lx"
		DCB	&0a
		DCD	0

	MACRO
$label	DBG6_8
$label	mov	r0,r0
	PUSH	r0-r5
	mov	r3,r8
	mov	r2,r7
	mov	r1,r6
	ADRL	r0,DebugCount6_8
	bl	kprintf
	POP	r0-r5
	MEND	

;print out registers R9-R11

DebugCount9_11	DCB	"r9= %lx, r10= %lx, r11 =%lx"
		DCB	&0a
		DCD	0

	MACRO
$label	DBG9_11
$label	mov	r0,r0
	PUSH	r0-r5
	mov	r3,r11
	mov	r2,r10
	mov	r1,r9
	ADRL	r0,DebugCount9_11
	bl	kprintf
	POP	r0-r5
	MEND	

	MACRO
$label	DUMP_REGISTERS
$label	DBG0_2
	DBG3_5
	DBG6_8
	DBG9_11
	MEND
	
;print out tags

Aaa_tag	DCB	"aaa"
		DCB	&0a
		DCD	0

Bbb_tag	DCB	"bbb"
		DCB	&0a
		DCD	0

Ccc_tag	DCB	"ccc"
		DCB	&0a
		DCD	0

Ddd_tag	DCB	"ddd"
		DCB	&0a
		DCD	0

	MACRO
$label	DBGAAA
$label	mov	r0,r0
	PUSH	r0-r5
	mov	r3,r5
	mov	r2,r4
	mov	r1,r3
	ADRL	r0,Aaa_tag
	bl	kprintf
	POP	r0-r5
	MEND	

	MACRO
$label	DBGBBB
$label	mov	r0,r0
	PUSH	r0-r5
	mov	r3,r5
	mov	r2,r4
	mov	r1,r3
	ADRL	r0,Bbb_tag
	bl	kprintf
	POP	r0-r5
	MEND	

	MACRO
$label	DBGCCC
$label	mov	r0,r0
	PUSH	r0-r5
	mov	r3,r5
	mov	r2,r4
	mov	r1,r3
	ADRL	r0,Ccc_tag
	bl	kprintf
	POP	r0-r5
	MEND	

	MACRO
$label	DBGDDD
$label	mov	r0,r0
	PUSH	r0-r5
	mov	r3,r5
	mov	r2,r4
	mov	r1,r3
	ADRL	r0,Ddd_tag
	bl	kprintf
	POP	r0-r5
	MEND	


RadianQuad	EQU	&400000	; 64.0 = One quadrant of radian.

;----------  Matrix indexes.

	BEGINSTRUCT	mat3x3
		STRUCT	LONG,x1
		STRUCT	LONG,y1
		STRUCT	LONG,z1
		STRUCT	LONG,x2
		STRUCT	LONG,y2
		STRUCT	LONG,z2
		STRUCT	LONG,x3
		STRUCT	LONG,y3
		STRUCT	LONG,z3
	ENDSTRUCT

	MACRO
$label	MATCALL
$label	bl	GlueMulMat33Mat33_F16
	MEND
;-----------------------------------------------
;
; GlueMulMat33Mat33_F16 (Same stuff as MulMat33Mat33_F16)
;
;-----------------------------------------------
	EXPORT	GlueMulMat33Mat33_F16
GlueMulMat33Mat33_F16
		ENTER	r4-r9		;MODIFIED for StackChecking 'C' code.
		PUSH	r0
		sub	sp,sp,#36
		mov	r0,sp
		LIBCALL	MulMat33Mat33_F16
		ldmia	sp,{r1-r9}
		add	sp,sp,#36
		POP	r0
		stmia	r0,{r1-r9}
		EXIT	r4-r9

;----------------------------------------------
;
; NormalizeVector(*vector)
; 2.30 scale
;
;----------------------------------------------
NormalizeVectorXX
		ENTER	r4-r9
		sub	sp,sp,#8
		mov	r4,r0		;Pointer to vector to normalize
		bl	VectorLength

		;We need to shift the Vector length over until bit 31 is set,
		;(vl > 4000 0000) then take that count and shift over each of
		;the x,y,z components.

		;Now it needs to be scaled down, so we take 4000 0000 0000 0000
		;and divide it by the vector length.

		;Then each of the components gets multiplied by this unsigned
		;32 bit value taking the top 32 bits as the resulatant size.

		movs	r5,#0

CheckAgain
		movne	r0,r0,lsl #1
		addne	r5,r5,#1
		cmp	r0,#&40000000		;0x4000 0000
		blt	CheckAgain		;r5 = shift count


;		mov	r1,r5			;For DBG to see.
;		DBG



		mov	r1,r0
		add	r0,sp,#0	;
		mov	r2,#&40000000	;0x4000 0000, top 32 bits
		str	r2,[r0]
		mov	r2,#0
		str	r2,[r0,#4]      ;bottom 32 bits
		bl	DivU6432		;Divide unsigned 64 by 32 yield
						;32 bit result.r0 = 32 bit 
						;fractional multiply amount.
;		DBG
		DBGAAA
		DBG0_2
		ldmia	r4,{r6,r7,r8}		;x,y,z
		DBGBBB
		DBG6_8
		mov	r9,r0			;Fractional amount.

		mov	r1,r6,lsl r5
;		DBG
		bl	muls32Fract		;x<<r5*Fract
;		DBG
		mov	r6,r0,asr #14

		mov	r0,r9
		mov	r1,r7,lsl r5
;		DBG
		bl	muls32Fract
;		DBG
		mov	r7,r0,asr #14		;y<<r5*Fract

		mov	r0,r9
		DBGCCC
		DBG6_8
		mov	r1,r8,lsl r5
		DBGDDD
		DBG0_2
;		DBG
		bl	muls32Fract
;		DBG
		mov	r8,r0,asr #14		;z<<r5*Fract
		DBGAAA
		DBG6_8
		stmia	r4,{r6-r8}
		add	sp,sp,#8
		EXIT	r4-r9


;----------------------------------------------
;
; VectorLength(*vector)
; Any dot scale
;
;----------------------------------------------
VectorLength
		ENTER	r4-r6

		sub	sp,sp,#16		;Allocate two 64bit values on stack
		ldmia	r0,{r4-r6}
		add	r0,sp,#0	;
		movs	r1,r4
		rsbmi	r1,r1,#0
		mov	r2,r1
		LIBCALL	MulU32_64	;Temp64=abs(x)^2

		
		add	r0,sp,#8	;Temp64b
		movs	r1,r5
		rsbmi	r1,r1,#0
		mov	r2,r1
		bl	MulU32_64	;Temp64b=abs(y)^2
		
		add	r0,sp,#8	;Temp64b
		ldmia	r0,{r1,r2}
		add	r3,sp,#0	;Temp64
		ldmia	r3,{r3,r4}
		adds	r2,r4,r2
		adc	r1,r3,r1
		stmia	r0,{r1,r2}	;Temp64b=Temp64+Temp64b

		movs	r1,r6
		rsbmi	r1,r1,#0
		mov	r2,r1
		add	r0,sp,#0
		bl	MulU32_64	;Temp64=abs(z)^2

		
		add	r0,sp,#0
		ldmia	r0,{r1,r2}
		add	r3,sp,#8	;[pc,#(Temp64ptr-.-8)]
		ldmia	r3,{r3,r4}
		adds	r2,r4,r2
		adc	r1,r3,r1
		stmia	r0,{r1,r2}
		LIBCALL	Sqrt64_32	;r0=sqrt(x^2+y^2+z^2)
		add	sp,sp,#16
		EXIT	r4-r6


;----------------------------------------------
;
;	BuildRotateX (Angle,*matrix)
;
;					|1  0  0|
; Builds and X rotation matrix.		|0  C  S|
;					|0 -S  C|
;To:  DotScaleDef
;----------------------------------------------

BuildRotateX
		ENTER	r4-r9
		mov	r9,r1
		bl	CosSin
		mov	r4,r0		;cos
		mov	r8,r0		;cos
		mov	r5,r1		;sin
		rsb	r7,r1,#0	;-sin
		mov	r0,#&10000	;  1
		mov	r1,#0
		mov	r2,#0
		mov	r3,#0
		mov	r6,#0
		stmia	r9,{r0-r8}
		EXIT	r4-r9

;----------------------------------------------------
;
;	BuildRotateY (Angle,*matrix)
;
;					| C  0  S|
; Builds and Y rotation matrix.		| 0  1  0|
;To: DotScaleDef			|-S  0  C|
;----------------------------------------------------

BuildRotateY
		ENTER	r4-r9
		mov	r9,r1
		bl	CosSin
		mov	r2,r1		; Sin
		mov	r8,r0		; Cos
		rsb	r6,r2,#0	;-Sin
		mov	r4,#&10000	;  1
		mov	r1,#0
		mov	r3,#0
		mov	r5,#0
		mov	r7,#0
		stmia	r9,{r0-r8}
		EXIT	r4-r9

;----------------------------------------------
;
;	BuildRotateZ (Angle,*matrix)
;
;					| C  S  0|
; Builds and Z rotation matrix.		|-S  C  0|
;To: DotScaleDef			| 0  0  1|
;----------------------------------------------

BuildRotateZ
		ENTER	r4-r9
		mov	r9,r1
		bl	CosSin
		mov	r4,r0		; Cos
		rsb	r3,r1,#0	;-Sin
		mov	r8,#&10000	; 1
		mov	r2,#0
		mov	r5,#0
		mov	r6,#0
		mov	r7,#0
		stmia	r9,{r0-r8}
		EXIT	r4-r9


;-----------------------------------------------
;
;muls32Fract	(s30_2 ,s30_2)
;		r0 is the unsigned fractional amount to multiply by r1
;		r1 is the signed value.
;
;		Lower bits are lost.  
;
; r0=r0*r1>>32    r2,r3 destroyed.
;
;-----------------------------------------------
muls32Fract	PROC
	IF bits32=1
		movs	r1,r1			;Check sign of r1
		rsbmi	r1,r1,#0		;Make positive
		mov	r2,r0,lsr #16		;The high order bits.
		mov	r3,r1,lsr #16		;The high order bits.
		bic	r0,r0,r2,lsl #16	;Clear out high order bits
		bic	r1,r1,r3,lsl #16	;Clear out high order bits

		mul	r0,r3,r0		;low * high
		mul	r1,r2,r1		;low * high
		mul	r2,r3,r2		;high * high

		add	r2,r2,r0,lsr #16
		add	r2,r2,r1,lsr #16
		mov	r0,r2
		rsbmi	r0,r0,#0		;Correct the sign of result.
	ELSE
		mov	r1,r1,asr #15
		mov	r0,r0,asr #15
		mul	r0,r1,r0
	ENDIF

		mov	pc,lk


;-------------------------------------------------
;
; DivU6432 (int64 Dividend, int32 Divisor)
;{}	
;		*r0 64 bit Dividend
;		r1  32 bit Divisor
;
;	Divides 64 bit unsigned value by 32 bit unsigned value
;	yielding 32 bit result.
;
;-------------------------------------------------
DivU6432
		ENTER
		sub	sp,sp,#24
		mov	r2,r0
		add	r3,sp,#0	;[pc,#Temp64bptr-.-8]
		str	r1,[r3,#4]	;Store the low long
		mov	r1,#0
		str	r1,[r3,#0]	;0 in the high long
		add	r0,sp,#16	;[pc,#Temp64cptr-.-8]	;The answer
		add	r1,sp,#8	;[pc,#Temp64dptr-.-8]	;The remainder.
		LIBCALL	DivU64
		ldr	r0,[sp,#20]	;Get the 32 bit answer.
		add	sp,sp,#24
		EXIT

;-------------------------------------------------------
;
;RotateOnX(Angle,*matrix)
;
;-------------------------------------------------------
RotateOnX
		ENTER	r4
		sub	sp,sp,#36
		mov	r4,r1
		add	r1,sp,#0	;[pc,#TempMptr-.-8]
		bl	BuildRotateX
		mov	r0,r4
		add	r2,sp,#0
		mov	r1,r0
		MATCALL
		add	sp,sp,#36
		EXIT	r4

;-------------------------------------------------------
;
;RotateOnY(Angle,*matrix)
;
;-------------------------------------------------------
RotateOnY
		ENTER	r4
		sub	sp,sp,#36
		mov	r4,r1
		add	r1,sp,#0
		bl	BuildRotateY
		mov	r0,r4
		mov	r1,r0
		add	r2,sp,#0
		MATCALL
		add	sp,sp,#36		
		EXIT	r4


;-------------------------------------------------------
;
;RotateOnZ(Angle,*matrix)
;
;-------------------------------------------------------
RotateOnZ
		ENTER	r4
		sub	sp,sp,#36
		mov	r4,r1
		add	r1,sp,#0
		bl	BuildRotateZ
		mov	r0,r4
		mov	r1,r0
		add	r2,sp,#0
		MATCALL
		add	sp,sp,#36
		EXIT	r4

;-----------------------------------------------------------------
;  void CosSin (int32 angle)
;  (NOT C CALLABLE BECAUSE IT RETURNS R0 AND R1
;  Returns the Cos and the Sin of an angle in 2.30 format.
;  r0=cos, r1=sin
;-----------------------------------------------------------------
CosSin		
	IF {FALSE}
		ENTER	r4-r5
		mov	r5,r0
		IF bits32=1
		bl	Sinf2
		mov	r4,r0
		ELSE
		bl	SinF16
		mov	r4,r0,asl #14
		ENDIF
		mov	r0,r5
		rsb	r0,r0,#RadianQuad
		IF bits32=1
		bl	Sinf2
	;	mov	r0,r0
		ELSE
		bl	SinF16
		mov	r0,r0,asl #14
		ENDIF
		mov	r1,r4
	ENDIF
	
		ENTER	r4-r5
		mov	r5,r0
		LIBCALL	SinF16
		mov	r4,r0
		mov	r0,r5
		rsb	r0,r0,#&400000
		LIBCALL	SinF16
		mov	r1,r4
		EXIT	r4-r5


	IF {FALSE}
Sinf2		PROC
		ENTER
		sub	sp,sp,#8
		mov	r1,r0
		mov	r0,sp
		bl	Sinf32
		ldmia	sp,{r0,r1}

		mov	r1,r1,lsr #2	;Convert answer from 32.32 to 2.30
		and	r0,r0,#3	
		orr	r0,r1,r0,ror #2

		add	sp,sp,#8
		EXIT

	ENDIF

;-------------------------------------------------------------------------
;
;  frac16 VectorDot  (vecf16 *Vect1,vec3f2 *Vect2)
;
;-------------------------------------------------------------------------

VectorDot	ENTER	r4-r9
		ldmia	r0,{r4-r6}
		ldmia	r1,{r7-r9}

		mov	r0,r4
		mov	r1,r7
		LIBCALL	MulSF16
		mov	r4,r0

		mov	r0,r5
		mov	r1,r8
		bl	MulSF16
		add	r4,r4,r0

		mov	r0,r6
		mov	r1,r9
		bl	MulSF16
		add	r0,r4,r0
	
		EXIT	r4-r9

;--------------------------------------------------------------------------
;
;  void VectorCross(vecf16 *ResultVect,vec3f2 *Vect1,vec3f2 *Vect2)
;
;--------------------------------------------------------------------------

VectorCross	ENTER	r4-r10
		mov	ip,r0
		ldmia	r1,{r4-r6}
		ldmia	r2,{r7-r9}
		
		mov	r0,r5
		mov	r1,r9		;y*z'
		bl	MulSF16
		mov	r10,r0

		mov	r0,r6
		mov	r1,r8
		bl	MulSF16		;z*y'
		sub	r10,r10,r0	
		str	r10,[ip]	;x=y*z' - z*y'

		mov	r0,r4
		mov	r1,r9
		bl	MulSF16		;x*z'
		mov	r10,r0

		mov	r0,r6
		mov	r1,r7
		bl	MulSF16		;z*x'
		sub	r10,r10,r0
		str	r10,[ip,#4]	;y=x*z' - z*x'

		mov	r0,r4
		mov	r1,r8
		bl	MulSF16		;x*y'
		mov	r10,r0

		mov	r0,r5
		mov	r1,r7
		bl	MulSF16		;y*x'
		sub	r10,r10,r0
		str	r10,[ip,#8]	;z=x*y' - y*x'
		
		EXIT	r4-r10



	END
