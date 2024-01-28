 IF :DEF:|_SEMAPHORE_I|
 ELSE
	GBLL	|_SEMAPHORE_I|

;*****

; $Id: semaphore.i,v 1.5 1994/01/28 23:20:24 sdas Exp $

; $Log: semaphore.i,v $
;Revision 1.5  1994/01/28  23:20:24  sdas
;Include Files Self-consistency - includes dependant include files
;
;Revision 1.4  1993/06/09  00:43:15  andy
;Padded semaphore wait to 16 byte multiple
;
;Revision 1.3  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.2  1992/10/29  02:23:29  dale
;added Id and Log
;

;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE	structs.i
	INCLUDE	nodes.i
	INCLUDE	item.i
	INCLUDE	list.i

	BEGINSTRUCT	SemaphoreWaitNode
		STRUCT	NamelessNode,swn_swn
		UINT32	swn_sig
		ITEM	swn_Task
	ARRAY	UINT32, swn_Pad,2
	ENDSTRUCT

	BEGINSTRUCT	Semaphore
		STRUCT	ItemNode,s_s
		INT32	sem_bit
		ITEM	sem_Owner
		INT32	sem_NestCnt
		STRUCT	List,sem_TaskWaitingList
	ENDSTRUCT

 ENDIF	; |_SEMAPHORE_I|

	END
