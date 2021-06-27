
#ifdef imagefile
/*
 Version 0.3 September 1, 1992
 	Changes from last version
		Added conditional includes to define data types
		which are defined in the Opera environment but
		not in the MAC environment.  This file should now
		be able to be included in either environment.
		Changes the SPITEHDR to contain an exact copy
		of an Opera SCOB data structure.
 Version 0.2
	Changes from last version:
		Added a sprite Header chunk as suggested by slandrum
		Incorporated slandrum's suggested changes to CLUTLISTS
		Changed some fields in image header from uint16 to Int32
		
This document proposes a straw man image file format for
the Opera Hardware.  This format is designed to
	1) Provide an efficient native file format for
		images and sprites on the opera target
	2) Provide an archieval format for image storage 
		and image processing on development platforms.
	3) Be as simple as possible.
		
The idea is to have a common format which is capable of storing
24 bit images, perhaps with associated copyright notice, for storage
on a development station server machine.  This image will then be
processed into a background or sprite image and placed on a CD ROM 
for use by a product.  This format should be capable of encoding
both of these types of images.

Overview

This is a very simple tagged format.  A file is composed of one or more
chunks.  Each chunk contains a chunk_ID, a chunk_size and the data for the
chunk.  Chunks are not nested.  Chunks are concatenated together in a file.
The only required chunks for an image are a header chunk (image header or 
sprite header) and the pixel data. A header chunk should be the first chunk
in the file.  Other than this there should be no restrictions on chunk order. 
Also if multiple images are of the same type and same size they can be stored
in the same file by having one header chunk and multiple pixel data chunks.

A file should not contain more than one header chunk.

*/

Preliminaries:
	This file format assumes a "big-endian" architecture,
	that is multi-byte values have their most significant
	bytes stored at the lowest memory addresses.  This is
	the standard memory organization for the 68000 and is
	used in the ARM6 on the opera machine.
	
	
	
	{file}  		::=	{chunk} | {file}{chunk}
	{chunk_header}	::=	{chunk_ID} {chunk_size}
	{chunk} 		::=	{chunk_header}{chunk body}
	
	{chunk_ID} 		::= 'IMAG'	|			/* Image header  */
						'SPRH'	|			/* Sprite header */
						'PDAT'	|			/* Pixel Data	 */
						'PIPT'	|			/* PIP Table     */
						'CLUT'	|			/* CLUT list     */
						'MCLT'	|			/* Multiple Line CLUT List */
						'CPYR'	|			/* Copyright Notice */
						'DESC'	|			/* Text Description of image */
						'KWRD'	|			/* Text Keywords associated with image */
						'CRDT'				/* Text credits associated with image */
						
	{chunk_size}::= Unsigned 32 bit integer (includes size of chunk body plus size
					of the chunk header).  Chunk_size is 8 plus size of the chunk_body.
					Chunks must be Quad byte alligned.  Chunks will be padded with
					zeros to fill up the quad word alignment.
/*

ISSUES:
	Should the chunk_size include the 8 bytes needed for the Chunk ID and the chunk size?
	If we don't include these 8 bytes then just after you have read the size you can
	skip that many bytes and find the next Chunk_ID  (or the EOF).
	Alignment:  I suspect we will be better off by making all chunks 4 byte aligned.
	The Arm processor is much better at loading 32 bit quantities from quad word addresses.
	Since not all chunks will be a multiple of 4 bytes in lenght we are forced to put the actual
	chunk length in the chunk_size field and then round up to find the next chunk boundary.

The QUADALIGN macro can be useful in dealing with chunk sizes
*/


/*
ISSUE:

Should fields that require 16 bits such as width and height be stored as 16 or
32 bit fields in the files structure?  The Arm Processor provides more efficient
access to 32 bit fields, but the 16 bit fields save space.  To access 16 bit fields
the Arm processor actually performs two byte access and then shifts and adds to 
assemble the 16 bit value.
*/
#endif

#ifndef __IMAGEFILE__
#define __IMAGEFILE__


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

#ifndef __graphics_h

typedef unsigned char		ubyte;
typedef unsigned long		ulong;


typedef Int32 Color;
typedef Int32 Coord;
typedef Int32 CLUTEntry;
typedef Int32 RGB888;


typedef struct OperaPoint {
  Coord pt_X, pt_Y;
} OperaPoint;
#endif

#define QUADALIGN(x)  ( ((x+3)>>2)<<2)


typedef unsigned short uint16;


#define imagetype 'IMAG'

#define 	kNormal 1
#define 	kLinear	2
#define 	kbpp0 1
#define 	kbpp1	2
#define 	kbpp2	3	
#define 	kbpp4	4	
#define 	kbpp6	5	
#define 	kbpp8	6	
#define 	kbpp16	7	

#define nWOffset8 	0xff000000
#define shiftWOffset8 	24
#define nWOffset10 	0x03ff0000
#define nBothOffsets 0xffff0000
#define shiftWOffset10 	16
#define nNoSWAP 		0x00004000
#define nTLLSB 			0x00003000
#define shiftTLLSB 		12
#define nLRFORM 		0x00000800
#define nTLHPCNT   	0x000007ff

#define 		nSKIP  0x80000000
#define 		nLAST   0x40000000
#define 		nNPABS   0x20000000
#define 		nSPABS   0x10000000
#define 		nPPABS   0x08000000
#define 		nLDSIZE   0x04000000
#define 		nLDPRS   0x02000000
#define 		nLDPPMP   0x01000000
#define 		nLDPIP   0x00800000
#define 		nSCoBPRE  0x00400000
#define 		nACW   0x00040000
#define 		nACCW   0x00020000
#define 		nTWD   0x00010000
#define 		nLCE   0x00008000
#define 		nACE   0x00004000
#define			nPXOR	0x00000800
#define 		nPacked 0x00000200
#define 		nDOver   0x00000180
#define 		shiftDOver   7
#define 		nPIPPOS   0x00000040
#define 		nBGND   0x00000020
#define 		nNOBLK   0x00000010
#define 		nPIPA    0x0000000F
#define 		nVCNT   0x0000FFC0
#define 		shiftVCNT   6
#define 		nIPNT   0x00000020
#define 		nLINEAR   0x00000010
#define 		nREP8   0x00000008
#define 		nBPP   0x00000007

	
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

typedef struct IMAGEHDR 
	{
	Int32 	chunk_ID;		/* 'IMAG' Magic number to identify this image headr chunk */
	Int32	chunk_size;		/*  size in bytes of chunk including chunk_size  (24)  */

	Int32	w;				/*  width in pixels */			
	Int32	h;				/*  height in pixels */
	Int32	bytesperrow;	/*  may include pad bytes at row end for alignment */
	ubyte	bitsperpixel;	/*  1,2,4,6,8,16,24 or 32*/
	ubyte	numcomponents;	/*  3 => RGB Red Green Blue, 1 => color index */
							/*  3 => linear (8 or 16 bits per pixel)   */
							/*  1 => Normal mode (1,2,4,6,8, or 16 bits per pixel) */
	ubyte	numplanes;		/*  1 => chunky;  3=> planar  */	
							/*  although the hardware does not support planar modes */
							/*  it is useful for some compression methods to separate */
							/*  the image into RGB planes or into YCrCb planes */
							/*  numcomponents must be greater than 1 for planar to have */
							/*  any effect */
	ubyte	colorspace;		/*  0 => RGB, 1 => YCrCb   */
	ubyte	comptype;		/*  compression type; 0 => uncompressed */
							/*  1=Sprite bit packed */
							/*  other compression types will be defined later */
	ubyte 	hvformat;		/*  0 => 0555;  1=> 0554h;  2=> 0554v; 3=> v554h  */	
	ubyte 	pixelorder;		/*  0 => (0,0), (1,0),  (2,0)   (x,y) is (row,column) */
							/*  1 => (0,0), (0,1), (1,0), (1,1)  Sherrie LRform  */	
							/*  2 => (0,1), (0,0), (1,1), (1,0)  UGO LRform  */	
	ubyte 	version;		/*  file format version identifier.  0 for now  */	
	} imagehdr;
	
	
struct PIXELDATA
	{
	Int32 	chunk_ID;				/* 'PDAT' Magic number to identify pixel data */
	Int32	chunk_size;				/*  size in bytes of chunk including chunk_size */
	ubyte	pixels[1];				/*  pixel data (format depends upon description in the imagehdr */
	};
	
typedef struct PIXELDATA pixels;

/* Notes this data structure is the same size and shares common fields with */
/* the SCB data structure.  Common fields are in the same locations         */
/* This is done so that a program can allocate a chunk of memory, read the body */
/* of a spritehdr chunk into this memory block, then use the block as a SCoB */
/* (Sprite Control Block) data structure after setting up the appropriate link */
/* pointer data fields */


struct SPRITEHDR
 	{
	Int32	chunk_ID;		/* 'SPRH' Magic number to identify pixel data */
	Int32	chunk_size;		/*  size in bytes of chunk including chunk_size */
	ulong	scobversion;	/* version number of the scob data structure.  0 for now */
	ulong	scb_Flags;		/* 32 bits of SCOB flags */
	ulong	scb_NextPtr;
	ulong	scb_SpriteData;
	ulong	scb_PIPPtr;
	long	scb_X;
	long	scb_Y;
	long	scb_hdx;
	long	scb_hdy;
	long	scb_vdx;
	long	scb_vdy;
	long	scb_ddx;
	long	scb_ddy;
	ulong	scb_PPMPC;
	ulong	scb_PRE0;		/* Sprite Preamble Word 0 */
	ulong	scb_PRE1;		/* Sprite Preamble Word 1 */
	long	scb_Width;
	long	scb_Height;
	};

typedef struct SPRITEHDR spritehdr;
	

/* typedef unsigned short RGB555;  */

struct PIPTABLE
	{
	Int32 	chunk_ID;			/* 'PIPT' Magic number to identify pixel data */
	Int32	chunk_size;			/*  size in bytes of chunk including chunk_size */
	Int32	numentries;			/* 	number of entries in PIP Table */
	RGB555  pip[1];				/*  pip entries  */
	};

typedef struct PIPTABLE piptable;
	


typedef unsigned long clutentry; 	/* contains RGB8 triple and control bits */
							/* see hardware documentation  */

	
struct CLUTCHUNK			/* used for a standard 33 entry clut list */
	{
	Int32 	chunk_ID;		/* 'CLUT' Magic number to identify pixel data */
	Int32	chunk_size;		/*  size in bytes of chunk including chunk_size */
	clutentry  cluts[33];	/*  CLUT entries  */
	};

typedef struct CLUTCHUNK clutchunk;

struct CLUTLINEGROUP
	{
	uint16	loadline;  	  /* vertical screen index at which to load this CLUT List */
	uint16  clutentries;  /* count of the number of CLUTENTRY's to follow */
	clutentry cluts[1];		  /* actually a variable sized array */
	};
	
typedef struct CLUTLINEGROUP clutlinegroup;


struct MCLUTCHUNK		/* used for a fancy clut list */
	{
	Int32 	chunk_ID;			/* 'MCLT' Magic number to identify pixel data */
	Int32	chunk_size;			/*  size in bytes of chunk including chunk_size */
	Int32	linegroupcount;		/* count of number of line groups following */
	clutlinegroup clutlines[1];			/* actually a variable sized array */
	};

typedef struct MCLUTCHUNK mclutchunk;

struct CPYR
	{
	Int32 	chunk_ID;					/* 'CPYR' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	copyright[1];				/*  C String ASCII Copyright Notice  */
	};

typedef struct CPYR cpyr;

struct DESC
	{
	Int32 	chunk_ID;					/* 'DESC' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	descrip[1];					/*  C String ASCII image description  */
	};

typedef struct DESC desc;

struct KWRD
	{
	Int32 	chunk_ID;					/* 'KWRD' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	keywords[1];					/*  C String ASCII keywords, separated by ';'   */
	};

typedef struct KWRD kwrd;

struct CRDT
	{
	Int32 	chunk_ID;					/* 'CRDT' Magic number to identify pixel data */
	Int32	chunk_size;					/*  size in bytes of chunk including chunk_size */
	char  	credits[1];					/*  C String ASCII credits for this image  */
	};

typedef struct CRDT crdt;

#endif	/* __IMAGEFILE__ */