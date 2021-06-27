
 IF	:DEF:|_DEVICE_I|
 ELSE
	GBLL	|_DEVICE_I|

;*****

; $Id: device.i,v 1.7 1994/01/28 23:20:12 sdas Exp $

; $Log: device.i,v $
;Revision 1.7  1994/01/28  23:20:12  sdas
;Include Files Self-consistency - includes dependant include files
;
;Revision 1.6  1993/06/13  07:11:20  andy
;added TimerDevice typedef
;
;Revision 1.5  1993/06/09  01:25:57  andy
;Added TimerDevice typedev
;
;Revision 1.4  1993/06/09  00:43:15  andy
;Added reserved field to driver to pad it to 16 byte multiple.
;
;Revision 1.3  1993/03/26  18:28:00  dale
;dev_DeleteDev
;
;Revision 1.2  1993/03/16  09:24:35  dale
;fix comment
;
;Revision 1.1  1993/03/16  06:52:45  dale
;Initial revision
;
;Revision 1.5  1993/02/11  04:59:43  dale
;M6 name update
;
;Revision 1.4  1992/12/08  10:01:02  dale
;new devInit entry for device initialization.`
;
;Revision 1.3  1992/10/29  01:09:40  dale
;now use typedefs
;
;Revision 1.2  1992/10/29  00:30:27  dale
;fixed comments for rcs
;
; Revision 1.1  1992/10/29  00:29:36  dale
; Initial revision
;

;*****/
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE item.i
	INCLUDE structs.i
	INCLUDE nodes.i
	INCLUDE list.i

	BEGINSTRUCT	Device
		STRUCT	ItemNode,dev_dev
		PTR	dev_Driver
		INT32	dev_OpenCnt
		PTR	dev_CreateIOReq
		PTR	dev_DeleteIOReq
		PTR	dev_Open
		PTR	dev_Close
		INT32	dev_IOReqSize
		STRUCT	List,dev_IOReqs
		UINT8	dev_MaxUnitNum
		ARRAY	UINT8,dev_Reserved,3
		PTR	dev_Init
		PTR	dev_DeleteDev
		UINT32	dev_Reserved4
	ENDSTRUCT

CREATEDEVICE_TAG_DRVR   EQU TAG_ITEM_LAST+1
CREATEDEVICE_TAG_CRIO   EQU CREATEDEVICE_TAG_DRVR+1 ; createIO routine
CREATEDEVICE_TAG_DLIO   EQU CREATEDEVICE_TAG_CRIO+1
CREATEDEVICE_TAG_OPEN   EQU CREATEDEVICE_TAG_DLIO+1 ; open routine
CREATEDEVICE_TAG_CLOSE  EQU CREATEDEVICE_TAG_OPEN+1 ; close routine
CREATEDEVICE_TAG_IOREQSZ EQU CREATEDEVICE_TAG_CLOSE+1 ; optional request size
CREATEDEVICE_TAG_INIT	EQU CREATEDEVICE_TAG_IOREQSZ+1 ; init rtn to call



	BEGINSTRUCT	TimerDevice
		STRUCT	Device,timerdev_dev
		UINT32  timerdev_VBlankCountOverFlow
		UINT32  timerdev_VBlankCount
	ENDSTRUCT

 ENDIF	; |_DEVICES_I|

	END
