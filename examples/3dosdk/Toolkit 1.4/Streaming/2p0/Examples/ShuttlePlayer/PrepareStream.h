/*******************************************************************************************
 *	File:			PrepareStream.h
 *
 *	Contains:		definitions for high level stream playback
 *preparation
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	10/20/93		jb		New today
 *
 *******************************************************************************************/

#ifndef _PREPARESTREAM_H_
#define _PREPARESTREAM_H_
#include "CPakSubscriberS.h"

#ifndef _TYPES_H
#include "Types.h"
#endif

#include "DataStreamLib.h"

#ifndef _DSSTREAMHEADER_H_
#include "DSStreamHeader.h"
#endif

/* Size of the buffer we use to read the start of the stream file.
 * For now, we require that the stream header, if present, be located
 * at the very first byte of the file. This may get better, eventually.
 */
#define FIND_HEADER_BUFFER_SIZE 2048

/***************/
/* Error codes */
/***************/
typedef enum
{
  kPSVersionErr = -2001,
  kPSMemFullErr = -2002,
  kPSUnknownSubscriber = -2003,
  kPSHeaderNotFound = -2004
};

/*****************************/
/* Public routine prototypes */
/*****************************/

int32 FindAndLoadStreamHeader (DSHeaderChunkPtr headerPtr,
                               char *streamFileName);
DSDataBufPtr CreateBufferList (long numBuffers, long bufferSize);

#endif /* _PREPARESTREAM_H_ */
