 IF :DEF:|_MSGPORT_I|
 ELSE
	GBLL	|_MSGPORT_I|

;*****

; $Id: msgport.i,v 1.4 1993/06/10 18:19:43 andy Exp $

; $Log: msgport.i,v $
;Revision 1.4  1993/06/10  18:19:43  andy
;SigItem change to int32
;
;Revision 1.3  1993/06/09  00:43:15  andy
;Added UserData pointer and Reserved to MsgPort, and defined new
;tag to set UserData pointer.  Changed msg_Reserved of Message to
;a msg_DataPtrSize, for the new seperately allocated pass_by_valye
;area, and defined a tag to set that up.
;
;Revision 1.2  1993/03/16  09:25:27  dale
;fix comment
;
;Revision 1.1  1993/03/16  06:43:22  dale
;Initial revision
;
;Revision 1.6  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.5  1992/11/25  00:01:25  dale
;new msgs
;
;Revision 1.4  1992/10/30  00:01:12  dale
;checkpoint
;
;Revision 1.3  1992/10/29  23:25:28  dale
;removed msg_LastSent
;
;Revision 1.2  1992/10/29  02:10:54  dale
;added Id Log
;

;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE	structs.i
	INCLUDE list.i

	BEGINSTRUCT	MsgPort
		STRUCT	ItemNode,mp_mp
		UINT32	mp_Signal
		STRUCT	List,mp_Msgs
		PTR	mp_UserDaya
		UINT32	mp_Reserved
	ENDSTRUCT

	BEGINSTRUCT	Message
		STRUCT	ItemNode,msg_msg
		INT32	msg_ReplyPort
		UINT32	msg_Result
		PTR	msg_DataPtr
		INT32	msg_DataSize
		INT32	msg_MsgPort
		UINT32	msg_DataPtrSize
		INT32	msg_SigItem
	ENDSTRUCT

MESSAGE_SENT	EQU	1
MESSAGE_REPLIED	EQU	2
MESSAGE_SMALL	EQU	4
MESSAGE_PASS_BY_VALUE EQU	8

 ENDIF	; |_MSGPORTS_I|

	END
