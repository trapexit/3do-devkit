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
 *  06/30/94	DLD		Added fWriteHaltChunk and HaltChunkList to
 *WeaveParams structure, to support the new weaver command "WriteHaltChunk"
 *  04/25/94	dtc		Version 2.2
 *						Changed definition of InStream to allow
 *unlimited number of input streams. It currently handles 20 filenames. Added
 *the definition of ControlChunk struct to handle output of stop and goto chunk
 *if requested by the user.
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

#ifndef __HALTCHUNK_H__
#include "HaltChunk.h"
#endif

#define MAX_INPUT_STREAMS                                                     \
  20 /* Max number of input files open using standard I/O */
     /* The actual limit set by standard I/O is 12        */

/* The following compile switch determines whether or not to allow unlimited
 * number of input streams through USE_MAC_IO. If you don't have to weave more
 * than 15 files, portability will be maintained by setting USE_MAC_IO to 0.
 * USE_MAC_IO is defined in Weaver.make.
 */
#if USE_MAC_IO
#include <files.h>
#endif

#define INFINITY_TIME (0x7fffffff)

#define CHAR4LITERAL(a, b, c, d)                                              \
  ((unsigned long)(a << 24) | (b << 16) | (c << 8) | d)

#define FILLER_DATA_TYPE CHAR4LITERAL ('F', 'I', 'L', 'L')
#define CONTROL_CHUNK_TYPE                                                    \
  CHAR4LITERAL ('C', 'T', 'R', 'L') /* Chunk type for this subscriber */

#define ALRM_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('A', 'L', 'R', 'M') /* ALRM chunk sub-type            */
#define CONTROL_SYNC_SUBTYPE                                                  \
  CHAR4LITERAL ('S', 'Y', 'N', 'C') /* SYNC chunk sub-type            */
#define GOTO_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('G', 'O', 'T', 'O') /* GOTO chunk sub-type            */
#define STOP_CHUNK_SUBTYPE                                                    \
  CHAR4LITERAL ('S', 'T', 'O', 'P') /* STOP chunk sub-type            */

/**********************************************/
/* Format of a data chunk for subscribers	  */
/**********************************************/

#define int32 long
#define uint32 unsigned long

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
#if USE_MAC_IO
  short fd; /* Mac file descriptor for reading file data */
  IOParam
      iopb; /* Need to use a PBOpen call to get read/only for input files */
#else
  FILE *fd; /* Standard I/O file descriptor for reading file data */
#endif
  WeaveChunk buf; /* buffer for current header */
  struct InStream *Next;
} InStream, *InStreamPtr;

typedef struct ControlChunk
{
  SUBS_CHUNK_COMMON; /* from SubscriberUtils.h */
  union
  {
    struct
    { /* sub-type 'GOTO' */
      uint32 value;
    } marker;

    struct
    { /* sub-type 'SYNC' */
      uint32 value;
    } sync;

    struct
    { /* sub-type 'ALRM' */
      uint32 options;
      uint32 newTime;
    } alarm;

    struct
    { /* sub-type 'STOP' */
      uint32 options;
    } stop;
  } u;

} ControlChunk, *ControlChunkPtr;

typedef struct CtrlChunks
{
  ControlChunk DataChunk;
  struct CtrlChunks *Next;
} CtrlChunks, *CtrlChunksPtr;

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
#if USE_MAC_IO
  short outputStreamFd; /* output stream file descriptor, Mac I/O */
#else
  FILE *outputStreamFd; /* output stream file descriptor, standard I/O */
#endif
  char *scratchBuf; /* scratch buffer for file copying */

  long numInputStreams; /* number of input streams */
  InStreamPtr InStreamList;

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

  /***************************/
  /* GOTO chunk related info */
  /***************************/
  Boolean fWriteGotoChunk; /* flag: write stop chunk data to output stream */
  CtrlChunksPtr GotoChunkList; /* Linked list of GOTO chunks */

  /***************************/
  /* STOP chunk related info */
  /***************************/

  Boolean fWriteStopChunk; /* flag: write stop chunk data to output stream */
  CtrlChunksPtr StopChunkList; /* Linked list of GOTO chunks */

  /***************************/
  /* HALT chunk related info */
  /***************************/

  Boolean fWriteHaltChunk; /* flag: write halt chunk data to output stream */
  HaltChunksPtr HaltChunkList; /* Linked list of HALT chunks */

} WeaveParams, *WeaveParamsPtr;

/*******************/
/* Public routines */
/*******************/
Boolean OpenDataStreams (WeaveParamsPtr pb);
void CloseDataStreams (WeaveParamsPtr pb);
long WeaveStreams (WeaveParamsPtr paramBlk);

#endif /* __WEAVESTREAM_H__ */
