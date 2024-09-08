 IF :DEF:|_LIST_I|
 ELSE
	GBLL	|_LIST_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: list.i,v 1.5 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel list management definitions
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE nodes.i
	INCLUDE item.i

	BEGINSTRUCT	List
		STRUCT	Node,l_n
		PTR	l_head_flink
		PTR	l_filler
		PTR	l_tail_blink
	ENDSTRUCT

l_head_blink	EQU l_filler
l_tail_flink	EQU l_filler
l_Head		EQU l_head_flink
l_Tail		EQU l_tail_blink
l_Middle	EQU l_filler

 ENDIF	; |_LIST_I|

	END
