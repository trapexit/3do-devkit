#ifndef __FORM3DO_H
#define __FORM3DO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: form3do.h,v 1.12 1994/11/23 00:01:50 vertex Exp $
**
**  Lib3DO structures and constants related to 3DO file formats.
**
**  This file format assumes a "big-endian" architecture, that is multi-byte values
**  have their most significant bytes stored at the lowest memory addresses. This is
**  the standard memory organization as the 68000 and is used in the 3DO machine.
**
**  {file}                  ::= {chunk} | {file}{chunk}
**  {chunk_header}  ::= {chunk_ID} {chunk_size}
**  {chunk}                 ::= {chunk_header}{chunk body}
**
**  {chunk_ID}              ::= 'IMAG'  |           Image header
**                                          'CCB '  |           CEL header
**                                          'PDAT'  |           Pixel Data
**                                          'PLUT'  |           PLUT Table (Pixel Lookup Table)
**                                          'ANIM'  |           Animation Info
**                                          'VDL '  |           VDL list
**                                          'CPYR'  |           Copyright Notice
**                                          'DESC'  |           Text Description of image
**                                          'KWRD'  |           Text Keywords associated with image
**                                          'CRDT'              Text credits associated with image
**
**  {chunk_size}::= Unsigned 32 bit integer (includes size of chunk body plus size
**                                  of the chunk header).  Chunk_size is 8 plus size of the chunk_body.
**                                  Chunks must be Quad byte alligned.      Chunks will be padded with
**                                  zeros to fill up the quad word alignment.
**
******************************************************************************/


#ifndef __cplusplus
#ifndef __CC_NORCROFT
#include <stddef.h>
typedef signed long     int32;
typedef signed short    int16;
typedef signed char     int8;
typedef unsigned long   uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;
typedef long            Color;
typedef long            Coord;
typedef long            VDLEntry;
typedef long            RGB888;
typedef unsigned long   CelData[];
#else
#include "types.h"
#include "graphics.h"
#endif
#else
#include "types.h"
#include "graphics.h"
#endif

/*
 * Integer types
 *	These are evil, please avoid them.  They are maintained for
 *	compatibility with old code, but new code should use the
 *	all-lowercase names found in types.h.
 */

typedef signed char             Int8;
typedef unsigned char           UInt8;
typedef short                   Int16;
typedef unsigned short          UInt16;
typedef long                    Int32;
typedef unsigned long           UInt32;

/* Type describing a color in RGB 5-5-5 format */

typedef uint16	RGB555;
#define RGB555_ALPHA_MASK	0x8000
#define RGB555_RED_MASK 	0x7C00
#define RGB555_GREEN_MASK	0x03E0
#define RGB555_BLUE_MASK	0x001F

#define QUADALIGN(x)  ( (((x+3)>>2)<<2) )
#define kCLUTWords 32

struct RGB888_Tag {
	unsigned char unused;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

typedef struct RGB888_Tag RGB;

#define Format555 1
#define Format554 2
#define Format888 3
#define Format555Lin 4

/* The following macro makes a 32-bit unsigned int32 scalar out of 4
 * char's as input. This macro is included to avoid compiler warnings from
 * compilers that object to 4 character literals, for example, 'IMAG'.
 */

#ifndef CHAR4LITERAL
  #define CHAR4LITERAL(a,b,c,d)	((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)
#endif

/* The following are definitions for constants that mark the beginning of
 * data chunka in a 3DO file.
 */

#define CHUNK_3DO		CHAR4LITERAL('3','D','O',' ')   /* '3DO ' wrapper chunk */
#define CHUNK_IMAG		CHAR4LITERAL('I','M','A','G')   /* 'IMAG' the image control chunk */
#define CHUNK_PDAT		CHAR4LITERAL('P','D','A','T')   /* 'PDAT' pixel data */
#define CHUNK_CCB		CHAR4LITERAL('C','C','B',' ')   /* 'CCB ' cel control */
#define CHUNK_ANIM		CHAR4LITERAL('A','N','I','M')   /* 'ANIM' ANIM chunk */
#define CHUNK_PLUT		CHAR4LITERAL('P','L','U','T')   /* 'PLUT' pixel lookup table */
#define CHUNK_VDL		CHAR4LITERAL('V','D','L',' ')   /* 'VDL ' VDL chunk */
#define CHUNK_CPYR		CHAR4LITERAL('C','P','Y','R')   /* 'CPYR' copyright text*/
#define CHUNK_DESC		CHAR4LITERAL('D','E','S','C')   /* 'DESC' description text */
#define CHUNK_KWRD		CHAR4LITERAL('K','W','R','D')   /* 'KWRD' keyword text */
#define CHUNK_CRDT		CHAR4LITERAL('C','R','D','T')   /* 'CRDT' credits text */
#define CHUNK_XTRA		CHAR4LITERAL('X','T','R','A')   /* 'XTRA' 3DO Animator creates these */

#define imagetype		CHUNK_IMAG		/* included for compatibility */

/* Wrapper Chunk */

typedef struct WrapperChunk 	/* Optional  chunk. Must be first if present */
	{
	int32	chunk_ID;			/* '3DO '  Magic number to identify wrapper chunk */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	uint8	data[1];			/*	contains a collection of atomic chunks	*/
	} WrapperChunk;

/* Image Control Chunk */
typedef struct ImageCC
	{
	int32	chunk_ID;			/* 'IMAG' Magic number to identify the image control chunk */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size  (24)  */

	int32	w;					/*	width in pixels */
	int32	h;					/*	height in pixels */
	int32	bytesperrow;		/*	may include pad bytes at row end for alignment */
	uint8	bitsperpixel;		/*	8,16,24 */
	uint8	numcomponents;		/*	3 => RGB (or YUV) , 1 => color index */
								/*	3 => RGB (8  16 or 24 bits per pixel)	*/
								/*		 8 bit is 332 RGB  (or YUV) */
								/*		 16 bit is 555 RGB	(or YUV) */
								/*		 24 bit is 888 RGB	(or YUV) */
								/* 1 => coded  meaning	color indexed;	 */
								/*			 Coded images Require a Pixel Lookup Table Chunk */
	uint8	numplanes;			/*	1 => chunky;  3=> planar  */
								/*	although the hardware does not support planar modes */
								/*	it is useful for some compression methods to separate */
								/*	the image into RGB planes or into YCrCb planes */
								/*	numcomponents must be greater than 1 for planar to */
								/*	have any effect */
	uint8	colorspace; 		/*	0 => RGB, 1 => YCrCb   */
	uint8	comptype;			/*	compression type; 0 => uncompressed */
								/*	1=Cel bit packed */
								/*	other compression types will be defined later */
	uint8	hvformat;			/*	0 => 0555;	1=> 0554h;	2=> 0554v; 3=> v554h  */
	uint8	pixelorder; 		/*	0 => (0,0), (1,0),	(2,0)	(x,y) is (row,column) */
								/*	1 => (0,0), (0,1), (1,0), (1,1)  Sherrie LRform  */
								/*	2 => (0,1), (0,0), (1,1), (1,0)  UGO LRform  */
	uint8	version;			/*	file format version identifier.  0 for now	*/
	} ImageCC;


typedef struct PixelChunk
	{
	int32	chunk_ID;				/* 'PDAT' Magic number to identify pixel data */
	int32	chunk_size; 			/*	size in bytes of chunk including chunk_size */
	uint8	pixels[1];				/*	pixel data (format depends upon description in the imagehdr */
	} PixelChunk;

/* Notes this data structure is the same size and shares common fields with */
/* the ccb data structure.	Common fields are in the same locations 		*/
/* This is done so that a program can allocate a chunk of memory, read the body */
/* of a spritehdr chunk into this memory block, then use the block as a SCoB */
/* (Sprite Control Block) data structure after setting up the appropriate link */
/* pointer data fields */

/* Cel Control Block Chunk	 */
typedef struct CCC
	{
	int32	chunk_ID;			/* 'CCB ' Magic number to identify pixel data */
	int32	chunk_size; 		/* size in bytes of chunk including chunk_size */
	uint32	ccbversion; 		/* version number of the scob data structure.  0 for now */
	uint32	ccb_Flags;			/* 32 bits of CCB flags */
	struct	CCB *ccb_NextPtr;
	CelData    *ccb_CelData;
	void	   *ccb_PIPPtr; 	/* This will change to ccb_PLUTPtr in the next release */

	Coord	ccb_X;
	Coord	ccb_Y;
	int32	ccb_hdx;
	int32	ccb_hdy;
	int32	ccb_vdx;
	int32	ccb_vdy;
	int32	ccb_ddx;
	int32	ccb_ddy;
	uint32	ccb_PPMPC;
	uint32	ccb_PRE0;			/* Sprite Preamble Word 0 */
	uint32	ccb_PRE1;			/* Sprite Preamble Word 1 */
	int32	ccb_Width;
	int32	ccb_Height;
	} CCC;


/* The currently defined anim types for animations are:
 * 0) multi-CCB 	- each frame has its own CCB  and PDAT chunks [and PLUT'S]
 * 1) single-CCB	- there is one CCB	followed by frames containing PDAT chunks [and PLUT'S]
 */

#define ANIM_MULTI_CCB	0
#define ANIM_SINGLE_CCB 1

typedef struct LoopRec
	{
	int32	loopStart;			/*	start frame for a loop in the animation */
	int32	loopEnd;			/*	end frame for a loop in the animation */
	int32	repeatCount;		/*	number of times to repeat the looped portion */
	int32	repeatDelay;		/*	number of 1/60s of a sec to delay each time thru loop */
	} LoopRec;


typedef struct AnimChunk
	{
	int32	chunk_ID;			/* 'ANIM' Magic number to identify ANIM chunk */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	int32	version;			/*	current version = 0 */
	int32	animType;			/*	0 = multi-CCB ; 1 = single CCB	*/
	int32	numFrames;			/*	number of frames for this animation */
	int32	frameRate;			/*	number of 1/60s of a sec to display each frame */
	int32	startFrame; 		/*	the first frame in the anim. Can be non zero */
	int32	numLoops;			/*	number of loops in loop array. Loops are executed serially */
	LoopRec loop[1];			/*	array of loop info. see numLoops */
	} AnimChunk;


typedef struct PLUTChunk
	{
	int32	chunk_ID;			/* 'PLUT' Magic number to identify pixel data */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	int32	numentries; 		/*	number of entries in PLUT Table */
	RGB555	PLUT[1];			/*	PLUT entries  */
	} PLUTChunk;


/***************************************/
/* NOTE:  VDL_REC will probably change */
/***************************************/
typedef struct VDL_REC
{
	uint32	controlword;					/*	VDL display control word (+ number of int32words in this entry - 4) */
											/*	(+ number of lines that this vdl is in effect -1) */
	uint32	curLineBuffer;					/*	1st byte of frame buffer */
	uint32	prevLineBuffer; 				/*	1st byte of frame buffer */
	uint32	nextVDLEntry;					/*	GrafBase->gf_VDLPostDisplay for last VDL Entry */
	uint32	displayControl; 				/*	Setup control info: DEFAULT_DISPCTRL */
	uint32	CLUTEntry[kCLUTWords];			/*	32 Clut entries for each R, G, and B */
	uint32	backgroundEntry;				/*	RGB 000 will use this entry */
	uint32	filler1;						/*	need 40 entries for now, hardware bug */
	uint32	filler2;
} VDL_REC;

typedef struct VDLCHUNK 		/* used for a standard 33 entry vdl list */
	{
	int32	chunk_ID;			/* 'VDL ' Magic number to identify VDL chunk */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	int32	vdlcount;			/*	count of number of vdls following */
	VDL_REC vdl[1]; 			/*	VDL control words and entries  */
	} VdlChunk;


typedef struct Cpyr
	{
	int32	chunk_ID;			/* 'CPYR' Magic number to identify pixel data */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	char	copyright[1];		/*	C String ASCII Copyright Notice  */
	} Cpyr;


typedef struct Desc
	{
	int32	chunk_ID;			/* 'DESC' Magic number to identify pixel data */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	char	descrip[1]; 		/*	C String ASCII image description  */
	} Desc;

typedef struct Kwrd
	{
	int32	chunk_ID;			/* 'KWRD' Magic number to identify pixel data */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	char	keywords[1];		/*	C String ASCII keywords, separated by ';'   */
	} Kwrd;

typedef struct Crdt
	{
	int32	chunk_ID;			/* 'CRDT' Magic number to identify pixel data */
	int32	chunk_size; 		/*	size in bytes of chunk including chunk_size */
	char	credits[1]; 		/*	C String ASCII credits for this image  */
	} Crdt;

/*
 * for chunks which are structured as control information and related data
 * (as opposed to just control information), these macros provide the size
 * of the control information part of the chunk.  to put it another way, you
 * can add this many bytes to a chunk pointer to access the chunk's data.
 *
 * CHUNKHDR_SIZE is the size of the common chunk fields (chunk_ID and chunk_size).
 */

#define WRAPPER_CHUNKHDR_SIZE		offsetof(WrapperChunk, data)
#define PDAT_CHUNKHDR_SIZE			offsetof(PixelChunk, pixels)
#define ANIM_CHUNKHDR_SIZE			offsetof(AnimChunk, loop)
#define PLUT_CHUNKHDR_SIZE			offsetof(PLUTChunk, PLUT)
#define VDL_CHUNKHDR_SIZE			offsetof(VDLCHUNK, vdl)
#define CPYR_CHUNKHDR_SIZE			offsetof(Cpyr, copyright)
#define DESC_CHUNKHDR_SIZE			offsetof(Desc, descrip)
#define KWRD_CHUNKHDR_SIZE			offsetof(Kwrd, keywords)
#define CRDT_CHUNKHDR_SIZE			offsetof(Crdt, credits)

#define CHUNKHDR_SIZE				WRAPPER_CHUNKHDR_SIZE

/* service routines used internally and also generally useful... */

#ifdef __cplusplus
  extern "C" {
#endif

char *	GetChunk( uint32 *chunk_ID, char **buffer, int32 *bufLen );

#ifdef __cplusplus
  }
#endif


#endif /* __FORM3DO_H */
