#ifndef __BLOCKFILE_H
#define __BLOCKFILE_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: blockfile.h,v 1.6 1994/11/01 03:51:46 vertex Exp $
**
**  Lib3DO utilities for block-level file IO
**
******************************************************************************/


#include "types.h"
#include "filesystem.h"
#include "filefunctions.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef struct BlockFile {
	Item		fDevice;		/* file device Item */
	FileStatus	fStatus;		/* status record */
} BlockFile, *BlockFilePtr;

typedef struct LoadFileInfo {
	void *		userData;		/* purely for client's use, internals don't touch it */
	void *		buffer;			/* pointer to buffer, allocated if not set by client */
	uint32		bufSize;		/* size of buffer */
	uint32		memTypeBits;	/* mem type if buffer to be internally allocated */
	Item 		ioDonePort;		/* if set by client, ioreq returns to this port */
	BlockFile	bf;				/* client must treat this as read-only */
	Item		internalIOReq;	/* IOReq used internally */
	IOReq *		internalIORPtr;	/* pointer to above IOReq item */
	uint32		internalFlags;	/* flags used internally */
} LoadFileInfo;


/*----------------------------------------------------------------------------
 * low-level block IO routines
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

Item	CreateBlockFileIOReq(Item deviceItem, Item iodoneReplyPort);
Err		OpenBlockFile(char *name, BlockFilePtr bf);
void	CloseBlockFile(BlockFilePtr bf);
int32	GetBlockFileSize(BlockFilePtr bf);
int32	GetBlockFileBlockSize(BlockFilePtr bf);
Err		AsynchReadBlockFile(BlockFilePtr bf, Item ioreqItem, void* buffer, int32 count, int32 offset);
Boolean	ReadDoneBlockFile(Item ioreqItem);
Err		WaitReadDoneBlockFile(Item ioreqItem);

/*----------------------------------------------------------------------------
 * higher-level file-oriented routines that use block IO
 *--------------------------------------------------------------------------*/

void	UnloadFile(void *bufptr);

void *	LoadFile(char *filename, int32 *pfsize, uint32 memTypeBits);
void *	LoadFileHere(char *fname, int32 *pfsize, void *buffer, int32 bufsize);

Err		AsyncLoadFile(char *fname, LoadFileInfo *lf);
Err		CheckAsyncLoadFile(LoadFileInfo *lf);
Err		FinishAsyncLoadFile(LoadFileInfo *lf, Err loadStatus);
Err		WaitAsyncLoadFile(LoadFileInfo *lf);
Err		AbortAsyncLoadFile(LoadFileInfo *lf);

Err 	SaveFile(char *filename, void *buffer, int32 bufsize, int32 extrabytes);

int32  	WriteMacFile(char *filename, void *buf, int32 count);

#ifdef __cplusplus
  }
#endif



#endif	/* __BLOCKFILE_H */
