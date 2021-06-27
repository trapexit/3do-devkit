/*
	File:		Films.h

	Contains:	Structures for an example film format

	Written by:	Peter Barrett

	Copyright:	© 1993 by The 3DO Company, all rights reserved.

	Revision History:
	
	Date		Author		Comment
	________________________________________________________________________
	05/25/94	dtc			Integrated Lynn Ackler's fix to distinguish
							between key frames and difference frames.
	4/17/93		NJC			New data structures for use with the DSH
	
	________________________________________________________________________
	
	To Do:
*/

/*========================================================================*/

#define	kFillerChunkType				'FILL'

#define	kSoundChunkType					'SNDS'
#define	kSoundHeaderType				'SHDR'
#define	kSoundSampleType				'SSMP'

#define	kVideoChunkType					'FILM'
#define	kVideoHeaderType				'FHDR'
#define	kVideoKeyFrameType				'FRME'
#define	kVideoDifferenceFrameType		'DFRM'
#define	kVideoFrameType					'FRME'

#define	kDependentChunkFlag		   0	/* indicates a chunk that does depend
										 * on its sibling predecessors */
#define	kKeyChunkFlag			1<<0	/* indicates a chunk that does not
										 * depend on its sibling predecessors */
										 
#define	CTRL_CHUNK_TYPE		'CTRL'	/* chunk type for this subscriber */
#define	GOTO_CHUNK_TYPE		'GOTO'	/* GOTO type for this subscriber */

#define	kLastSoundChunk		0x8000

typedef	struct FillerChunk {
	long		chunkType;		/*	'FILL'						*/
	long		chunkSize;
} FillerChunk, *FillerChunkPtr;

typedef unsigned long	UInt32;


/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
#define	SUBS_CHUNK_COMMON	\
	long		chunkType;		/* chunk type */					\
	long		chunkSize;		/* chunk size including header */	\
	long		time;			/* position in stream time */		\
	short		chunkFlags;		/* reserved for flags		 */		\
	short		channel;		/* logical channel number */		\
	long		subChunkType	/* data sub-type */

typedef	struct FilmHeader {
SUBS_CHUNK_COMMON;
	long		version;		/*	0 for this version			*/
	long		cType;			/*	video compression type		*/
	long		height;			/*	Height of each frame		*/
	long		width;			/*	Width of each frame			*/
	long		scale;			/*	Timescale of Film			*/
	long		count;			/*	Number of frames			*/
} FilmHeader, *FilmHeaderPtr;


typedef	struct	FilmFrame {
SUBS_CHUNK_COMMON;
	long		duration;		/*	Duration of this sample		*/
	long		frameSize;		/*	Number of bytes in frame	*/
/*	char		frameData[4];		compressed frame data...	*/
} FilmFrame, *FilmFramePtr;



typedef	struct StreamSoundHeader {
SUBS_CHUNK_COMMON;
	long		version;		/*	0 for this version			*/
	long		dataFormat;		/*	4 char ident (e.g. 'raw ')	*/
	long		numChannels;	/*	1 = mono, 2 = stereo, etc.	*/
	long		sampleSize;		/*	in bits/sample (8 or 16)	*/
	UInt32		sampleRate;		/*	samples/sec in 16.16 format	*/
	long		sampleCount;	/*	Number of samples in sound	*/
} StreamSoundHeader, *StreamSoundHeaderPtr;


typedef	struct StreamSoundChunk {
SUBS_CHUNK_COMMON;
/*	char		soundData[0];		sound data...				*/
} StreamSoundChunk, *StreamSoundChunkPtr;

typedef	struct CtrlMarkerChunk {
SUBS_CHUNK_COMMON;
	long		marker;			/*	new position for stream		*/
} CtrlMarkerChunk, *CtrlMarkerChunkPtr;



/*========================================================================*/
