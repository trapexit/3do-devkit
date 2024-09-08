 IF :DEF:|__structs_i|
 ELSE
	GBLL	|__structs_i|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: structs.i,v 1.7 1994/09/10 02:00:43 vertex Exp $
;*
;*  Standard Portfolio assembly macros for structure object declaration
;*  and structure size calculation.
;*
;*****************************************************************************

; To declare a structure type:
;		BEGINSTRUCT	<name>
;		<member allocation macros>
;		ENDSTRUCT
;
; Structure declarations cannot be nested.
; After a structure type is defined, two symbols will be created.  The symbols
; _SIZEOF_<name> and _ALIGNOF_<name> will be assigned the size and alignment
; requirements of the structure.  If you need to reference these symbols,
; remember that you must quote them with vertical bars for the assembler to
; recognize them (as with any symbol with non-alphanumeric characters).
;
; Most member allocation macros are of the form:
;		<macro>	[<membername>]
; Where <macro> is one of:  BYTE, UBYTE, SHORT, USHORT, LONG, ULONG or PTR.
; The symbol <membername> will be assigned the value of the offset from the
; beginning of the structure, and the structure allocation pointer will be
; updated.  If <membername> is not supplied, then space will be reserved in
; the structure, but no symbol will be created.
;
; The member allocation macro:
;		STRUCT	<type>[,<membername>]
; allocates space for an object of a previously declared structure of type
; <type>.  If membername is supplied, a symbol will be created with the value
; of the offset to the start of the reserved space.
;
; The member allocation macro:
;		ARRAY	<type>,[<membername>],<count>
; allocates space for an array of objects of type <type>.  The type <type> can
; can be that of any previously declared structure, or one of BYTE, UBYTE,
; SHORT, USHORT, LONG, ULONG or PTR.
;
; Member allocation macros can also be used outside of a structure declaration
; to name and reserve space for an object of a pre-defined type.
;
; The macro:
;		TYPEDEF	<type>,<name>
; makes <name> equivalent to previously defined <type> for use in the STRUCT
; macro.



; Declare some internal variables to be used during assembly

		GBLL	|_STRUCTOPEN_|
		GBLA	|_STRUCTPC_|
		GBLS	|_STRUCTNAME_|
		GBLA	|_STRUCTALIGN_|
		GBLS	|_BAR_|
		GBLS	|_SIZEOF_|
		GBLS	|_ALIGNOF_|

; Give the assembly variables some initial values

|_STRUCTOPEN_|	SETL	{FALSE}
|_STRUCTPC_|	SETA	0
|_STRUCTNAME_|	SETS	""
|_STRUCTALIGN_|	SETA	1
|_BAR_|		SETS	"|"
|_SIZEOF_|	SETS	"_SIZEOF_"
|_ALIGNOF_|	SETS	"_ALIGNOF_"

; Define sizeof and alignof values for array declarations

|_SIZEOF_BYTE|		EQU	1
|_SIZEOF_UBYTE|		EQU	1
|_SIZEOF_INT8|		EQU	1
|_SIZEOF_UINT8|		EQU	1
|_SIZEOF_SHORT|		EQU	2
|_SIZEOF_USHORT|	EQU	2
|_SIZEOF_LONG|		EQU	4
|_SIZEOF_ULONG|		EQU	4
|_SIZEOF_INT32|		EQU	4
|_SIZEOF_UINT32|	EQU	4
|_SIZEOF_PTR|		EQU	4
|_SIZEOF_ITEM|		EQU	4

|_ALIGNOF_BYTE|		EQU	1
|_ALIGNOF_UBYTE|	EQU	1
|_ALIGNOF_INT8|		EQU	1
|_ALIGNOF_UINT8|	EQU	1
|_ALIGNOF_SHORT|	EQU	2
|_ALIGNOF_USHORT|	EQU	2
|_ALIGNOF_LONG|		EQU	4
|_ALIGNOF_ULONG|	EQU	4
|_ALIGNOF_INT32|	EQU	4
|_ALIGNOF_UINT32|	EQU	4
|_ALIGNOF_PTR|		EQU	4
|_ALIGNOF_ITEM|		EQU	4

; Macro to begin a structure definition

		MACRO
		BEGINSTRUCT	$name
	IF |_STRUCTOPEN_|
		ERROR - Nested structure declarations
	ELSE
|_STRUCTNAME_|	SETS	"$name"
	  IF |_STRUCTNAME_|:LEFT:1="|"
|_STRUCTNAME_|	SETS	|_STRUCTNAME_|:LEFT:[:LEN:|_STRUCTNAME_|-1]
|_STRUCTNAME_|	SETS	|_STRUCTNAME_|:RIGHT:[:LEN:|_STRUCTNAME_|-1]
	  ENDIF
|_STRUCTPC_|	SETA	0
|_STRUCTALIGN_|	SETA	4
|_STRUCTOPEN_|	SETL	{TRUE}
	ENDIF
		MEND


; Internal macro used for all structure member allocation

		MACRO
		STRUCTRESERVE	$name,$size,$align
	IF |_STRUCTOPEN_|
|_STRUCTPC_|	SETA	(|_STRUCTPC_|+$align-1):AND:(-$align)
	  IF "$name"<>""
$name		EQU	|_STRUCTPC_|
	  ENDIF
|_STRUCTPC_|	SETA	|_STRUCTPC_|+$size
	  IF |_STRUCTALIGN_|<$align
|_STRUCTALIGN_|	SETA	$align
	  ENDIF
	ELSE
		ALIGN	$align
$name		%	$size
	ENDIF
		MEND


; Some predefined space allocation macros for structure building


		MACRO
		BYTE	$name
		STRUCTRESERVE	$name,1,1
		MEND

		MACRO
		INT8	$name
		STRUCTRESERVE	$name,1,1
		MEND

		MACRO
		UBYTE	$name
		STRUCTRESERVE	$name,1,1
		MEND

		MACRO
		UINT8	$name
		STRUCTRESERVE	$name,1,1
		MEND

		MACRO
		SHORT	$name
		STRUCTRESERVE	$name,2,2
		MEND


		MACRO
		USHORT	$name
		STRUCTRESERVE	$name,2,2
		MEND


		MACRO
		LONG	$name
		STRUCTRESERVE	$name,4,4
		MEND


		MACRO
		ULONG	$name
		STRUCTRESERVE	$name,4,4
		MEND


		MACRO
		INT32	$name
		STRUCTRESERVE	$name,4,4
		MEND


		MACRO
		UINT32	$name
		STRUCTRESERVE	$name,4,4
		MEND

		MACRO
		PTR	$name
		STRUCTRESERVE	$name,4,4
		MEND

		MACRO
		ITEM	$name
		STRUCTRESERVE	$name,4,4
		MEND

		MACRO
		STRUCT	$type,$name
		LCLS	|_TEMPNAME_|
		LCLS	|_TEMPSIZE_|
		LCLS	|_TEMPALIGN_|
|_TEMPNAME_|	SETS	"$type"
	  IF |_TEMPNAME_|:LEFT:1="|"
|_TEMPNAME_|	SETS	|_TEMPNAME_|:LEFT:[:LEN:|_TEMPNAME_|-1]
|_TEMPNAME_|	SETS	|_TEMPNAME_|:RIGHT:[:LEN:|_TEMPNAME_|-1]
	  ENDIF
|_TEMPSIZE_|	SETS	"|_SIZEOF_":CC:|_TEMPNAME_|:CC:"|"
|_TEMPALIGN_|	SETS	"|_ALIGNOF_":CC:|_TEMPNAME_|:CC:"|"
		STRUCTRESERVE	$name,$|_TEMPSIZE_|,$|_TEMPALIGN_|
		MEND


		MACRO
		ARRAY	$type,$name,$count
		LCLS	|_TEMPNAME_|
		LCLS	|_TEMPSIZE_|
		LCLS	|_TEMPALIGN_|
		LCLA	|_TEMPSPACE|
|_TEMPNAME_|	SETS	"$type"
	  IF |_TEMPNAME_|:LEFT:1="|"
|_TEMPNAME_|	SETS	|_TEMPNAME_|:LEFT:[:LEN:|_TEMPNAME_|-1]
|_TEMPNAME_|	SETS	|_TEMPNAME_|:RIGHT:[:LEN:|_TEMPNAME_|-1]
	  ENDIF
|_TEMPSIZE_|	SETS	"|_SIZEOF_":CC:|_TEMPNAME_|:CC:"|"
|_TEMPALIGN_|	SETS	"|_ALIGNOF_":CC:|_TEMPNAME_|:CC:"|"
		STRUCTRESERVE	$name,$|_TEMPSIZE_|*$count,$|_TEMPALIGN_|
		MEND



; Macro to close off a structure definition

		MACRO
		ENDSTRUCT
	IF |_STRUCTOPEN_|
		STRUCTRESERVE "",0,|_STRUCTALIGN_|
$|_BAR_|.$|_SIZEOF_|.$|_STRUCTNAME_|.$|_BAR_|	EQU |_STRUCTPC_|
$|_BAR_|.$|_ALIGNOF_|.$|_STRUCTNAME_|.$|_BAR_|	EQU |_STRUCTALIGN_|
|_STRUCTOPEN_|	SETL	{FALSE}
	ELSE
		ERROR - ENDSTRUCT Macro used with no open structure
	ENDIF
		MEND


; Macro to create an alias to a previously defined type

		MACRO
		TYPEDEF	$type,$name
		LCLS	|_TEMPNAME_|
		LCLS	|_TEMPSIZE_|
		LCLS	|_TEMPALIGN_|
		LCLS	|_TEMPNAME2_|
		LCLS	|_TEMPSIZE2_|
		LCLS	|_TEMPALIGN2_|
|_TEMPNAME_|	SETS	"$type"
	  IF |_TEMPNAME_|:LEFT:1="|"
|_TEMPNAME_|	SETS	|_TEMPNAME_|:LEFT:[:LEN:|_TEMPNAME_|-1]
|_TEMPNAME_|	SETS	|_TEMPNAME_|:RIGHT:[:LEN:|_TEMPNAME_|-1]
	  ENDIF
|_TEMPSIZE_|	SETS	"|_SIZEOF_":CC:|_TEMPNAME_|:CC:"|"
|_TEMPALIGN_|	SETS	"|_ALIGNOF_":CC:|_TEMPNAME_|:CC:"|"
|_TEMPNAME2_|	SETS	"$name"
	  IF |_TEMPNAME2_|:LEFT:1="|"
|_TEMPNAME2_|	SETS	|_TEMPNAME2_|:LEFT:[:LEN:|_TEMPNAME2_|-1]
|_TEMPNAME2_|	SETS	|_TEMPNAME2_|:RIGHT:[:LEN:|_TEMPNAME2_|-1]
	  ENDIF
|_TEMPSIZE2_|	SETS	"|_SIZEOF_":CC:|_TEMPNAME2_|:CC:"|"
|_TEMPALIGN2_|	SETS	"|_ALIGNOF_":CC:|_TEMPNAME2_|:CC:"|"
$|_TEMPSIZE2_|	EQU	$|_TEMPSIZE_|
$|_TEMPALIGN2_|	EQU	$|_TEMPALIGN_|
		MEND



 ENDIF

		END


