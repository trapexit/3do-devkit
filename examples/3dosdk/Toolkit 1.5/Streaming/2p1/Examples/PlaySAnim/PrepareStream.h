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
 *	07/12/94		DLD		Fixed definition of CHECK_SA_RESULT
 *when in non-debug mode. 10/20/93		jb		New today
 *
 *******************************************************************************************/

#ifndef _PREPARESTREAM_H_
#define _PREPARESTREAM_H_

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

/*********************/
/* Default constants */
/*********************/

#define kDefaultBlockSize 32768 /* size of stream buffers */
#define kDefaultNumBuffers 4    /* suggested number of stream buffers to use */
#define kStreamerDeltaPri 6     /* delta priority for streamer thread */
#define kDataAcqDeltaPri 8 /* delta priority for data acquisition thread */
#define kNumSubsMsgs 256   /* number of subscriber messages to allocate */

#define kAudioClockChan 0  /* logical channel number of audio clock channel */
#define kEnableAudioChan 0 /* mask of audio channels to enable */

#define kSoundsPriority 10
#define kSAnimPriority 7
#define kControlPriority 11

/***************/
/* Error codes */
/***************/
typedef enum
{
  kPSVersionErr = -2001,
  kPSMemFullErr = -2002,
  kPSUnknownSubscriber = -2003,
  kPSHeaderNotFound = -2004
} PrepareStreamErrorCodes;

/*****************************/
/* Macro Definitions         */
/*****************************/
#if DEBUG
#ifndef _STDLIB_H
#include "stdlib.h" /* for exit() */
#endif

#ifndef _STDIO_H
#include "stdio.h" /* for printf() */
#endif

#define CHECK_SA_RESULT(name, dsResult)                                       \
  if (((int32)dsResult) < 0)                                                  \
    {                                                                         \
      printf ("Failure in Streamed Anim: %s: $%lx\n", name,                   \
              ((int32)dsResult));                                             \
      PrintfSAError (((int32)dsResult));                                      \
      CHECK_DS_RESULT (name, dsResult);                                       \
      exit (0);                                                               \
    }
#else
#define CHECK_SA_RESULT(name, dsResult)
#endif
/*****************************/
/* Public routine prototypes */
/*****************************/
void PrintfSAError (int32 dsResult);

int32 FindAndLoadStreamHeader (DSHeaderChunkPtr headerPtr,
                               char *streamFileName);

int32 UseDefaultStreamHeader (DSHeaderChunkPtr headerPtr);

#endif /* _PREPARESTREAM_H_ */
