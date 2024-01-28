#ifndef __NODES_H
#define __NODES_H

/*****

$Id: nodes.h,v 1.21 1994/01/21 02:24:30 limes Exp $

$Log: nodes.h,v $
 * Revision 1.21  1994/01/21  02:24:30  limes
 * recover from rcs bobble
 *
 * Revision 1.22  1994/01/20  02:07:10  sdas
 * C++ compatibility - updated
 *
 * Revision 1.21  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.20  1993/12/17  20:16:01  dale
 * added NST_CABLE
 *
 * Revision 1.19  1993/10/25  18:08:38  dale
 * added NST_ types.
 *
 * Revision 1.18  1993/07/08  21:48:22  andy
 * added constant name flag
 *
 * Revision 1.1  1993/07/08  14:46:57  andy
 * Initial revision
 *
 * Revision 1.17  1993/06/30  06:44:26  andy
 * defined new flag for item creation
 *
 * Revision 1.16  1993/06/09  00:43:15  andy
 * changed one of the reserved Kernel flags to a NODE_SIZELOCKED flag, which
 * we use on objects where we control the size absolutely.
 *
 * Revision 1.15  1993/03/16  06:46:12  dale
 * api
 *
 * Revision 1.14  1993/03/04  22:25:29  dale
 * beginning global debug stuff
 *
 * Revision 1.13  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.12  1993/02/09  07:47:01  dale
 * added new MinNode
 *
 * Revision 1.11  1992/11/22  21:28:44  dale
 * fix warnings
 *
 * Revision 1.10  1992/10/29  02:59:17  dale
 * fixed an open comment
 *
 * Revision 1.9  1992/10/29  01:30:01  dale
 * added commends
 *
 * Revision 1.8  1992/10/27  03:09:22  dale
 * added comments to nodes struct
 *
 * Revision 1.7  1992/10/24  01:41:07  dale
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
#include "types.h"

/* predefined NODE SUBSYS TYPES */

#define NST_KERNEL	1	/* system core */
#define NST_GRAPHICS	2	/* system graphics */
#define NST_FILESYS	3	/* file access */
#define NST_AUDIO	4	/* system audio */
#define NST_MATH	5	/* advanced math */
#define NST_NETWORK	6	/* network functions */
#define NST_JETSTREAM	7	/* data streaming */
#define NST_CABLE	8	/* cable folio */
#define NST_SECURITY	15	/* system security */

#define MKNODEID(a,b)	(int32)( ((a)<<8) | (b))
#define SUBSYSPART(n)	((uint8)((n)>>8))
#define NODEPART(n)	((uint8)((n) & 0xff))

#define MkNodeID(a,b)	(int32)( ((a)<<8) | (b))
#define SubsysPart(n)	((uint8)((n)>>8))
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
#define ITEMNODE_NOTREADY	0x80	/* item is not yet ready for use */
#define ITEMNODE_CONSTANT_NAME	0x40	/* constant name pointer for ROM use */

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
