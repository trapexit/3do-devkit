#ifndef __FOLIO_H
#define __FOLIO_H

/*****

$Id: folio.h,v 1.28 1994/01/28 23:20:13 sdas Exp $

$Log: folio.h,v $
 * Revision 1.28  1994/01/28  23:20:13  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.27  1994/01/21  02:08:43  limes
 * recover from rcs bobble:
 *
 * +  * Revision 1.28  1994/01/20  01:55:06  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.27  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 *
 * Revision 1.28  1994/01/20  01:55:06  sdas
 * C++ compatibility - updated
 *
 * Revision 1.27  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.26  1993/12/14  23:55:01  sdas
 * Missing KBV_* vector numbers added to support lib/vector*.c
 * These new definitions are equivalent to ones previously in lib/kbvectors.s
 *
 * Revision 1.25  1993/11/05  03:27:17  limes
 * Get autodocs out of include files, for now.
 *
 * Revision 1.24  1993/11/05  03:25:09  limes
 * Autodoc Merge
 *
 * Revision 1.23  1993/11/05  03:07:25  limes
 * *** empty log message ***
 *
 * Revision 1.22  1993/09/17  01:18:51  dale
 * added INSERTNODE and SETNODEPRI
 *
 * Revision 1.21  1993/08/13  03:42:25  dale
 * Find convenience macro
 *
 * Revision 1.20  1993/08/04  22:26:07  dale
 * SetFunction
 *
 * Revision 1.19  1993/08/04  22:09:50  dale
 * SetItemOwner returns Err.
 *
 * Revision 1.18  1993/03/16  06:50:59  dale
 * api
 *
 * Revision 1.17  1993/03/08  23:43:18  dale
 * new debug table
 *
 * Revision 1.16  1993/02/19  02:12:07  dale
 * new kernel routines, slack timer, TagProcessor
 *
 * Revision 1.15  1993/02/11  04:58:58  dale
 * name update for M6
 *
 * Revision 1.14  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.13  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.12  1992/12/16  21:55:50  dale
 * printf and syserr
 *
 * Revision 1.11  1992/12/01  22:49:59  dale
 * new OpenItem void * parm
 *
 * Revision 1.10  1992/12/01  04:30:42  dale
 * new FindItem/OpenItem
 *
 * Revision 1.9  1992/11/05  20:08:39  dale
 * Pass targarg ptr to Folio CreateTask routines.
 *
 * Revision 1.8  1992/10/24  01:10:34  dale
 * rcs again
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

#include "types.h"
#include "nodes.h"
#include "item.h"
#include "task.h"

/* This is the structure of the Node Creation DataBase that each */
/* folio must have to use the CreateItem interface */

typedef struct NodeData
{
	uint8 size;
	uint8 flags;
} NodeData;

/* The upper 4 bits of the flags field are put in the n_Flags */
/* field of the created Node. The other four bits are used to */
/* tell the allocator other things for initialization */

typedef struct ItemRoutines
{
	/* Folio helper routines for its Items */
	Item  (*ir_Find)(int32 ntype, TagArg *tp);
	Item  (*ir_Create)(void *n, uint8 ntype, void *args);
	int32 (*ir_Delete)(Item it, struct Task *t);
	Item  (*ir_Open)(Node *n, void *args);
	int32 (*ir_Close)(Item it,struct Task *t);
	int32 (*ir_SetPriority)(ItemNode *n, uint8 pri, struct Task *t);
	Err   (*ir_SetOwner)(ItemNode *n, Item newOwner, struct Task *t);
} ItemRoutines;

typedef struct Folio
{
	ItemNode fn;
	int32 f_OpenCount;
	uint8  f_TaskDataIndex;
	uint8 f_MaxSwiFunctions;
	uint8 f_MaxUserFunctions;
	uint8 f_MaxNodeType;
	NodeData *f_NodeDB;
	int32  (*f_OpenFolio)(struct Folio *f);
	void  (*f_CloseFolio)(struct Folio *f);
	int32  (*f_DeleteFolio)(struct Folio *f);

	/* Folio helper routines for its Items */
	ItemRoutines *f_ItemRoutines;
	/* routines for task creation/deletion swapping */
	int32  (*f_FolioCreateTask)(struct Task *t,TagArg *tagpt);
	void (*f_FolioDeleteTask)(struct Task *t);
	void (*f_FolioRestoreTask)(struct Task *t);
	void (*f_FolioSaveTask)(struct Task *t);
	/* reserved for future expansion */
	uint32	*f_DebugTable;
	int32	reserved[7];
} Folio, *FolioP;

enum folio_tags
{
	CREATEFOLIO_TAG_NUSERVECS = TAG_ITEM_LAST+1,	/* # of uservecs */
	CREATEFOLIO_TAG_USERFUNCS,	/* ptr to uservec table */
	CREATEFOLIO_TAG_DATASIZE,
	CREATEFOLIO_TAG_INIT,
	CREATEFOLIO_TAG_NODEDATABASE,
	CREATEFOLIO_TAG_MAXNODETYPE,
	CREATEFOLIO_TAG_ITEM,
	CREATEFOLIO_TAG_OPENF,
	CREATEFOLIO_TAG_CLOSEF,
	CREATEFOLIO_TAG_DELETEF,
	CREATEFOLIO_TAG_NSWIS,		/* number of swis */
	CREATEFOLIO_TAG_SWIS,		/* ptr to SWI table */
	CREATEFOLIO_TAG_TASKDATA	/* per task data struct */
};

/* To interpret this mismash of * and () */
/* _f is a pointer to table of function pointers */
/* This table extends in the negative direction */
#define CALLFOLIO(folio,func,args) \
	{ \
		void (* *_f)() = (void (* *)())folio; \
		(*_f[func])args; \
	}
#define CALLFOLIORET(folio,func,args,ret,cast) \
	{ \
		int32 (* *_f)() = (int32 (* *)())folio; \
		ret = cast (*_f[func])args; \
	}
#define CallFolio(folio,func,args) \
	{ \
		void (* *_f)() = (void (* *)())folio; \
		(*_f[func])args; \
	}
#define CallFolioRet(folio,func,args,ret,cast) \
	{ \
		int32 (* *_f)() = (int32 (* *)())folio; \
		ret = cast (*_f[func])args; \
	}

/* Kernel vectors */
#define KBV_REMHEAD		-1
#define KBV_ADDHEAD		-2
#define KBV_REMTAIL		-3
#define KBV_ADDTAIL		-4
#define KBV_INSERTTAIL	-5
#define KBV_REMOVENODE	-6
#define KBV_ALLOCMEM	-7
#define KBV_FREEMEM		-8
#define KBV_INITLIST	-9
#define KBV_FINDNAMEDNODE	-10
#define KBV_SCAVENGEMEM	-11
#define KBV_LOCATEITEM	-12
#define KBV_MEMSET		-13
#define KBV_MEMCPY		-14
#define KBV_GETPAGESIZE	-15
#define KBV_CHECKITEM	-16
#define KBV_FINSERT		-17
#define KBV_USEC2TICKS	-18
#define KBV_SETNODEPRI	-19
#define KBV_INSERTHEAD	-20
#define KBV_VFPRINTF	-21
#define KBV_GETSYSERR	-22
#define KBV_TICKS2TIMEVAL	-23
#define KBV_WAITPORT	-24
#define KBV_FINDMH	-25
#define KBV_ALLOCMEMML	-26
#define KBV_FREEMEMML	-27
#define KBV_ALLOCMEMLIST	-28
#define KBV_FREEMEMLIST	-29
#define KBV_ITEMOPENED	-32

#define VTYPE_SWI	0
#define VTYPE_USER	1
#define VTYPE_SUPER	2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void __swi(KERNELSWI+23) *SetFunction(Item, int32 vnum, int32 vtype, void *newfunc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define FindFolio(n)     FindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),(n))

#endif	/* __FOLIO_H */
