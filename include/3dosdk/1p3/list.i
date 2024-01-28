 IF :DEF:|_LIST_I|
 ELSE
	GBLL	|_LIST_I|

;*****

; $Id: list.i,v 1.3 1994/01/28 23:20:19 sdas Exp $

; $Log: list.i,v $
;Revision 1.3  1994/01/28  23:20:19  sdas
;Include Files Self-consistency - includes dependant include files
;
;Revision 1.2  1993/03/16  07:25:34  dale
;fix comment leader
;
;Revision 1.1  1993/03/16  06:43:34  dale
;Initial revision
;
;Revision 1.6  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.5  1992/10/29  01:32:50  dale
;cleanup
;
;Revision 1.4  1992/10/24  01:41:07  dale
;rcs
;
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE	structs.i
	INCLUDE nodes.i
	INCLUDE item.i

	BEGINSTRUCT	List
		STRUCT	Node,l_n
		STRUCT	PTR,l_head_flink
		STRUCT	PTR,l_filler
		STRUCT	PTR,l_tail_blink
	ENDSTRUCT

l_head_blink	EQU l_filler
l_tail_flink	EQU l_filler
l_Head		EQU l_head_flink
l_Tail		EQU l_tail_blink
l_Middle	EQU l_filler

 ENDIF	; |_LISTS_I|

	END
