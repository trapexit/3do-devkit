 IF	:DEF:|_FOLIO_I|
 ELSE
	GBLL	|_FOLIO_I|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: folio.i,v 1.20 1994/11/23 21:23:41 vertex Exp $
;*
;*  Kernel folio management definitions
;*
;*****************************************************************************

	INCLUDE	structs.i
	INCLUDE nodes.i
	INCLUDE item.i

	BEGINSTRUCT Folio
		STRUCT	ItemNode,fl_fn
		STRUCT	UINT32,fl_OpenCount
		STRUCT	UINT8,fl_TaskDataIndex
		STRUCT	UINT8,fl_MaxSwiFunctions
		STRUCT	UINT8,fl_MaxUserFunctions
		STRUCT	UINT8,fl_MaxNodeType
		ARRAY	UINT32,fl_Private,17
	ENDSTRUCT

CREATEFOLIO_TAG_NUSERVECS     EQU 1+TAG_ITEM_LAST
CREATEFOLIO_TAG_USERFUNCS     EQU 1+CREATEFOLIO_TAG_NUSERVECS

KBV_REMHEAD             EQU -1
KBV_ADDHEAD             EQU -2
KBV_REMTAIL             EQU -3
KBV_ADDTAIL             EQU -4
KBV_INSERTTAIL  	EQU -5
KBV_REMOVENODE  	EQU -6
KBV_ALLOCMEM    	EQU -7
KBV_FREEMEM             EQU -8
KBV_INITLIST    	EQU -9
KBV_FINDNAMEDNODE       EQU -10
KBV_SCAVENGEMEM 	EQU -11
KBV_LOCATEITEM  	EQU -12
KBV_LOOKUPITEM		EQU -12
KBV_MEMSET              EQU -13
KBV_MEMCPY              EQU -14
KBV_GETPAGESIZE 	EQU -15
KBV_FINSERT             EQU -17
KBV_USEC2TICKS		EQU -18
KBV_SETNODEPRI		EQU -19
KBV_INSERTHEAD		EQU -20
KBV_VFPRINTF		EQU -21
KBV_GETSYSERR		EQU -22
KBV_TICKS2TIMEVAL	EQU -23
KBV_FINDMH		EQU -25
KBV_ALLOCMEMML		EQU -26
KBV_FREEMEMML		EQU -27
KBV_ALLOCMEMLIST	EQU -28
KBV_FREEMEMLIST		EQU -29
KBV_ITEMOPENED		EQU -32
KBV_FINDIMAGE		EQU -38
KBV_GETMEMALLOCALIGNMENT EQU -39
KBV_GETMEMTRACKSIZE	EQU -40
KBV_CHECKIO       	EQU -41
KBV_ISMEMREADABLE 	EQU -42
KBV_ISMEMWRITABLE 	EQU -43
KBV_FINDTAGARG    	EQU -44
KBV_NEXTTAGARG    	EQU -45

 ENDIF ; |_FOLIO_I|

	END
