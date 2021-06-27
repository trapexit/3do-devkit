/*******************************************************************************************
 *	File:			HaltChunk.h
 *
 *	Contains:		definitions for a halt chunk
 *
 *	Written by:		Donn Denman
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	07/07/94		dtc		Changed haltPart and subsPart to streamerChunk and
 *							subscriberChunk in definition of HaltChunk.
 *	07/06/94		dtc		Added a newline at end of file, otherwise the compiler chokes.
 *	06/30/94		jb		New today.
 *
 *******************************************************************************************/

#ifndef __HALTCHUNK_H__
#define __HALTCHUNK_H__

#include "dsstreamheader.h"

/* The following flag is used to allow the HALT feature of the data
 *  streamer.  When a halt chunk is encountered, the data streamer is
 *  immediately halted, and no further subscriber messages will be
 *  sent until resume time.  The HALT chunk has embedded in it a chunk
 *  for some subscriber - that chunk is sent to the subscriber immediately
 *  prior to halting.  When a reply is recieved for that embeded chunk
 *  the data streamer exits HALT mode by resuming normal execution.
 */

#define HALT_ENABLE 1		/* 0 turns it off, 1 turns it on... */

#ifndef CHAR4LITERAL
#define	CHAR4LITERAL(a,b,c,d) ((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)
#endif

#define	STREAM_CHUNK_TYPE CHAR4LITERAL('S','T','R','M') /* chunk type for Streamer Control */
#define	HALT_CHUNK_TYPE	  CHAR4LITERAL('H','A','L','T') /* sub-type for streamer HALT function */

typedef struct SimpleChunk {
  SUBS_CHUNK_COMMON;		/* a simple chunk	*/
} SimpleChunk;

typedef struct HaltChunk {
  SimpleChunk streamerChunk;
  SimpleChunk subscriberChunk;
} HaltChunk;

typedef struct HaltChunks {
  HaltChunk	     DataChunk;
  struct HaltChunks* Next;
} HaltChunks, *HaltChunksPtr;

#endif
