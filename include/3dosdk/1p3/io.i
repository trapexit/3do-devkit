 IF	:DEF:|_IO_I|
 ELSE
	GBLL	|_IO_I|

;*****

; $Id: io.i,v 1.8 1993/07/25 05:30:30 andy Exp $

; $Log: io.i,v $
;Revision 1.8  1993/07/25  05:30:30  andy
;changed pad to flags2
;
;Revision 1.8  1993/07/25  03:11:05  andy
;changed field name
;
;Revision 1.7  1993/06/17  07:50:14  andy
;sigitem now a seperate field
;
;Revision 1.6  1993/06/09  00:43:15  andy
;Changed io_Reserved field into io_SigItem field.
;
;Revision 1.5  1993/05/15  22:22:27  dale
;removed DevItem
;
;Revision 1.4  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.3  1992/12/02  21:58:35  dale
;new field in IOInfo structure
;
;Revision 1.2  1992/10/29  01:12:25  dale
;added Id and Log
;

;*****/
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE	structs.i
	INCLUDE nodes.i

	BEGINSTRUCT IOBuf
	    STRUCT  PTR,iob_Buffer
	    STRUCT  INT32,iob_Len
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

; standard commands
CMD_WRITE	EQU	0
CMD_READ	EQU	1
CMD_STATUS	EQU	2

CDROMCMD_SETUNITNAME	EQU	3


; timer device commands
TIMERCMD_DELAY	EQU	3	; vblank delay
TIMERCMD_MDELAY	EQU	4	; usec delay

MACCMD_PRINT    EQU 3       ; print string to note window */
MACCMD_ASK      EQU 4       ; print string wait for response */
MACCMD_APPEND   EQU 5       ; append data to existing file */
MACCMD_FILELEN  EQU 6       ; return filelength bufsize=4, FileInfo */
MACCMD_FILEINFO EQU	 MACCMD_FILELEN
MACCMD_WRITECR  EQU 7       ; create file, write at offset 0, truncate */
MACCMD_READDIR  EQU 9       ; get an entry from mac directory */

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
	    ITEM	io_SigItem
	ENDSTRUCT

; io_Flags
IO_DONE		EQU	1
IO_QUICK	EQU	2
IO_INTERNAL	EQU	4

 ENDIF ; |_IO_I|

	END
