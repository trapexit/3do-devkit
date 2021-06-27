/*****************************************************************************
 *	File:		DeleteCelMagic.c
 *
 *	Contains:	Routines to implement universal DeleteCel() routine.
 *
 *	Written by: Ian Lepore
 *
 *	Copyright:	© 1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History (most recent first):
 *
 *	03/24/94  Ian	Modified the scheme to account for AA cels and other such
 *					constructs where a logical 'cel' may be a list of related
 *					cels, which may then become part of a larger list.
 *
 *	03/15/94  Ian	Added creatorData field to DeleteCelMagic structure.  Turns 
 *					out a file CCC chunk has 12 bytes of space before the CCB 
 *					that we can usurp.
 *
 *	03/03/94  Ian	New today.
 * 
 *	Implementation notes:
 *
 *	This is another one of those conspiracy-of-agreement scams.  Any routine in
 *	the library that creates cels can do so using this system so that DeleteCel()
 *	knows how to free up all resources acquired when the cel was created.
 *	This can help simplify an application, which can now link together lists of
 *	cels in any old way, then later walk the list freeing things without 
 *	having to worry about how they were created.
 *
 *	It works like this... Whenever a library routine creates a CCB, it places
 *	three magical longwords immediately ahead of the CCB in memory.  (The easiest
 *	way to do this is to allocate the CCB via AllocMagicCel_().)  The first
 *	longword is a magic number that validates the CCB as being a member of
 *	the conspiracy.  The 2nd longword is either a pointer to some data 
 *	to be freed along with the CCB when the cel is deleted, or a pointer to a 
 *	callback function that does the freeing.  The 3rd longword is either a pointer
 *	to the last cel in a group of logically related cels treated as a single entity,
 *	or it is some arbitrary data that means something to a callback deletor routine.
 *	The flavor of magic number used to validate the CCB determines the interpretation
 *	of the 2nd & 3rd  longwords.
 *
 *	The magic number flavors available and the action DeleteCel takes are:
 *	  -	CCB_ONLY		Free the CCB only.
 *	  -	DATA_ONLY		Free the associated data, but not the CCB.
 *	  -	CCB_AND_DATA	Free both the associated data and the CCB.
 *	  -	CALLBACK		Call the supplied custom delete routine.
 *	
 *	Using exactly three longwords preceeding the CCB allows LoadCel() to be a
 *	participant in this scheme.  A disk-based CCB is part of a CCC chunk,
 *	and a CCC happens to have exactly three longwords before the CCB part in
 *	the loaded file image.  This lets LoadCel() load a file image, parse it
 *	out to find the CCB pointer it wants to return within the file image, and
 *	modify the longwords in front of that CCB (which are no longer needed
 *	once the CCB has been located, since they're just the CCC chunk ID, size,
 *	and version fields).  LoadCel() uses the DATA_ONLY magic number, and the
 *	data pointer it supplies is a pointer to the loaded file image.
 *
 *	If a cel creator is creating just a CCB (perhaps attaching some static
 *	pixel data to it or something), it can use the CCB_ONLY magic number so
 *	that DeleteCel() just frees the CCB and nothing else.  
 *
 *	If a cel creator also allocates a pixel buffer or other memory resources
 *	associated with the cel, it can use the CCB_AND_DATA magic number, which 
 *	will free the memory resource pointed to by the free.data pointer, 
 *	and then free the CCB itself.  Of course, the memory in question
 *	must have been allocated via Malloc(), since DeleteCel() will call Free().
 *	With CCB_AND_DATA, an attempt is made to Free() the free.data memory
 *	only if the pointer is non-NULL.
 *
 *	And finally, if a cel creator allocates multiple memory resources, or for
 *	some other reason wants to be more fully involved in the deletion process,
 *	it can use the CALLBACK magic number and supply a pointer to the function
 *	to call back when DeleteCel() is called for that cel.  The callback function
 *	receives two parms: The CCB pointer for the cel being deleted, and the 
 *	creatorData for the cel.  The callback function is responsible for calling
 *	FreeMagicCel_() if the CCB was allocated via AllocMagicCel_().
 *
 *	The interpretation of the creatorData field depends on the flavor of magic
 *	number.  For CALLBACKs, it is just passed along to the callback routine.  
 *	For other flavors, it is a pointer to the last CCB in a group of related
 *	CCBs, if non-NULL.  This gets a bit tricky; it's all involved with supporting
 *	DeleteCelList() properly.  Sometimes a cel creator routine actually creates
 *	several cels and links them together, and this mini-list of cels makes up a 
 *	single logical entity.  (Anti-aliased cels are an example of this.)  The 
 *	application often treats this is a single cel, which is fine.  The problem
 *	arises when the application links such an entity into a larger list of cels,
 *	and then later calls DeleteCelList().  The deletion activity for the first
 *	CCB in the mini-list effectively frees multiple cels on the list, but the 
 *	DeleteCelList() routine is walking the NextPtr links doing deletions for
 *	every CCB it finds.  Once the first CCB deletion activity is done for a mini-
 *	list of related cels in the larger list, the DeleteCelList() routine would be
 *	walking NextPtr links through cels that have already been freed.  The memory
 *	could already have been given to and modified by another task, and we could
 *	end up in the middle of nowhere (or worse, end up crashing the machine).  To
 *	allow DeleteCelList() to work correctly, a creator that returns a mini-list of
 *	related cels can set the creatorData field to point to the last cel in the
 *	mini-list.  The internal_delete_cel() routine uses this to advance to the end
 *	of the mini-list, so that DeleteCelList() effectively jumps over the mini-list
 *	after it is deleted, and continues processing with any other cels linked into
 *	the larger list created by the application.  When CALLBACK style deletion is
 *	used, the creatorData field is already used for callback data.  In this case,
 *	the callback routine is responsible for returning a pointer to the next CCB
 *	following the CCB(s) it deleted.  If the callback returns NULL, that indicates
 *	that no CCBs were linked into the list beyond the ones the callback deleted.
 *	NOTE that there is a very important subtle distinction here:  For non-callback
 *	deletion, the creatorData points to the last CCB in the mini-list (or is NULL);
 *	the internal_delete_cel() function calculates the next larger-list pointer by
 *	following NextPtr from the last CCB pointed to by creatorData.  For callback
 *	deletion, the following of NextPtr is done by the callback routine; the 
 *	callback's return value is not a pointer to the last CCB it deleted, it is a
 *	pointer to the next CCB in the larger list, or NULL if there is no next CCB.
 *
 *	Helpful hint for callbacks:  While there is only one longword available 
 *	for creatorData ahead of the CCB in the magic data area, the CCB could
 *	could have any number of extra longwords at the end of it, which might
 *	mean something to the callback routine.  The creator of the cel can
 *	simply allocate a bit more memory than is needed for the CCB, and hide
 *	its extra data there, then the callback routine can find it there at
 *	delete time and do something with it.  This is why the AllocMagicCel_()
 *	routine takes an extraBytes parm that says how much extra memory to
 *	allocate as part of the magic data/CCB allocation.  
 ****************************************************************************/

#include "CelUtils.h"
#include "DeleteCelMagic.h"
#include "UMemory.h"
#include "Debug3DO.h"

/*----------------------------------------------------------------------------
 * private datatypes
 *--------------------------------------------------------------------------*/

#define DATAMAGIC_DEAD		0x02DEAD02	// what UMemory.c uses to mark Free'd data

typedef struct DeleteCelMagic {
	uint32		magic;
	union {
		void *	data;
		CCB *	(*func)(void *creatorData, CCB *cel);
	} 			free;
	void *		creatorData;
	CCB			ccb;
} DeleteCelMagic;

#define DELETECELMAGIC_OFFSET_SIZE	((int32)(offsetof(DeleteCelMagic, ccb)))

#define MagicPtrFromCCBPtr(p)	(DeleteCelMagic *)(AddToPtr((p), -DELETECELMAGIC_OFFSET_SIZE))

/*----------------------------------------------------------------------------
 * 
 *--------------------------------------------------------------------------*/

void FreeMagicCel_(CCB *cel)
{
	DeleteCelMagic * mcel;

	if (cel) {
		mcel = MagicPtrFromCCBPtr(cel);
		mcel->magic = DATAMAGIC_DEAD; // lay in tombstone to help warn of double-deletions
		Free(mcel);
	}
}

/*----------------------------------------------------------------------------
 * 
 *--------------------------------------------------------------------------*/

CCB * AllocMagicCel_(uint32 extraBytes, uint32 freeMagic, void *freeData, void *creatorData)
{
	DeleteCelMagic * mcel;
	
	mcel = (DeleteCelMagic *)Malloc(sizeof(DeleteCelMagic)+extraBytes, MEMTYPE_CEL|MEMTYPE_FILL|0);

	if (mcel) {
		mcel->magic 		= freeMagic;
		mcel->free.data 	= freeData;
		mcel->creatorData	= creatorData;
		return &mcel->ccb;
	} else {
		return NULL;
	}
}

/*----------------------------------------------------------------------------
 * 
 *--------------------------------------------------------------------------*/

void ModifyMagicCel_(CCB *cel, uint32 freeMagic, void *freeData, void *creatorData)
{
	DeleteCelMagic * mcel;
	
	mcel = MagicPtrFromCCBPtr(cel);
	mcel->magic 		= freeMagic;
	mcel->free.data 	= freeData;
	mcel->creatorData	= creatorData;
}

/*----------------------------------------------------------------------------
 * 
 *--------------------------------------------------------------------------*/

static CCB * internal_delete_cel(CCB *cel)
{
	DeleteCelMagic * mcel;
	uint32			magic;
	CCB	*			 next = NULL;
	
	if (cel) {
		mcel  = MagicPtrFromCCBPtr(cel);
		magic = mcel->magic;
		switch (magic) {

		  case DELETECELMAGIC_CCB_ONLY:
		  case DELETECELMAGIC_DATA_ONLY:
		  case DELETECELMAGIC_CCB_AND_DATA:

			if (mcel->creatorData == NULL) {
				next = CEL_NEXTPTR(cel);
			} else {
				next = CEL_NEXTPTR((CCB *)(mcel->creatorData));
			}

			if (magic != DELETECELMAGIC_CCB_ONLY && mcel->free.data != NULL) {
				Free(mcel->free.data);
			}
			
			if (magic != DELETECELMAGIC_DATA_ONLY) {
				FreeMagicCel_(cel);
			}
			break;

		  case DELETECELMAGIC_CALLBACK:

			next = mcel->free.func(mcel->creatorData, cel);
			break;

		  case DATAMAGIC_DEAD:

			next = NULL;
		  	DIAGNOSE(("DeleteCel(%08p) - attempt to delete this cel twice.\n", cel));
			break;

		  default:

			next = NULL;
			DIAGNOSE(("DeleteCel(%08p) - cell not allocated by a known method\n", cel));
			break;
		}
	}
	
	return next;
}


/*----------------------------------------------------------------------------
 * 
 *--------------------------------------------------------------------------*/

CCB * DeleteCel(CCB *cel)
{
	internal_delete_cel(cel);
	return NULL;
}

/*----------------------------------------------------------------------------
 * 
 *--------------------------------------------------------------------------*/

CCB * DeleteCelList(CCB *celList)
{
	CCB *	cur;
	CCB *	next;
	
	for (cur = celList; cur != NULL; cur = next) {
		next = internal_delete_cel(cur);
	}
	
	return NULL;
}
