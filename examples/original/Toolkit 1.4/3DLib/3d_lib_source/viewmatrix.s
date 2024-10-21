
;-----------------------------------------------------
;
;  viewmatrix.s : constructs the 3d rotation matrices
;
;  This is used as an "include" file by math_3d.s
;
;  Written by Dan Duncalf
;
;  Copyright 1993, The 3DO Company
;  All Rights Reserved
;  This document is proprietary and confidential
;
;------------------------------------------------------
;------------------------------------------------------------
; 
;BuildXYZ(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;BuildXZY(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;BuildYXZ(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;BuildYZX(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;BuildZXY(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;BuildZYX(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;
;------------------------------------------------------------

	EXPORT  MakeRotationMatrix
	EXPORT	MakeCRotationMatrix
	EXPORT	BuildXYZ
	EXPORT	BuildXZY
	EXPORT	BuildYXZ
	EXPORT	BuildYZX
	EXPORT	BuildZXY
	EXPORT	BuildZYX
		

Cx	RN	4
Sx	RN	5
Cy	RN	6
Sy	RN	7
Cz	RN	8
Sz	RN	9
		
;-----------------------------------------------------------------
;
; MakeCRotationMatrix (*WorldOrientation,*matrix,method)
;
;-------------------------------------------------------------

MakeCRotationMatrix
		ENTER	r4-r10
		mov	r4,r2,lsl #2
		mov	r3,r1
		ldmia	r0,{r0-r2}
		ADR	r5,OrientationTable
		and	r4,r4,#&01c
		ldr	pc,[r5,r4]		

;--------------------------------------------------------
;
;MakeRotationMatrix (x,y,z,*matrix,method)
;
;
;MakeRotationMatrix, makes a matrix x=r0,y=r1,z=r2, *matrix=r3 and
;			matrix order = r4 {0,4,8,12,16,20}
;
;--------------------------------------------------------
MakeRotationMatrix
		ENTER	r4-r10
		ldr	r4,[r12]
		and	r4,r4,#&01c
		ADR	r5,OrientationTable
		ldr	pc,[r5,r4]
				
OrientationTable	DCD	BuildXYZjmp
			DCD	BuildXZYjmp
			DCD	BuildYXZjmp
			DCD	BuildYZXjmp
			DCD	BuildZXYjmp
			DCD	BuildZYXjmp

			DCD	BuildXYZjmp	;In case of overflow in r4
			DCD	BuildXYZjmp

;---------------------------------------------
;BuildXYZ(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;  The resultant matrix is this.  ie.  Rx * Ry * Rz
;
; |  CyCz           CySz         Sy     |     
; | -SxSyCz-CxSz   -SxSySz+CxCz    SxCy |
; | -CxSyCz+SxSz   -CxSySz-SxCz    CxCy |
; 
;---------------------------------------------

BuildXYZ	ENTER	r4-r10
BuildXYZjmp		
		mov	r9,r3
		mov	r7,r1			;r7 - Y rotation
		mov	r8,r2			;r8 - Z rotation.
		bl	CosSin
		mov	Cx,r0			;r4 = Cos(X)
		mov	Sx,r1			;r5 = Sin(X)
		mov	r0,r7
		bl	CosSin
		mov	Cy,r0			;r6 = Cos(Y)
		mov	Sy,r1			;r7 = Sin(Y)
		mov	r0,r8
		bl	CosSin
		mov	r10,r9			
		mov	Cz,r0			;r8 = Cos(Z)
		mov	Sz,r1			;r9 = Sin(Z)

		mov	r0,Cy
		bl	MulF16
		mov	r3,r0			;CySz

		mov	r0,Cy
		mov	r1,Cz
		bl	MulF16			;CyCz

		stmia	r10!,{r0,r3,Sy}

		rsb	r0,Sx,#0
		mov	r1,Sy
		bl	MulF16
		mov	r3,r0			;r3=-SxSy
		mov	r1,Cz
		bl	MulF16
		mov	ip,r0
		rsb	r0,Sz,#0
		mov	r1,Cx
		bl	MulF16
		add	r0,r0,ip
		stmia	r10!,{r0}

		mov	r0,Cz
		bl	MulF16
		mov	ip,r0
		mov	r0,r3
		mov	r1,Sz
		bl	MulF16
		add	r0,r0,ip
		stmia	r10!,{r0}

		mov	r1,Sx
		mov	r0,Cy
		bl	MulF16
		stmia	r10!,{r0}		;Done with second row

		rsb	r0,Cz,#0
		bl	MulF16			;-SxCz
		mov	ip,r0
		rsb	r0,Cx,#0
		mov	r1,Sy
		bl	MulF16			;-CxSy
		mov	r3,r0
		mov	r1,Sz
		bl	MulF16			;-CxSySz
		add	r7,r0,ip		;Store -CxSySz-SxCz in Sy register

		mov	r0,Sx
		bl	MulF16
		mov	ip,r0			;Store SxSz
		mov	r1,Cz
		mov	r0,r3
		bl	MulF16
		add	r3,r0,ip		;Store -CxSyCz+sx+sz in Cz register

		mov	r0,Cx
		mov	r1,Cy
		bl	MulF16
		mov	r9,r0
		stmia	r10!,{r3,r7,r9}		;Store bottom row.

		EXIT	r4-r10



;---------------------------------------------
;BuildXZY(Frac16 XAngle,Frac16 YAngle,Frac16 ZAangle,Mat33f16 *Matrix)
;  The resultant matrix is this.  ie.  Rx * Rz * Ry
;
; |  CyCz          Sz     SyCz         |
; | -CxCySz-SxSy   CxCz  -CxSySz+SxCy  |
; |  SxCySz-CxSy  -SxCz   SxSySz+CxCy  |
; 
;---------------------------------------------

BuildXZY	ENTER	r4-r10
BuildXZYjmp
		mov	r9,r3
		mov	r7,r1			;r7 - Y rotation
		mov	r8,r2			;r8 - Z rotation.
		bl	CosSin
		mov	Cx,r0			;r4 = Cos(X)
		mov	Sx,r1			;r5 = Sin(X)
		mov	r0,r7
		bl	CosSin
		mov	Cy,r0			;r6 = Cos(Y)
		mov	Sy,r1			;r7 = Sin(Y)
		mov	r0,r8
		bl	CosSin
		mov	r10,r9			
		mov	Cz,r0			;r8 = Cos(Z)
		mov	Sz,r1			;r9 = Sin(Z)

		mov	r1,Sy
		bl	MulF16
		mov	ip,r0			;SyCz

		mov	r1,Cy
		mov	r0,Cz
		bl	MulF16			;CyCz
		stmia	r10!,{r0,Sz,ip}

		rsb	r0,Cx,#0
		mov	r1,Sz
		bl	MulF16
		mov	r3,r0			;-CxCz
		mov	r1,Cy
		bl	MulF16
		mov	ip,r0
		mov	r1,Sx
		rsb	r0,Sy,#0
		bl	MulF16
		add	r0,r0,ip
		stmia	r10!,{r0}

		mov	r0,Cy
		bl	MulF16
		mov	ip,r0
		mov	r0,r3
		mov	r1,Sy
		bl	MulF16
		add	ip,ip,r0

		mov	r1,Cx
		mov	r0,Cz
		bl	MulF16
		stmia	r10!,{r0,ip}

		mov	r0,Cy
		bl	MulF16
		mov	r3,r0
		mov	r0,Sx
		mov	r1,Sz
		bl	MulF16
		mov	ip,r0			;SxSz
		mov	r1,Sy
		bl	MulF16
		add	Sz,r0,r3		;Sz = SxSySz+CxCy

		rsb	r0,Sx,#0
		mov	r1,Cz
		bl	MulF16			; -SxCz
		mov	r3,r0

		mov	r0,Cy
		mov	r1,ip
		bl	MulF16
		mov	ip,r0
		rsb	r0,Cx,#0
		mov	r1,Sy
		bl	MulF16
		add	r0,r0,ip

		stmia	r10!,{r0,r3,Sz}
		

		EXIT	r4-r10	



;-------------------------------------------
;BuildYXZ(Frac16 XAngle,Frac16 YAngle,Frac16 ZAngle,Mat33f16 *Matrix)
;    The resultant matrix is this.  ie Ry * Rx * Rz
;
;   |  CyCz+SxSySz     CySz-SxSyCz     CxSy  |
;   | -CxSz            CxCz            Sx    |
;   | -SyCz+SxCySz    -SySz-SxCyCz     CxCy  |
;
;-------------------------------------------


BuildYXZ	ENTER	r4-r10
BuildYXZjmp		
		mov	r9,r3
		mov	r7,r1			;r7 - Y rotation
		mov	r8,r2			;r8 - Z rotation.
		bl	CosSin
		mov	Cx,r0			;r4 = Cos(X)
		mov	Sx,r1			;r5 = Sin(X)
		mov	r0,r7
		bl	CosSin
		mov	Cy,r0			;r6 = Cos(Y)
		mov	Sy,r1			;r7 = Sin(Y)
		mov	r0,r8
		bl	CosSin
		mov	r10,r9			
		mov	Cz,r0			;r8 = Cos(Z)
		mov	Sz,r1			;r9 = Sin(Z)

		mov	r1,Cy
		bl	MulF16			;Cy*Cz
		mov	ip,r0
		mov	r0,Sx			;Sx
		mov	r1,Sy			;Sy
		bl	MulF16
		mov	r3,r0			;Sx*Sy
		mov	r1,Sz			;Sz	
		bl	MulF16
		add	r0,r0,ip		;SxSySz+CyCz
		stmia	r10!,{r0}
		
		mov	r0,r3			;SxSy
		mov	r1,Cz			;Cz
		bl	MulF16
		mov	r3,r0			;SxSyCz
		mov	r0,Cy			;Cy
		mov	r1,Sz			;Sz
		bl	MulF16
		sub	r0,r0,r3		;CySz-SxSyCz
		stmia	r10!,{r0}
		
		mov	r1,Cx			;Cx
		mov	r0,Sy			;Sy
		bl	MulF16
		stmia	r10!,{r0}

;;;;		mov	r1,r4			;Cx
		mov	r0,Cz			;Cz
		bl	MulF16			;CxCz
		mov	r3,r0
		
		rsb	r0,Sz,#0		;-Sz
		bl	MulF16			;-CxSz
		stmia	r10!,{r0,r3,r5}		;Stores  -CxSz  CxCz  Sx
		
		mov	r0,Sx			;Sx
		mov	r1,Cy			;Cy
		bl	MulF16
		mov	r3,r0			;SxCy
		mov	r1,Sz			;Sz
		bl	MulF16
		mov	ip,r0			;SxCySz
		mov	r0,Cz			;Cz
		rsb	r1,Sy,#0		;-Sy
		bl	MulF16
		add	r0,r0,ip		;-SyCz+SxCySz
		mov	r5,r0

		mov	r0,Cx			;Cx
		mov	r1,Cy			;Cy
		bl	MulF16
		
		rsb	r1,Sy,#0
		mov	r7,r0			;CxSy	(3x3 term)

		mov	r0,Sz			;Sz
		bl	MulF16			;-SySz
		mov	ip,r0
		mov	r0,r3			;SxCy
		rsb	r1,Cz,#0		;-Cz
		bl	MulF16			;-SxCyCz
		add	r6,r0,ip		;-SySz-SxCyCz
		stmia	r10!,{r5-r7}		;Stores -SyCz+SxCySz  -SySz-SxCyCz  CxSy
		
		EXIT	r4-r10


;-------------------------------------------
;BuildYZX(Frac16 XAngle,Frac16 YAngle,Frac16 ZAngle,Mat33f16 *Matrix)
;    The resultant matrix is this.  ie Ry * Rz * Rx
;
;   |  CyCz   CxCySz-SxSy    SxCySz+CxSy  |
;   | -Sz     CxCz           SxCz         |
;   | -SyCz  -CxSySz        -SxSySz+CxCy  |
;
;-------------------------------------------


BuildYZX	ENTER	r4-r10
BuildYZXjmp
		mov	r9,r3
		mov	r7,r1			;r7 - Y rotation
		mov	r8,r2			;r8 - Z rotation.
		bl	CosSin
		mov	Cx,r0			;r4 = Cos(X)
		mov	Sx,r1			;r5 = Sin(X)
		mov	r0,r7
		bl	CosSin
		mov	Cy,r0			;r6 = Cos(Y)
		mov	Sy,r1			;r7 = Sin(Y)
		mov	r0,r8
		bl	CosSin
		mov	r10,r9			
		mov	Cz,r0			;r8 = Cos(Z)
		mov	Sz,r1			;r9 = Sin(Z)

		mov	r0,Cy
		bl	MulF16
		mov	ip,r0
		mov	r1,Cx
		bl	MulF16
		mov	r3,r0
		rsb	r0,Sx,#0
		mov	r1,Sy
		bl	MulF16
		add	r3,r3,r0

		mov	r0,Cy
		mov	r1,Cz
		bl	MulF16
		stmia	r10!,{r0,r3}

		mov	r0,ip			;CySz
		mov	r1,Sx
		bl	MulF16
		mov	r3,r0
		mov	r0,Cx
		mov	r1,Sy
		bl	MulF16
		add	r0,r0,r3
		rsb	r1,Sz,#0
		stmia	r10!,{r0,r1}

		mov	r1,Cz
		rsb	r0,Sy,#0
		bl	MulF16
		mov	ip,r0

		mov	r0,Sx
		bl	MulF16
		mov	r3,r0
		
		mov	r0,Cx
		bl	MulF16
		stmia	r10!,{r0,r3,ip}

		mov	r0,Sz
		rsb	r1,Sy,#0
		bl	MulF16
		mov	ip,r0
		mov	r1,Sx
		bl	MulF16
		mov	r3,r0
		mov	r0,Cx
		mov	r1,Cy
		bl	MulF16
		add	Sz,r0,r3

		rsb	r0,Sx,#0
		bl	MulF16
		mov	r3,r0
		mov	r0,ip
		mov	r1,Cx
		bl	MulF16
		add	r0,r0,r3
		stmia	r10!,{r0,Sz}
		
		EXIT	r4-r10
			

;-------------------------------------------
;BuildZXY(Frac16 XAngle,Frac16 YAngle,Frac16 ZAngle,Mat33f16 *Matrix)
;    The resultant matrix is this.  ie Rz * Rx * Ry
;
;   |  CyCz-SxSySz    CxSz    SyCz+SxCySz  |  
;   | -CySz-SxSyCz    CxCz   -SySz+SxSyCz  |
;   | -CxSy          -Sx      CxCy         |
;
;-------------------------------------------

BuildZXY	ENTER	r4-r10
BuildZXYjmp
		mov	r9,r3
		mov	r7,r1			;r7 - Y rotation
		mov	r8,r2			;r8 - Z rotation.
		bl	CosSin
		mov	Cx,r0			;r4 = Cos(X)
		mov	Sx,r1			;r5 = Sin(X)
		mov	r0,r7
		bl	CosSin
		mov	Cy,r0			;r6 = Cos(Y)
		mov	Sy,r1			;r7 = Sin(Y)
		mov	r0,r8
		bl	CosSin
		mov	r10,r9	
		mov	Cz,r0			;r8 = Cos(Z)
		mov	Sz,r1			;r9 = Sin(Z)

		mov	r1,Cy
		bl	MulF16
		mov	ip,r0
		mov	r0,Sx
		mov	r1,Sz
		bl	MulF16
		mov	r3,r0
		rsb	r1,Sy,#0
		bl	MulF16
		add	r0,r0,ip
		stmia	r10!,{r0}

		mov	r0,Cx
		mov	r1,Sz
		bl	MulF16
		stmia	r10!,{r0}

		mov	r0,r3
		mov	r1,Cy
		bl	MulF16
		mov	r3,r0
		mov	r1,Cz
		mov	r0,Sy
		bl	MulF16
		add	r0,r0,r3
		stmia	r10!,{r0}

		mov	r0,Sx
		bl	MulF16
		mov	r3,r0			;SxCz
		mov	r1,Cy
		bl	MulF16			;SxCyCz
		mov	ip,r0
		rsb	r0,Sy,#0
		mov	r1,Sz
		bl	MulF16
		add	ip,ip,r0		;-SySz+SxCyCz

		mov	r0,Cx
		mov	r1,Cz
		bl	MulF16
		mov	Cz,r0

		mov	r0,r3
		rsb	r1,Sy,#0
		bl	MulF16
		mov	r3,r0
		mov	r1,Sz
		rsb	r0,Cy,#0
		bl	MulF16
		add	r0,r0,r3
		stmia	r10!,{r0,Cz,ip}			;Stores second row

		mov	r1,Cx
		mov	r0,Cy
		bl	MulF16
		mov	Sz,r0

		rsb	Cz,Sx,#0
		rsb	r0,Sy,#0
		bl	MulF16
		stmia	r10!,{r0,Cz,Sz}			;Stores last row.

		EXIT	r4-r10

;-------------------------------------------
;BuildZYX(Frac16 XAngle,Frac16 YAngle,Frac16 ZAngle,Mat33f16 *Matrix)
;    The resultant matrix is this.  ie Rz * Ry * Rx
;
;   |   CyCz    CxSz-SxSyCz    SxSz+CxSyCz   | 
;   |  -CySz    CxCz+SxSySz    SxCz-CxSySz   |
;   |  -Sy     -SxCy           CxCy          |
;
;-------------------------------------------
BuildZYX	ENTER	r4-r10
BuildZYXjmp
		mov	r9,r3
		mov	r7,r1			;r7 - Y rotation
		mov	r8,r2			;r8 - Z rotation.
		bl	CosSin
		mov	Cx,r0			;r4 = Cos(X)
		mov	Sx,r1			;r5 = Sin(X)
		mov	r0,r7
		bl	CosSin
		mov	Cy,r0			;r6 = Cos(Y)
		mov	Sy,r1			;r7 = Sin(Y)
		mov	r0,r8
		bl	CosSin
		mov	r10,r9	
		mov	Cz,r0			;r8 = Cos(Z)
		mov	Sz,r1			;r9 = Sin(Z)

		mov	r1,Cy
		bl	MulF16
		stmia	r10!,{r0}

		mov	r0,Sy
		mov	r1,Cz
		bl	MulF16
		mov	ip,r0
		rsb	r1,Sx,#0
		bl	MulF16
		mov	r3,r0
		mov	r1,Cx
		mov	r0,Sz
		bl	MulF16
		add	r3,r3,r0

		mov	r0,ip
		bl	MulF16
		mov	ip,r0
		mov	r0,Sx
		mov	r1,Sz
		bl	MulF16
		add	ip,ip,r0
		stmia	r10!,{r3,ip}

		rsb	r0,Cy,#0
		bl	MulF16
		
		rsb	r1,Sz,#0
		mov	r3,r0			;Keep the [1][2] in Sz

		mov	r0,Sy
		bl	MulF16
		mov	ip,r0			;-SySz
		mov	r1,Cx
		bl	MulF16
		mov	Sz,r0
		mov	r1,Sx
		mov	r0,Cz
		bl	MulF16
		add	Sz,Sz,r0		;Paramater [3][2] in Sz

		mov	r0,ip
		bl	MulF16
		mov	ip,r0
		mov	r1,Cz
		mov	r0,Cx
		bl	MulF16
		sub	Cz,r0,ip		;Paramater [2][2]				

		stmia	r10!,{r3,Cz,Sz}		;Second row.

		mov	r0,Cx
		mov	r1,Cy
		bl	MulF16
		mov	ip,r0

		mov	r0,Sx
		rsb	r1,Cy,#0
		bl	MulF16
		
		mov	r2,r0
		rsb	r1,Sy,#0
		stmia	r10!,{r1,r2,ip}

		EXIT	r4-r10

;---------------------------------------------------
;
;MulF16(Frac16 r0,Frac16 r1) 
;     is a specially optimized stub for the 3DFolio that multiplies
;     two 16.16 numbers r0 & r1 where  
;		 -1 <= r0 => 1   and,
;		 -1 <= r1 => 1
;   R0 returns the answer, r2 is destroyed, all other registers are 
;   preserved.  [Bad Coding note:  MANY EXIT POINTS]
;
;   Note:  A faster but less precise way, would be to multiply as 17.15 
;          numbers and avoiding all these sign problems by having the 
;          sign bit in the 16th bit.
;
;---------------------------------------------------
		
MulF16		
		movs	r2,r0,lsl #16		;Test lower 16 bits of r0
		beq	Op0Is0			;R0 is either -1, 0 or 1.

		movs	r2,r1,lsl #16		;Test lower 16 bits of r1
		beq	Op1Is0			;R1 is either -1, 0 or 1.
		
		eors	r2,r1,r0		;Get the sign bits.
		mul	r0,r1,r0		;Get the multiply result.
		mov	r0,r0,lsr #16		;Decimal point align
		mov	r2,r2,lsr #16		;Clear out the lower bits of r2
		
		orr	r0,r0,r2,lsl #16	;Fix the sign of the answer.
	 	mov	pc,lk


Op0Is0		movs	r0,r0
		moveq	pc,lk			;if r0=0 then return 0.
		mov	r0,r1			;Answer is +/- r1
		rsbmi	r0,r0,#0		;Correct the sign
		mov	pc,lk			
			
Op1Is0		cmp	r1,#0
		moveq	r0,#0			;if r1=0 then return 0
		moveq	pc,lk
		
		rsbmi	r0,r0,#0		;Otherwise answer is +/- r0
		mov	pc,lk
		
		
		END
		
