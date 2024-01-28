/*******************************************************************************************
 *	File:			BlockFile.h
 *
 *	Contains:		Utilities for block-level filesystem IO.
 *
 *	Copyright (c) 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	07/11/94  Ian	Change #includes to just the things really needed.
 *					Standarized protective wrapper macro name to
 *BLOCKFILE_H. Standardized comment blocks. Added SaveFile() and
 *AsyncLoadFile(). General library cleanup and reorg.
 *	01/20/94  rdg	make C++ compatible
 *	10/05/93  jb	Fix day-1 "GetBlockFIleBlockSize" typeo
 *	04/05/93  jb	Added CreateBlockFileIOReq(), change to not assume
 *					only one I/O request per open
 *BlockFile. 03/25/93  jb	New today, lifted from demo code
 *
 *******************************************************************************************/

#ifndef BLOCKFILE_H
#define BLOCKFILE_H

#include "filefunctions.h"
#include "filesystem.h"
#include "types.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

typedef struct BlockFile
{
  Item fDevice;       /* file device Item */
  FileStatus fStatus; /* status record */
} BlockFile, *BlockFilePtr;

typedef struct LoadFileInfo
{
  void *userData;      /* purely for client's use, internals don't touch it */
  void *buffer;        /* pointer to buffer, allocated if not set by client */
  uint32 bufSize;      /* size of buffer */
  uint32 memTypeBits;  /* mem type if buffer to be internally allocated */
  Item ioDonePort;     /* if set by client, ioreq returns to this port */
  uint32 reserved1[2]; /* for future expansion of client-supplied info */
  BlockFile bf;        /* client must treat this as read-only */
  Item internalIOReq;  /* IOReq used internally */
  IOReq *internalIORPtr; /* pointer to above IOReq item */
  uint32 internalFlags;  /* flags used internally */
  uint32 reserved2[2];   /* for future expansion of internal info */
} LoadFileInfo;

/*----------------------------------------------------------------------------
 * low-level block IO routines
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

  /* The following fixes a day-1 typo (FIle versus File) */

#define GetBlockFIleBlockSize GetBlockFileBlockSize

  Item CreateBlockFileIOReq (Item deviceItem, Item iodoneReplyPort);
  Err OpenBlockFile (char *name, BlockFilePtr bf);
  void CloseBlockFile (BlockFilePtr bf);
  int32 GetBlockFileSize (BlockFilePtr bf);
  int32 GetBlockFileBlockSize (BlockFilePtr bf);
  Err AsynchReadBlockFile (BlockFilePtr bf, Item ioreqItem, void *buffer,
                           int32 count, int32 offset);
  Boolean ReadDoneBlockFile (Item ioreqItem);
  Err WaitReadDoneBlockFile (Item ioreqItem);

  /*----------------------------------------------------------------------------
   * higher-level file-oriented routines that use block IO
   *--------------------------------------------------------------------------*/

  void UnloadFile (void *bufptr);

  void *LoadFile (char *filename, int32 *pfsize, uint32 memTypeBits);
  void *LoadFileHere (char *fname, int32 *pfsize, void *buffer, int32 bufsize);

  Err AsyncLoadFile (char *fname, LoadFileInfo *lf);
  Err CheckAsyncLoadFile (LoadFileInfo *lf);
  Err FinishAsyncLoadFile (LoadFileInfo *lf, Err loadStatus);
  Err WaitAsyncLoadFile (LoadFileInfo *lf);
  Err AbortAsyncLoadFile (LoadFileInfo *lf);

  Err SaveFile (char *filename, void *buffer, int32 bufsize, int32 extrabytes);

  int32 WriteMacFile (char *filename, void *buf, int32 count);

#ifdef __cplusplus
}
#endif

#endif
