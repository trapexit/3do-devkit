/*******************************************************************************************
 *	File:			DSStreamHeader.h
 *
 *	Contains:		definitions for stream header chunk
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94			rdg		make C++ compatible
 *	12/2/93			jb		Added 'numSubsMsgs' to stream
 *header; changed stream version to 2. 10/13/93		jb		New
 *today.
 *
 *******************************************************************************************/
#ifndef _DSSTREAMHEADER_H_
#define _DSSTREAMHEADER_H_

#include "types.h"

#ifndef __CC_NORCROFT
/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
#define SUBS_CHUNK_COMMON                                                     \
  long chunkType;    /* chunk type */                                         \
  long chunkSize;    /* chunk size including header */                        \
  long time;         /* position in stream time */                            \
  long channel;      /* logical channel number */                             \
  long subChunkType; /* data sub-type */
#else
#include "SubscriberUtils.h"
#endif

/**********************/
/* Internal constants */
/**********************/

#define HEADER_CHUNK_TYPE (0x53484452) /* 'SHDR' */

#define DS_STREAM_VERSION 2 /* Stream header version number */

/********************************************************/
/* Each subscriber entry in the header consists of a	*/
/* data type for the subscriber and a delta priority at	*/
/* which the subscriber is instantiated.				*/
/********************************************************/
typedef struct DSHeaderSubs
{
  long subscriberType; /* data type for subscriber */
  long deltaPriority;  /* its delta priority */
} DSHeaderSubs, *DSHeaderSubsPtr;

/************************************/
/* Format of a stream header chunk	*/
/************************************/
#define DS_HDR_MAX_PRELOADINST 16
#define DS_HDR_MAX_SUBSCRIBER 16

typedef struct DSHeaderChunk
{
  SUBS_CHUNK_COMMON /* from SubscriberUtils.h */

      long headerVersion; /* version of header data */

  long streamBlockSize;  /* size of stream buffers in this stream */
  long streamBuffers;    /* suggested number of stream buffers to use */
  long streamerDeltaPri; /* delta priority for streamer thread */
  long dataAcqDeltaPri;  /* delta priority for data acquisition thread */
  long numSubsMsgs;      /* number of subscriber messages to allocate */

  long audioClockChan;  /* logical channel number of audio clock channel */
  long enableAudioChan; /* mask of audio channels to enable */

  long preloadInstList[DS_HDR_MAX_PRELOADINST];
  /* NULL terminated preloaded instrument list */

  DSHeaderSubs subscriberList[DS_HDR_MAX_SUBSCRIBER];
  /* NULL terminated list of subscriber tags */

} DSHeaderChunk, *DSHeaderChunkPtr;

#endif /* _DSSTREAMHEADER_H_ */
