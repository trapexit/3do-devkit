
#if 0
	All rights reserved. The 3DO Company.
	------------------------------------------
	
	This material constitutes confidential and
	proprietary information of the 3DO Company
	and shall not be used by any Person or for
	any purpose except as expressly authorized
	in writing by the 3DO Company.



	Preliminaries
	------------------------------------------
	This file format assumes a "big-endian" architecture,
	that is multi-byte values have their most significant
	bytes stored at the lowest memory addresses.  This is
	the standard memory organization as the 68000 and is
	used in the 3DO machine.
	
	
	
	{file}  		::=	{chunk} | {file}{chunk}
	{chunk_header}	::=	{chunk_ID} {chunk_size}
	{chunk} 		::=	{chunk_header}{chunk body}
	
	{chunk_ID} 		::= 'IMAG'	|			/* Image header  */
						'CCB '	|			/* CEL header */
						'PDAT'	|			/* Pixel Data	 */
						'PLUT'	|			/* PLUT Table (Pixel Lookup Table) */
						'ANIM'	|			/* Animation Info*/
						'VDL '	|			/* VDL  list     */
						'CPYR'	|			/* Copyright Notice */
						'DESC'	|			/* Text Description of image */
						'KWRD'	|			/* Text Keywords associated with image */
						'CRDT'				/* Text credits associated with image */
						
	{chunk_size}::= Unsigned 32 bit integer (includes size of chunk body plus size
					of the chunk header).  Chunk_size is 8 plus size of the chunk_body.
					Chunks must be Quad byte alligned.  Chunks will be padded with
					zeros to fill up the quad word alignment.



	Revision History
	--------------------------------------------------------------------
	11/11/92		DSM		Initial Developers Release

#endif

#ifndef __3DOFILE__
#define __3DOFILE__

#ifndef __CC_NORCROFT
typedef unsigned long		CelData[];
#endif

#define __INTTYPES__


/*	Integer types */

typedef signed char			Int8;
typedef unsigned char		UInt8;
typedef short				Int16;
typedef unsigned short		UInt16;
typedef long				Int32;
typedef unsigned long		UInt32;


/*	Fixed point types */ 

typedef Int32				Int1616;
typedef UInt32				UInt1616;
typedef Int32				Int1220;
typedef UInt32				UInt1220;


/*	Type describing a color in RGB 5-5-5 format */

typedef struct RGB555 {
	unsigned		alpha : 1;
	unsigned		red : 5;
	unsigned		green : 5;
	unsigned		blue : 5;
} RGB555;

#ifndef __GRAPHICS_H

typedef unsigned char		ubyte;
typedef unsigned long		ulong;


typedef Int32 Color;
typedef Int32 Coord;
typedef Int32 VDLEntry;
typedef Int32 RGB888;


#endif

#define QUADALIGN(x)  ( (((x+3)>>2)<<2) )


#define imagetype 'IMAG'

	
struct RGB888
	{
	unsigned char unused;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	};
	
typedef struct RGB888 RGB;

#define Format555 1
#define Format554 2
#define Format888 3
#define Format555Lin 4


/* Wrapper Chunk */

typedef struct WrapperChunk            /* Optional  chunk. Must be first if present */
	{
	Int32 	chunk_ID;			/* '3DO '  Magic number to identify wrapper chunk */
	Int32	chunk_size;		/*  size in bytes of chunk including chunk_size */
	ubyte	data[1];			      /*  contains a collection of atomic chunks  */
	} WrapperChunk;

/* Image Control Chunk */
typedef struct ImageCC 
	{
	Int32 	chunk_ID;		/* 'IMAG' Magic number to identify the image control chunk */
	Int32	chunk_size;	/*  size in bytes of chunk including chunk_size  (24)  */

	Int32	w;				/*  width in pixels */			
	Int32	h;				/*  height in pixels */
	Int32	bytesperrow;	/*  may include pad bytes at row end for alignment */
	ubyte	bitsperpixel;	/*  8,16,24 */
	ubyte	numcomponents;	/*  3 => RGB (or YUV) , 1 => color index */
							/*  3 => RGB (8  16 or 24 bits per pixel)   */
							/*       8 bit is 332 RGB  (or YUV) */
							/*       16 bit is 555 RGB  (or YUV) */
							/*       24 bit is 888 RGB  (or YUV) */
							/* 1 => coded  meaning  color indexed;   */
							/*           Coded images Require a Pixel Lookup Table Chunk */
	ubyte	numplanes;	/*  1 => chunky;  3=> planar  */	
							/*  although the hardware does not support planar modes */
							/*  it is useful for some compression methods to separate */
							/*  the image into RGB planes or into YCrCb planes */
							/*  numcomponents must be greater than 1 for planar to */
							/*  have any effect */
	ubyte	colorspace;	/*  0 => RGB, 1 => YCrCb   */
	ubyte	comptype;		/*  compression type; 0 => uncompressed */
							/*  1=Cel bit packed */
							/*  other compression types will be defined later */
	ubyte 	hvformat;		/*  0 => 0555;  1=> 0554h;  2=> 0554v; 3=> v554h  */	
	ubyte 	pixelorder;		/*  0 => (0,0), (1,0),  (2,0)   (x,y) is (row,column) */
							/*  1 => (0,0), (0,1), (1,0), (1,1)  Sherrie LRform  */	
							/*  2 => (0,1), (0,0), (1,1), (1,0)  UGO LRform  */	
	ubyte 	version;		/*  file format version identifier.  0 for now  */	
	} ImageCC;
	
	
typedef struct PixelChunk
	{
	Int32 	chunk_ID;				/* 'PDAT' Magic number to identify pixel data */
	Int32	chunk_size;				/*  size in bytes of chunk including chunk_size */
	ubyte	pixels[1];				/*  pixel data (format depends upon description in the imagehdr */
	} PixelChunk;

/* Notes this data structure is the same size and shares common fields with */
/* the ccb data structure.  Common fields are in the same locations         */
/* This is done so that a program can allocate a chunk of memory, read the body */
/* of a spritehdr chunk into this memory block, then use the block as a SCoB */
/* (Sprite Control Block) data structure after setting up the appropriate link */
/* pointer data fields */

/* Cel Control Block Chunk   */
typedef struct CCC
 	{
	Int32	chunk_ID;		/* 'CCB ' Magic number to identify pixel data */
	Int32	chunk_size;		/*  size in bytes of chunk including chunk_size */
	ulong	ccbversion;	/* version number of the scob data structure.  0 for now */
	ulong	ccb_Flags;		/* 32 bits of CCB flags */
	struct  CCB *ccb_NextPtr;
	CelData    *ccb_CelData;
	void       *ccb_PIPPtr;  /* This will change to ccb_PLUTPtr in the next release */
	
	Coord 	ccb_X;
	Coord 	ccb_Y;
	long	ccb_hdx;
	long	ccb_hdy;
	long	ccb_vdx;
	long	ccb_vdy;
	long	ccb_ddx;
	long	ccb_ddy;
	ulong	ccb_PPMPC;
	ulong	ccb_PRE0;		/* Sprite Preamble Word 0 */
	ulong	ccb_PRE1;		/* Sprite Preamble Word 1 */
	long	ccb_Width;
	long	ccb_Height;
	} CCC;

	
/* The currently defined anim types for animations are:
 * 0) multi-CCB 	- each frame has its own CCB  and PDAT chunks [and PLUT'S]
 * 1) single-CCB 	- there is one CCB  followed by frames containing PDAT chunks [and PLUT'S]
 */

typedef struct LoopRec
	{
	Int32	loopStart;			/* 	start frame for a loop in the animation */
	Int32	loopEnd;			/* 	end frame for a loop in the animation */
	Int32	repeatCount;		/* 	number of times to repeat the looped portion */
	Int32	repeatDelay;		/* 	number of 1/60s of a sec to delay each time thru loop */
	} LoopRec;
	
	
typedef struct AnimChunk
	{
	Int32 	chunk_ID;			/* 'ANIM' Magic number to identify ANIM chunk */
	Int32	chunk_size;			/*  size in bytes of chunk including chunk_size */
	Int32	version;			/*  current version = 0 */
	Int32	animType;			/* 	0 = multi-CCB ; 1 = single CCB  */
	Int32	numFrames;			/* 	number of frames for this animation */
	Int32	frameRate;			/* 	number of 1/60s of a sec to display each frame */
	Int32	startFrame;			/* 	the first frame in the anim. Can be non zero */
	Int32	numLoops;			/* 	number of loops in loop array. Loops are executed serially */
	LoopRec	loop[1];			/*  array of loop info. see numLoops */
	} AnimChunk;
	

typedef struct PLUTChunk
	{
	Int32 	chunk_ID;			/* 'PLUT' Magic number to identify pixel data */
	Int32	chunk_size;			/*  size in bytes of chunk including chunk_size */
	Int32	numentries;			/* 	number of entries in PLUT Table */
	RGB555  PLUT[1];				/*  PLUT entries  */
	} PLUTChunk; 

	


typedef unsigned long vdlentry; 	/* contains RGB8 triple and control bits */
									/* see hardware documentation  */

typedef struct A_VDL
	{
	Int32 		palettePtr;	
	Int32		dmaControl;	/*  */
	vdlentry	vdls[33];	/* VDL entries */
	Int32  		filler;		/* the length seems to need to be 144 for complex VDLs. */
	} A_VDL;
	
typedef struct VDLCHUNK				/* used for a standard 33 entry vdl list */
	{
	Int32 	chunk_ID;		/* 'VDL ' Magic number to identify VDL chunk */
	Int32	chunk_size;		/*  size in bytes of chunk including chunk_size */
	Int32	vdlcount;		/*  count of number of vdls following */
	A_VDL	vdl[1];			/*  VDL control words and entries  */
	} VdlChunk;


typedef struct Cpyr
	{
	Int32 	chunk_ID;					/* 'CPYR' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	copyright[1];				/*  C String ASCII Copyright Notice  */
	} Cpyr;


typedef struct Desc
	{
	Int32 	chunk_ID;					/* 'DESC' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	descrip[1];					/*  C String ASCII image description  */
	} Desc;

typedef struct Kwrd
	{
	Int32 	chunk_ID;					/* 'KWRD' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	keywords[1];					/*  C String ASCII keywords, separated by ';'   */
	} Kwrd;

typedef struct Crdt
	{
	Int32 	chunk_ID;					/* 'CRDT' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	credits[1];					/*  C String ASCII credits for this image  */
	} Crdt;

#endif	/* __IMAGEFILE__ */
