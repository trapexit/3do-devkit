/**************************************************************************************
 *	Project:		SAudioTool -	An MPW Tool to print formatted
 *output for .saudio header data and allow modification of the header and data
 *chunks..
 *
 *	This File:		SAudioTool.h
 *
 *
 *	Copyright © 1993 The 3DO Company
 *
 *	All rights reserved. This material constitutes confidential and
 *proprietary information of the 3DO Company and shall not be used by any
 *Person or for any purpose except as expressly authorized in writing by the
 *3DO Company.
 *
 *
 * History:
 * -------
 *	9/10/93		RDG		Version 1.1
 *						Move includes to .c
 *	8/4/93		RDG		Version 1.0
 *	8/3/93		RDG		New today
 *
 *******************************************************************************************/

/*********************************************************************
 * 		Typedefs
 *********************************************************************/
typedef unsigned long ulong;

/**********************************************/
/* Format of a data chunk for subscribers	  */
/**********************************************/

/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
#define SUBS_CHUNK_COMMON                                                     \
  long chunkType;    /* chunk type */                                         \
  long chunkSize;    /* chunk size including header */                        \
  long time;         /* position in stream time */                            \
  long channel;      /* logical channel number */                             \
  long subChunkType; /* data sub-type */

/******************************************************/
/* Format of a header chunk for the SAudio subscriber */
/******************************************************/

typedef struct SAudioSampleDescriptor
{
  ulong dataFormat;      /* 4 char ident (e.g. 'raw ')	*/
  long sampleSize;       /* bits per sample, typically 16 */
  long sampleRate;       /* 44100 or 22050 */
  long numChannels;      /* channels per frame, 1 = mono, 2=stereo */
  ulong compressionType; /* eg. SDX2 , 930210 */
  long compressionRatio;
  long sampleCount; /*	Number of samples in sound	*/
} SAudioSampleDescriptor, *SAudioSampleDescriptorPtr;

typedef struct SAudioHeaderChunk
{
  SUBS_CHUNK_COMMON
  long version;    /*	0 for this version */
  long numBuffers; /*  max # of buffers that can be queued to the AudioFolio
                    */
  long initialAmplitude;             /*  initial volume of the stream */
  long initialPan;                   /*  initial pan of the stream */
  SAudioSampleDescriptor sampleDesc; /*  info about sound format */
} SAudioHeaderChunk, *SAudioHeaderChunkPtr;

typedef struct SAudioDataChunk
{
  SUBS_CHUNK_COMMON
  long actualSampleSize; /* actual number of bytes in the sample data */
  char samples[4];       /* start of audio samples */
} SAudioDataChunk, *SAudioDataChunkPtr;
