 IF :DEF:|_MEM_I|
 ELSE
	GBLL	|_MEM_I|

;*****

; $Id: mem.i,v 1.7 1993/07/17 05:12:47 andy Exp $

; $Log: mem.i,v $
;Revision 1.7  1993/07/17  05:12:47  andy
;changed define
;
;Revision 1.6  1993/07/17  04:32:54  andy
;*** empty log message ***
;
;Revision 1.5  1993/07/05  06:50:42  andy
;systempage definition added
;
;Revision 1.4  1993/06/29  07:24:07  andy
;MEMTYPE_SPRITE->MEMTYPE_CEL
;
;Revision 1.3  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.2  1992/10/29  01:58:48  dale
;fixed errors
;

;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE structs.i
	INCLUDE nodes.i

	BEGINSTRUCT	MemHdr
		STRUCT	Node,memh_n
		UINT32	memh_Types
		INT32	memh_PageSize
		UINT32	memh_PageMask
		INT32	memh_VRAMPageSize
		UINT32	memh_VRAMPageMask
		UINT32	memh_FreePageBits
		PTR	memh_MemBase
		PTR	memh_MemTop
		UINT8	memh_FreePageBitsSize
		UINT8	memh_PageShift
		UINT8	memh_VRAMPageShift
		UINT8	memh_Reserved
	ENDSTRUCT

;* define location, size flags */
MEMTYPE_ANY        EQU 0

;* low 8 bits are optional fill value */
MEMTYPE_MASK       EQU &ffffff00
MEMTYPE_FILLMASK   EQU &000000ff

MEMTYPE_FILL       EQU &00000100 ;* fill memory with value */
MEMTYPE_MYPOOL     EQU &00000200 ;* do not get more memory from system */
MEMTYPE_TASKMEM    EQU &00008000      ;* internal use only */

;* memory type bits */
MEMTYPE_VRAM       EQU &00010000 ;* block must be in VRAM */
MEMTYPE_DMA        EQU &00020000 ;* accessable by hardware */
MEMTYPE_CEL     EQU &00040000 ;* accessable by sprite engine */

;* alignment bits */
MEMTYPE_INPAGE     EQU &01000000 ;* no page crossings */
MEMTYPE_STARTPAGE  EQU &02000000 ;* block must start on VRAM boundary */
MEMTYPE_SYSTEMPAGE EQU &04000000 ;* block must start on system boundry

;      ;* MEMTYPE_VRAM sets PAGESIZE=VRAM PAGESIZE */
;      ;* otherwise PAGESIZE = physical page size of memory protect system */

MEMF_ALIGNMENTS EQU (MEMTYPE_INPAGE|MEMTYPE_STARTPAGE)

;* VRAM bank select bits */
MEMTYPE_BANKSELECT      EQU &40000000 ;* bank required */
MEMTYPE_BANKSELECTMSK   EQU &30000000 ;* 2 max banks */
MEMTYPE_BANK1           EQU &10000000 ;* first bank */
MEMTYPE_BANK2           EQU &20000000 ;* second bank */

;* undefined memtype bits */
;* &0CF87C00 */

;* ControlMem commands */
MEMC_NOWRITE   EQU 1       ;* make memory unwritable for a task */
MEMC_OKWRITE   EQU 2       ;* make memory writable for a task */
MEMC_GIVE      EQU 3       ;* give memory away, or back to system */
MEMC_SC_GIVE   EQU 4       ;* special give for scavengemem */

	BEGINSTRUCT MemList
	    STRUCT	Node,meml_n
	    UINT32	meml_Types
	    PTR		meml_OwnBits
	    PTR		meml_WriteBits
	    PTR		meml_MemHdr
	    PTR		meml_l
	    ITEM	meml_Sema4
	    UINT8	meml_OwnBitsSize
	    ARRAY	UINT8,meml_Reserved,3
	ENDSTRUCT

 ENDIF	; |_MEM_I|

	END
