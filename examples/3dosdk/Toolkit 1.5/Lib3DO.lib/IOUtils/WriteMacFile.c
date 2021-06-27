/*****************************************************************************
 *	File:			WriteMacFile.c 
 *
 *	Contains:		Routine to write a buffer to a file on the Mac filesystem.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian 	Complete rewrite; no longer relies on global variables
 *					set by the (now obsolete) OpenMacLink() function.
 *	01/17/94  JAY	change calling parameters in memset call to match prototype.
 *	08/13/93  JAY	clean up for 1p01 release
 *
 *	Implementation notes:
 *
 *	Writes count bytes from the specified buffer into the file filename.  
 *	Returns the actual length of what was written, which may be larger than
 *	the specified count because the count is padded to the next greater 
 *	word multiple.  (I dunno if this is necessary, but it was in the original
 *	code I started with. - Ian)
 *
 *	How to specify Mac path info?  Randy Carr indicated some trouble with 
 *	this on the debugger side, need to investigate and document.  
 ****************************************************************************/

#include "Utils3DO.h"
#include "string.h"
#include "Debug3DO.h"

int32 WriteMacFile(char *filename, void *buf, int32 count)
{
	Err		err;
	Item	macdev = 0;
	Item	ioreq  = 0;
	IOReq *	ior;
	IOInfo	ioinfo;

	if ((macdev = OpenNamedDevice("mac", 0)) < 0) {
		err = macdev;
		DIAGNOSE_SYSERR(err, ("Cannot find/open 'mac' device\n"));
		goto ERROR_EXIT;
	}

	if ((ioreq = CreateIOReq(NULL, 0, macdev, 0)) < 0) {
		err = ioreq;
		DIAGNOSE_SYSERR(err, ("Cannot create IOReq for 'mac' device\n"));
		goto ERROR_EXIT;
	}
	ior = (IOReq *)LookupItem(ioreq);
	
	memset(&ioinfo, 0, sizeof(IOInfo));
	ioinfo.ioi_Command			= CMD_WRITE;
	ioinfo.ioi_Unit				= 0;
	ioinfo.ioi_Send.iob_Buffer	= buf;
	ioinfo.ioi_Send.iob_Len 	= (count+3) & ~3;
	ioinfo.ioi_Recv.iob_Buffer	= filename;
	ioinfo.ioi_Recv.iob_Len		= strlen(filename) + 1L;
	ioinfo.ioi_Offset			= 0;

	if ((err = DoIO(ioreq, &ioinfo)) >= 0 ) {
		err = ior->io_Error;
	}
	
	if (err < 0) {
		DIAGNOSE_SYSERR(err, ("Cannot write file %s to 'mac' device", filename));
		goto ERROR_EXIT;
	}
	
	err = ior->io_Actual;

ERROR_EXIT:

	if (ioreq > 0) {
		DeleteItem(ioreq);
	}
	
	if (macdev > 0) {
		CloseItem(macdev);
	}
	
	return err;
}
