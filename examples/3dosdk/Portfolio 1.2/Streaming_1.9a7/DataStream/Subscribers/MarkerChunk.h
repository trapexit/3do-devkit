/*******************************************************************************************
 *	File:			MarkerChunk.h
 *
 *	Contains:		definitions of the MarkerChunk stream data type
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	11/9/93		jb		Moved definition of MarkerRec here.
 *	10/25/93	jb		New today.
 *
 *******************************************************************************************/

#ifndef __MARKERCHUNK_H__
#define __MARKERCHUNK_H__

#ifndef __TYPES__
#include "Types.h"
#endif

#ifndef __SUBSCRIBERUTILS_H__
#include "SubscriberUtils.h"
#endif

/*********************************************************/
/* Marker record, associates a time with a file position */
/*********************************************************/
typedef struct MarkerRec
{
  long markerTime;   /* stream time */
  long markerOffset; /* file position in output stream */
} MarkerRec, *MarkerRecPtr;

/********************************************************************/
/* Marker chunk record, used by DataAcq.c to translate stream time	*/
/* to stream file position.
 */
/********************************************************************/
#define DATAACQ_CHUNK_TYPE (0x44414351)   /* 'DACQ' */
#define MARKER_TABLE_SUBTYPE (0x4D54424C) /* 'MTBL' */

typedef struct MarkerChunk
{
  SUBS_CHUNK_COMMON
} MarkerChunk, *MarkerChunkPtr;

#endif /* __MARKERCHUNK_H__ */
