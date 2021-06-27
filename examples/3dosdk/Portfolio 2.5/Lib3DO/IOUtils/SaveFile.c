
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: SaveFile.c,v 1.4 1994/11/02 00:25:05 vertex Exp $
**
**  Lib3DO routine to dump an entire buffer into a file, completely replacing
**  the current contents of the file (if any).  The file is created
**  if it didn't already exist.
**
**  This currently works only for nvram files, since that's the only writable
**  filesystem (WriteMacFile notwithstanding).
**
**  The CMD_ALLOCBLOCKS device command adds the specified number of blocks to
**  the file as it exists already.  The docs lead you to believe that it will
**  make the file grow to the specified number of blocks, but it really adds
**  the specified number of blocks.
**
******************************************************************************/


#include "types.h"
#include "mem.h"
#include "blockfile.h"
#include "operror.h"
#include "string.h"
#include "debug3do.h"

/*****************************************************************************
 * SaveFile()
 *	Dump the contents of a buffer into a file (right now, it has to be an
 *	NVRAM file -- that is, the filename must start with "/NVRAM/" -- but if
 *	other writeable filesystems come along this should still work just fine).
 *
 *	If the file doesn't exist it is created.  If the file does exist, but
 *	it currently isn't big enough to hold the data, it is expanded to the
 *	new size needed + extrabytes.  The extrabytes parm is used ONLY when
 *	the file is first created, or expanded.  Since expanding a file is a
 *	relatively expensive operation, the extrabytes parm lets you grow a file
 *	a little at a time while doing the expensive part of the growth only
 *	occasionally.  For most uses (saving and restoring game state and other
 *	such fixed-size things) an extrabytes value of zero should be used.
 ****************************************************************************/

Err SaveFile(char *filename, void *buffer, int32 bufsize, int32 extrabytes)
{
	Err			err;
	Item 		fileItem;
	BlockFile	thefile;
	Item		ioreq;
	IOReq *		ior;
	IOInfo		ioinfo;
	int32		blocksize;
	int32		curblocks;
	int32		needblocks;

	fileItem		= 0;
	thefile.fDevice = 0;
	ioreq	 		= 0;

	if ((err = OpenBlockFile(filename, &thefile)) < 0) {
		if ((fileItem = CreateFile(filename)) < 0) {
			err = fileItem;
			goto ERROR_EXIT;
		}
		if ((err = OpenBlockFile(filename, &thefile)) < 0) {
			goto ERROR_EXIT;
		}
	}

	if ((ioreq = CreateBlockFileIOReq(thefile.fDevice, 0)) < 0) {
		err = ioreq;
		goto ERROR_EXIT;
	}
	ior = (IOReq *)LookupItem(ioreq);

	blocksize	= thefile.fStatus.fs.ds_DeviceBlockSize;
	curblocks	= thefile.fStatus.fs.ds_DeviceBlockCount;
	needblocks	= (bufsize + blocksize - 1) / blocksize;

	if (curblocks < needblocks) {
		needblocks = (bufsize + extrabytes + blocksize - 1) / blocksize;
		memset(&ioinfo, 0, sizeof(ioinfo));
		ioinfo.ioi_Command = FILECMD_ALLOCBLOCKS;
		ioinfo.ioi_Offset  = needblocks - curblocks;
		if ((err = DoIO(ioreq, &ioinfo)) < 0) {
			goto ERROR_EXIT;
		}
	}

	memset(&ioinfo, 0, sizeof(ioinfo));
	ioinfo.ioi_Command = CMD_WRITE;
	ioinfo.ioi_Send.iob_Buffer = buffer;
	ioinfo.ioi_Send.iob_Len    = bufsize;

	if ((err = DoIO(ioreq, &ioinfo)) < 0) {
		goto ERROR_EXIT;
	}

	memset(&ioinfo, 0, sizeof(ioinfo));
	ioinfo.ioi_Command = FILECMD_SETEOF;
	ioinfo.ioi_Offset  = bufsize;

	if ((err = DoIO(ioreq, &ioinfo)) < 0) {
		goto ERROR_EXIT;
	}

	err = 0;

ERROR_EXIT:

	DeleteIOReq(ioreq);

	CloseBlockFile(&thefile);

	return err;
}
