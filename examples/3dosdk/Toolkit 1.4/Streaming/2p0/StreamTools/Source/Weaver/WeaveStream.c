/*******************************************************************************************
 *	File:			WeaveStream.c
 *
 *	Contains:		data stream merging routines
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  04/25/94	dtc		Version 2.2
 *						Allow unlimited number of input streams through USE_MAC_IO. The
 *						limit was set to 20 files using standard I/O.
 *						Added functions WriteAlrmChunk WriteGotoChunk, WriteStopChunk,
 *						WriteFillerChunk to process commands to accept stop and goto
 *						control chunks:
 *						writegotochunk <streamtime> <destinationtime>
 *						writestopchunk <streamtime>
 *						Replaced SortStreamFiles with InsertStreamFiles to handle sorting
 *						of linked list of input streams in ascending priority order.
 *						SortStreamFiles sorted the fixed size array of input streams.
 *						Moved the definition of ControlChunk struct to WeaveStream.h.
 *	12/2/93		jb		Propagate numSubsMsgs from weave param block into header data
 *	10/25/93	jb		Change fopen() mode to "wb+"; add code to write the marker
 *						table into the stream. Force stream data onto block #1 of output
 *						file if either stream header or marker table are present in the
 *						output file.
 *	10/20/93	jb		Make output of stream header chunk optional.
 *						Added some very questionable code in ReadChunkHeader() to
 *						handle input streawms with partially formed FILL chunks.
 *						This is to allow woven streams as input files. This should
 *						be fixed in the future by requiring woven streams to contain
 *						a stream header. This way, we can use the stream block size
 *						value in the input stream to determine start of block addresses
 *						much the same way it is done in the DumpStream tool.
 *	10/19/93	jb		Added WriteHeaderChunk() routine which outputs a descriptive
 *						header data chunk to make generalized playback functions
 *						possible.
 *	10/11/93	jb		Check chunk size <= stream block size in ReadChunkHeader().
 *	9/12/93		rdg		Spin cursor.
 *	6/27/93		jb		Allocate large I/O buffers to make things go faster
 *	6/21/93		jb		Change Control Subscriber main chunk type to 'CTRL' to match 
 *						everything else in the world.
 *	6/17/93		jb		Output Control Subscriber 'SYNC' chunks at the beginning of every
 *						marker block to allow the implementation of a "conditional flush".
 *						Added WriteSyncChunk() routine.
 *	5/18/93		jb		Add 'currMarkerIndex' to param block.
 *	5/17/93		jb		First pass implementation complete.
 *	5/15/93		jb		New today.
 *
 *******************************************************************************************/

#include <String.h>
#include <Memory.h>
#include <cursorctl.h>
#include <StdLib.h>

#include "WeaveStream.h"
#include "DSStreamHeader.h"
#include "MarkerChunk.h"

#if USE_MAC_IO
#include <errors.h>
#include <files.h>
#include <strings.h>
#endif

/* The following compile switch determines whether or not larger I/O buffers are used
 * for stream I/O. This SIGNIFICANTLY improves performance, but is not ANSI compliant
 * because it uses an MPW-specific library call to set the buffer size. It is recommended
 * that this option be left ON.
 */
#define	USE_BIGGER_BUFFERS		1

/* The following compile switch determines whether or not the beginning of the
 * stream data will be forced onto a stream block boundary. Setting this switch results
 * in wasting space in the stream file, but may be useful if the application does a 
 * considerable amount of stream looping, whereby the header and marker table data,
 * if present, could cause performance problems. It is recommended that this be left OFF.
 */
#define	FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY	0


/****************************/
/* Local routine prototypes */
/****************************/
static InStreamPtr	GetEarliestBelowNextMarker( WeaveParamsPtr pb, long sizeRemaining );
static InStreamPtr	GetBestFitBelowNextMarker( WeaveParamsPtr pb, long sizeRemaining );
static char*		GetIOBuffer( long bufferSize );
static Boolean		InputDataRemaining( WeaveParamsPtr pb );
static Boolean		ReadChunkHeader( WeaveParamsPtr pb, InStreamPtr isp );
static void			SortMarkers( WeaveParamsPtr pb );
static Boolean		WriteChunk( WeaveParamsPtr pb, InStreamPtr isp, long* bytesRemaining );
static Boolean		WriteSyncChunk( WeaveParamsPtr pb, long* bytesRemaining, long timeValue );
static Boolean		WriteMarkerTable( WeaveParamsPtr pb, long* bytesRemaining );
static Boolean		WriteHeaderChunk( WeaveParamsPtr pb, long* bytesRemaining );
static Boolean		WriteAlrmChunk( WeaveParamsPtr pb, long* bytesRemaining, long timeValue );
static Boolean 		WriteFillerChunk
	( WeaveParamsPtr pb, WeaveChunkPtr pFillerBlk, long* bytesRemaining );
static Boolean		WriteGotoChunk
	( WeaveParamsPtr pb, InStreamPtr isp, WeaveChunkPtr pFillerBlk, long* bytesRemaining );
static Boolean		WriteStopChunk
	( WeaveParamsPtr pb, InStreamPtr isp, WeaveChunkPtr pFillerBlk, long* bytesRemaining );


/*==========================================================================================
  ==========================================================================================
									Private Utility Routines
  ==========================================================================================
  ==========================================================================================*/



/*******************************************************************************************
 * Routine to allocate alternate I/O buffers for the weaving process. Bigger is better!
 *******************************************************************************************/
static char*	GetIOBuffer( long bufferSize )
	{
	char*	ioBuf;

	ioBuf = (char*) NewPtr( bufferSize );
	if ( ioBuf == NULL )
		{
		fprintf( stderr, "GetIOBuffer() - failed. Use larger MPW memory partition!\n" );
		fprintf( stderr, "                Use Get Info... from the Finder to set it.\n" );
		}

	return ioBuf;
	}


/*******************************************************************************************
 * Routine to sort the marker records into ascending order before the weaving
 * process begins.
 *******************************************************************************************/
static void		SortMarkers( WeaveParamsPtr pb )
	{
	long		k;
	long		n;
	MarkerRec	temp;

	SpinCursor(32);

	/* Insert an 'infinity' marker so that marker comparisons
	 * will always work properly.
	 */
	if ( pb->numMarkers < (pb->maxMarkers - 1) )
		{
		pb->marker[ pb->numMarkers++ ].markerTime = INFINITY_TIME;
		}
	else
		{
		fprintf( stderr, "SortMarkers() - unable to add infinity marker to table\n" );
		return;
		}

	/* The El Cheapo bubble sort to put markers
	 * into ascending order.
	 */
	for ( k = 0; k < pb->numMarkers - 1; k++ )
		{
		for ( n = k + 1; n < pb->numMarkers; n++ )
			{
			if ( pb->marker[n].markerTime < pb->marker[k].markerTime )
				{
				temp = pb->marker[k];
				pb->marker[k] = pb->marker[n];
				pb->marker[n] = temp;
				}
			}
		}
	}


/*******************************************************************************************
 * Routine to search the input streams and determine if any data is left to output.
 *******************************************************************************************/
static Boolean	InputDataRemaining( WeaveParamsPtr pb )
	{
	InStreamPtr	isp = pb->InStreamList;
	Boolean		eof = false;

	while ( (isp != nil) && (eof = isp->eof) )
		isp = isp->Next;
		
	return !eof;
	}


/*******************************************************************************************
 * Routine to search the input streams and find the stream whose chunk is temporally
 * "next" in time (least value numerically), and whose time is before the next marker
 * time in the stream.
 *******************************************************************************************/
static InStreamPtr	GetEarliestBelowNextMarker( WeaveParamsPtr pb, long sizeRemaining )
	{
	InStreamPtr	winner = nil;
	InStreamPtr	isp	= pb->InStreamList;
	long		timeToBeat = INFINITY_TIME;
	long		nextMarker = pb->marker[ pb->currMarkerIndex ].markerTime;

	/* Search the input stream buffers for a chunk whose timestamp 
	 * is "ealiest", ignoring any files whose EOF has been reached, and
	 * ignoring whether or not the block will fit in the current output block.
	 */
	for ( ; isp != nil; isp = isp->Next )
		{
		if ( (! isp->eof) && (isp->buf.weaveChunkTime < timeToBeat) )
			{
			timeToBeat = isp->buf.weaveChunkTime;
			winner = isp;
			}
		}

	/* Make sure we found something. It is an error if we don't
	 * because an external check is made to insure that there is at
	 * least one file which hasn't reached EOF.
	 */
	if ( winner == nil )
		{
		fprintf( stderr, "GetEarliestBelowNextMarker() - found nothing!\n" );
		return nil;
		}

	/* Force a marker table update if the "earliest" chunk we can find
	 * is at or after the next marker in time. It doesn't matter at this
	 * point whether or not the chunk can fit into the current output
	 * block because marker time takes precedence.
	 */
	if ( winner->buf.weaveChunkTime >= nextMarker )
		{
		pb->fUpdateMarkerTable = true;
		return nil;
		}

	/* If the "earliest" chunk will both fit into the space remaining
	 * and is below the next marker time, then output this chunk.
	 */
	if ( winner->buf.weaveChunkSize <= sizeRemaining )
		return winner;
	else
		return nil;
	}


/*******************************************************************************************
 * Routine to find the next block in the stream which is both below the next marker
 * temporally, and is a best fit for the 'size' given.
 *******************************************************************************************/
static InStreamPtr	GetBestFitBelowNextMarker( WeaveParamsPtr pb, long sizeRemaining )
	{
	InStreamPtr	winner = nil;
	InStreamPtr	isp = pb->InStreamList;
	long		sizeToBeat = -1;
	long		nextMarker = pb->marker[ pb->currMarkerIndex ].markerTime;

	/* Search the input stream buffers for the chunk whose
	 * timestamp is lowest. Ignore any files whose EOF has been reached.
	 */
	for ( ; isp != nil; isp = isp->Next )
		{
		if ( ( ! isp->eof ) && ( isp->buf.weaveChunkTime < nextMarker ) )
			{
			/* See if this chunk is a better fit than the one
			 * we currently have selected. If so, it becomes the
			 * selected chunk.
			 */
			if ( (isp->buf.weaveChunkSize <= sizeRemaining)
				&& (isp->buf.weaveChunkSize > sizeToBeat) )
				{
				sizeToBeat = isp->buf.weaveChunkSize;
				winner = isp;
				}
			}
		} /* end for loop */

	/* Return "best fit" or NIL */
	return winner;
	} /* end GetBestFitBelowNextMarker */


/*******************************************************************************************
 * Routine to read the next chunk header for a given input stream. Sets any necessary
 * flags and outputs diagnostic messages if there's any error.
 *******************************************************************************************/
static Boolean	ReadChunkHeader( WeaveParamsPtr pb, InStreamPtr isp )
	{
	long	fillChunkSize;

#if USE_MAC_IO
	long	bytesToRead;
	short	status;
#endif

	/* Load the next chunk header from the file we just wrote
	 * data from. If we reach EOF, set the EOF flag in the input
	 * stream control block. Skip any FILLER chunks we find.
	 */
	 
#if USE_MAC_IO
	bytesToRead = sizeof(WeaveChunk);
	while ( noErr == ( status = FSRead( isp->fd, &bytesToRead, &isp->buf ) ) )
#else
	while ( 1 == fread( &isp->buf, sizeof(WeaveChunk), 1, isp->fd ) )
#endif
		{
		/* We don't want to weave fillers into the output
		 * stream, so ignore any we encounter.
		 */
		if ( isp->buf.weaveChunkType == FILLER_DATA_TYPE )
			{
			/* =============== UNSPEAKABLE HACK !!! =================
			 * The following is done because it is possible to have written
			 * out a _partial_ FILL chunk due to the fixed buffer sizes we use.
			 * I.e., there may not have been enough space to write an entire
			 * FILL chunk to the output stream. We have three cases to consider:
			 *		1.	a well formed FILL chunk
			 *		2.	a FILL chunk with the 'FILL' and size intact
			 *		3.	a FILL chunk with only the 'FILL' intact
			 *
			 * Here's what we do: if the chunk size "looks like" it is bad
			 * (there are bits set in the high 8 bits of a 32-bit count), 
			 * we assume that we have case #3, above, and assume the chunk
			 * size is 4 bytes (just the 'FILL' header). Otherwise, we use
			 * the size as-is, even if it is smaller than sizeof(WeaveChunk)
			 * because the fseek() will take a negative value if the calculation
			 * to get us to the start of the next block requires us to "back up".
			 */
			if ( (isp->buf.weaveChunkSize & 0xff000000) != 0 )
				/* Assume only the 'FILL' is there */
				fillChunkSize = 4;
			else
				/* May or may not be a 12 byte basic chunk, but assume size is good */
				fillChunkSize = isp->buf.weaveChunkSize;

			/* Seek past the filler chunk so we don't try to weave it
			 * into the output stream.
			 */
#if USE_MAC_IO
			if ( noErr == SetFPos( isp->fd, fsFromMark,  fillChunkSize - sizeof(WeaveChunk) ) )
				continue;
#else
			if ( 0 == fseek( isp->fd, fillChunkSize - sizeof(WeaveChunk), SEEK_CUR ) )
				continue;
#endif
				
			fprintf( stderr, "ReadChunkHeader() - error seeking past filler in file: %s\n",
						isp->fileName );
				
			return false;
			} /* weaveChunkType == FILLER_DATA_TYPE */

		/* Check that the chunk size is <= the stream block size. If not,
		 * output an error message and bail out.
		 */
		if ( isp->buf.weaveChunkSize > pb->streamBlockSize )
			{
			fprintf( stderr, "ReadChunkHeader() - chunk size > streamblocksize in file: %s\n",
						isp->fileName );
			return false;
			}

		/* Add the starting time constant into the stream time
		 * for the newly read chunk. This causes it to sort into the
		 * output stream at the specified starting time. This also
		 * assumes that input streams are zero-relative.
		 */
		isp->buf.weaveChunkTime += isp->startTime;
		return true;
		} /* end while */

	/* Check for an EOF return. Set the stream's eof flag if we
	 * detect one, otherwise we've gotten a read error of some kind.
	 */
#if USE_MAC_IO
	if ( status == eofErr )
		{
		isp->eof = true;
		FSClose( isp->fd );
		PBFlushVolSync( (ParmBlkPtr) &isp->iopb );
		}
	else
		{
		fprintf( stderr, "ReadChunkHeader() - error reading next chunk header from: %s\n",
						isp->fileName );
		return false;
		}
#else
	if ( feof( isp->fd ) )
		{
		isp->eof = true;
		fclose ( isp->fd );
		}
	else
		{
		fprintf( stderr, "ReadChunkHeader() - error reading next chunk header from: %s\n",
						isp->fileName );
		return false;
		}
#endif
		
	return true;
	}


/*******************************************************************************************
 * Routine to write out the chunk specified by 'isp' to the stream specified by
 * 'pb'. Update the bytes remaining count by the number of bytes written. Also,
 * read the next chunk from the same file, setting the file's EOF flag if
 * appropriate.
 *******************************************************************************************/
static Boolean	WriteChunk( WeaveParamsPtr pb, InStreamPtr isp, long* bytesRemaining )
	{
	long	bytesToRead, bytesToWrite;

	/* Make sure there's space in the current block for the chunk
	 * we are about to write. This should never happen because the 
	 * chunk selection process is supposed to prevent this.
	 */
	if ( isp->buf.weaveChunkSize > (*bytesRemaining) )
		{
		fprintf( stderr, "WriteChunk() - chunk size too big!\n" );
		return false;
		}

	/* Write out the chunk header from the buffer in the input stream
	 * descriptor. Add the output stream base time to the time in the
	 * chunk to create the desired time bias in the output stream.
	 */
	isp->buf.weaveChunkTime += pb->outputBaseTime;
	
	bytesToWrite = sizeof(WeaveChunk);
	
#if USE_MAC_IO
	if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, &isp->buf ) )
#else
	if ( 1 != fwrite( &isp->buf, sizeof(WeaveChunk), 1, pb->outputStreamFd ) )
#endif
		{
		fprintf( stderr, "WriteChunk() - error writing chunk header\n" );
		return false;
		}

	/* Read the actual chunk data from the selected input file
	 */
	bytesToRead = isp->buf.weaveChunkSize - sizeof(WeaveChunk);
#if USE_MAC_IO
	if ( noErr != FSRead( isp->fd, &bytesToRead, pb->scratchBuf ) )
#else
	if ( 1 != fread( pb->scratchBuf, bytesToRead, 1, isp->fd ) )
#endif
		{
		fprintf( stderr, "WriteChunk() - error reading file: %s\n",	isp->fileName );
		return false;
		}

	/* Write out the chunk data we just read */
#if USE_MAC_IO
	if ( noErr != FSWrite( pb->outputStreamFd, &bytesToRead, pb->scratchBuf ) )
#else
	if ( 1 != fwrite( pb->scratchBuf, bytesToRead, 1, pb->outputStreamFd ) )
#endif
		{
		fprintf( stderr, "WriteChunk() - error writing chunk data\n" );
		return false;
		}

	/* Update remaining free bytes in the theoretical output buffer.
	 * NOTE: the 'remaining' count is maintained only to know when a block
	 *			boundary is approaching, not that any buffer is actually being
	 *			consumed. This allow the algorithm to write out blocks that
	 *			will match the stream reader's input buffers.
	 */
	(*bytesRemaining) -= sizeof(WeaveChunk) + bytesToRead;
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	/* Load the next chunk header from the file we just wrote
	 * data from. If we reach EOF, set the EOF flag in the input
	 * stream control block.
	 */
	return ReadChunkHeader( pb, isp );
	}


/*******************************************************************************************
 * Routine to write out the stream header chunk. 
 *******************************************************************************************/
static Boolean		WriteHeaderChunk( WeaveParamsPtr pb, long* bytesRemaining )
	{
	long			index, bytesToWrite;
	DSHeaderChunk	header;

	/* If the flag that says we should write a header out is false,
	 * then we don't write anything.
	 */
	if ( ! pb->fWriteStreamHeader )
		return true;

	/* Make sure there's space in the current block for the chunk
	 * we are about to write.
	 */
	if ( sizeof(DSHeaderChunk) > (*bytesRemaining) )
		{
		fprintf( stderr, "WriteHeaderChunk() - not enough space!\n" );
		return false;
		}

	/* Format the header chunk for output using parameters passed
	 * to us initially.
	 */
	header.chunkType		= HEADER_CHUNK_TYPE;
	header.subChunkType		= 0;
	header.chunkSize		= sizeof(DSHeaderChunk);
	header.channel			= 0;
	header.time				= 0;
	header.headerVersion	= DS_STREAM_VERSION;
	header.streamBlockSize	= pb->streamBlockSize;
	header.streamBuffers	= pb->streamBuffers;
	header.audioClockChan	= pb->audioClockChan;
	header.enableAudioChan	= pb->enableAudioChan;
	header.streamerDeltaPri	= pb->streamerDeltaPri;	
	header.dataAcqDeltaPri	= pb->dataAcqDeltaPri;
	header.numSubsMsgs		= pb->numSubsMsgs;

	/* Copy the instrument preload table */
	for ( index = 0; index < sizeof(pb->preloadInstList)
							/ sizeof(pb->preloadInstList[0]); index++ )
		header.preloadInstList[ index ] = pb->preloadInstList[ index ];
	
	/* Copy the subscriber tag table */
	for ( index = 0; index < sizeof(pb->subscriberList)
							/ sizeof(pb->subscriberList[0]); index++ )
		header.subscriberList[ index ] = pb->subscriberList[ index ];

	/* Write the header chunk to the output file */
	bytesToWrite = sizeof(DSHeaderChunk);
#if USE_MAC_IO
	if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, &header ) )
#else
	if ( 1 != fwrite( &header, bytesToWrite, 1, pb->outputStreamFd ) )
#endif
		{
		fprintf( stderr, "WriteHeaderChunk() - error writing stream header\n" );
		return false;
		}

	/* Update remaining free bytes in the theoretical output buffer.
	 */
	(*bytesRemaining) -= sizeof(DSHeaderChunk);
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	return true;
	}


/*******************************************************************************************
 * Routine to write out the marker table chunk. 
 *******************************************************************************************/
static Boolean		WriteMarkerTable( WeaveParamsPtr pb, long* bytesRemaining )
	{
	long			sizeOfMarkerData, bytesToWrite;
	MarkerChunk		markerChunk;

	/* If the flag that says we should write a marker table is false,
	 * then we don't write anything.
	 */
	if ( ! pb->fWriteMarkerTable )
		return true;

	/* Calculate the size of the marker table data.
	 *
	 * NOTE: 	We intentionally omit the last marker in the
	 *			table because it is an infinity placeholder
	 *			that we create at the start of the weaving 
	 *			process. This is done to keep the marker
	 *			comparison logic simple.
	 */
	sizeOfMarkerData = (pb->numMarkers - 1) * sizeof(MarkerRec);

	/* Format the marker table chunk for output using parameters passed
	 * to us initially.
	 */
	markerChunk.chunkType		= DATAACQ_CHUNK_TYPE;
	markerChunk.subChunkType	= MARKER_TABLE_SUBTYPE;
	markerChunk.chunkSize		= sizeof(MarkerChunk) + sizeOfMarkerData;
	markerChunk.channel			= 0;
	markerChunk.time			= 0;


	/* Make sure there's space in the current block for the chunk
	 * we are about to write.
	 */
	if ( markerChunk.chunkSize > (*bytesRemaining) )
		{
		fprintf( stderr, "WriteMarkerTable() - marker table will not into first stream block\n" );
		return false;
		}

	/* Write the marker table chunk header to the output file */
	bytesToWrite = sizeof(MarkerChunk);
	
#if USE_MAC_IO
	if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, &markerChunk ) )
#else
 	if ( 1 != fwrite( &markerChunk, sizeof(MarkerChunk), 1, pb->outputStreamFd ) )
#endif
		{
		fprintf( stderr, "WriteMarkerTable() - error writing marker table header\n" );
		return false;
		}

	/* Write the marker data to the output file */
	bytesToWrite = sizeOfMarkerData;
#if USE_MAC_IO
	if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, pb->marker) )
#else
	if ( 1 != fwrite( pb->marker, sizeOfMarkerData, 1, pb->outputStreamFd ) )
#endif
		{
		fprintf( stderr, "WriteMarkerTable() - error writing marker table data\n" );
		return false;
		}

	/* Update remaining free bytes in the theoretical output buffer.
	 */
	(*bytesRemaining) -= markerChunk.chunkSize;
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	return true;
	}


/*******************************************************************************************
 * Routine to write out a sync chunk for the Control Subscriber. This is done at the 
 * beginning of every marker block to allow the Control Subscriber to conditionally send
 * out a "clock sync" call at runtime. The conditional part comes from not wanting to
 * cause buffer flushing in subscribers unless the receipt of the data is as a result
 * of a branching operation (normal playthrough should not cause flushing and clock sync
 * should not be called).
 *******************************************************************************************/
static Boolean	WriteSyncChunk( WeaveParamsPtr pb, long* bytesRemaining, long timeValue )
	{
	ControlChunk	control;
	long bytesToWrite;	

	/* Make sure there's space in the current block for the chunk
	 * we are about to write. This should never happen because the 
	 * chunk selection process is supposed to prevent this.
	 */
	if ( sizeof(ControlChunk) > (*bytesRemaining) )
		{
		fprintf( stderr, "WriteSyncChunk() - not enough space!\n" );
		return false;
		}

	/* Format the Control Subscriber 'SYNC' chunk for output. 
	 * Add the output stream base time to the time in the chunk to 
	 * create the desired time bias in the output stream.
	 */
	control.chunkType		= CONTROL_CHUNK_TYPE;
	control.subChunkType	= CONTROL_SYNC_SUBTYPE;
	control.chunkSize		= sizeof(ControlChunk);
	control.channel			= 0;
	control.time			= timeValue + pb->outputBaseTime;
	control.u.sync.value	= control.time;

	/* Write the sync chunk to the output file */
	bytesToWrite = sizeof(ControlChunk);

#if USE_MAC_IO
	if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, &control) )
#else
 	if ( 1 != fwrite( &control, sizeof(ControlChunk), 1, pb->outputStreamFd ) )
#endif

		{
		fprintf( stderr, "WriteSyncChunk() - error writing SYNC chunk\n" );
		return false;
		}

	/* Update remaining free bytes in the theoretical output buffer.
	 */
	(*bytesRemaining) -= sizeof(ControlChunk);
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	return true;
	}

/*******************************************************************************************
 * Routine to write out a alrm chunk for the Control Subscriber. This is done after the 
 * a goto chunk.
 *******************************************************************************************/
static Boolean	WriteAlrmChunk( WeaveParamsPtr pb, long* bytesRemaining, long timeValue )
	{
	ControlChunk	Alrm_Chunk;
	long bytesToWrite;	

	/* Make sure there's space in the current block for the alrm chunk
	 * we are about to write. This should never happen because the
	 * previous chunk processing should have prevented this.
	 */
	if ( sizeof(ControlChunk) > (*bytesRemaining) )					
		{
		fprintf( stderr, "WriteAlrmChunk() - not enough space!\n" );
		return false;
		}

	/* Format the Control Subscriber 'ALRM' chunk for output. 
	 * Time is the absolute time. This time should already
	 * include the output stream base time to create the desired
	 * time bias in the output stream.
	 */
	Alrm_Chunk.chunkType		= CONTROL_CHUNK_TYPE;
	Alrm_Chunk.subChunkType		= ALRM_CHUNK_SUBTYPE;
	Alrm_Chunk.chunkSize		= sizeof(ControlChunk);
	Alrm_Chunk.channel			= 0;
	Alrm_Chunk.time				= timeValue; 
	Alrm_Chunk.u.alarm.options	= 0;
	Alrm_Chunk.u.alarm.newTime	= 0;

	/* Write the sync chunk to the output file */
	bytesToWrite = sizeof(ControlChunk);

#if USE_MAC_IO
	if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, &Alrm_Chunk) )
#else
 	if ( 1 != fwrite( &Alrm_Chunk, sizeof(ControlChunk), 1, pb->outputStreamFd ) )
#endif

		{
		fprintf( stderr, "WriteAlrmChunk() - error writing ALRM chunk\n" );
		return false;
		}

	/* Update remaining free bytes in the theoretical output buffer.
	 */
	(*bytesRemaining) -= sizeof(ControlChunk);
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	return true;

	} /* end WriteAlrmChunk */
	
/*******************************************************************************************
 * Routine to write out the filler chunk.
 *******************************************************************************************/
static Boolean WriteFillerChunk( WeaveParamsPtr pb, WeaveChunkPtr pFillerBlk, long* bytesRemaining )

	{ /* Begin WriteFillerChunk */
	
	/* Write a filler block at the end of the current block. */
	pFillerBlk->weaveChunkSize	= *bytesRemaining;

#if USE_MAC_IO			
	if ( noErr != FSWrite( pb->outputStreamFd, bytesRemaining, pFillerBlk ) )
#else			
	if ( 1 != fwrite( pFillerBlk, *bytesRemaining, 1, pb->outputStreamFd ) )
#endif
		{
		fprintf( stderr, "WriteFillerChunk() - error writing Filler chunk\n" );
		return false;
		}

	/* Reset free buffer space variable */
	*bytesRemaining = pb->streamBlockSize;
	
	return true;

	} /* end WriteFillerChunk */
				

/*******************************************************************************************
 * Routine to write out a goto chunk for the Alrm_Chunk Subscriber.
 *******************************************************************************************/
static Boolean WriteGotoChunk
	( WeaveParamsPtr pb, InStreamPtr isp, WeaveChunkPtr pFillerBlk, long* bytesRemaining )
	{ /* Begin WriteGotoChunk */

	long			bytesToWrite;	
	CtrlChunksPtr	tempPtr;

	/* Write out any last goto chunk or write out the goto chunk if
	/* the "earliest" chunk found is after the goto chunk time.
	 */
	 if ( ( isp->buf.weaveChunkTime > pb->GotoChunkList->DataChunk.time ) ||
	 	 ( isp->eof ) )
		{

		/* Make sure there's space in the current block for the 2 chunks
		 * we are about to write.	 */
		if ( 2 * sizeof(ControlChunk) > (*bytesRemaining) )

			/* Write a filler block at the end of the current block. */
			if ( ! WriteFillerChunk( pb, pFillerBlk, bytesRemaining ) )
				return false;
			
		/* Add the output stream base time to the time in the chunk
		 * to create the desired time bias in the output stream.
		 */
		 pb->GotoChunkList->DataChunk.time += pb->outputBaseTime;

		/* Write out the goto chunk */
		bytesToWrite = sizeof(ControlChunk);		

#if USE_MAC_IO
		if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, &pb->GotoChunkList->DataChunk ) )
#else
		if ( 1 != fwrite( &pb->GotoChunkList->DataChunk, bytesToWrite, 1, pb->outputStreamFd ) )
#endif

			{
			fprintf( stderr, "WriteGotoChunk() - error writing GOTO chunk\n" );
			return false;
			}

		/* Update remaining free bytes in the theoretical output buffer. */
		(*bytesRemaining) -= bytesToWrite;

		/* Write the alrm chunk. Goto chunk is followed by the alrm chunk.
		/* There should be enough space left to write the alrm chunk.
		 */
		if ( ! WriteAlrmChunk( pb, bytesRemaining, pb->GotoChunkList->DataChunk.time ) )
			{
			fprintf( stderr, "WriteGotoChunk() - error writing ALRM chunk\n" );
			return false;
			}

		/* Update remaining free bytes in the theoretical output buffer. */
		if ( ( *bytesRemaining) == 0 )
			(*bytesRemaining) = pb->streamBlockSize;

		/* Write a filler block at the end of the current block. */
		else if ( ! WriteFillerChunk( pb, pFillerBlk, bytesRemaining ) )
				return false;

		/* Delete the current Goto Chunk object that was just written */
		tempPtr					= pb->GotoChunkList->Next;
		free ( pb->GotoChunkList );
		pb->GotoChunkList		= tempPtr;
		if ( pb->GotoChunkList == nil )
			pb->fWriteGotoChunk = false;
		
	} /* end if write goto chunk */
	
	return true;

	} /* End WriteGotoChunk */
	
/*******************************************************************************************
 * Routine to write out a goto chunk for the Control Subscriber.
 *******************************************************************************************/
static Boolean WriteStopChunk
	( WeaveParamsPtr pb, InStreamPtr isp, WeaveChunkPtr pFillerBlk, long* bytesRemaining )
	{ /* Begin WriteStopChunk */

	long			bytesToWrite;	
	CtrlChunksPtr	tempPtr;

	/* Write out any last stop chunk or write out the stop chunk if
	/* the "earliest" chunk found is after the stop chunk time.
	 */
	 if ( ( isp->buf.weaveChunkTime > pb->StopChunkList->DataChunk.time ) ||
		 ( isp->eof ) )
		{

		/* Make sure there's space in the current block for the chunk
		 * we are about to write.	 */
		if ( sizeof(ControlChunk) > (*bytesRemaining) )

			/* Write a filler block at the end of the current block. */
			if ( ! WriteFillerChunk( pb, pFillerBlk, bytesRemaining ) )
				return false;
			
		/* Add the output stream base time to the time in the chunk
		 * to create the desired time bias in the output stream.
		 */
		 pb->StopChunkList->DataChunk.time += pb->outputBaseTime;

		/* Write out the stop chunk */
		bytesToWrite = sizeof(ControlChunk);		

#if USE_MAC_IO
		if ( noErr != FSWrite( pb->outputStreamFd, &bytesToWrite, &pb->StopChunkList->DataChunk ) )
#else
		if ( 1 != fwrite( &pb->StopChunkList->DataChunk, bytesToWrite, 1, pb->outputStreamFd ) )
#endif

			{
			fprintf( stderr, "WriteStopChunk() - error writing stop chunk\n" );
			return false;
			}

		/* Update remaining free bytes in the theoretical output buffer. */
		(*bytesRemaining) -= bytesToWrite;
		if ( ( *bytesRemaining) == 0 )
			(*bytesRemaining) = pb->streamBlockSize;

		/* Write a filler block at the end of the current block. */
		else if ( ! WriteFillerChunk( pb, pFillerBlk, bytesRemaining ) )
				return false;


		/* Delete the current Stop Chunk object that was just written */
		tempPtr					= pb->StopChunkList->Next;
		free ( pb->StopChunkList );
		pb->StopChunkList		= tempPtr;
		if ( pb->StopChunkList == nil )
			pb->fWriteStopChunk = false;

	} /* end if write stop chunk */
	
	return true;

	} /* End WriteStopChunk */


/*==========================================================================================
  ==========================================================================================
										Public Routines
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * Routine to open all the stream files. The file name pointers have been
 * set up before we get here.
 *******************************************************************************************/
Boolean		OpenDataStreams( WeaveParamsPtr pb )
	{

	InStreamPtr	isp = pb->InStreamList;
	
#if USE_MAC_IO
	short		status;
	Str255		pName;
	StringPtr	pStr = pName;
#endif

	/* Sort the marker array in ascending time order.
	 */
	SortMarkers( pb );

	/* Open the output stream file
	 */

#if USE_MAC_IO

	strcpy ( (char*) pStr, pb->outputStreamName );
	pStr = C2PStr(pStr);
	status = FSOpen( pStr, 0, &pb->outputStreamFd );
	if (status == noErr)
		{
		status = FSClose( pb->outputStreamFd );
		status = FSDelete( pStr, 0 );
		}
		
	status = Create( pStr, 0, 'MPS ', '????' );
	if (status != noErr )
		{
		fprintf (stderr, "error %d, unable to create output data stream file: %s\n", 
					status, pb->outputStreamName );
		return false;
		}
	
	status = FSOpen( pStr, 0, &pb->outputStreamFd );

	if (status != noErr )
		{
		fprintf (stderr, "error %d, unable to open output data stream file: %s\n", 
					status, pb->outputStreamName );
		return false;
		}
		
#else
	pb->outputStreamFd = fopen( pb->outputStreamName, "wb+" );
	if ( pb->outputStreamFd == nil )
		{
		fprintf (stderr, "unable to open output data stream file: %s\n", 
					pb->outputStreamName );
		return false;
		}
		
#if USE_BIGGER_BUFFERS
	if ( setvbuf( pb->outputStreamFd, GetIOBuffer( pb->ioBufferSize ), _IOFBF, pb->ioBufferSize ) )
		{
		fprintf (stderr, "setvbuf() failed for file: %s\n", pb->outputStreamName );
		return false;
		}
#endif

#endif

	/* Open all of the input stream files
	 */
	for ( ; isp != nil; isp = isp->Next )
		{

#if USE_MAC_IO

		strcpy ( (char *) pStr, isp->fileName );
		pStr = C2PStr(pStr);
		
		isp->iopb.ioCompletion	= NULL;
		isp->iopb.ioNamePtr		= pStr;
		isp->iopb.ioVRefNum		= 0;
		isp->iopb.ioVersNum		= 0;
		isp->iopb.ioPermssn		= fsRdPerm;
		isp->iopb.ioMisc		= NULL;
		
		status = PBOpenSync( (ParmBlkPtr)&isp->iopb ); 

		if (status != noErr )
			{
			fprintf (stderr, "error %d, unable to open input data stream file: %s\n", 
								status, isp->fileName );
			return false;
			}
			
		isp->fd					= isp->iopb.ioRefNum;

#else
		isp->fd = fopen( isp->fileName, "r" );
		if ( isp->fd == nil )
			{
			fprintf (stderr, "unable to open input data stream file: %s\n", 
								isp->fileName );
			return false;
			}

#if USE_BIGGER_BUFFERS
		if ( setvbuf( isp->fd, GetIOBuffer( pb->ioBufferSize ), _IOFBF, pb->ioBufferSize ) )
			{
			fprintf (stderr, "setvbuf() failed for file: %s\n", isp->fileName );
			return false;
			}
#endif

#endif

		} /*end for loop */
		

	return true;
	}


/*******************************************************************************************
 * Routine to close all the open stream files.
 *******************************************************************************************/
void		CloseDataStreams( WeaveParamsPtr pb )
	{
	InStreamPtr	isp = pb->InStreamList;

	/* Close the output file */
	if ( pb->outputStreamFd )

#if USE_MAC_IO
		FSClose( pb->outputStreamFd );
		FlushVol( NULL, pb->outputStreamFd );
#else
		fclose( pb->outputStreamFd );
#endif

	/* All of the input stream files are
	 * closed already.
	 */

	/* Free the scratch buffer memory */
	if ( pb->scratchBuf )
		DisposPtr( pb->scratchBuf );

	} /* end CloseDataStreams */


/*******************************************************************************************
 * Routine to merge all input data streams into a single output data stream. The
 * parameter block contains all the info we need to do this already set up for us.
 *
 *	Rules for output chunk selection:
 *	---------------------------------
 *	1.	write the lowest TIME chunk that will fit into the current block
 *		whose time is below the next MARKER
 *
 *	2.	write any chunk that will fit into the current block
 *		whose time is below the next MARKER
 *
 *	3.	write a FILLER chunk
 *
 *	while there's any data remaining in the input collection
 *		get next chunk as selected by lowest TIME value
 *		if the selected chunk will fit into the current block
 *				&& the chunk's TIME is below the next MARKER value
 *					write the selected chunk
 *		else if there's another chunk in the input collection that will fit into the current block
 *				&& the chunk's TIME is below the next MARKER value
 *					write that block
 *		else
 *			write a FILLER block
 *
 *******************************************************************************************/
long		WeaveStreams( WeaveParamsPtr pb )
	{
	InStreamPtr		isp;
	WeaveChunkPtr	pFillerBlk;
	long			remainingInCurrentBlock;
	long			status;
	long			syncChunkTime;
	Boolean			fWriteSyncChunk;
	long			markerChunkFPOS;

	/* Start marker processing at the first entry */
	pb->currMarkerIndex = 0;

	/* Allocate the output stream's scratch buffer. This buffer
	 * is used to copy data from input streams to the output stream.
	 */
	pb->scratchBuf = (char*) NewPtr( pb->streamBlockSize );
	if ( pb->scratchBuf == nil )
		{
		status = -1;
		goto BAILOUT;
		}

	/* Allocate the "filler chunk" buffer. This is allocated to accomodate
	 * the largest possible filler chunk. As we need to write out fillers,
	 * we stuff the *actual* filler chunk size into the header of this buffer
	 * and write out only what we need. 
	 */
	remainingInCurrentBlock = pb->streamBlockSize;
	pFillerBlk = (WeaveChunkPtr) NewPtr( pb->streamBlockSize );
	if ( pFillerBlk == nil )
		{
		status = -2;
		goto BAILOUT;
		}

	/* Initialize the filler buffer */
	memset( pFillerBlk, 0, pb->streamBlockSize );
	pFillerBlk->weaveChunkType	= FILLER_DATA_TYPE;
	pFillerBlk->weaveChunkSize	= 0;
	pFillerBlk->weaveChunkTime	= 0;

	/* Pre-load chunk headers from all input streams */
	for ( isp = pb->InStreamList ; isp != nil; isp = isp->Next )
		if ( ! ReadChunkHeader( pb, isp ) )
			{
			status = -43;	
			goto BAILOUT;
			}

	/* Setup to force a sync chunk to be written at the beginning of the
	 * file. This essentially simulates a marker at time zero.
	 */
	syncChunkTime = 0;
/*	fWriteSyncChunk = true;*/
	fWriteSyncChunk = false;

	/* Write out a stream header chunk if the user has specified that one
	 * should be output.
	 */
	if ( ! WriteHeaderChunk( pb, &remainingInCurrentBlock ) )
		{
		status = -399;
		goto BAILOUT;
		}

	/* Write out the marker table, this time as a place holder since none of
	 * its entries will have been filled in yet.
	 */
	markerChunkFPOS = pb->streamBlockSize - remainingInCurrentBlock;

	if ( ! WriteMarkerTable( pb, &remainingInCurrentBlock ) )
		{
		status = -400;
		goto BAILOUT;
		}

#if FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY
	/* Force input stream data onto the next stream block boundary so that branching
	 * to time zero can be done meaningfully. If options dictate that neither a header
	 * nor a marker table have been written to the output stream, then skip writing
	 * a filler chunk.
	 */
	if ( pb->fWriteStreamHeader || pb->fWriteMarkerTable )
		{
		pFillerBlk->weaveChunkSize	= remainingInCurrentBlock;
#if USE_MAC_IO
		if ( noErr != FSWrite( pb->outputStreamFd, &remainingInCurrentBlock, pFillerBlk ) )
#else
		if ( 1 != fwrite( pFillerBlk, remainingInCurrentBlock, 1, pb->outputStreamFd ) )
#endif
			{
			return -402;
			goto BAILOUT;
			}

		remainingInCurrentBlock = pb->streamBlockSize;
		}

#endif

	/*************************************************************************/
	/* Main loop to merge all input streams into a single output stream file */
	/*************************************************************************/
	while ( InputDataRemaining( pb ) )
		{
		SpinCursor(32);
		
		/* Write the sync chunk to the output stream */
		if ( fWriteSyncChunk )
			{
			/* Reset the flag that forces us to write out a Control Subscriber
			 * 'SYNC' chunk at the beginning of a marker block.
			 */
			fWriteSyncChunk = false;

			/* Write the sync chunk to the output stream */
			if ( ! WriteSyncChunk( pb, &remainingInCurrentBlock, syncChunkTime ) )
				{
				status = -15;
				goto BAILOUT;
				}
			} /* if fWriteSyncChunk */

		else if ( (isp = GetEarliestBelowNextMarker( pb, remainingInCurrentBlock )) != nil )
			{
			/* Write out a goto control chunk, if the user has specified that one
			 * should be output.
			 */ 
			if ( pb->fWriteGotoChunk )
				{
				if ( ! WriteGotoChunk( pb, isp, pFillerBlk, &remainingInCurrentBlock ) )
					{
					status = -15;
					goto BAILOUT;
					}
				
				} /*if fWriteGotoChunk */
	 		 
			/* Write out a stop control chunk, if the user has specified that one
			 * should be output.
			 */ 
			if ( pb->fWriteStopChunk )
				{
				if ( ! WriteStopChunk( pb, isp, pFillerBlk, &remainingInCurrentBlock ) )
					{
					status = -15;
					goto BAILOUT;
					}
				} /*if fWriteStopChunk */ 

			if ( ! WriteChunk( pb, isp, &remainingInCurrentBlock ) )
				{
				status = -3;
				goto BAILOUT;
				}
			} /* else-if GetEarliestBelowNextMarker != nil */

		else if ( (isp = GetBestFitBelowNextMarker( pb, remainingInCurrentBlock )) != nil )
			{
			if ( ! WriteChunk( pb, isp, &remainingInCurrentBlock ) )
				{
				status = -4;
				goto BAILOUT;
				}
			} /* else-if GetBestFitBelowNextMarker != nil */

		else
			/* Can't find anything to fit into the current block. This can be
			 * because there isn't enough space to write the next chunk, or
			 * because we're hitting a marker and need to get all blocks in the
			 * output stream to align in time to the next marker. Hence, we
			 * write a filler block at the end of the current block.
			 */
			{
			pFillerBlk->weaveChunkSize	= remainingInCurrentBlock;

#if USE_MAC_IO			
			if ( noErr != FSWrite( pb->outputStreamFd, &remainingInCurrentBlock, pFillerBlk ) )
#else			
			if ( 1 != fwrite( pFillerBlk, remainingInCurrentBlock, 1, pb->outputStreamFd ) )
#endif
				{
				status = -5;
				goto BAILOUT;
				}

			/* Reset free buffer space variable */
			remainingInCurrentBlock = pb->streamBlockSize;
			} /* else Can't find anything to fit into current block */

		/* Update the marker table if one of the search processes 
		 * determined that it was necessary. This must be done after the
		 * optional writing of a filler chunk so that the file position
		 * reported by ftell() will be correct.
		 */
		if ( pb->fUpdateMarkerTable )
			{
			/* Reset the update flag */
			pb->fUpdateMarkerTable = false;
		
			/* Update the marker's file position value and time, and
			 * bump the next marker index to the next position
			 * in the marker table.
			 */
			syncChunkTime = pb->marker[ pb->currMarkerIndex ].markerTime;
			fWriteSyncChunk = true;

#if USE_MAC_IO
			GetFPos(  pb->outputStreamFd, &pb->marker[ pb->currMarkerIndex ].markerOffset );
#else
			pb->marker[ pb->currMarkerIndex ].markerOffset = ftell( pb->outputStreamFd );
#endif

			/* Advance to next marker to be filled in the marker table */
			pb->currMarkerIndex++;

			if ( pb->currMarkerIndex >= pb->numMarkers )
				{
				fprintf( stderr, "WeaveStreams() - generating too many markers!\n" );
				return -10;
				}
			} /* if fUpdateMarkerTable */

		} /* end while */

		/* Write out any last stop or goto chunks.  There should only be one
		 * these chunk left since we are at the end.
		 */
		if ( pb->fWriteGotoChunk )
			{
			if ( ! WriteGotoChunk( pb, isp, pFillerBlk, &remainingInCurrentBlock ) )
				{
				status = -15;
				goto BAILOUT;
				}

			} /*if fWriteGotoChunk */
	
		else if ( pb->fWriteStopChunk )
			{
			if ( ! WriteStopChunk( pb, isp, pFillerBlk, &remainingInCurrentBlock ) )
				{
				status = -15;
				goto BAILOUT;
				}
			} /*if fWriteStopChunk */ 

		if ( remainingInCurrentBlock != pb->streamBlockSize )
			{
			/* Write a filler chunk to fill out the end of the last buffer 
			 * we will write to the output stream if we currently have 
			 * an incomplete block.
			 */
			pFillerBlk->weaveChunkSize	= remainingInCurrentBlock;

#if USE_MAC_IO
			if ( noErr != FSWrite( pb->outputStreamFd, &remainingInCurrentBlock, pFillerBlk ) )
#else
			if ( 1 != fwrite( pFillerBlk, remainingInCurrentBlock, 1, pb->outputStreamFd ) )
#endif
				{
				return -6;
				goto BAILOUT;
				}
			} /* if remainingInCurrentBlock != pb->streamBlockSize */ 

	/* Write out the marker table, this time the table will have all its entries
	 * filled in. First, seek back to the place where it was first written, and
	 * then OVERWRITE it in place.
	 */
#if USE_MAC_IO
	if ( noErr != SetFPos( pb->outputStreamFd, fsFromStart, markerChunkFPOS ) )
#else
	if ( 0 != fseek( pb->outputStreamFd, markerChunkFPOS, SEEK_SET ) )
#endif
			{
			return -401;
			goto BAILOUT;
			}

	if ( ! WriteMarkerTable( pb, &remainingInCurrentBlock ) )
		{
		status = -400;
		goto BAILOUT;
		}

	status = 0;

BAILOUT:
	if ( pFillerBlk )
		DisposPtr( (void*) pFillerBlk );

	return status;
	
	} /* end WeaveStreams */