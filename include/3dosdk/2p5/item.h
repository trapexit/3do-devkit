#ifndef __ITEM_H
#define __ITEM_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: item.h,v 1.17 1994/10/07 20:13:33 vertex Exp $
**
**  Kernel item management definitions
**
**  The programmer interface is done via Items, which are passed to the system
**  instead of direct pointers. The system keeps a database of Items and their
**  associated structures
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

Item  __swi(KERNELSWI+0)  CreateSizedItem(int32 ctype,TagArg *p,int32 size);
Err   __swi(KERNELSWI+3)  DeleteItem(Item i);
Item  __swi(KERNELSWI+4)  FindItem(int32 ctype,TagArg *tp);
Err   __swi(KERNELSWI+8)  CloseItem(Item i);
Item  __swi(KERNELSWI+5)  OpenItem(Item foundItem, void *args);
int32 __swi(KERNELSWI+10) SetItemPri(Item i,uint8 newpri);
Err   __swi(KERNELSWI+28) SetItemOwner(Item i,Item newOwner);
int32 __swi(KERNELSWI+7)  LockItem(Item s,uint32 flags);
Err   __swi(KERNELSWI+6)  UnlockItem(Item s);
Item  __swi(KERNELSWI+36) FindAndOpenItem(int32 ctype,TagArg *tp);

/* varargs variants of some of the above */
Item CreateSizedItemVA(int32 ctype, int32 size, uint32 tags, ...);
Item CreateItemVA(int32 ctype, uint32 tags, ...);
Item FindItemVA(int32 ctype, uint32 tags, ...);
Item FindAndOpenItemVA(int32 ctype, uint32 tags, ...);

/* learn more about an item */
void *LookupItem(Item i);
void *CheckItem(Item i,uint8 ftype,uint8 ntype);
Err IsItemOpened(Item task, Item i);

/* convenience routines */
Item FindNamedItem(int32 ctype, const char *name);
Item FindVersionedItem(int32 ctype, const char *name, uint8 vers, uint8 rev);
Item FindAndOpenNamedItem(int32 ctype, const char *name);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#define CreateItem(ct,p) CreateSizedItem((ct),(p),0)

/* flags for LockItem() and LockSemaphore() */
#define SEM_WAIT	1
#define SEM_SHAREDREAD	2

/* Common tags for all Items, useful when creating any type of item. */
enum item_tags
{
	TAG_ITEM_END = TAG_END,	/* 0 */
	TAG_ITEM_NAME,		/* 1 */
	TAG_ITEM_PRI,		/* 2 */
	TAG_ITEM_VERSION,	/* 3 */
	TAG_ITEM_REVISION,	/* 4 */
	TAG_ITEM_CONSTANT_NAME, /* 5 */
	TAG_ITEM_UNIQUE_NAME,   /* 6 */
	TAG_ITEM_RESERVED7,
	TAG_ITEM_RESERVED8,
	TAG_ITEM_RESERVED9,
	TAG_ITEM_LAST = TAG_ITEM_RESERVED9
};


/* argc values passed to main() of demand-loaded items */
#define DEMANDLOAD_MAIN_CREATE -1
#define DEMANDLOAD_MAIN_DELETE -2


/*****************************************************************************/


#endif /* __ITEM_H */
