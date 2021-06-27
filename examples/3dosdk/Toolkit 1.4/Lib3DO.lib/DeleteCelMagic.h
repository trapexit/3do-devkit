/*****************************************************************************
 *	File:		DeleteCelMagic.h
 *
 *	Contains:	Private Lib3DO internals header to support DeleteCel.
 *
 *	Written by: Ian Lepore
 *
 *	Copyright:	© 1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History (most recent first):
 *
 *	03/03/94  Ian	New today.
 * 
 *	Implementation notes:
 *
 *	This file is private to Lib3DO internals, and should not appear in the
 *	normal 3DO:Interfaces:Includes folder.  As of this release, no client
 *	code should be using this header file, since it's all intended to be
 *	private to the library, and thus could change without notice.
 *
 *	See the notes in DeleteCelMagic.c if you need to use this stuff in 
 *	new Lib3DO modules.
 ****************************************************************************/

#pragma include_only_once

enum {
	DELETECELMAGIC_CCB_ONLY		= 0x0de11CCB,	// <- 'dellCCB' (sorta). cute, huh?
	DELETECELMAGIC_DATA_ONLY,
	DELETECELMAGIC_CCB_AND_DATA,
	DELETECELMAGIC_CALLBACK
};

extern struct CCB *	AllocMagicCel_(uint32 extraBytes, uint32 freeMagic, void *freeData, void *creatorData);
extern void			FreeMagicCel_(struct CCB *cel);	// callbacks use this
extern void			ModifyMagicCel_(struct CCB *cel, uint32 freeMagic, void *freeData, void *creatorData);

extern CCB *		DeleteCel(CCB *cel);			// also prototyped in Parse3DO.h (for now)
extern CCB *		DeleteCelList(CCB *celList);	// also prototyped in Parse3DO.h (for now)
