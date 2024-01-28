#ifndef __MEM_H
#define __MEM_H

/*****

$Id: mem.h,v 1.24 1994/01/21 02:21:49 limes Exp $

$Log: mem.h,v $
 * Revision 1.24  1994/01/21  02:21:49  limes
 * recover from rcs bobble
 *
 * Revision 1.25  1994/01/20  02:03:35  sdas
 * C++ compatibility - updated
 *
 * Revision 1.24  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.23  1993/12/17  20:18:25  dale
 * added availMem
 *
 * Revision 1.22  1993/09/23  17:16:37  limes
 * Define MEMTYPE_FROMTOP, a new flag for the memory type field, which
 * tells the allocator to allocate from the high end of the list
 * instead of from the low end; and when a large enough fragment is
 * found, allocate from the top of the fragment, not the bottom.
 *
 * Revision 1.21  1993/08/20  21:56:55  bryce
 * Add structure for use by AvailMem(0 library call.
 * /.
 *
 * Revision 1.20  1993/08/07  03:49:23  andy
 * SystemScavengeMem call
 *
 * Revision 1.19  1993/07/17  04:30:42  andy
 * added new mem command for scavengemem
 *
 * Revision 1.18  1993/07/03  18:01:46  andy
 * added new memtype to differtiate system pages and graphics/vram pages.
 *
 * Revision 1.17  1993/06/14  21:23:01  rjmical
 * Moved USER_ SUPER_ MEM macros from graphics.h to mem.h
 *
 * Revision 1.16  1993/04/07  22:38:27  dale
 * fixed comments
 *
 * Revision 1.15  1993/04/02  01:11:57  dale
 * new private memlist functions, provisional MEMTYPE_AUDIO/DSP
 *
 * Revision 1.14  1993/03/30  05:44:29  dale
 * added externs to prototypes.
 *
 * Revision 1.13  1993/03/29  22:09:06  dale
 * MEMTYPE_SPRITE -> MEMTYPE_CEL
 *
 * Revision 1.12  1993/03/16  06:50:03  dale
 * api
 *
 * Revision 1.11  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.11  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.10  1992/10/29  01:59:44  dale
 * manually line up structs with uint8 pads
 *
 * Revision 1.9  1992/10/24  01:41:07  dale
 * rcs
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

#include "list.h"
#include "item.h"
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

extern void *malloc(int32);
extern void free(void *p);

extern void *AllocMemFromMemLists(List *l,int32 size, uint32 typebits);
extern void FreeMemToMemLists(List *l, void *p, int32 size);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#ifndef _KERNEL_H
#include "kernel.h"
#endif

/* Define mode-explicite mem alloc */
#undef  USER_ALLOCMEM
#define USER_ALLOCMEM(s,t)	AllocMemFromMemLists( \
			KernelBase->kb_CurrentTask->t_FreeMemoryLists,(s),(t))
#undef  USER_FREEMEM
#define USER_FREEMEM(p,s)	FreeMemToMemLists( \
			KernelBase->kb_CurrentTask->t_FreeMemoryLists,(p),(s))

#undef  SUPER_ALLOCMEM
#define SUPER_ALLOCMEM(s,t)	AllocMemFromMemLists( \
			KernelBase->kb_MemFreeLists,(s),(t))
#undef  SUPER_FREEMEM
#define SUPER_FREEMEM(p,s)	FreeMemToMemLists( \
			KernelBase->kb_MemFreeLists,(p),(s))


#ifdef SUPER
#define FREEMEM(p,s)	FreeMemToMemLists(KernelBase->kb_MemFreeLists,p,s)
#define ALLOCMEM(s,t)	AllocMemFromMemLists(KernelBase->kb_MemFreeLists,s,t)
#define FreeMem(p,s)	FreeMemToMemLists(KernelBase->kb_MemFreeLists,p,s)
#define AllocMem(s,t)	AllocMemFromMemLists(KernelBase->kb_MemFreeLists,s,t)
#else
#define FREEMEM(p,s)	FreeMemToMemLists( \
			KernelBase->kb_CurrentTask->t_FreeMemoryLists,p,s)
#define ALLOCMEM(s,t)	AllocMemFromMemLists( \
			KernelBase->kb_CurrentTask->t_FreeMemoryLists,s,t)
#define FreeMem(p,s)	FreeMemToMemLists( \
			KernelBase->kb_CurrentTask->t_FreeMemoryLists,p,s)
#define AllocMem(s,t)	AllocMemFromMemLists( \
			KernelBase->kb_CurrentTask->t_FreeMemoryLists,s,t)
#endif
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

void __swi(KERNELSWI+13)	*AllocMemBlocks(int32 size, uint32 typebits);
Err __swi(KERNELSWI+20)	ControlMem(void *p,int32 size,int32 cmd,Item task);
int32 __swi(KERNELSWI+33) SystemScavengeMem(void);

extern int32 ScavengeMem(void);

extern int32 GetPageSize(uint32 typebits);

uint32 bankbits(void *p);
extern uint32 GetMemType(void *p);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 


/*
**	Structure passed to the AvailMem() library call.
*/
typedef struct MemInfo
{
	uint32  minfo_SysFree;     // Bytes free in system memory (free pages)
	uint32  minfo_SysLargest;  //  Largest span of free system memory pages
	uint32  minfo_TaskFree;    // Bytes of "odds & ends" in task memory
	uint32  minfo_TaskLargest; //  Largest "odd or end" in task memory
} MemInfo;


/* Each Memory Header contains information on the type */
/* of Memory it contains */
typedef struct MemHdr
{
	Node	memh_n;
	uint32	memh_Types;	/* MEMTYPE BITS */
	int32	memh_PageSize;	/* basic page size */
	uint32	memh_PageMask;
	int32	memh_VRAMPageSize;	/* basic page size */
	uint32	memh_VRAMPageMask;
	uint32	*memh_FreePageBits;	/* bit per block */
	uint8	*memh_MemBase;	/* range in these two values */
	uint8	*memh_MemTop;
	uint8	memh_FreePageBitsSize; /* in units (uint32s) */
	uint8	memh_PageShift;
	uint8	memh_VRAMPageShift;
	uint8	memh_Reserved;
} MemHdr;

/* define location, size flags */
#define MEMTYPE_ANY		(uint32)0

/* low 8 bits are optional fill value */
#define MEMTYPE_MASK		(uint32)0xffffff00
#define MEMTYPE_FILLMASK	(uint32)0x000000ff

#define MEMTYPE_FILL		(uint32)0x00000100 /* fill memory with value */
#define MEMTYPE_MYPOOL		(uint32)0x00000200 /* do not get more memory from system */
#define MEMTYPE_FROMTOP		(uint32)0x00004000 /* allocate from top */
#define MEMTYPE_TASKMEM		(uint32)0x00008000	/* internal use only */

/* memory type bits */
#define MEMTYPE_VRAM		(uint32)0x00010000 /* block must be in VRAM */
#define MEMTYPE_DMA		(uint32)0x00020000 /* accessable by hardware */
#define MEMTYPE_CEL		(uint32)0x00040000 /* accessable by sprite engine */
#define MEMTYPE_DRAM		(uint32)0x00080000 /* block must not be in VRAM */

#define MEMTYPE_AUDIO		(uint32)0x00100000 /* accessable by audio  */
#define MEMTYPE_DSP		(uint32)0x00200000 /* accessable by DSP */

/* alignment bits */
#define MEMTYPE_INPAGE		(uint32)0x01000000 /* no page crossings */
#define MEMTYPE_STARTPAGE	(uint32)0x02000000 /* block must start on VRAM boundary */
#define MEMTYPE_SYSTEMPAGESIZE	(uint32)0x04000000 /* block must start on system page boundary */

	/* MEMTYPE_VRAM sets PAGESIZE=VRAM PAGESIZE */
	/* otherwise PAGESIZE = physical page size of memory protect system */

#define MEMF_ALIGNMENTS	(MEMTYPE_INPAGE|MEMTYPE_STARTPAGE)

/* VRAM bank select bits */
#define MEMTYPE_BANKSELECT	(uint32)0x40000000 /* bank required */
#define MEMTYPE_BANKSELECTMSK	(uint32)0x30000000 /* 2 max banks */
#define MEMTYPE_BANK1		(uint32)0x10000000 /* first bank */
#define MEMTYPE_BANK2		(uint32)0x20000000 /* second bank */

#define GETBANKBITS(a) ((MEMTYPE_BANKSELECT|MEMTYPE_BANKSELECTMSK)&GetMemType(a))
#define GetBankBits(a) ((MEMTYPE_BANKSELECT|MEMTYPE_BANKSELECTMSK)&GetMemType(a))

/* undefined memtype bits */
/* It is an error to set any of these bits */
/* 0x0CF87C00 */

/* ControlMem commands */
#define MEMC_NOWRITE	1	/* make memory unwritable for a task */
#define MEMC_OKWRITE	2	/* make memory writable for a task */
#define MEMC_GIVE	3	/* give memory away, or back to system */
#define MEMC_SC_GIVE    4       /* special give for scavengemem */

typedef struct MemList
{
	Node meml_n;	/* need to link these together */
	uint32 meml_Types;	/* copy of meml_mh->memh_Types;*/
	uint32 *meml_OwnBits;	/* mem we own */
	uint32 *meml_WriteBits;	/* mem we can write to */
	MemHdr *meml_MemHdr;
	List *meml_l;
	Item   meml_Sema4;
	uint8 meml_OwnBitsSize;	/* in uint32s (fd_set) */
	uint8 meml_Reserved[3];
} MemList;

/* Note: memh_Types and meml_Types must be at the same offset */
/* in their data structures */

/* routines for managing private pools (lists) of memory */
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

struct MemList *AllocMemList(void *p,char *name);
void FreeMemList(struct MemList *ml);
void *AllocMemFromMemList(struct MemList *ml,int32 size, uint32 memflags);
void FreeMemToMemList(struct MemList *ml,void *p, int32 size);

void availMem( MemInfo *minfo, uint32 flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __MEM_H */
