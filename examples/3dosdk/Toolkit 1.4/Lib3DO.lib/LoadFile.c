/*******************************************************************************************
 *
 *	File:		LoadFile.c
 * 
 * 	Contains:	Routine to allocate a buffer and load an entire
 * 				file into it, using fast block I/O. Also, a routine
 * 				to unload the file (ie, free its buffer).
 * 
 *	Written by:	Ian Lepore
 *
 *	Copyright:	© 1993 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any Person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company.
 *
 *	Change History (most recent first):
 *       04/12/94	Ian		Modified so that a file of size zero doesn't cause Malloc()
 *							to spit out an error about allocating a block of size zero.
 *							Zero-length files are indicated by a NULL return code with
 *							*pfilesize set to zero; whether this should be interpreted
 *							as an error is up to the caller.
 *	<3>  02/18/94	Ian		Reworked error detection logic.  The old way was failing
 *							to return bad status if the open worked but the read failed.
 *	<2>  09/06/93	JAY		change free to Free in two error paths in LoadFile.
 *	<1>  08/20/93	JAY		first checked in
 *
 *******************************************************************************************/

#include "BlockFile.h"
#include "UMemory.h"
#include "operror.h"
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

void * LoadFile(char *pfname, long *pfsize, uint32 memTypeBits)
{
	long		rv;
	long		filesize;
	long		blocksize;
	long		bufsize;
	void *		buffer;
	Item		ioreqItem;
	BlockFile	thefile;

	thefile.fDevice = 0;
	buffer 			= NULL;
	ioreqItem 		= 0;
	
	if ((rv = OpenBlockFile(pfname, &thefile)) < 0) {
		DIAGNOSE_SYSERR(rv, ("Can't open file %s\n", pfname));
		goto ERROR_EXIT;
	}

	filesize  = thefile.fStatus.fs_ByteCount;
	blocksize = thefile.fStatus.fs.ds_DeviceBlockSize;
	bufsize   = ((filesize + blocksize - 1) / blocksize) * blocksize;

	if (filesize > 0) {
		if ((buffer = Malloc(bufsize, memTypeBits)) == NULL) {
			rv = NOMEM;	// status constant from operror.h
			DIAGNOSE_SYSERR(rv, ("Can't allocate buffer for file %s\n", pfname));
			goto ERROR_EXIT;
		}
	
		if ((ioreqItem = CreateBlockFileIOReq(thefile.fDevice, 0)) < 0) {
			rv = ioreqItem;
			DIAGNOSE_SYSERR(rv, ("Can't create IOReq for file %s\n", pfname));
			goto ERROR_EXIT;
		}
	
		if ((rv = AsynchReadBlockFile(&thefile, ioreqItem, buffer, bufsize, 0)) >= 0) {
			rv = WaitReadDoneBlockFile(ioreqItem);
			if (rv >= 0) {
				rv = ((IOReq *)LookupItem(ioreqItem))->io_Error;
			}
		}
		
		if (rv < 0) {
			DIAGNOSE_SYSERR(rv, ("Error reading file %s\n", pfname));
			goto ERROR_EXIT;
		}
	}
	
	rv = filesize;

ERROR_EXIT:

	CloseBlockFile(&thefile);	// this is safe even if file isn't open.
	
	if (ioreqItem > 0) {		// if we obtained an IOReq, 
		DeleteItem(ioreqItem);	// delete it.
	}
	
	if (rv < 0) {				// if exiting with bad status
		if (buffer) {			// and we acquired a file buffer,
			Free(buffer);		// free it, and set the pointer
		}						// to NULL so that the function
		buffer = NULL;			// return value indicates error.
	}

	if (pfsize) {				// if the caller provided a status/size
		*pfsize = rv;			// return value pointer, return the value.
	}
	
	return buffer;				// return buffer pointer (NULL on error)
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
