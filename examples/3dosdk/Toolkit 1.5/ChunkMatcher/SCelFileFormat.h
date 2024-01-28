/*******************************************************************************************
 *	File:			SCelFileFormat.h
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

#ifndef __SCELFILEFORMAT__
#define __SCELFILEFORMAT__

/***********/
/*  TYPES  */
/***********/

// SCel chunk definitions

typedef struct SCelHeaderChunk
{
  SUBS_CHUNK_COMMON;
  int32 version;       // version control for stream data
  int32 numFrames;     // count of frames in this segment
  int32 totalDuration; // total duration of this segment (in audio ticks)
  int32 reserved;
} SCelHeaderChunk;

typedef struct SCelDataChunk
{
  SUBS_CHUNK_COMMON;
  int32 duration;    // duration of this frame (in audio ticks)
  int32 offset;      // offset to first CCB from top of SCelDataChunk
  int32 celListType; // sub-type of CLST chunk
                     // encapsulated cel list is placed here
} SCelDataChunk;

#endif
