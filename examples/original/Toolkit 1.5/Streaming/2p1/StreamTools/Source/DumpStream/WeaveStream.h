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
 *	9/16/94		dsm		Added include DSStreamHeader.h.
 *	5/18/93		jb		Switch to using hexadecimal const for
 *FILLER_DATA_TYPE to keep compiler from being confused about scalar size.
 *	5/16/93		jb		New today.
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

#define MAX_INPUT_STREAMS 10
#define MAX_MARKERS 64
#define INFINITY_TIME (0x7fffffff)

#define FILLER_DATA_TYPE (0x46494c4cL) /* 'FILL' */

/*********************************************************/
/* The data header that preceeds every stream data chunk */
/*********************************************************/
typedef struct WeaveChunk
{
  unsigned long weaveChunkType; /* 4 byte ASCII subscriber data type */
  unsigned long weaveChunkSize; /* size of chunk including this header */
  unsigned long weaveChunkTime; /* time when chunk should be output */
} WeaveChunk, *WeaveChunkPtr;

/*********************************************************/
/* Marker record, associates a time with a file position */
/*********************************************************/
typedef struct MarkerRec
{
  long markerTime;   /* stream time */
  long markerOffset; /* file position in output stream */
} MarkerRec, *MarkerRecPtr;

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
  long mediaBlockSize;    /* physical block size of the output media */
  long streamBlockSize;   /* logical buffering block size for stream */
  long outputBaseTime;    /* beginning time for output file */
  char *outputStreamName; /* name of output stream file */
  FILE *outputStreamFd;   /* output stream file descriptor */

  char *scratchBuf; /* scratch buffer for file copying */

  Boolean fUpdateMarkerTable; /* flag to force update of marker table */
  long numMarkers;            /* number of marker entries */
  long nextMarkerIndex;       /* offset to next marker value to use */
  long currMarkerIndex;       /* state variable for processing markers */
  MarkerRecPtr marker;        /* ptr to array of markers to fill in */

  long numInputStreams; /* number of input streams */
  InStream inStream[MAX_INPUT_STREAMS];

} WeaveParams, *WeaveParamsPtr;

/*******************/
/* Public routines */
/*******************/
Boolean OpenDataStreams (WeaveParamsPtr pb);
void CloseDataStreams (WeaveParamsPtr pb);
long WeaveStreams (WeaveParamsPtr paramBlk);

#endif /* __WEAVESTREAM_H__ */
