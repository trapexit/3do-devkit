/*******************************************************************************************
 *	File:			PortableSANMDefs.h
 *
 *	Contains:		defintions are needed to write out the streamed
 *animation data files
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	8/23/93		jb		New today
 *
 *******************************************************************************************/

#ifndef _PORTABLESANMDEFS_H_
#define _PORTABLESANMDEFS_H_

#ifndef SUBS_CHUNK_COMMON
/* The following preamble is used at the top of each subscriber
 * chunk passed in from the streamer.
 */
#define SUBS_CHUNK_COMMON                                                     \
  long chunkType;    /* chunk type */                                         \
  long chunkSize;    /* chunk size including header */                        \
  long time;         /* position in stream time */                            \
  long channel;      /* logical channel number */                             \
  long subChunkType; /* data sub-type */
#endif

typedef struct SubsChunkData
{
  SUBS_CHUNK_COMMON
} SubsChunkData, *SubsChunkDataPtr;

typedef struct StreamAnimHeader
{ /*	subType = 'AHDR'			*/
  SUBS_CHUNK_COMMON
  long version;   /*	current version = 0			*/
  long animType;  /* 	3 = streamed Anim			*/
  long numFrames; /* 	number of frames in anim	*/
  long frameRate; /* 	in frames per second		*/
} StreamAnimHeader, *StreamAnimHeaderPtr;

typedef struct StreamAnimCCB
{ /*	subType = 'ACCB'			*/
  SUBS_CHUNK_COMMON
  unsigned long ccb_Flags; /*	32 bits of CCB flags		*/
  struct CCB *ccb_NextPtr;
  long *ccb_CelData;
  void *ccb_PLUTPtr;
  long ccb_X;
  long ccb_Y;
  long ccb_hdx;
  long ccb_hdy;
  long ccb_vdx;
  long ccb_vdy;
  long ccb_ddx;
  long ccb_ddy;
  unsigned long ccb_PPMPC;
  unsigned long ccb_PRE0; /* Sprite Preamble Word 0 */
  unsigned long ccb_PRE1; /* Sprite Preamble Word 1 */
  long ccb_Width;
  long ccb_Height;
} StreamAnimCCB, *StreamAnimCCBPtr;

typedef struct StreamAnimFrame
{ /* subType = 'FRME' */
  SUBS_CHUNK_COMMON
  long duration; /* in audio ticks */

  /*	In here goes a PDAT (and maybe a PLUT chunk) */

} StreamAnimFrame, *StreamAnimFramePtr;

/* Definitions for streamed animation blocks */
#ifndef SANM_CHUNK_TYPE
#define SANM_CHUNK_TYPE ('SANM')
#define SANM_HEADER_TYPE ('AHDR')
#define AHDR_CHUNK_TYPE ('AHDR')
#define ACCB_CHUNK_TYPE ('ACCB')
#define FRME_CHUNK_TYPE ('FRME')
#endif

#endif /* _PORTABLESANMDEFS_H_ */
