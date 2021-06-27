#ifndef __VDL24UTIL_H
#define __VDL24UTIL_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: vdl24util.h,v 1.4 1995/01/02 04:05:35 ceckhaus Exp $
**
******************************************************************************/


#include "types.h"

typedef struct
{
	unsigned int flag		: 1;	// [    31] one   bits of zero to signify a color value VDL word
	unsigned int select		: 2;	// [ 30-29] two   bits of color select 0=>all 01=>blue 10=>green 11=>red
	unsigned int index		: 5;	// [ 28-24] five  bits of color index
	unsigned int red		: 8;	// [ 23-16] eight bits of red
	unsigned int green		: 8;	// [ 15- 8] eight bits of green
	unsigned int blue		: 8;	// [  7- 0] eight bits of blue
} VDLColorEntry;

typedef struct
{
	uint32 						SVDLDMA;
	uint32 				  		SVDLCurBuf;
	uint32						SVDLPrevBuf;
	uint32						SVDLNextVDL;
	uint32						SVDLDisplay;
	VDLColorEntry				SVDLColors[32];
	uint32						backgroundEntry; //	RGB 000 will use this entry
	uint32						SVDLFiller1;
	uint32						SVDLFiller2;
} SVDL;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void InitVDL480( int32 *vdl, Bitmap *destBitmap, int32 bitmapindex, int32 displayMode );

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __VDL24UTIL_H */
