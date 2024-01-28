#ifndef __ITEM_H
#define __ITEM_H

/*****

$Id: item.h,v 1.4 1994/01/21 02:15:22 limes Exp $

$Log: item.h,v $
 * Revision 1.4  1994/01/21  02:15:22  limes
 * recover from rcs bobble:
 *
 * +  * Revision 1.5  1994/01/20  02:00:09  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.4  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 *
 * Revision 1.5  1994/01/20  02:00:09  sdas
 * C++ compatibility - updated
 *
 * Revision 1.4  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.3  1993/07/08  21:48:22  andy
 * added constant name tag
 *
 * Revision 1.2  1993/05/31  00:46:49  dale
 * new item table architecture.
 *
 * Revision 1.1  1993/03/16  06:43:48  dale
 * Initial revision
 *
 * Revision 1.14  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.13  1992/12/01  22:49:59  dale
 * new OpenItem void * parm
 *
 * Revision 1.12  1992/12/01  04:30:42  dale
 * new FindItem/OpenItem
 *
 * Revision 1.11  1992/11/25  06:17:51  dale
 * removes LocateItem once and forall
 *
 * Revision 1.10  1992/11/25  00:01:25  dale
 * new msgs
 *
 * Revision 1.9  1992/10/24  01:37:21  dale
 * fix id
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

/* The programmer interface is really done via Items */
/* which are passed to the system, the system keeps */
/* a database of Items and their associated structures */
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

Item __swi(KERNELSWI+0) CreateSizedItem(int32 ctype,TagArg *p,int32 size);

/* used for testing arm cc */
/*Item __swi(kernelitem & 0xff) CreateSizedItem(int32 ctype,void *p,int32 size);*/

Err __swi(KERNELSWI+3) DeleteItem(Item i);
Item __swi(KERNELSWI+4) FindItem(int32 ctype,TagArg *tp);
Err __swi(KERNELSWI+8) CloseItem(Item i);
Item __swi(KERNELSWI+5) OpenItem(Item FoundItem, void *args);
int32 __swi(KERNELSWI+10) SetItemPri(Item i,uint8 newpri);
Err __swi(KERNELSWI+28) SetItemOwner(Item i,Item newOwner);
int32 __swi(KERNELSWI+7)        LockItem(Item s,uint32 flags);
Err __swi(KERNELSWI+6)        UnlockItem(Item s);

/* flags for LockItem */
#define SEM_WAIT	1
#define SEM_SHAREDREAD	2

/* helper routines */
Item FindNamedItem(int32 ctype, char *name);
Item FindVersionedItem(int32 ctype, char *name, uint8 vers, uint8 rev);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#define CreateItem(ct,p)	CreateSizedItem((ct),(p),0)

/* common TagArg commands for all Items */
/* All system routines that create Items must assign their */
/* TAGS after ITEM_TAG_LAST */
enum item_tags
{
	TAG_ITEM_END = TAG_END,	/* 0 */
	TAG_ITEM_NAME,		/* 1 */
	TAG_ITEM_PRI,		/* 2 */
	TAG_ITEM_VERSION,	/* 3 */
	TAG_ITEM_REVISION,	/* 4 */
	TAG_ITEM_CONSTANT_NAME, /* 5 */
	TAG_ITEM_RESERVED6,
	TAG_ITEM_RESERVED7,
	TAG_ITEM_RESERVED8,
	TAG_ITEM_RESERVED9,
	TAG_ITEM_LAST = TAG_ITEM_RESERVED9
};

typedef struct ItemEntry
{
	void	*ie_ItemAddr;
	uint32	ie_ItemInfo;
} ItemEntry;

#define	ITEM_GEN_MASK	0x7fff0000
#define ITEM_INDX_MASK	0x00000fff
#define ITEM_FLGS_MASK	0x0000f000
  
#ifdef  __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 

void *LookupItem(Item i);
void *CheckItem(Item i,uint8 ftype,uint8 ntype);
  
#ifdef  __cplusplus 
}
#endif  /* __cplusplus */ 

#endif /* __ITEM_H */

