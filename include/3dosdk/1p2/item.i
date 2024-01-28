 IF :DEF:|_ITEM_I|
 ELSE
	GBLL	|_ITEM_I|

;*****
; $Id: item.i,v 1.3 1993/08/09 18:37:34 andy Exp $
; $Log: item.i,v $
;Revision 1.3  1993/08/09  18:37:34  andy
;added TAG_NOP and TAG_JUMP
;
;Revision 1.2  1993/03/16  07:24:59  dale
;fix comment leader
;
;Revision 1.1  1993/03/16  06:43:48  dale
;Initial revision
;
;Revision 1.4  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.3  1992/12/17  22:25:37  dale
;added macros for swi functions
;
;Revision 1.2  1992/10/30  01:26:48  dale
;added Id and Log
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

KERNELSWI	EQU	0x10000

	MACRO
	CreateSizedItem
	swi	KERNELSWI+0
	MEND

	MACRO
	DeleteItem
	swi	KERNELSWI+3
	MEND

	MACRO
	FindItem
	swi	KERNELSWI+4
	MEND

	MACRO
	CloseItem
	swi	KERNELSWI+8
	MEND

	MACRO
	OpenItem
	swi	KERNELSWI+5
	MEND

	MACRO
	SetItemPri
	swi	KERNELSWI+10
	MEND

	MACRO
	SetItemOwner
	swi	KERNELSWI+28
	MEND

TAG_JUMP	EQU	254
TAG_NOP		EQU	255
TAG_END		EQU	0

TAG_ITEM_END	EQU	TAG_END		; 0
TAG_ITEM_NAME	EQU	1+TAG_END	; 1
TAG_ITEM_PRI	EQU	1+TAG_ITEM_NAME	; 2
TAG_ITEM_RESERVED3 EQU	1+TAG_ITEM_PRI	; 3
TAG_ITEM_RESERVED4 EQU	1+TAG_ITEM_RESERVED3	; 4
TAG_ITEM_RESERVED5 EQU	1+TAG_ITEM_RESERVED4	; 5
TAG_ITEM_RESERVED6 EQU	1+TAG_ITEM_RESERVED5	; 6
TAG_ITEM_RESERVED7 EQU	1+TAG_ITEM_RESERVED6	; 7
TAG_ITEM_RESERVED8 EQU	1+TAG_ITEM_RESERVED7	; 8
TAG_ITEM_RESERVED9 EQU	1+TAG_ITEM_RESERVED8	; 9
TAG_ITEM_LAST EQU	TAG_ITEM_RESERVED9	; 9

 ENDIF	; |_ITEMS_I|

	END
