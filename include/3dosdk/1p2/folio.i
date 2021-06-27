 IF	:DEF:|_FOLIO_I|
 ELSE
	GBLL	|_FOLIO_I|

;*****

; $Id: folio.i,v 1.14 1993/03/16 06:50:59 dale Exp $

; $Log: folio.i,v $
;Revision 1.14  1993/03/16  06:50:59  dale
;api
;
;Revision 1.13  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.12  1992/10/29  00:45:11  dale
;updated and cleaned up
;
;Revision 1.11  1992/10/24  01:22:19  dale
;nothgin
;
;Revision 1.10  1992/10/24  01:19:38  dale
;rcs done.
;
;Revision 1.9  1992/10/24  01:18:48  dale
;rcs really
;
;Revision 1.8  1992/10/24  01:17:39  dale
;rcs again
;

;*****/
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE	structs.i
	INCLUDE nodes.i
	INCLUDE item.i

	BEGINSTRUCT ItemRoutines
	    STRUCT  PTR,ir_Find
	    STRUCT  PTR,ir_Create
	    STRUCT  PTR,ir_Delete
	    STRUCT  PTR,ir_Open
	    STRUCT  PTR,ir_Close
	    STRUCT  PTR,ir_SetPriority
	    STRUCT  PTR,ir_SetOwner
	ENDSTRUCT


	BEGINSTRUCT Folio
		STRUCT	ItemNode,fl_fn
		STRUCT	UINT32,fl_OpenCount
		STRUCT	UINT8,fl_TaskDataIndex
		STRUCT	UINT8,fl_MaxSwiFunctions
		STRUCT	UINT8,fl_MaxUserFunctions
		STRUCT	UINT8,fl_MaxNodeType
		STRUCT	PTR,fl_NodeDB
		STRUCT	PTR,fl_OpenFolio
		STRUCT	PTR,fl_CloseFolio
		STRUCT	PTR,fl_DeleteFolio
		STRUCT	PTR,fl_ItemRoutines
		STRUCT	PTR,fl_FolioCreateTask
		STRUCT	PTR,fl_FolioDeleteTask
		STRUCT	PTR,fl_FolioRestoreTask
		STRUCT	PTR,fl_FolioSaveTask
		ARRAY	PTR,fl_reserved,8	; give me 32 bytes
	ENDSTRUCT

CREATEFOLIO_TAG_NUSERVECS     EQU 1+TAG_ITEM_LAST       ; # of uservecs */
CREATEFOLIO_TAG_USERFUNCS     EQU 1+CREATEFOLIO_TAG_NUSERVECS ; ptr to uservec table */
CREATEFOLIO_TAG_DATASIZE      EQU 1+CREATEFOLIO_TAG_USERFUNCS
CREATEFOLIO_TAG_INIT          EQU 1+CREATEFOLIO_TAG_DATASIZE
CREATEFOLIO_TAG_NODEDATABASE  EQU 1+CREATEFOLIO_TAG_INIT
CREATEFOLIO_TAG_MAXNODETYPE   EQU 1+CREATEFOLIO_TAG_NODEDATABASE
CREATEFOLIO_TAG_ITEM          EQU 1+CREATEFOLIO_TAG_MAXNODETYPE
CREATEFOLIO_TAG_OPENF         EQU 1+CREATEFOLIO_TAG_ITEM
CREATEFOLIO_TAG_CLOSEF        EQU 1+CREATEFOLIO_TAG_OPENF
CREATEFOLIO_TAG_DELETEF       EQU 1+CREATEFOLIO_TAG_CLOSEF
CREATEFOLIO_TAG_NSWIS         EQU 1+CREATEFOLIO_TAG_DELETEF ; number of swis */
CREATEFOLIO_TAG_SWIS          EQU 1+CREATEFOLIO_TAG_NSWIS ; ptr to SWI table */
CREATEFOLIO_TAG_TASKDATA      EQU 1+CREATEFOLIO_TAG_SWIS ; per task data struct */

;/* Kernel vectors */
KBV_REMHEAD             EQU -1
KBV_ADDHEAD             EQU -2
KBV_REMTAIL             EQU -3
KBV_ADDTAIL             EQU -4
KBV_INSERTTAIL  EQU -5
KBV_REMOVENODE  EQU -6
KBV_ALLOCMEM    EQU -7
KBV_FREEMEM             EQU -8
KBV_INITLIST    EQU -9
KBV_FINDNAMEDNODE       EQU -10
KBV_SCAVENGEMEM EQU -11
KBV_LOCATEITEM  EQU -12
KBV_MEMSET              EQU -13
KBV_MEMCPY              EQU -14
KBV_GETPAGESIZE EQU -15
KBV_FINSERT             EQU -17
KBV_TIMESTAMP   EQU -18

 ENDIF ; |_FOLIO_I|

	END
