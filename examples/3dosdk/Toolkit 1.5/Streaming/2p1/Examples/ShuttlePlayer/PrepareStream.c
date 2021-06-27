/*******************************************************************************************
 *	File:			PrepareStream.c
 *
 *	Contains:		definitions for high level stream playback preparation
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright й 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  08/22/94		dtc		Version 1.1
 *							Replaced printf with printf.
 *	10/20/93		jb		New today
 *
 *******************************************************************************************/

#include "Types.h"
#include "UMemory.h"
#include "BlockFile.h"
#include "PrepareStream.h"

#include "DataStreamDebug.h"

/*********************************************************************************************
 * Routine to allocate and initialize a buffer list for use with the streamer
 *********************************************************************************************/
DSDataBufPtr	CreateBufferList( long numBuffers, long bufferSize )
	{
#	define			ROUND_TO_LONG(x)	((x + 3) & ~3)
	DSDataBufPtr	bp;
	DSDataBufPtr	firstBp;
	long			totalBufferSpace;
	long			actualEntrySize;
	long			bufferNum;
	
	actualEntrySize = sizeof(DSDataBuf) + ROUND_TO_LONG( bufferSize );
	totalBufferSpace = numBuffers * actualEntrySize;

	/* Try to allocate the needed memory */
	firstBp = (DSDataBufPtr) NewPtr( totalBufferSpace, MEMTYPE_ANY );
	if ( firstBp == NULL )
		return NULL;

	/* Loop to take the big buffer and split it into "N" buffers
	 * of size "bufferSize", linked together.
	 */
	for ( bp = firstBp, bufferNum = 0; bufferNum < (numBuffers - 1); bufferNum++ )
		{
		bp->next			= (DSDataBufPtr) ( ((char *) bp) + actualEntrySize );
		bp->permanentNext	= bp->next;

		/* Advance to point to the next entry */
		bp = bp->next;
		}

	/* Make last entry's forward link point to nil */
	bp->next = NULL;
	bp->permanentNext = NULL;

	/* Return a pointer to the first buffer in the list 
	 */
	return firstBp;
	}


/*********************************************************************************************
 * Routine to find and load a stream header into memory given a stream file name.
 *********************************************************************************************/
int32	FindAndLoadStreamHeader( DSHeaderChunkPtr headerPtr, char *fileName )
	{
	int32			status;
	BlockFile		blockFile;
	Item			ioDoneReplyPort;
	Item			ioReqItem;
	long			*pLong;
	char			*buffer;

	/* Init our stuff */
	blockFile.fDevice	= 0;
	ioDoneReplyPort		= 0;
	ioReqItem			= 0;
	buffer				= 0;

	/* Allocate a buffer to hold some of the first data 
	 * in the stream 
	 */
	buffer = (char *) NewPtr( FIND_HEADER_BUFFER_SIZE, MEMTYPE_ANY );
	if ( buffer == NULL )
		return kPSMemFullErr;

	/* Open the specified file */
	status = OpenBlockFile( fileName, &blockFile );
	if ( status != 0 )
		{
		printf ("File %s\n", fileName);
		CHECK_DS_RESULT( "OpenBlockFile", status );
		return status;
		}

	/* Create a reply port for the I/O message */
	ioDoneReplyPort = NewMsgPort( NULL );
	if ( ioDoneReplyPort < 0 )
		{
		status = ioDoneReplyPort;
		CHECK_DS_RESULT( "NewMsgPort", status );
		goto BAILOUT;
		}

	/* Create an I/O request item */
	ioReqItem = CreateBlockFileIOReq( blockFile.fDevice, ioDoneReplyPort );
	if ( ioReqItem < 0 )
		{
		status = ioReqItem;
		CHECK_DS_RESULT( "CreateBlockFileIOReq", status );
		goto BAILOUT;
		}

	/* Read the first block from the purported stream file */
	status = AsynchReadBlockFile( 
				&blockFile, 				/* block file */
				ioReqItem, 					/* ioreq Item */
				buffer,			 			/* place to READ data into */
				FIND_HEADER_BUFFER_SIZE,	/* amount of data to READ */
				0 );						/* offset of data in the file */
	/* Read the first block from the purported stream file */
	if ( status != 0 )
		{
		CHECK_DS_RESULT( "AsynchReadBlockFile", status );
		goto BAILOUT;
		}

	/* Wait for the read to complete */
	status = WaitReadDoneBlockFile( ioReqItem );
	if ( status != 0 )
		{
		CHECK_DS_RESULT( "WaitReadDoneBlockFile", status );
		goto BAILOUT;
		}

	/* Done with the file so close it */
	CloseBlockFile( &blockFile );
	blockFile.fDevice = 0;

	/* Search the buffer for the header information */
	/* NOTE: еее FOR NOW, WE ASSUME THE HEADER IS THE FIRST DATA IN THE FILE еее */
	pLong = (long *) buffer;

	if ( *pLong == HEADER_CHUNK_TYPE )
		{
		*headerPtr = *((DSHeaderChunkPtr) pLong);
		status = 0;
		}
	else
		{
		CHECK_DS_RESULT( "kPSHeaderNotFound", status );
		status = kPSHeaderNotFound;
		}

BAILOUT:
	if ( buffer )				FreePtr( buffer );
	if ( blockFile.fDevice )	CloseBlockFile( &blockFile );
	if ( ioDoneReplyPort )		DeleteItem( ioDoneReplyPort );
	if ( ioReqItem )			DeleteItem( ioReqItem );

	return status;
	}

