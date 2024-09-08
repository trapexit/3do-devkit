 IF :DEF:|_NODES_I|
 ELSE
	GBLL	|_NODES_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: nodes.i,v 1.14 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel node definitions
;*
;*****************************************************************************

	INCLUDE	structs.i

NST_KERNEL	EQU	1
NST_GRAPHICS	EQU	2
NST_FILESYS	EQU	3
NST_AUDIO	EQU	4
NST_MATH	EQU	5
NST_NETWORK	EQU	6
NST_JETSTREAM	EQU	7
NST_AV		EQU	8
NST_INTL	EQU	9
NST_CONNECTION	EQU	10
NST_SECURITY	EQU	15

	BEGINSTRUCT	Node
		PTR	N_Next
		PTR	N_Prev
		UINT8	N_SubsysType
		UINT8	N_Type
		UINT8	N_Priority
		UINT8	N_Flags
		INT32	N_Size
		PTR	N_Name
	ENDSTRUCT

	BEGINSTRUCT	NamelessNode
		PTR	NN_Next
		PTR	NN_Prev
		UINT8	NN_SubsysType
		UINT8	NN_Type
		UINT8	NN_Priority
		UINT8	NN_Flags
		INT32	NN_Size
	ENDSTRUCT

	BEGINSTRUCT	ItemNode
		STRUCT	Node,N_dummy
		UINT8	N_Version
		UINT8	N_Revision
		UINT8	N_FolioFlags
		UINT8	N_ItemFlags
		ITEM	N_Item
		ITEM	N_Owner
		PTR	N_ReservedP
	ENDSTRUCT

NODE_RSRV1	EQU 64
NODE_SIZELOCKED	EQU 32
NODE_ITEMVALID	EQU 16
NODE_NAMEVALID	EQU 128

ITEMNODE_NOTREADY	EQU 128
ITEMNODE_CONSTANT_NAME	EQU 64
ITEMNODE_PRIVILEGED	EQU 32
ITEMNODE_UNIQUE_NAME	EQU 16

FF_DEBUG1	EQU 1
FF_DEBUG2	EQU 2

	BEGINSTRUCT	MinNode
		PTR	NNN_Next
		PTR	NNN_Prev
	ENDSTRUCT

 ENDIF ; |_NODES_I|

	END
