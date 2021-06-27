/*******************************************************************************************
 *	File:			BlockFile.h
 *
 *	Contains:		definitions for BlockFile.c, block oriented file access routines
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	4/5/93		jb		Added CreateBlockFileIOReq(), change to not assume
 *						only one I/O request per open BlockFile.
 *	3/25/93		jb		New today, lifted from demo code
 *
 *******************************************************************************************/
#ifndef __BLOCKFILE_H__
#define __BLOCKFILE_H__

#include "Portfolio.h"

/**************************************************/
/* Data structure for use with BlockFile routines */
/**************************************************/

typedef struct BlockFile {
	Item		fDevice;		/* file device Item */
	FileStatus	fStatus;		/* status record */
} BlockFile, *BlockFilePtr;


/**********************/
/* Routine prototypes */
/**********************/

Item	CreateBlockFileIOReq( Item deviceItem, Item iodoneReplyPort );

int32	OpenBlockFile( char *name, BlockFilePtr bf );

void	CloseBlockFile( BlockFilePtr bf );

int32	GetBlockFileSize( BlockFilePtr bf );

int32	GetBlockFIleBlockSize( BlockFilePtr bf );

int32	AsynchReadBlockFile( BlockFilePtr bf, Item ioreqItem, void* buffer, long count, 
						long offset );

bool	ReadDoneBlockFile( Item ioreqItem );

int32	WaitReadDoneBlockFile( Item ioreqItem );

#endif	/* __BLOCKFILE_H__ */

