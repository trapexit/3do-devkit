

;*****

; $Id: nodes.i,v 1.12 1993/06/30 06:44:26 andy Exp $

; $Log: nodes.i,v $
;Revision 1.12  1993/06/30  06:44:26  andy
;defined new flag for item creation
;
;Revision 1.11  1993/06/15  02:11:16  andy
;needed to include structs.i
;
;Revision 1.10  1993/06/09  00:43:15  andy
;changed one of the reserved Kernel flags to a NODE_SIZELOCKED flag, which
;we use on objects where we control the size absolutely.
;
;Revision 1.9  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.8  1992/10/29  02:22:26  dale
;added NamelessNode definition
;
;Revision 1.7  1992/10/29  01:26:53  dale
;removed definition of KERNELNODE and FOLIONODE
;
;Revision 1.6  1992/10/24  01:41:07  dale
;rcs
;

;*****/
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

 IF :DEF:|_NODES_I|
 ELSE
	GBLL	|_NODES_I|

	INCLUDE	structs.i

	BEGINSTRUCT	Node
		STRUCT	PTR,N_Next
		STRUCT	PTR,N_Prev
		STRUCT	UBYTE,N_SubsysType
		STRUCT	UBYTE,N_Type
		STRUCT	UBYTE,N_Priority
		STRUCT	UBYTE,N_Flags
		STRUCT	ULONG,N_Size
		STRUCT	PTR,N_Name
	ENDSTRUCT

	BEGINSTRUCT	NamelessNode
		STRUCT	PTR,NN_Next
		STRUCT	PTR,NN_Prev
		STRUCT	UBYTE,NN_SubsysType
		STRUCT	UBYTE,NN_Type
		STRUCT	UBYTE,NN_Priority
		STRUCT	UBYTE,NN_Flags
		STRUCT	ULONG,NN_Size
	ENDSTRUCT

	BEGINSTRUCT	ItemNode
		STRUCT	Node,N_dummy
		STRUCT	UBYTE,N_Version
		STRUCT	UBYTE,N_Revision
		STRUCT	UBYTE,N_FolioFlags
		STRUCT	UBYTE,N_ItemFlags
		STRUCT	ULONG,N_Item
		STRUCT	ULONG,N_Owner
		STRUCT	PTR,N_ReservedP
	ENDSTRUCT

	BEGINSTRUCT	MinNode
		STRUCT	PTR,NNN_Next
		STRUCT	PTR,NNN_Prev
	ENDSTRUCT

 ENDIF ; |_NODES_I|

	END
