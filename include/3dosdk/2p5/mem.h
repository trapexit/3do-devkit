#ifndef __MEM_H
#define __MEM_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: mem.h,v 1.53 1994/11/18 02:34:59 vertex Exp $
**
**  Kernel memory management definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __LIST_H
#include "list.h"
#endif

#ifndef __ITEM_H
#include "item.h"
#endif

#ifndef __KERNEL_H
#include "kernel.h"
#endif

#ifndef SUPER
#include "stdlib.h"	/* temporarily left for application src compatibility */
#endif			/* malloc(),free() in stdlib.h used to be defined here*/


/****************************************************************************/


/* Structure passed to AvailMem() */
typedef struct MemInfo
{
	uint32  minfo_SysFree;     /* Bytes free in system memory (free pages) */
	uint32  minfo_SysLargest;  /* Largest span of free system memory pages */
	uint32  minfo_TaskFree;    /* Bytes of "odds & ends" in task memory    */
	uint32  minfo_TaskLargest; /* Largest "odd or end" in task memory      */
} MemInfo;


/****************************************************************************/


/* Memory Headers provide information on the type of memory they contain */
typedef struct MemHdr
{
        Node     memh_n;
        uint32   memh_Types;            /* MEMTYPE BITS              */
        int32    memh_PageSize;         /* basic page size           */
        uint32   memh_PageMask;
        int32    memh_VRAMPageSize;     /* basic page size           */
        uint32   memh_VRAMPageMask;
        uint32  *memh_FreePageBits;     /* bit per block             */
        uint8   *memh_MemBase;          /* range in these two values */
        uint8   *memh_MemTop;
        uint8    memh_FreePageBitsSize; /* in units (uint32s)        */
        uint8    memh_PageShift;
        uint8    memh_VRAMPageShift;
} MemHdr;


/****************************************************************************/


/* define location, size flags */
#define MEMTYPE_ANY		(uint32)0

/* low 8 bits are optional fill value */
#define MEMTYPE_MASK		(uint32)0xffffff00
#define MEMTYPE_FILLMASK	(uint32)0x000000ff

#define MEMTYPE_FILL		(uint32)0x00000100 /* fill memory with value */
#define MEMTYPE_MYPOOL		(uint32)0x00000200 /* do not get more memory from system */
#define MEMTYPE_FROMTOP		(uint32)0x00004000 /* allocate from top      */
#define MEMTYPE_TASKMEM		(uint32)0x00008000 /* internal use only      */
#define MEMTYPE_TRACKSIZE	(uint32)0x00400000 /* track allocation size  */

/* memory type bits */
#define MEMTYPE_DMA		(uint32)0x00020000 /* accessable by hardware    */
#define MEMTYPE_CEL		(uint32)0x00040000 /* accessable by cel engine  */
#define MEMTYPE_DRAM		(uint32)0x00080000 /* block must not be in VRAM */
#define MEMTYPE_AUDIO		(uint32)0x00100000 /* accessible by audio       */
#define MEMTYPE_DSP		(uint32)0x00200000 /* accessible by DSP         */
#define MEMTYPE_VRAM		(uint32)0x00010000 /* block must be in VRAM     */
#define MEMTYPE_VRAM_BANK1	(MEMTYPE_VRAM | MEMTYPE_BANKSELECT | MEMTYPE_BANK1)
#define MEMTYPE_VRAM_BANK2	(MEMTYPE_VRAM | MEMTYPE_BANKSELECT | MEMTYPE_BANK2)

/* alignment bits */
#define MEMTYPE_INPAGE		(uint32)0x01000000 /* no page crossings */
#define MEMTYPE_STARTPAGE	(uint32)0x02000000 /* block must start on VRAM boundary */
#define MEMTYPE_SYSTEMPAGESIZE	(uint32)0x04000000 /* block must start on system page boundary */

/* If MEMTYPE_VRAM is set, PAGESIZE = (VRAM PAGESIZE),
 *              otherwise, PAGESIZE = (physical page size of mem protect system)
 */

#define MEMF_ALIGNMENTS	(MEMTYPE_INPAGE|MEMTYPE_STARTPAGE)

/* VRAM bank select bits */
#define MEMTYPE_BANKSELECT	(uint32)0x40000000   /* bank required */
#define MEMTYPE_BANKSELECTMSK	(uint32)0x30000000   /* 2 max banks   */
#define MEMTYPE_BANK1		(uint32)0x10000000   /* first bank    */
#define MEMTYPE_BANK2		(uint32)0x20000000   /* second bank   */


/****************************************************************************/


/* ControlMem() commands */
#define MEMC_NOWRITE	1	/* make memory unwritable for a task   */
#define MEMC_OKWRITE	2	/* make memory writable for a task     */
#define MEMC_GIVE	3	/* give memory away, or back to system */
#define MEMC_SC_GIVE    4       /* special give for ScavengeMem()      */


/****************************************************************************/


typedef struct MemList
{
        Node    meml_n;           /* need to link these together */
        uint32  meml_Types;       /* copy of meml_mh->memh_Types */
        uint32 *meml_OwnBits;     /* memory we own               */
        uint32 *meml_WriteBits;   /* memory we can write to      */
        MemHdr *meml_MemHdr;
        List   *meml_l;
        Item    meml_Sema4;
        uint8   meml_OwnBitsSize; /* in uint32s (fd_set)         */
        uint8   meml_Reserved[3];
        List   *meml_AlignedTrackSize;
} MemList;


/*****************************************************************************/


/* debugging control flags for CreateMemDebug() */
#define MEMDEBUGF_ALLOC_PATTERNS       (1 << 0) /* fill memory when allocating  */
#define MEMDEBUGF_FREE_PATTERNS        (1 << 1) /* fill memory when freeing     */
#define MEMDEBUGF_PAD_COOKIES          (1 << 2) /* put cookies around allocs    */
#define MEMDEBUGF_DEBUG_ON_ERRORS      (1 << 3) /* invoke debugger on errors    */
#define MEMDEBUGF_ALLOW_OWNER_SWITCH   (1 << 4) /* let an alien thread free mem */
#define MEMDEBUGF_CHECK_ALLOC_FAILURES (1 << 5) /* complain when alloc fails    */
#define MEMDEBUGF_KEEP_TASK_DATA       (1 << 6) /* always keep stats around     */
#define MEMDEBUGF_USE_VRAM             (1 << 7) /* keep tracking info in VRAM   */

/* fill patterns used by memdebug subsystem */
#define MEMDEBUG_ALLOC_PATTERN 0xac0debad   /* fill value after allocating */
#define MEMDEBUG_FREE_PATTERN  0xfc0debad   /* fill value before freeing   */

/* types of memory calls */
typedef enum MemDebugCalls
{
    MEMDEBUG_CALL_ALLOCMEM,
    MEMDEBUG_CALL_ALLOCMEMFROMMEMLISTS,
    MEMDEBUG_CALL_MALLOC,
    MEMDEBUG_CALL_CALLOC,
    MEMDEBUG_CALL_REALLOC,
    MEMDEBUG_CALL_ALLOCMEMBLOCKS,

    MEMDEBUG_CALL_FREEMEM,
    MEMDEBUG_CALL_FREEMEMTOMEMLISTS,
    MEMDEBUG_CALL_FREE
} MemDebugCalls;

/* error codes returned by CreateMemDebug() */

/* Bad tag supplied for args parameter   */
#define MEMDEBUG_ERR_BADTAG    MakeLErr(ER_MEMDEBUG,ER_SEVERE,ER_C_STND,ER_BadTagArg)

/* Bad bit set in controlFlags parameter */
#define MEMDEBUG_ERR_BADFLAGS  MakeLErr(ER_MEMDEBUG,ER_SEVERE,ER_C_NSTND,1)


/****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/****************************************************************************/


#ifdef MEMDEBUG

#define AllocMemFromMemLists(l,s,t) AllocMemFromMemListsDebug(l,s,t,__FILE__,__LINE__,MEMDEBUG_CALL_ALLOCMEMFROMMEMLISTS)
#define FreeMemToMemLists(l,p,s)    FreeMemToMemListsDebug(l,p,s,__FILE__,__LINE__,MEMDEBUG_CALL_FREEMEMTOMEMLISTS)
#define AllocMemBlocks(s,t)         AllocMemBlocksDebug(s,t,__FILE__,__LINE__)

Err CreateMemDebug(uint32 controlFlags, const TagArg *args);
Err DeleteMemDebug(void);
Err DumpMemDebug(const TagArg *args);
Err SanityCheckMemDebug(const TagArg *args);
void *AllocMemFromMemListsDebug(List *l, uint32 memSize, uint32 memFlags, const char *sourceFile, uint32 sourceLine, MemDebugCalls call);
void FreeMemToMemListsDebug(List *l, void *mem, uint32 memSize, const char *sourceFile, uint32 sourceLine, MemDebugCalls call);
void *AllocMemBlocksDebug(int32 size, uint32 typebits, const char *sourceFile, uint32 sourceLine);

#else

void *AllocMemFromMemLists(List *l, int32 size, uint32 typebits);
void FreeMemToMemLists(List *l, void *p, int32 size);
void __swi(KERNELSWI+13) *AllocMemBlocks(int32 size, uint32 typebits);

#define CreateMemDebug(cf,a)                   (0)
#define DeleteMemDebug()                       (0)
#define DumpMemDebug(a)                        (0)
#define SanityCheckMemDebug(a)                 (0)
#define AllocMemFromMemListsDebug(l,s,t,f,n,c) AllocMemFromMemLists(l,s,t)
#define FreeMemToMemListsDebug(l,p,s,f,n,c)    FreeMemToMemLists(l,p,s)
#define AllocMemBlocksDebug(s,t,f,n)           AllocMemBlocks(s,t)

#endif

Err __swi(KERNELSWI+20)	  ControlMem(void *p, int32 size, int32 cmd, Item task);
int32 __swi(KERNELSWI+33) SystemScavengeMem(void);

int32 ScavengeMem(void);
int32 GetPageSize(uint32 typebits);
int32 GetMemAllocAlignment(uint32 typebits);
uint32 GetMemType(void *p);
void availMem(MemInfo *minfo, uint32 flags);
int32 GetMemTrackSize(const void *p);

/* routines for managing private pools (lists) of memory */
MemList *AllocMemList(const void *p, const char *name);
void FreeMemList(MemList *ml);
void *AllocMemFromMemList(MemList *ml, int32 size, uint32 memflags);
void FreeMemToMemList(MemList *ml, void *p, int32 size);

/* sanity check memory pointers */
bool IsMemReadable(const void *p, int32 size);
bool IsMemWritable(const void *p, int32 size);


/****************************************************************************/


#ifdef __cplusplus
}
#endif /* __cplusplus */


/****************************************************************************/


/* Useful macros */

#undef USER_ALLOCMEM
#undef USER_FREEMEM

#define USER_ALLOCMEM(s,t)	AllocMemFromMemLists(CURRENTTASK->t_FreeMemoryLists,(s),(t))
#define USER_FREEMEM(p,s)	FreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,(p),(s))
#define UserAllocMem(s,t)	AllocMemFromMemLists(CURRENTTASK->t_FreeMemoryLists,(s),(t))
#define UserFreeMem(p,s)	FreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,(p),(s))

#define AllocMem(s,t)	AllocMemFromMemLists(CURRENTTASK->t_FreeMemoryLists,s,t)
#define FreeMem(p,s)	FreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,p,s)
#define ALLOCMEM(s,t)	AllocMemFromMemLists(CURRENTTASK->t_FreeMemoryLists,s,t)
#define FREEMEM(p,s)	FreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,p,s)

#define GETBANKBITS(a) ((MEMTYPE_BANKSELECT|MEMTYPE_BANKSELECTMSK)&GetMemType(a))
#define GetBankBits(a) ((MEMTYPE_BANKSELECT|MEMTYPE_BANKSELECTMSK)&GetMemType(a))
#define AvailMem(m,f)  availMem(m,f)


/****************************************************************************/


#endif	/* __MEM_H */
