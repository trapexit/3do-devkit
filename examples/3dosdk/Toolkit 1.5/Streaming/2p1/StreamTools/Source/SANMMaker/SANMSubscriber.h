/*******************************************************************************************
 *	File:			SAnimSubscriber.h
 *
 *	Contains:		definitions for SAnimSubscriber.c
 *
 *	Written by:		Neil Cormia (variations on a theme by Joe Buczek)
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	6/15/93		jb		Changed MARKER_CHUNK_TYPE to 'GOTO' from 'MRKR'
 *	4/23/93		JML		Simplified for CELS2SANM program.
 *	4/22/93		njc		New today.
 *
 *******************************************************************************************/
/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

/* The following preamble is used at the top of each SANM chunk
 * passed in from the DSH.
 */

#define	FILLER_CHUNK_TYPE	'FILL'
#define MARKER_CHUNK_TYPE	'GOTO'
#define	CTRL_CHUNK_TYPE		'CTRL'	/* chunk type for this subscriber */
#define MAX_CHANNELS 20

#define	STREAM_ANIM_TYPE	3
#define	STREAM_AA_ANIM_TYPE	4

typedef	struct StreamMarkerChunk {
	Int32		chunkType;		/*	'CTRL'						*/	
	Int32		chunkSize;		/*  20							*/									
	Int32		time;			/*  pos. in stream in 240ths	*/									
	Int32		channel;		/*	which anim in the stream	*/	
	Int32		subChunkType;	/*	'MRKR'						*/
	Int32		marker;			/*	go to offset				*/	
} StreamMarkerChunk, *StreamMarkerChunkPtr;

typedef	struct StreamAnimHeader {
	Int32		chunkType;		/*	'SANM'						*/	
	Int32		chunkSize;											
	Int32		time;			/*  pos. in stream in 240ths	*/									
	Int32		channel;		/*	which anim in the stream	*/	
	Int32		subChunkType;	/*	'AHDR'						*/
	Int32		version;		/*  current version = 0			*/
	Int32		animType;		/* 	3 = Anim, 4 = AA Anim 		*/
	Int32		numFrames;		/* 	number of frames in anim	*/
	Int32		frameRate;		/* 	in frames per second		*/
} StreamAnimHeader, *StreamAnimHeaderPtr;


typedef	struct StreamAnimCCB {
	Int32		chunkType;		/*	'SANM'						*/	
	Int32		chunkSize;											
	Int32		time;			/*  pos. in stream in 240ths	*/									
	Int32		channel;		/*	which anim in the stream	*/	
	Int32		subChunkType;	/*	'ACCB'						*/
	UInt32		ccb_Flags;		/*	32 bits of CCB flags		*/
	struct  	CCB *ccb_NextPtr;
	void    	*ccb_CelData;
	void       	*ccb_PLUTPtr;
	Coord 		ccb_X;
	Coord 		ccb_Y;
	Int32		ccb_hdx;
	Int32		ccb_hdy;
	Int32		ccb_vdx;
	Int32		ccb_vdy;
	Int32		ccb_ddx;
	Int32		ccb_ddy;
	UInt32		ccb_PPMPC;
	UInt32		ccb_PRE0;		/*	Sprite Preamble Word 0		*/
	UInt32		ccb_PRE1;		/*	Sprite Preamble Word 1		*/
	Int32		ccb_Width;
	Int32		ccb_Height;
} StreamAnimCCB, *StreamAnimCCBPtr;


typedef	struct StreamAnimPLUT {
	Int32		chunkType;		/*	'SANM'						*/	
	Int32		chunkSize;											
	Int32		time;			/*  pos. in stream in 240ths	*/									
	Int32		channel;		/*	which anim in the stream	*/	
	Int32		subChunkType;	/*	'APLT'						*/
/*	char		data[???];			a  PLUT 					*/
} StreamAnimPLUT, *StreamAnimPLUTPtr;

typedef	struct StreamAnimFrame {
	Int32		chunkType;		/*	'SANM'						*/	
	Int32		chunkSize;											
	Int32		time;			/*  pos. in stream in 240ths	*/									
	Int32		channel;		/*	which anim in the stream	*/	
	Int32		subChunkType;	/*	'FRME'						*/
	Int32		duration;		/*	which anim in the stream	*/	
/*	char		data[???];		a PDAT 							*/
} StreamAnimFrame, *StreamAnimFramePtr;

