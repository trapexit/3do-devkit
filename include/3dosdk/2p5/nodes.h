#ifndef __NODES_H
#define __NODES_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: nodes.h,v 1.31 1994/09/14 01:35:14 sdas Exp $
**
**  Kernel node definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

/* predefined NODE SUBSYS TYPES */

#define NST_KERNEL	1	/* system core */
#define NST_GRAPHICS	2	/* system graphics */
#define NST_FILESYS	3	/* file access */
#define NST_AUDIO	4	/* system audio */
#define NST_MATH	5	/* advanced math */
#define NST_NETWORK	6	/* network functions */
#define NST_JETSTREAM	7	/* data streaming */
#define NST_AV		8	/* a/v folio */
#define NST_INTL	9	/* international folio */
#define NST_CONNECTION	10	/* datacom, connection folio */
#define NST_SECURITY	15	/* system security */

#define	NST_SYSITEMMASK	0xff	/* Mask for itemnodes with system privileges */

/* For dynamically defined NODE SUBSYS TYPES */
/* define mask to get subsys type from the node item */
/* NOTE: nodes with system privileges must have item numbers < 256 */
#define	NST_ITEMMASK	ITEM_INDX_MASK

#define	NODETOSUBSYS(a)	((uint32)(((ItemNode *)(a))->n_Item & NST_ITEMMASK))

#define	NodeToSubsys(a)	NODETOSUBSYS(a)

#define MKNODEID(a,b)	(int32)( ((a)<<8) | (b))
#define SUBSYSPART(n)	((uint32)(((n)>>8) & 0xfff))
#define NODEPART(n)	((uint8)((n) & 0xff))

#define MkNodeID(a,b)	(int32)( ((a)<<8) | (b))
#define SubsysPart(n)	((uint32)(((n)>>8) & 0xfff))
#define NodePart(n)	((uint8)((n) & 0xff))

/* Standard Node structure */
typedef struct Node
{
	struct Node *n_Next;	/* pointer to next node in list */
	struct Node *n_Prev;	/* pointer to previous node in list */
	uint8 n_SubsysType;	/* what folio manages this node */
	uint8 n_Type;		/* what type of node for the folio */
	uint8 n_Priority;	/* queueing priority */
	uint8 n_Flags;		/* misc flags, see below */
	int32  n_Size;		/* total size of node including hdr */
	/* Optional part starts here */
	char *n_Name;		/* ptr to null terminated string or NULL */
} Node, *NodeP;

/* Node structure used when the Name is not needed */
typedef struct NamelessNode
{
	struct NamelessNode *n_Next;
	struct NamelessNode *n_Prev;
	uint8 n_SubsysType;
	uint8 n_Type;
	uint8 n_Priority;
	uint8 n_Flags;
	int32 n_Size;
} NamelessNode, *NamelessNodeP;

typedef struct ItemNode
{
	struct ItemNode *n_Next; /* pointer to next itemnode in list */
	struct ItemNode *n_Prev; /* pointer to previous itemnode in list */
	uint8 n_SubsysType;	/* what folio manages this node */
	uint8 n_Type;		/* what type of node for the folio */
	uint8 n_Priority;	/* queueing priority */
	uint8 n_Flags;		/* misc flags, see below */
	int32 n_Size;		/* total size of node including hdr */
	char *n_Name;		/* ptr to null terminated string or NULL */
	uint8 n_Version;	/* version of of this itemnode */
	uint8 n_Revision;	/* revision of this itemnode */
	uint8 n_FolioFlags;	/* flags for this item's folio */
	uint8 n_ItemFlags;	/* additional system item flags */
	Item  n_Item;		/* ItemNumber for this data structure */
	Item  n_Owner;		/* creator, present owner, disposer */
	void *n_ReservedP;	/* Reserved pointer */
} ItemNode, *ItemNodeP;

/* Version and Revision are for that Item itself */
/* The kernels version and revision number will be stored in */
/* only one place, the Kernel data structure, which is a Folio */
/* Other folios will also have their own versions and revisions, independent */
/* of the kernels */

/* n_Flag bits */
/* bits 4-7 are reserved for the system */
/* bits 0-3 are available for node specific use */
#define NODE_RSRV1	0x40
#define NODE_SIZELOCKED	0x20	/* The size of this item has been locked down */
#define NODE_ITEMVALID	0x10	/* This is an ItemNode */
#define NODE_NAMEVALID	0x80	/* This node's namefield is valid */

/* n_ItemFlags bits */
#define ITEMNODE_NOTREADY	  0x80	/* item is not yet ready for use     */
#define ITEMNODE_CONSTANT_NAME	  0x40	/* constant name pointer for ROM use */
#define ITEMNODE_PRIVILEGED       0x20  /* privileged item                   */
#define ITEMNODE_UNIQUE_NAME	  0x10	/* item has a unique name in its type*/

/* FolioFlags */
#define FF_DEBUG1	1	/* turn on level1 debugging */
#define FF_DEBUG2	2	/* turn on level2 debugging */

/* Node structure used for linking only */
typedef struct MinNode
{
	struct MinNode *n_Next;
	struct MinNode *n_Prev;
} MinNode;

#endif /* __NODES_H */
