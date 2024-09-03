 IF :DEF:|_SEMAPHORE_I|
 ELSE
	GBLL	|_SEMAPHORE_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: semaphore.i,v 1.7 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel semaphore management definitions
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE	nodes.i
	INCLUDE	item.i
	INCLUDE	list.i

	BEGINSTRUCT	Semaphore
		STRUCT	ItemNode,s_s
		UINT32	sem_bit
		ITEM	sem_Owner
		INT32	sem_NestCnt
		STRUCT	List,sem_TaskWaitingList
	ENDSTRUCT

	MACRO
	LockSemaphore
	swi	KERNELSWI+7
	MEND

	MACRO
	UnlockSemaphore
	swi	KERNELSWI+6
	MEND

 ENDIF	; |_SEMAPHORE_I|

	END
