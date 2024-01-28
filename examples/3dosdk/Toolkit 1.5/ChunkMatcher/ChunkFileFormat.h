/*******************************************************************************************
 *	File:			ChunkFileFormat.h
 *
 *	Contains:		Common type declarations for interpreting chunk
 *files.
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

#ifndef __CHUNKFILEFORMAT__
#define __CHUNKFILEFORMAT__

/*********************/
/*  Primitive Types  */
/*********************/

typedef long int32;
typedef unsigned long uint32;
typedef short int16;
typedef unsigned short uint16;
typedef char int8;
typedef unsigned char uint8;
typedef unsigned char ubyte;

/***************************/
/*  Common Chunk Preamble  */
/***************************/

#define CHUNK_COMMON                                                          \
  uint32 type; /* chunk type */                                               \
  uint32 size  /* chunk size including header */

/******************/
/*  Chunk Header  */
/******************/

typedef struct
{
  CHUNK_COMMON;
} ChunkHeader;

/**********************************/
/*  Common Stream-Chunk Preamble  */
/**********************************/

#define SUBS_CHUNK_COMMON                                                     \
  CHUNK_COMMON;      /* chunk type and size */                                \
  int32 time;        /* position in stream time */                            \
  int32 channel;     /* logical channel number */                             \
  int32 subChunkType /* data sub-type */

/*************************/
/*  Stream Chunk Header  */
/*************************/

typedef struct
{
  SUBS_CHUNK_COMMON;
} StreamChunkHeader;

#endif
