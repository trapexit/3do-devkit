/*****************************************************************************
 *	File:			AsyncLoadFile.c 
 *
 * 	Contains:		Async file loader.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reseerred.
 *
 *	History:
 *	07/12/94  Ian	New today.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "operror.h"
#include "BlockFile.h"
#include "UMemory.h"
#include "Debug3DO.h"

#define WE_ALLOCATED_BUFFER	0x00010000

/*----------------------------------------------------------------------------
 * AsyncLoadFile()
 *	Routine to process async file loads.
 *
 *	Note that the IO is purposely done WITHOUT the IO_QUICK flag.  This means
 *	that even if the IO is completed at SendIO() time (due to caching perhaps)
 *	the client will still receive a notification message at the specified
 *	msgport (if one is specified).
 *--------------------------------------------------------------------------*/
 
Err AsyncLoadFile(char *fileName, LoadFileInfo *lf)
{
	Err			err;
	int32		fileSize;
	int32		blockSize;
	int32		bufSize;
	IOInfo		ioInfo;

	lf->internalFlags	= 0;
	lf->internalIOReq 	= 0;
	lf->bf.fDevice	 	= 0;
	
	if ((err = OpenBlockFile(fileName, &lf->bf)) < 0) {
		DIAGNOSE_SYSERR(err, ("Can't open file %s\n", fileName));
		goto ERROR_EXIT;
	}

	fileSize  = lf->bf.fStatus.fs_ByteCount;
	blockSize = lf->bf.fStatus.fs.ds_DeviceBlockSize;
	bufSize   = ((fileSize + blockSize - 1) / blockSize) * blockSize;
	
	if (fileSize > 0) {
		if (lf->buffer != NULL) {
			if (lf->bufSize != 0 && lf->bufSize < bufSize) {
				err = BADSIZE;	// status constant from operror.h
				DIAGNOSE_SYSERR(err, ("Supplied buffer is too small for file %s\n", fileName));
				goto ERROR_EXIT;
			}
		} else {
			if ((lf->buffer = Malloc(bufSize, lf->memTypeBits)) == NULL) {
				err = NOMEM;	// status constant from operror.h
				DIAGNOSE_SYSERR(err, ("Can't allocate buffer for file %s\n", fileName));
				goto ERROR_EXIT;
			}
			lf->internalFlags |= WE_ALLOCATED_BUFFER;
			lf->bufSize = bufSize;
		} 

		if ((lf->internalIOReq = CreateIOReq(fileName, 0, lf->bf.fDevice, lf->ioDonePort)) < 0) {
			err = lf->internalIOReq;
			DIAGNOSE_SYSERR(err, ("Can't create IOReq for file %s\n", fileName));
			goto ERROR_EXIT;
		}
		lf->internalIORPtr = (IOReq *)LookupItem(lf->internalIOReq);
	
		ioInfo.ioi_Command			= CMD_READ;
		ioInfo.ioi_User				= (uint32)lf;
		ioInfo.ioi_Offset 			= 0;
		ioInfo.ioi_Recv.iob_Buffer	= lf->buffer;
		ioInfo.ioi_Recv.iob_Len		= lf->bufSize;	
		ioInfo.ioi_Send.iob_Buffer	= 0;
		ioInfo.ioi_Send.iob_Len		= 0;
		ioInfo.ioi_Flags			= 0;
		ioInfo.ioi_Unit				= 0;
		ioInfo.ioi_Flags2			= 0;
		ioInfo.ioi_CmdOptions		= 0;

		if ((err = SendIO(lf->internalIOReq, &ioInfo)) < 0) {
			DIAGNOSE_SYSERR(err, ("Error reading file %s\n", fileName));
			goto ERROR_EXIT;
		}
	}

	err = 0;
	
ERROR_EXIT:
	
	if (err < 0) {							// if exiting with bad status
		FinishAsyncLoadFile(lf, err);		// cleanup resources we may have
	}										// acquired.

	return err;								// return status
}

/*----------------------------------------------------------------------------
 * FinishAsyncLoadFile()
 *	Clean up resources no longer needed after the load is complete.  If the 
 *	loadStatus parm indicates finish is due to error, and we allocated the 
 *	file buffer, free it up as well.
 *
 *	Clients should call this function after CheckAsyncLoadFile() indicates
 *	the load is completed (or errored out), or after receiving notification
 *	of IO completion (good or bad) at the msgport they supplied when they
 *	called AsyncLoadFile().
 *--------------------------------------------------------------------------*/

Err FinishAsyncLoadFile(LoadFileInfo *lf, Err loadStatus)
{
	if (lf->internalIOReq) {
		DeleteItem(lf->internalIOReq);
		lf->internalIOReq  = 0;
		lf->internalIORPtr = NULL;
	}
	
	CloseBlockFile(&lf->bf);
	
	if (loadStatus < 0) {
		if (lf->internalFlags & WE_ALLOCATED_BUFFER) {
			if (lf->buffer) {
				Free(lf->buffer);
				lf->buffer	= NULL;
				lf->bufSize	= 0;
			}
		}
	}
	
	return loadStatus;
}

/*----------------------------------------------------------------------------
 * CheckAsyncLoadFile()
 *	Return status of async load:
 *		zero 		means IO still in progress
 *		negative	means IO error ocurred
 *		positive	means IO completed successfully
 *	Client needs to call FinishAsyncLoadFile() after a non-zero return
 *	from this function.
 *--------------------------------------------------------------------------*/

Err CheckAsyncLoadFile(LoadFileInfo *lf)
{
	Err	err;
	
	if ((err = CheckIO(lf->internalIOReq)) > 0) {
		if ((err = lf->internalIORPtr->io_Error) >= 0) {
			err = 1;
		}
	}
	
	return err;
}

/*----------------------------------------------------------------------------
 * WaitAsyncLoadFile()
 *	Wait for completion of async load, return load status.  Client should NOT
 *	call FinishAsyncLoadFile() after this function.
 *--------------------------------------------------------------------------*/

Err WaitAsyncLoadFile(LoadFileInfo *lf)
{
	Err	err;
	
	if (lf->ioDonePort) {
		err = WaitPort(lf->ioDonePort, lf->internalIORPtr->io_MsgItem);
	} else {
		err = WaitIO(lf->internalIOReq);
	}

	if (err >= 0) {
		err = lf->internalIORPtr->io_Error;
	}
	
	return FinishAsyncLoadFile(lf, err);
}

/*----------------------------------------------------------------------------
 * AbortAsyncLoadFile()
 *	Abort an async load.  Always returns ABORTED status.  Client should NOT
 *	call FinishAsyncLoadFile() after this function.  If client asked 
 *	AsyncLoadFile() to allocate a file buffer, the buffer will be freed 
 *	by this function.  If client supplied the buffer it is (of course) not
 *	freed by this function, but its contents are indeterminate.
 *--------------------------------------------------------------------------*/

Err AbortAsyncLoadFile(LoadFileInfo *lf)
{
	AbortIO(lf->internalIOReq);
	
	if (lf->ioDonePort) {
		WaitPort(lf->ioDonePort, lf->internalIORPtr->io_MsgItem);
	} else {
		WaitIO(lf->internalIOReq);
	}
	
	return FinishAsyncLoadFile(lf, ABORTED);
}

