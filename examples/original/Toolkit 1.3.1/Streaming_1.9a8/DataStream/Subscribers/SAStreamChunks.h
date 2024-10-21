/*******************************************************************************************
 *	File:			SAStreamChunks.h
 *
 *	Contains:		definitions of SAudioSubscriber stream chunk
 *format
 *
 *	Written by:		Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	1/20/94		rdg		make C++ compatible
 *	10/15/93	rdg		Version 2.8 - New Today
 *						Split source into separate
 *files.
 */

#ifndef __SASTREAMCHUNKS_H__
#define __SASTREAMCHUNKS_H__

#include "SubscriberUtils.h"

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
  long version;    /*	version control for audio stream data */
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

#endif /* __SASTREAMCHUNKS_H__ */
