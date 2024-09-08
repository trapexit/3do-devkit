 IF	:DEF:|_TASK_I|
 ELSE
	GBLL	|_TASK_I|

;*****

; $Id: task.i,v 1.14 1993/06/29 07:21:48 andy Exp $

; $Log: task.i,v $
;Revision 1.14  1993/06/29  07:21:48  andy
;added new tag for createtask
;
;Revision 1.13  1993/06/09  00:43:15  andy
;Added List for stacks associated with this task
;
;Revision 1.12  1993/05/28  00:54:28  andy
;needed ssl var for superstack limit storage
;
;Revision 1.11  1993/03/16  06:48:43  dale
;api
;
;Revision 1.10  1993/02/11  04:58:58  dale
;name update for M6
;
;Revision 1.9  1992/12/17  22:25:37  dale
;added macros for swi functions
;
;Revision 1.8  1992/10/29  02:32:37  dale
;updated with defines
;
;Revision 1.7  1992/10/24  01:46:09  dale
;rcsa
;
;*****/
;
;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE	structs.i
	INCLUDE	nodes.i
	INCLUDE item.i

TASK_READY   EQU 1
TASK_WAITING EQU 2
TASK_RUNNING EQU 4
TASK_SUPER   EQU 8

TASK_MAX_PRIORITY EQU 254
TASK_MIN_PRIORITY EQU 1


	BEGINSTRUCT	Task
		STRUCT	ItemNode,t_t
		PTR	t_ThreadTask
		PTR	t_ResourceTable
		UINT32	t_ResourceCnt
		UINT32	t_WaitBits
		UINT32	t_SigBits
		UINT32	t_AllocatedSigs
		PTR	t_StackBase
		UINT32	t_StackSize
		ARRAY	UINT32,t_regs,13
		PTR	t_sp
		UINT32	t_lk
		UINT32	t_pc
		UINT32	t_psr
		UINT32	t_Userpsr
		PTR	t_ssp
		PTR	t_Usersp
		UINT32	t_Userlk
		UINT32	t_ElapsedQuanta
		UINT32	t_SuperStackSize
		PTR	t_SuperStackBase
		PTR	t_ObsoleteMemProtectBits
		PTR	t_FreeMemoryLists
		PTR	t_FolioData
		UINT32	t_FolioContext
		ARRAY	INT32,t_ElapsedTime,2
		UINT32	t_MaxTicks
		UINT32	t_MaxUsecs
		PTR	t_PageTable
		PTR	t_ssl
		PTR	t_UserStackList
	ENDSTRUCT

;* The resource table has some bits packed in the upper */
;* bits of the Item, since Items will max out at 4096 */
ITEM_WAS_OPENED EQU 0x4000
ITEM_NOT_AN_ITEM        EQU 0x80000000

;* predefined signals */

SIGF_MEMLOW  EQU   2       ;* first warning of low memory */
SIGF_MEMGONE EQU   1       ;* major memory exhaustion */

SIGF_ABORT   EQU   4       ;* abort current operation */
SIGF_IODONE  EQU   8       ;* Polled IO signal */
;* By default, this signal is orred into all Wait requests */
;* so all callers of wait must be ready to deal with an */
;* abnormal return */

;* Tag Args for CreateTask */

CREATETASK_TAG_PC       EQU 1+TAG_ITEM_LAST
CREATETASK_TAG_MAXQ     EQU 1+CREATETASK_TAG_PC
CREATETASK_TAG_STACKSIZE        EQU 1+CREATETASK_TAG_MAXQ
CREATETASK_TAG_ARGC     EQU 1+CREATETASK_TAG_STACKSIZE ; initial r0 */
CREATETASK_TAG_ARGP     EQU 1+CREATETASK_TAG_ARGC ; initial r1 */
CREATETASK_TAG_SP       EQU 1+CREATETASK_TAG_ARGP ; Makes task a thread */
CREATETASK_TAG_BASE     EQU 1+CREATETASK_TAG_SP ; initial r9 */
CREATETASK_TAG_IMAGE    EQU 1+CREATETASK_TAG_BASE ; load image starts at */
CREATETASK_TAG_IMAGESZ  EQU 1+CREATETASK_TAG_IMAGE ; OBSOLETE: size of initial load image
CREATETASK_TAG_AIF      EQU 1+CREATETASK_TAG_IMAGESZ ; points to AIF header */
CREATETASK_TAG_CMDSTR   EQU 1+CREATETASK_TAG_AIF ; ptr to cmd string to stack
CREATETASK_TAG_SUPER	EQU 1+CREATETASK_TAG_CMDSTR
CREATETASK_TAG_RSA	EQU 1+CREATETASK_TAG_SUPER
CREATETASK_TAG_USERONLY EQU 1+CREATETASK_TAG_RSA ; task with no super stack

SIGF_DEADTASK   EQU 0x10    ;* signal sent to parent of dead task */

	MACRO
	Wait
	swi	KERNELSWI+1
	MEND

	MACRO
	Signal
	swi	KERNELSWI+2
	MEND

	MACRO
	Yield
	swi	KERNELSWI+9
	MEND

	MACRO
	AllocSignal
	swi	KERNELSWI+21
	MEND

	MACRO
	FreeSignal
	swi	KERNELSWI+22
	MEND


 ENDIF ; |_TASK_I|
	END
