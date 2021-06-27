/*****************************************************************************
 *	File:			LoadAnim.c
 *
 *	Contains:		Routine to allocate a buffer and load an anim file
 *					into it, using fast block I/O, then parse the anim
 *					file into an ANIM structure with attached AnimFrames.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/25/93 	ian Now uses new faster LoadFile() block I/O instead of
 *					filestream I/O.
 *
 *	Implementation notes:
 *
 *	Iterate through each chunk and fill the AnimFrames with pointers
 *	to CCBs, PLUTs, and Pixel Data for each frame. If an Anim has one
 *	CCB and many PDATs the CCB pointer is used repeatedly for each frame
 *	in the AnimFrame. We end up with an array of AnimEntries that we can
 *	index into at any point and display a valid frame from each entry.
 *
 *	If we run across an ANIM chunk before we've allocated any AnimFrames,
 *	we gain a bit of efficiency by allocating all the AnimFrames we'll
 *	need at once.  If there is no ANIM chunk in the file, we'll expand
 *	the array of AnimFrames on the fly as needed.
 *
 *	We handle both types of ANIM files which are currently fashionable:
 *	3DO animator format, where PLUTs come before PDATs, and "glom-them-
 *	together-with-MPW-cat-command" where PDATs come before PLUTs. In an
 *	ideal world, there'd be a tool to create Animator-like ANIM files
 *	from a collection of cels, and everyone would promptly convert all
 *	their existing ANIMs.  ::sigh::
 *
 ****************************************************************************/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "BlockFile.h"
#include "UMemory.h"

#include "Debug3DO.h"

/*----------------------------------------------------------------------------
 * LoadAnim
 *	Load an ANIM file, parse it into an ANIM structure and a set of AnimFrames.
 *--------------------------------------------------------------------------*/

ANIM * LoadAnim(char *name, uint32 memTypeBits)
{
	long		filesize;
	void *		filebuf;
	ANIM *		pAnim;

	/*------------------------------------------------------------------------
	 * Load the file into a buffer, call ParseAnim() to do the real work.
	 *----------------------------------------------------------------------*/

	if (NULL == (filebuf = LoadFile(name, &filesize, memTypeBits))) {
		DIAGNOSE(("Can't load file %s\n", name));
		return NULL;
	}

	if (NULL == (pAnim = ParseAnim(filebuf, filesize, memTypeBits))) {
		UnloadFile(filebuf);
		DIAGNOSE(("Can't parse file %s\n", name));
		return NULL;
	}

	pAnim->dataBuffer = filebuf;	// attach buffer so Unload can free it.

	return pAnim;
}

/*----------------------------------------------------------------------------
 *
 *--------------------------------------------------------------------------*/

void UnloadAnim(ANIM *anim)
{
	if (anim != NULL) {
		if (anim->dataBuffer != NULL) {
			UnloadFile(anim->dataBuffer);
		}
		if (anim->pentries != NULL) {
			Free(anim->pentries);
		}
		Free(anim);
	}
}
