#pragma once

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

#include "extern_c.h"

#include "types.h"
#include "filesystem.h"
#include "filefunctions.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef struct BlockFile {
  Item	     fDevice;		/* file device Item */
  FileStatus fStatus;		/* status record */
} BlockFile, *BlockFilePtr;

typedef struct LoadFileInfo {
  void *    userData;		/* purely for client's use, internals don't touch it */
  void *    buffer;		/* pointer to buffer, allocated if not set by client */
  u32    bufSize;		/* size of buffer */
  u32    memTypeBits;	/* mem type if buffer to be internally allocated */
  Item 	    ioDonePort;		/* if set by client, ioreq returns to this port */
  BlockFile bf;			/* client must treat this as read-only */
  Item	    internalIOReq;	/* IOReq used internally */
  IOReq *   internalIORPtr;	/* pointer to above IOReq item */
  u32    internalFlags;	/* flags used internally */
} LoadFileInfo;


/*----------------------------------------------------------------------------
 * low-level block IO routines
 *--------------------------------------------------------------------------*/

EXTERN_C_BEGIN

Item    CreateBlockFileIOReq(Item deviceItem, Item iodoneReplyPort);
Err	OpenBlockFile(char *name, BlockFilePtr bf);
void    CloseBlockFile(BlockFilePtr bf);
s32   GetBlockFileSize(BlockFilePtr bf);
s32   GetBlockFileBlockSize(BlockFilePtr bf);
Err	AsynchReadBlockFile(BlockFilePtr bf, Item ioreqItem, void* buffer, s32 count, s32 offset);
boolean ReadDoneBlockFile(Item ioreqItem);
Err	WaitReadDoneBlockFile(Item ioreqItem);

/*----------------------------------------------------------------------------
 * higher-level file-oriented routines that use block IO
 *--------------------------------------------------------------------------*/

void UnloadFile(void *bufptr);

void *LoadFile(const char *filename, s32 *pfsize, u32 memTypeBits);
void *LoadFileHere(const char *fname, s32 *pfsize, void *buffer, s32 bufsize);

Err AsyncLoadFile(const char *fname, LoadFileInfo *lf);
Err CheckAsyncLoadFile(LoadFileInfo *lf);
Err FinishAsyncLoadFile(LoadFileInfo *lf, Err loadStatus);
Err WaitAsyncLoadFile(LoadFileInfo *lf);
Err AbortAsyncLoadFile(LoadFileInfo *lf);

Err SaveFile(const char *filename, void *buffer, s32 bufsize, s32 extrabytes);

s32 WriteMacFile(const char *filename, void *buf, s32 count);

EXTERN_C_END
