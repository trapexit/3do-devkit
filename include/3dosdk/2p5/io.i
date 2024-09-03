 IF	:DEF:|_IO_I|
 ELSE
	GBLL	|_IO_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: io.i,v 1.11 1994/11/19 01:49:45 sdas Exp $
;*
;*  Kernel device IO definitions
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE nodes.i

	BEGINSTRUCT IOBuf
	    PTR		iob_Buffer
	    INT32	iob_Len
	ENDSTRUCT

	BEGINSTRUCT IOInfo
		UINT8	ioi_Command
		UINT8	ioi_Flags
		UINT8	ioi_Unit
		UINT8	ioi_Flags2
		UINT32	ioi_CmdOptions
		UINT32	ioi_User
		INT32	ioi_Offset
		STRUCT	IOBuf,ioi_Send
		STRUCT	IOBuf,ioi_Recv
	ENDSTRUCT

CMD_WRITE	EQU	0
CMD_READ	EQU	1
CMD_STATUS	EQU	2

MACCMD_PRINT    EQU 3
MACCMD_ASK      EQU 4
MACCMD_APPEND   EQU 5
MACCMD_FILELEN  EQU 6
MACCMD_FILEINFO EQU	 MACCMD_FILELEN
MACCMD_WRITECR  EQU 7
MACCMD_READDIR  EQU 9
MACCMD_READCDDELAY	EQU 10

KPRINTF_STOP	EQU 1
KPRINTF_START	EQU 2
KPRINTF_DISABLE EQU 4

	BEGINSTRUCT IOReq
	    STRUCT	ItemNode,io_N
	    STRUCT	MinNode,io_Link
	    PTR		io_Dev
	    PTR		io_CallBack
	    STRUCT	IOInfo,io_Info
	    INT32	io_Actual
	    UINT32	io_Flags
	    INT32	io_Error
	    ARRAY	INT32,io_Extension,2
	    ITEM	io_MsgItem
	    UINT32	io_Private0
	ENDSTRUCT

IO_DONE		EQU	1
IO_QUICK	EQU	2
IO_INTERNAL	EQU	4
IO_WAITING	EQU	8

CREATEIOREQ_TAG_REPLYPORT	EQU TAG_ITEM_LAST+1
CREATEIOREQ_TAG_DEVICE		EQU TAG_ITEM_LAST+2

	MACRO
	SendIO
	swi	KERNELSWI+24
	MEND

	MACRO
	DoIO
	swi	KERNELSWI+37
	MEND

	MACRO
	AbortIO
	swi	KERNELSWI+25
	MEND

	MACRO
	WaitIO
	swi	KERNELSWI+41
	MEND

 ENDIF ; |_IO_I|

	END
