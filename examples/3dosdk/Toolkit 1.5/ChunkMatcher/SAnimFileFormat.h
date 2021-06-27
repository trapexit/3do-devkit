/*******************************************************************************************
 *	File:			SAnimFileFormat.h
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

#ifndef __SANIMFILEFORMAT__
#define __SANIMFILEFORMAT__



/***********/
/*  TYPES  */
/***********/

// SANM chunk definitions

typedef	struct SAnimHeaderChunk {
	SUBS_CHUNK_COMMON;				// sub-type "AHDR"
	int32		version;			//  current version = 0
	int32		animType;			// 	3 = Anim, 4 = AA Anim
	int32		numFrames;			// 	number of frames in Anim
	int32		frameRate;			// 	in frames per second
} SAnimHeaderChunk;

typedef	struct SAnimCCBChunk {
	SUBS_CHUNK_COMMON;				// sub-type "ACCB"
	CCB			ccb;				// the encapsulated CCB structure
} SAnimCCBChunk;

typedef	struct SAnimPLUTChunk {
	SUBS_CHUNK_COMMON;				// sub-type "APLT"
	PLUTChunk	plutChunk;			// the encapsulated PLUT chunk
} SAnimPLUTChunk;

typedef	struct SAnimFrameChunk {
	SUBS_CHUNK_COMMON;				// sub-type "FRME"
	int32		duration;			// duration of this frame
	// PDAT and PLUT chunks are encapsulated here	
} SAnimFrameChunk;

typedef	struct SAnimPosChunk {
	SUBS_CHUNK_COMMON;				// sub-type "ACCB"
	int32		duration;			// in audio ticks
	int32		newXValue;			// new cel X position
	int32		newYValue;			// new cel Y position
} SAnimPosChunk;


#endif
