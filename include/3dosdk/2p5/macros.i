 IF :DEF:|__MACROS_I|
 ELSE
	GBLL	|__MACROS_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: macros.i,v 1.5 1994/11/23 19:28:24 vertex Exp $
;*
;*  Macros
;*
;*****************************************************************************

		MACRO
$label		WORD	$value
$label		DCD	$value
		MEND


		MACRO
$label		NOP
$label		MOV	R0, R0
		MEND


		MACRO
$label		RTS
$label		MOV	PC, LK
		MEND


		MACRO
$label		RTI
$label		SUBS	PC, LK, #4
		MEND


		MACRO
$label		POPRTS
$label		LDMFD	SP!, {PC}
		MEND


		MACRO
$label		PROC
		EXPORT	$label
		ALIGN
$label		ROUT
		MEND


; The LIBCALL macro checks to see if the named routine has a SWI vector
; associated with it, and if so drops an inline SWI instruction in the
; code.  If there is no SWI vector associated with the routine, then
; LIBCALL will IMPORT the name (if it has not already done so), and
; insert a call to the routine (using the BL instruction).
		MACRO
$label		LIBCALL	$name,$condition
		LCLS	_TEMPNAME_
		LCLS	_SWINAME_
		LCLS	_REFNAME_
		LCLS	_SWIVALNAME_
		LCLS	_OPCODE_
$label
_TEMPNAME_	SETS	"$name"
_SWINAME_	SETS	"_SWI_":CC:"$name"
_REFNAME_	SETS	"_REF_":CC:"$name"
	IF :DEF:$_SWINAME_
_OPCODE_	SETS	"SWI":CC:"$condition"
_SWIVALNAME_	SETS	"_SWIVAL_":CC:"$name"
		$_OPCODE_	$_SWIVALNAME_
	ELSE
	  IF :DEF:$_REFNAME_
	  ELSE
		GBLL	$_REFNAME_
		IMPORT	$_TEMPNAME_
	  ENDIF
_OPCODE_	SETS	"BL":CC:"$condition"
		$_OPCODE_	$_TEMPNAME_
	ENDIF
		MEND


; The SETSWI macro is used in preparation for the LIBCALL macro to
; generate an inline SWI call to the specified routine, passing the
; $value argument along to the SWI call.
		MACRO
		SETSWI	$name,$value
		LCLS	_TEMPNAME_
		LCLS	_SWINAME_
		LCLS	_SWIVALNAME_
_TEMPNAME_	SETS	"$name"
_SWINAME_	SETS	"_SWI_":CC:"$name"
		GBLL	$_SWINAME_
_SWIVALNAME_	SETS	"_SWIVAL_":CC:"$name"
$_SWIVALNAME_	EQU	$value
		MEND


 ENDIF

		END
