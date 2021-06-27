 IF :DEF:|_MSGPORT_I|
 ELSE
	GBLL	|_MSGPORT_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: msgport.i,v 1.6 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel messaging system definitions
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE item.i
	INCLUDE list.i

	BEGINSTRUCT	MsgPort
		STRUCT	ItemNode,mp_mp
		UINT32	mp_Signal
		STRUCT	List,mp_Msgs
		PTR	mp_UserData
		UINT32	mp_Reserved
	ENDSTRUCT

MSGPORT_SIGNAL_ALLOCATED	EQU	1

CREATEPORT_TAG_SIGNAL	EQU	TAG_ITEM_LAST+1
CREATEPORT_TAG_USERDATA	EQU	TAG_ITEM_LAST+2

	BEGINSTRUCT	Message
		STRUCT	ItemNode,msg_msg
		ITEM	msg_ReplyPort
		UINT32	msg_Result
		PTR	msg_DataPtr
		INT32	msg_DataSize
		ITEM	msg_MsgPort
		UINT32	msg_DataPtrSize
		ITEM	msg_SigItem
		UINT32	msg_Waiters
	ENDSTRUCT

MESSAGE_SENT	EQU	1
MESSAGE_REPLIED	EQU	2
MESSAGE_SMALL	EQU	4
MESSAGE_PASS_BY_VALUE EQU	8

CREATEMSG_TAG_REPLYPORT		EQU	TAG_ITEM_LAST+1
CREATEMSG_TAG_MSG_IS_SMALL	EQU	TAG_ITEM_LAST+2
CREATEMSG_TAG_DATA_SIZE		EQU	TAG_ITEM_LAST+3

	MACRO
	SendMsg
	swi	KERNELSWI+16
	MEND

	MACRO
	SendSmallMsg
	swi	KERNELSWI+16
	MEND

	MACRO
	GetMsg
	swi	KERNELSWI+19
	MEND

	MACRO
	GetThisMsg
	swi	KERNELSWI+15
	MEND

	MACRO
	WaitPort
	swi	KERNELSWI+40
	MEND

	MACRO
	ReplyMsg
	swi	KERNELSWI+18
	MEND

	MACRO
	ReplySmallMsg
	swi	KERNELSWI+18
	MEND

 ENDIF	; |_MSGPORT_I|

	END
