/*******************************************************************************************
 *	File:			SAudioFileFormat.h
 *
 *	Contains:		
 *
 *	Written by:		3DO Software Attic
 *					Chris McFall
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/16/94		crm		New today
 *
 *******************************************************************************************/

#ifndef __SAUDIOFILEFORMAT__
#define __SAUDIOFILEFORMAT__



/***********/
/*  TYPES  */
/***********/

// Streamed Audio chunk descriptions

typedef struct SAudioSampleDescriptor {
	long	dataFormat;					// 4 char ident (e.g. 'raw ')
	long	sampleSize;					// bits per sample, typically 16
	long 	sampleRate;    				// 44100 or 22050
	long	numChannels;				// channels per frame, 1 = mono, 2=stereo
	long  	compressionType;  			// eg. SDX2 , 930210
	long	compressionRatio;
	long	sampleCount;				//	Number of samples in sound
} SAudioSampleDescriptor;

typedef	struct SAudioHeaderChunk {
	SUBS_CHUNK_COMMON;
	long	version;					// version control for audio stream data
	long	numBuffers;					// max # of buffers that can be queued to the AudioFolio
	long	initialAmplitude;			// initial volume of the stream
	long	initialPan;					// initial pan of the stream
	SAudioSampleDescriptor sampleDesc;		// info about sound format
} SAudioHeaderChunk;

typedef struct SAudioSampleChunk {
	SUBS_CHUNK_COMMON;
	long	actualSampleSize;			// actual number of bytes in the sample data
	char	samples[4];					// start of audio samples
} SAudioSampleChunk;



#endif
