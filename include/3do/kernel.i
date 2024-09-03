

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: kernel.i,v 1.33 1994/12/08 19:07:09 markn Exp $
;*
;*  Kernel folio structure definition
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE	folio.i

 IF	:DEF:|_KERNEL_I|
 ELSE
	GBLL	|_KERNEL_I|

	BEGINSTRUCT	KernelBase
		STRUCT	Folio,kb
		PTR	kb_RomTags
		PTR	kb_MemFreeLists
		PTR	kb_MemHdrList
		PTR	kb_FolioList
		PTR	kb_Drivers
		PTR	kb_Devices
		PTR	kb_TaskWaitQ
		PTR	kb_TaskReadyQ
		PTR	kb_MsgPorts
		PTR	kb_Semaphores
		PTR	kb_CurrentTask
		PTR	kb_InterruptHandlers
		PTR	kb_TimerBits
		UINT32	kb_ElapsedQuanta
		ARRAY	UINT32,kb_Private0,17
		PTR	kb_ExtentedErrors
		UINT8	kb_MadamRev
		UINT8	kb_ClioRev
		ARRAY	UINT8,kb_Private1,2
		ARRAY	UINT32,kb_Private2,2
		UINT32	kb_NumTaskSwitches

		ARRAY	UINT32,kb_Private3,4
		PTR	kb_BootVolumeName
		PTR	kb_Tasks
	ENDSTRUCT


 ENDIF  ; |_KERNEL_I|

	END
