/*******************************************************************************************
 *	File:			JoinFileFormat.h
 *
 *	Contains:		
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __JOINFILEFORMAT__
#define __JOINFILEFORMAT__



/***********/
/*  TYPES  */
/***********/

typedef struct {
		SUBS_CHUNK_COMMON;
		int32		joinChunkType;			// 'JHDR' for JoinChunkFirst  or 'JDAT' for JoinChunkData
		int32		totalDataSize;			// the total size of the data in all chunks
		int32		ramType;				// AllocMem flags for this type of data
		int32		compType;				// type of compression used on this data
		int32		dataSize;				// the size of the data in this chunk
		// data follows immediately
} JoinHeader;

typedef struct {
		SUBS_CHUNK_COMMON;
		int32		joinChunkType;			// 'JHDR' for JoinChunkFirst  or 'JDAT' for JoinChunkData
		int32		dataSize;				// size of the data in this chunk
		// data follows immediately
} JoinData;



#endif
