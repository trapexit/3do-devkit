 IF :DEF:|_ITEM_I|
 ELSE
	GBLL	|_ITEM_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: item.i,v 1.6 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel item management definitions
;*
;*****************************************************************************

KERNELSWI	EQU	0x10000

SEM_WAIT	EQU	1
SEM_SHAREDRED	EQU	2

TAG_JUMP	EQU	254
TAG_NOP		EQU	255
TAG_END		EQU	0

TAG_ITEM_END		EQU	TAG_END			 ; 0
TAG_ITEM_NAME		EQU	1+TAG_END		 ; 1
TAG_ITEM_PRI		EQU	1+TAG_ITEM_NAME		 ; 2
TAG_ITEM_VERSION	EQU	1+TAG_ITEM_PRI		 ; 3
TAG_ITEM_REVISION	EQU	1+TAG_ITEM_VERSION	 ; 4
TAG_ITEM_CONSTANT_NAME	EQU	1+TAG_ITEM_REVISION	 ; 5
TAG_ITEM_UNIQUE_NAME	EQU	1+TAG_ITEM_CONSTANT_NAME ; 6
TAG_ITEM_RESERVED7	EQU	1+TAG_ITEM_UNIQUE_NAME	 ; 7
TAG_ITEM_RESERVED8	EQU	1+TAG_ITEM_RESERVED7	 ; 8
TAG_ITEM_RESERVED9	EQU	1+TAG_ITEM_RESERVED8	 ; 9
TAG_ITEM_LAST EQU	TAG_ITEM_RESERVED9	; 9

DEMANDLOAD_MAIN_CREATE	EQU	-1
DEMANDLOAD_MAIN_DELETE	EQU	-2

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

	MACRO
	LockItem
	swi	KERNELSWI+7
	MEND

	MACRO
	UnlockItem
	swi	KERNELSWI+6
	MEND

	MACRO
	FindAndOpenItem
	swi	KERNELSWI+36
	MEND

 ENDIF	; |_ITEMS_I|

	END
