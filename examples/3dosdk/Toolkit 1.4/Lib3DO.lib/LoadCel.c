/*****************************************************************************
 *	File:			LoadCel.c
 *
 *	Contains:		Routine to allocate a buffer and load a cel
 *					into it using fast block I/O, then parse the cel.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	03/24/94  Ian	Changed to new DeleteCelMagic standard that uses the 
 *					creatorData field to skip mini-lists of related CCBs
 *					such as AA cels.
 *	03/03/94  Ian	Converted to new DeleteCelMagic system.
 *	07/25/94  Ian	Now uses new faster LoadFile() block I/O instead of
 *					filestream I/O.
 *
 *	Implementation notes:
 *
 *	DeleteCel() compatible.
 *
 *	We have to meet a pair of goals here:  Load a cel file in such a way that
 *	it can later be unloaded with a call to UnloadCel() (or DeleteCel()), and 
 *	do so without loading into one buffer then copying to another (which requires
 *	twice as much memory, and is slower).  ParseCel() returns a pointer to
 *	a CCB which it has located within the file buffer.	That pointer won't
 *	be at a known offset from the start of the buffer however, so we need
 *	a way for UnloadCel() to be able to find the start of the allocated
 *	block containing the CCB we found when parsing the buffer.
 *
 *	One given that we do have to work with is that wherever we find the
 *	CCB within the file buffer, we know there are 12 bytes of groodah in
 *	front of it that can be safely trashed (the chunk ID, size, and version for
 *	the CCC chunk).  So, after we've parsed out the CCB from the cel and
 *	have its pointer all ready to return to the caller, we lay the DeleteCel
 *	magic into the 12 bytes in front of the CCB.  We use the DATA_ONLY flavor
 *	of DeleteCel magic, so that the DeleteCel() routine will free the file 
 *	buffer pointed to by the free.data word, but won't attempt to free the CCB 
 *	itself (since it's located within the file buffer).
 *
 *	This bit of sneakiness works because the DeleteCelMagic system was 
 *	designed with the LoadCel() function in mind - with LoadCel() we have 12 
 *	bytes of junk we can safely tweak right before the CCB, and DeleteCelMagic 
 *	requires exactly 12 bytes of magic appearing right before the CCB.
 *
 *	We use the ModifyMagicCel_() service routine to add the magic data to the
 *	12-byte memory chunk right before the CCB.
 ****************************************************************************/

#include "CelUtils.h"
#include "Form3DO.h"
#include "BlockFile.h"
#include "DeleteCelMagic.h"
#include "UMemory.h"
#include "Debug3DO.h"

/*----------------------------------------------------------------------------
 * CCB * LoadCel(char *filename, uint32 memTypeBits)
 *
 *	Loads a cel from a 3DO file.  Returns a pointer to the CCB for the cel;
 *	the CCB will contain pointers to the pixels and (optional) PLUT.  If the
 *	cel file contains an AA cel or multiple cels, the cels will be linked 
 *	together through the ccb_NextPtr fields (in the right order for drawing
 *	when it's an AA cel, in the order they're found in the file for others).
 *	The cel buffer can be freed later by passing the CCB pointer to UnloadCel(),
 *	or to DeleteCel().
 *--------------------------------------------------------------------------*/

CCB * LoadCel(char *name, uint32 memTypeBits)
{
	long	filesize;
	void *	filebuf;
	CCB *	pCCB;

	if (NULL == (filebuf = LoadFile(name, &filesize, memTypeBits))) {
		DIAGNOSE(("Can't load file %s\n", name));
		return NULL;
	}

	if (NULL == (pCCB = ParseCel(filebuf, filesize))) {
		UnloadFile(filebuf);
		DIAGNOSE(("Can't parse cel file %s\n", name));
		return NULL;
	}
	ModifyMagicCel_(pCCB, DELETECELMAGIC_DATA_ONLY, filebuf, LastCelInList(pCCB));
	
	return pCCB;

}

/*----------------------------------------------------------------------------
 * UnloadCel
 *	Unload a cel file previously loaded via LoadCel().
 *--------------------------------------------------------------------------*/

void UnloadCel(CCB *cel)
{
	DeleteCel(cel);
}
