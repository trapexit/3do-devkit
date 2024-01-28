/*******************************************************************************************
 *	File:			CinePakFileFormat.h
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

#ifndef __CINEPAKFILEFORMAT__
#define __CINEPAKFILEFORMAT__

/***********/
/*  TYPES  */
/***********/

typedef struct
{
  SUBS_CHUNK_COMMON;
  int32 version; //	0 for this version
  int32 cType;   //	video compression type
  int32 height;  //	height of each frame
  int32 width;   //	width of each frame
  int32 scale;   //	timescale of Film
  int32 count;   //	number of frames
} CinePakHeader;

typedef struct
{
  SUBS_CHUNK_COMMON;
  int32 duration;  //	duration of this sample
  int32 frameSize; //	number of bytes in frame
                   // compressed frame data follows immediately
} CinePakFrame;

#endif
