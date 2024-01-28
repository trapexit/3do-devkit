

;*****

; $Id: kernel.i,v 1.27 1993/06/14 17:43:56 andy Exp $

; $Log: kernel.i,v $
;Revision 1.27  1993/06/14  17:43:56  andy
;fixed typo
;
;Revision 1.26  1993/06/12  10:51:33  dale
;secret ptrs for unused VRAM
;
;Revision 1.25  1993/06/09  00:43:15  andy
;Added new KernelBase fields, and padded to 16 byte multiple.
;
;Revision 1.24  1993/05/17  00:49:15  andy
;added defines for red and green revs
;
;Revision 1.23  1993/05/17  00:44:18  andy
;switched to Clio and Madam version numbers
;
;Revision 1.22  1993/05/16  23:56:54  andy
;changed bits to split madam/clio revs
;
;Revision 1.21  1993/04/02  01:50:12  dale
;syntax brain error
;
;Revision 1.20  1993/04/02  01:11:02  dale
;updated to follow kernel.h
;
;Revision 1.19  1993/03/16  06:50:39  dale
;api
;
;Revision 1.18  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.17  1992/12/22  11:10:00  dale
;KB_CONTROLPORT added
;
;Revision 1.16  1992/12/16  21:55:50  dale
;printf and syserr
;
;Revision 1.15  1992/12/16  04:56:56  dale
;changes for modified memory allocator
;
;Revision 1.14  1992/12/13  23:41:02  dale
;support for QuietAbort handling
;
;Revision 1.13  1992/12/09  21:21:22  dale
;ces roms stuff
;
;Revision 1.12  1992/12/07  09:20:10  dale
;CES
;
;Revision 1.11  1992/10/29  01:29:23  dale
;checkpoint
;
;Revision 1.10  1992/10/29  01:21:03  dale
;cleanup
;
;Revision 1.9  1992/10/24  01:37:21  dale
;fix id
;

;*****/
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

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
		UINT32	kb_VRAMHACK
		PTR	kb_ItemTable
		UINT32	kb_MaxItem
		UINT32	kb_CPUFlags
		UINT8	kb_MaxInterrupts
		UINT8	kb_Forbid
		UINT8	kb_FolioTableSize
		UINT8	kb_PleaseReschedule
		UINT32	kb_MacPkt
		UINT32	kb_Flags
		UINT32	kb_timerCount
		UINT32	kb_numticks
		UINT32	kb_denomticks
		UINT32	kb_MSysBits
		UINT8	kb_FolioTaskDataCnt
		UINT8	kb_FolioTaskDataSize
		UINT8	kb_DRAMSetSize
		UINT8	kb_VRAMSetSize
		PTR	kb_DataFolios
		PTR	kb_CatchDataAborts
		UINT32	kb_QuietAborts
		PTR	kb_RamDiskAddr
		INT32	kb_RamDiskSize
		PTR	kb_ExtentedErrors
		UINT8	kb_MadamRev
		UINT8	kb_ClioRev
		UINT8	kb_Resbyte0
		UINT8	kb_Resbyte1
		UINT32	kb_DevSemaphore
		PTR	kb_SystemStackList
		UINT32	kb_Pad
		PTR	kb_VRAM0
		UINT32	kb_VRAM0Size
		PTR	kb_VRAM1
		UINT32	kb_VRAM1Size
	ENDSTRUCT

MAXFOLIOS	EQU	16


; kb_CPUFlags */

KB_BIGENDIAN   EQU 1       ;* we are Big Endian, 0=Little Endian */
KB_32BITMODE   EQU 2       ;* 32 Bit Address Operation, 0=26 bit */
KB_ARM600      EQU 4       ;* this is an ARM600 */
KB_SHERRY      EQU 8       ;* new hardware? */
KB_SHERRIE     EQU KB_SHERRY
KB_BLUE        EQU &10
KB_RED		EQU &20		; sherry red
KB_REDWW	EQU &40		; sherry red wire wrap
KB_GREEN	EQU &80		; temporary alias
KB_WIREWRAP	EQU &100	; green silicon wire wrap


KB_NODBGR	EQU &8000	; no debugger to catch us
KB_NODBGRPOOF	EQU &4000	; no debugger to catch us, just kidding
KB_CONTROLPORT	EQU &1000	; mei controlport

KB_BROOKTREE EQU &00010000
KB_REBOOTED  EQU &01000000

;;* other kernelbase flags */
KB_TASK_DBG    EQU 1       ;* debug out for createtask */

REDMHZx1000	EQU	36000	; 36mhz * 1000
MHZx1000       	EQU	49091   ; 49.0908mhz * 1000


CHIP_RED_REV	EQU 0
CHIP_GREEN_REV	EQU 1

 ENDIF  ; |_KERNEL_I|

	END
