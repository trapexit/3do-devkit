#ifndef __LIST_H
#define __LIST_H

/*****

$Id: list.h,v 1.6 1994/03/11 23:48:00 sdas Exp $

$Log: list.h,v $
 * Revision 1.6  1994/03/11  23:48:00  sdas
 * SCANLIST macro added
 *
 * Revision 1.5  1994/01/28  23:20:18  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.4  1994/01/21  18:04:53  sdas
 * C++ compatibility - updated
 *
 * Revision 1.3  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.2  1993/03/26  18:25:52  dale
 * docs
 *
 * Revision 1.1  1993/03/16  06:43:34  dale
 * Initial revision
 *
 * Revision 1.13  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.12  1992/11/22  21:28:44  dale
 * fix warnings
 *
 * Revision 1.11  1992/10/27  03:20:01  dale
 * added comments
 *
 * Revision 1.10  1992/10/27  03:10:24  dale
 * removed extranseous #defile of LISTS_H
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

#include "types.h"
#include "nodes.h"

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

#define ISEMPTYLIST(l)	(!ISNODE((l),FIRSTNODE(l)))
#define IsEmptyList(l)	(!IsNode((l),FirstNode(l)))

#define NEXTNODE(n)	(((Node *)(n))->n_Next)
#define NextNode(n)	(((Node *)(n))->n_Next)
#define PREVNODE(n)	(((Node *)(n))->n_Prev)
#define PrevNode(n)	(((Node *)(n))->n_Prev)
  
/* Scan list l, for nodes n, of type t */
#define SCANLIST(l,n,t) for (n=(t *)FIRSTNODE(l);ISNODE(l,n);n=(t *)NEXTNODE(n))

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
extern void InitList(List *l, char *name);

/* find a Node in a List with the name of <name> */
extern Node *FindNamedNode(List *l, char *name);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#endif /* __LISTS_H */
