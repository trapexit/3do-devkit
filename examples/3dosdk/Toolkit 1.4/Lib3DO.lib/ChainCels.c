/*****************************************************************************
 *	File:			ChainCels.c
 *
 *	Contains:		Routines to chain together lists of cels.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	02/15/94  Ian	Changed all routines so that NULL pointers are no longer
 *					reported as an error.  Now, each routine tries to do 
 *					something more or less useful with NULL cel list pointers.
 *					Mostly, this helps ChainCelsAtHead() -- having it accept
 *					a NULL oldlist pointer simplifies the caller's logic for
 *					building a linked list of cels from scratch.
 *
 *	10/10/93  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	The LinkCel() function links together two individual cels, or adds a new
 *	cel to the head of a list of cels.  It cannot, however, add cels to the
 *	end of a list, or link together two lists of cels.  These routines add
 *	that functionality to the library.
 ****************************************************************************/

#include "CelUtils.h"

/*------------------------------------------------------------------------------
 * LastCelInList()
 *	Return a pointer to the last cel in a linked list of one or more cels.
 *----------------------------------------------------------------------------*/

CCB * LastCelInList(CCB *list)
{

	if (list == NULL) {
		return NULL;
	}

	while (!IS_LASTCEL(list)) {
		list = CEL_NEXTPTR(list);
	}

	return list;
}

/*------------------------------------------------------------------------------
 * ChainCelsAtTail()
 *	Attach a list of one or more new cels to the end of a list of one or 
 *	more cels.  Returns a pointer to the last cel in the resulting list.
 *	(The returned pointer is suitable for passing back to this routine in
 *	subsequent calls; doing so will eliminate long list searches to find
 *	the end of the existing list.)
 *
 *	NULL pointers are allowed for both the old and new list pointers.  If the
 *	new list pointer is NULL, a pointer to the last cel in the old list is
 *	returned.  If the old list pointer is NULL, a pointer to the last cel in
 *	the new list is returned. If this is good for anything I can't think of it.
 *----------------------------------------------------------------------------*/

CCB * ChainCelsAtTail(CCB *oldlist, CCB *newlist)
{
	CCB *	last;

	last = LastCelInList(oldlist);
	
	if (newlist == NULL) {
		return last;
	} 
	
	if (last != NULL) {
		last->ccb_NextPtr = newlist;
		last->ccb_Flags   = (last->ccb_Flags & ~CCB_LAST) | CCB_NPABS;
	}
	
	return LastCelInList(newlist);
}

/*------------------------------------------------------------------------------
 * ChainCelsAtHead()
 *	Attach a list of one or more new cels to the head of a list of one or more
 *	more cels.  Returns a pointer to the head of the resulting list.
 *
 *	NULL pointers are valid for both the old and new lists.  This allows things
 *	like "listHead = ChainCelsAtHead(listHead, newCel);" without needing any
 *	special logic to handle the first cel added (IE, when listHead is NULL).
 *	Using a NULL newlist pointer doesn't have any such utility, but it is 
 *	accepted, and works as basically a no-op.
 *----------------------------------------------------------------------------*/

CCB * ChainCelsAtHead(CCB *oldlist, CCB *newlist)
{
	CCB *	last;	

	if (oldlist == NULL) {
		return newlist;
	}

	if (newlist == NULL) {
		return oldlist;
	}
	
	last = LastCelInList(newlist);
	last->ccb_NextPtr = oldlist;
	last->ccb_Flags   = (last->ccb_Flags & ~CCB_LAST) | CCB_NPABS;
	
	return newlist;
}

