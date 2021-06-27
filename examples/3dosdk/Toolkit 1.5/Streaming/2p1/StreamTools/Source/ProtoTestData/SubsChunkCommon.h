/*******************************************************************************************
 *	File:			SubsChunkCommon.h
 *
 *	Contains:		definitions for data chunk for subscribers
 *
 *	Written by:		Donn Denman
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	7/06/94			dtc		Changed file and Contains contents in this header block to
 *							reflect the correct information.
 *	7/06/94			dtc		Included typedef definition for SubsChunkData.
 *  7/06/94			DLD	    new today
 *******************************************************************************************/

#ifndef _SUBS_CHUNK_COMMON_
#define _SUBS_CHUNK_COMMON_

/**********************************************/
/* Format of a data chunk for subscribers	  */
/**********************************************/

/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
 
#if RELATIVE_BRANCHING
#define	SUBS_CHUNK_COMMON	\
	long		chunkType;		/* chunk type */					\
	long		chunkSize;		/* chunk size including header */	\
	long		time;			/* position in stream time */		\
	short		chunkFlags;		/* reserved for flags		 */		\
	short		channel;		/* logical channel number */		\
	long		subChunkType	/* data sub-type */
	
typedef	struct SubsChunkHeader {
	SUBS_CHUNK_COMMON;
} SubsChunkHeader, *SubsChunkHeaderPtr;
#else 
#define	SUBS_CHUNK_COMMON	\
	long		chunkType;		/* chunk type */					\
	long		chunkSize;		/* chunk size including header */	\
	long		time;			/* position in stream time */		\
	long		channel;		/* logical channel number */		\
	long		subChunkType	/* data sub-type */
	
#endif  /* RELATIVE_BRANCHING  */

typedef	struct SubsChunkData {
	SUBS_CHUNK_COMMON;
} SubsChunkData, *SubsChunkDataPtr;

#endif

