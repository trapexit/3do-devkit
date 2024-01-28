;-----------------------------------------------------
;
;  math_3d.s : 3D math and matrix assembly routines
;
;  Written by Dan Duncalf
;
;  Copyright 1993, The 3DO Company
;  All Rights Reserved
;  This document is proprietary and confidential
;
;-----------------------------------------------------

DEBUG	EQU	1

		INCLUDE	macros.i
		INCLUDE	structs.i
		INCLUDE	nodes.i
		INCLUDE	mem.i
		INCLUDE	folio.i
		INCLUDE	kernelmacros.i
		INCLUDE	kernel.i
		INCLUDE	registers.i
		INCLUDE	task.i
		INCLUDE stack.i
		INCLUDE operamath.i

		AREA	CODE_AREA,CODE

		INCLUDE userdefs3D.i
		INCLUDE 3dlib.i
		INCLUDE	math.s
		INCLUDE	viewmatrix.s

		END
		