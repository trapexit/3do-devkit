/*
	File:		LoadFile.c

	Contains:	Routine to allocate a buffer and load an entire
 				file into it, using fast block I/O. Also, a routine
 				to unload the file (ie, free its buffer).

	Written by:	Ian

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	  9/6/93	JAY		change free to Free in two error paths in LoadFile.
		 <1>	 8/20/93	JAY		first checked in

	To Do:
*/

#include "BlockFile.h"
#include "UMemory.h"

#include "Debug3DO.h"

/*******************************************************************************************
 * void *LoadFile(char *filename, long *pfilesize, uint32 memTypeBits)
 *
 *	Allocates a buffer (according to memTypeBits) and loads the named file into it.
 *
 *	If the load succeeds, the return value is a pointer to the buffer, and
 *	the longword at *pfilesize is set to the length of the file.  (Note that
 *	the buffer may be larger than the filesize, due to the need to load
 *	entire device-sized blocks.  The value at *pfilesize reflects the amount
 *	of valid data in the file, not the blocksize-aligned buffer size.)
 *
 *	If the load fails, the return value is NULL, and the longword at
 *	*pfilesize contains a negative error code value.
 *
 *******************************************************************************************/


void *LoadFile(char *pfname, long *pfsize, uint32 memTypeBits)
{
	long		rv;
	long		filesize;
	long		blocksize;
	long		bufsize;
	void *		buffer;
	BlockFile	thefile;
	Item		ioreqItem;

#if DEBUG
	if (pfsize == NULL) {
		DIAGNOSE(("NULL pfsize parm in LoadFile() call\n"));
		return NULL;
	}
#endif

	if (0 > (rv = OpenBlockFile(pfname, &thefile))) {
		DIAGNOSE(("Can't open file %s\n", pfname));
		*pfsize = rv;
		return NULL;
	}

	filesize  = GetBlockFileSize(&thefile);
	blocksize = GetBlockFIleBlockSize(&thefile);
	bufsize   = ((filesize + blocksize - 1) / blocksize) * blocksize;

	if (NULL == (buffer = Malloc(bufsize, memTypeBits))) {
		DIAGNOSE(("Can't allocate buffer for file %s\n", pfname));
		CloseBlockFile(&thefile);
		*pfsize = -3;
		return NULL;
	}

	if (0 > (ioreqItem = CreateBlockFileIOReq(thefile.fDevice, (Item)NULL))) {
		DIAGNOSE(("Can't create IOReq for file %s\n", pfname));
		Free(buffer);
		CloseBlockFile(&thefile);
		*pfsize = -3;
		return NULL;
	}

	if (0 > (rv = AsynchReadBlockFile(&thefile, ioreqItem, buffer, bufsize, 0))) {
		DIAGNOSE(("Error reading file %s\n", pfname));
		DeleteItem(ioreqItem);
		Free(buffer);
		CloseBlockFile(&thefile);
		*pfsize = -3;
		return NULL;
	}

	if (0 > (rv = WaitReadDoneBlockFile(ioreqItem))) {
		DIAGNOSE(("Error reading file %s\n", pfname));
		rv = filesize;
	} else {
		rv = filesize;
	}

	DeleteItem(ioreqItem);
	CloseBlockFile(&thefile);
	*pfsize = rv;
	return buffer;
}


/*******************************************************************************************
 * void UnloadFile(void *filebuf)
 *
 *	Releases a file buffer acquired by a call to LoadFile().  Using this
 *	call instead of a direct call to free() will let the internals of
 *	this module change without any need to change your source code.
 *******************************************************************************************/


void UnloadFile(void *filebuf)
{
	if (filebuf) {
		Free(filebuf);
	}
}
