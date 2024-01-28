#ifndef __LIST_H
#define __LIST_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: list.h,v 1.16 1994/09/21 19:45:43 vertex Exp $
**
**  Kernel list management definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __NODES_H
#include "nodes.h"
#endif

typedef struct Link
{
	struct Link *flink;	/* forward (next) link */
	struct Link *blink;	/* backward (prev) link */
} Link;

typedef union ListAnchor
{
    struct			/* ptr to first node */
    {				/* anchor for lastnode */
	Link links;
	Link *filler;
    } head;
    struct
    {
	Link *filler;
	Link links;		/* ptr to lastnode */
    } tail;			/* anchore for firstnode */
} ListAnchor;

typedef struct List
{
	Node l;			/* A list is a node itself */
	ListAnchor ListAnchor;	/* Anchor point for list of nodes */
} List, *ListP;

/* return the first node on the list or the anchor if empty */
#define FIRSTNODE(l)	((Node *)((l)->ListAnchor.head.links.flink))
#define FirstNode(l)	((Node *)((l)->ListAnchor.head.links.flink))

/* return the last node on the list or the anchor if empty */
#define LASTNODE(l)	((Node *)((l)->ListAnchor.tail.links.blink))
#define LastNode(l)	((Node *)((l)->ListAnchor.tail.links.blink))

/* define for finding end while using flink */
#define ISNODE(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.blink))
#define IsNode(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.blink))

/* define for finding end while using blink */
#define ISNODEB(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.flink))
#define IsNodeB(l,n)	(((Link**)(n)) != &((l)->ListAnchor.head.links.flink))

#define ISLISTEMPTY(l)	(!ISNODE((l),FIRSTNODE(l)))
#define IsListEmpty(l)	(!IsNode((l),FirstNode(l)))

#define ISEMPTYLIST(l)	(!ISNODE((l),FIRSTNODE(l)))
#define IsEmptyList(l)	(!IsNode((l),FirstNode(l)))

#define NEXTNODE(n)	(((Node *)(n))->n_Next)
#define NextNode(n)	(((Node *)(n))->n_Next)
#define PREVNODE(n)	(((Node *)(n))->n_Prev)
#define PrevNode(n)	(((Node *)(n))->n_Prev)

/* Scan list l, for nodes n, of type t */
/* NOTE: we need the node to be in the list at the end  of the */
/*       block following SCNALIST for the NEXTNODE computation */
#define SCANLIST(l,n,t) for (n=(t *)FIRSTNODE(l);ISNODE(l,n);n=(t *)NEXTNODE(n))
#define ScanList(l,n,t) for (n=(t *)FIRSTNODE(l);ISNODE(l,n);n=(t *)NEXTNODE(n))

/* Scan a list backward, from tail to head */
#define SCANLISTB(l,n,t) for (n=(t *)LASTNODE(l);ISNODEB(l,n);n=(t *)PREVNODE(n))
#define ScanListB(l,n,t) for (n=(t *)LASTNODE(l);ISNODEB(l,n);n=(t *)PREVNODE(n))

/* A macro to let you define statically initialized lists. You use this macro
 * like this:
 *
 *   static List l = INITLIST(l,"List Name");
 *
 * This makes a List variable "l" which is ready for use without having to
 * call InitList() on it.
 */
#define INITLIST(l,n) {{NULL,NULL,KERNELNODE,LISTNODE,0,(uint8)NODE_NAMEVALID,sizeof(List),n},\
                       {(Link *)&l.ListAnchor.tail.links.flink,NULL,(Link *)&l.ListAnchor.head.links.flink}}


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* set the priority of a node in a list */
extern uint8 SetNodePri(Node *n, uint8 newpri);	/* returns old pri */

/* insert a node in the prioritized list starting from the tail */
extern void InsertNodeFromTail(List *l, Node *n);

/* insert a node in the prioritized list starting from the head */
extern void InsertNodeFromHead(List *l, Node *n);

/* insert a node in the prioritized list using (*f)() as a cmp func */
/* starts at beginning of list and traverses till the end */
/* should return true if n should be inserted before this node m */
/* m is already in the list, n is the new node to be inserted */
extern void UniversalInsertNode(List *l, Node *n, bool (*f)(Node *n,Node *m));

/* Remove the first node on the list, return a ptr to it */
extern Node *RemHead(List *l);

/* Remove the last node on the list, return a ptr to it */
extern Node *RemTail(List *l);

/* Add a node to the end of the list, no priority */
extern void AddTail(List *l, Node *n);

/* Add a node to the head of the list, no priority */
extern void AddHead(List *l, Node *n);

/* remove a node from a list */
extern void RemNode( Node *n);

/* Initialize a list to the empty list */
extern void InitList(List *l, const char *name);

/* find a Node in a List with the name of <name> */
extern Node *FindNamedNode(const List *l, const char *name);

/* convenience routines */
extern uint32 GetNodeCount(const List *l);
extern void InsertNodeBefore(Node *oldNode, Node *newNode);
extern void InsertNodeAfter(Node *oldNode, Node *newNode);
extern Node *FindNodeFromHead(const List *l, uint32 position);
extern Node *FindNodeFromTail(const List *l, uint32 position);
extern int32 GetNodePosFromHead(const List *l, const Node *n);
extern int32 GetNodePosFromTail(const List *l, const Node *n);

/* debugging aid */
extern void DumpNode(const Node *n, const char *banner);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#endif /* __LIST_H */
