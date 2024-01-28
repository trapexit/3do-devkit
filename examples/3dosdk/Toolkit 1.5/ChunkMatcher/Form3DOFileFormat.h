/*******************************************************************************************
 *	File:			Form3DOFileFormat.h
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

#ifndef __FORM3DOFILEFORMAT__
#define __FORM3DOFILEFORMAT__

/***************/
/*  CONSTANTS  */
/***************/

#define PLUT_ENTRY_SIZE 2   // size of a PLUT entry in bytes
#define MAX_PLUT_ENTRIES 32 // number of entries in maximum PLUT
#define PLUT_SIZE                                                             \
  (MAX_PLUT_ENTRIES                                                           \
   * PLUT_ENTRY_SIZE) // PLUT holds 32 16-bit pixel color values

// Constants extracted from "graphics.h"

#define CCB_LAST 0x40000000
#define CCB_NPABS 0x20000000
#define CCB_SPABS 0x10000000
#define CCB_PPABS 0x08000000
#define CCB_LDPLUT 0x00800000

/***********/
/*  TYPES  */
/***********/

// Types extracted from "graphics.h"

typedef struct
{
  int32 ccb_Flags;

  struct CCB *ccb_NextPtr;
  char *ccb_SourcePtr;
  char *ccb_PLUTPtr;

  int32 ccb_XPos;
  int32 ccb_YPos;
  int32 ccb_HDX;
  int32 ccb_HDY;
  int32 ccb_VDX;
  int32 ccb_VDY;
  int32 ccb_HDDX;
  int32 ccb_HDDY;
  int32 ccb_PIXC;
  int32 ccb_PRE0;
  int32 ccb_PRE1;
  int32 ccb_Width;
  int32 ccb_Height;
} CCB;

/***************************/
/* Cel Control Block Chunk */
/***************************/

typedef struct
{
  CHUNK_COMMON;  // type and size
  int32 version; // version of this CCB-chunk format
  CCB ccb;       // encapsulated cel control block
} CCBChunk;

/*****************************/
/* Pixel Look-Up Table Chunk */
/*****************************/

typedef struct
{
  CHUNK_COMMON;                  // type and size
  int32 numEntries;              // number of entries used in PLUT
  uint32 PLUT[MAX_PLUT_ENTRIES]; // encapsulated PLUT
} PLUTChunk;

/*******************/
/* Animation Chunk */
/*******************/

typedef struct
{
  CHUNK_COMMON;      // type and size
  int32 version;     // current version = 0
  int32 animType;    // 0 = multi-CCB ; 1 = single CCB
  int32 numFrames;   // number of frames for this animation
  int32 frameRate;   // number of 1/60s of a sec to display each frame
  int32 startFrame;  // the first frame in the anim. Can be non zero
  int32 numLoops;    //	number of loops in loop array
  int32 loopStart;   //	start frame for a loop in the animation
  int32 loopEnd;     //	end frame for a loop in the animation
  int32 repeatCount; //	number of times to repeat the looped portion
  int32 repeatDelay; //	number of 1/60s of a sec to delay each time thru loop
} AnimChunk;

#endif
