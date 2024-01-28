/*******************************************************************************************
 *	File:			WeaveStream.h
 *
 *	Contains:		definitions for WeaveStream.c
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/2/93		jb		Add numSubsMsgs to weave parameter
 *block 11/9/93		jb		Move definition of MarkerRec to
 *MarkerChunk.h 10/25/93	jb		Add fWriteMarkerTable flag;
 *increase MAX_INPUT_STREAMS to 20; Add maxMarkers field to param block
 *	10/13/93	jb		Add fields to WeaveParams for stream header
 *output 6/27/93		jb		Add ioBufferSize to parameter
 *block 6/17/93		jb		Add CHAR4LITERAL macro and switch to
 *using it in place of character literals for portability's sake. 5/18/93
 *jb		Switch to using hexadecimal const for FILLER_DATA_TYPE to keep
 *						compiler from being confused about scalar
 *size. 5/16/93		jb		New today.
 *
 *******************************************************************************************/

#ifndef __WEAVESTREAM_H__
#define __WEAVESTREAM_H__

#ifndef __TYPES__
#include <Types.h>
#endif

#ifndef __STDIO__
#include <stdio.h>
#endif

#ifndef _DSSTREAMHEADER_H_
#include "DSStreamHeader.h"
#endif

#ifndef __MARKERCHUNK_H__
#include "MarkerChunk.h"
#endif

#define MAX_INPUT_STREAMS 20
#define INFINITY_TIME (0x7fffffff)

#define CHAR4LITERAL(a, b, c, d)                                              \
  ((unsigned long)(a << 24) | (b << 16) | (c << 8) | d)

#define FILLER_DATA_TYPE CHAR4LITERAL ('F', 'I', 'L', 'L')

/*********************************************************/
/* The data header that preceeds every stream data chunk */
/*********************************************************/
typedef struct WeaveChunk
{
  unsigned long weaveChunkType; /* 4 byte ASCII subscriber data type */
  unsigned long weaveChunkSize; /* size of chunk including this header */
  unsigned long weaveChunkTime; /* time when chunk should be output */
} WeaveChunk, *WeaveChunkPtr;

/*************************************************/
/* Input stream descriptor, one per input stream */
/*************************************************/
typedef struct InStream
{
  Boolean eof;    /* true when EOF reached */
  long priority;  /* file data priority */
  long startTime; /* file data start time in output stream */
  char *fileName; /* name of file containing stream data */
  FILE *fd;       /* file descriptor for reading file data */
  WeaveChunk buf; /* buffer for current header */
} InStream, *InStreamPtr;

/***************************************/
/* WeaveStream() input parameter block */
/***************************************/
typedef struct WeaveParams
{
  long ioBufferSize;      /* size of I/O buffers we should allocate */
  long mediaBlockSize;    /* physical block size of the output media */
  long streamBlockSize;   /* logical buffering block size for stream */
  long outputBaseTime;    /* beginning time for output file */
  char *outputStreamName; /* name of output stream file */
  FILE *outputStreamFd;   /* output stream file descriptor */
  char *scratchBuf;       /* scratch buffer for file copying */

  long numInputStreams; /* number of input streams */
  InStream inStream[MAX_INPUT_STREAMS];

  /**********************/
  /* Stream header info */
  /**********************/

  Boolean fWriteStreamHeader; /* flag: write a stream header */
  long streamBuffers;    /* number of stream buffers to be used for playback */
  long audioClockChan;   /* number of audio channel to be used for clock */
  long enableAudioChan;  /* mask of audio channels to be enabled initially */
  long streamerDeltaPri; /* streamer delta priority */
  long dataAcqDeltaPri;  /* data acquisition delta priority */
  long numSubsMsgs;      /* number of subscriber messages to allocate */

  long preloadInstList[16];
  /* NULL terminated table of tags of instruments
   * to preload for stream. Definitions of tag
   * values in SAudioSubscriber.h
   */

  DSHeaderSubs subscriberList[DS_HDR_MAX_SUBSCRIBER];
  /* NULL terminated table of tags of subscribers
   * used in this stream. Subscriber tags are
   * defined in header files for individual subscribers.
   */

  /***********************/
  /* Marker related info */
  /***********************/

  Boolean fWriteMarkerTable;  /* flag: write marker data to output stream */
  Boolean fUpdateMarkerTable; /* flag to force update of marker table */
  long maxMarkers;      /* physical size of marker table in # of entries */
  long numMarkers;      /* actual number of marker entries */
  long nextMarkerIndex; /* offset to next marker value to use */
  long currMarkerIndex; /* state variable for processing markers */
  MarkerRecPtr marker;  /* ptr to array of markers to fill in */

} WeaveParams, *WeaveParamsPtr;

/*******************/
/* Public routines */
/*******************/
Boolean OpenDataStreams (WeaveParamsPtr pb);
void CloseDataStreams (WeaveParamsPtr pb);
long WeaveStreams (WeaveParamsPtr paramBlk);

#endif /* __WEAVESTREAM_H__ */
