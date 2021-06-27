
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: vdl24util.c,v 1.6 1995/01/06 06:46:13 ceckhaus Exp $
**
******************************************************************************/

#include "types.h"
#include "graphics.h"
#include "stdio.h"

#include "getvideoinfo.h"
#include "vdlutil.h"
#include "vdl24util.h"


void InitVDL480( int32 *vdl, Bitmap *destBitmap, int32 bitmapindex, int32 displayMode )
/*
	Initialize a VDL for a double-height (480 lines in NTSC mode) screen
*/
{
	int32 j;

	/* Set the DMA control word */
	vdl[0] = VDL_ENVIDDMA			/* enable video DMA */
		| VDL_LDCUR					/* load address from current-bitmap-buffer-address word (see below) */
		| VDL_LDPREV				/* load address from previous-bitmap-buffer-address word (see below) */
		| (34 << VDL_LEN_SHIFT)		/* number of control words in this VDL entry -- we use 34 */
		| VDL_PREVSEL				/* add modulo ammount to previous-bitmap-buffer-address register;
										this uses the value specified by VDL_DISPMOD_nnn (see below) */
		| ( GetScreenHeight(displayMode) << VDL_LINE_SHIFT) /* persistence for VDL entry */
		| VDL_480RES;				/* 480 lines per frame */
	if ( PAL_DISPLAY(displayMode) )
		vdl[0] |= VDL_DISPMOD_384;
	else
		vdl[0] |= VDL_DISPMOD_320;

	/* The current-bitmap-buffer-address word */
	vdl[1] = ( (uint32) destBitmap->bm_Buffer );
	
	/* The previous-bitmap-buffer-address word */
	vdl[2] = ( (uint32) destBitmap->bm_Buffer ) + 2;
	
	/*
		The next-VDL-entry-address word -- VDL entries are 40 words;
		since we didn't set bit 18 (i.e, or with VDL_RELSEL) of the
		DMA control word above, this is interpreted as an offset (relative address),
		in words.
	*/
	vdl[3] = ( int32 ) ( vdl + 40 );
	
	/* RGB values for CLUT registers 0 to 8 */
	for ( j = 0; j < 8; j++ )
		vdl[ 4 + j ] = ( j << VDL_PEN_SHIFT ) | ( (j * 2) * 0x00010101 );

	/* RGB values for CLUT registers 8 to 16 */
	for ( j = 8; j < 16; j++ )
		vdl[ 4 + j ] = ( j << VDL_PEN_SHIFT ) | ( (j * 2 + 225) * 0x00010101 );

	/* RGB values for CLUT registers 16 to 31 */
	for ( j = 16; j < 32; j++)
		vdl[ 4 + j ] = ( j << VDL_PEN_SHIFT ) | ( ((31 - j) * 16) * 0x00010101 );


	/* The background-value word */
	vdl[ 4 + 32 ] = VDL_DISPCTRL
				| VDL_BACKGROUND;	/* These or'd values mean this is a background-value
										word.  The background value is set to the component
										values specified in the remainder of the word, namely
										zero. */
				
	/* The display control word */
	vdl[ 4 + 33 ] = VDL_DISPCTRL	/* this is a display control word */
									/* We want to use a custom CLUT, so we DON'T set bit 25
										(i.e., or with VDL_CLUTBYPASSEN) */
				| VDL_VINTEN		/* Enable vertical interpolation */
				| VDL_HSUB_ZERO		/* Set h-cornerweight to zero */
				| VDL_BLSB_BLUE		/* Use bit 0 of the bitmap-buffer pixel for the pixel's
										least-significant blue bit */
				| (int32) (bitmapindex ? VDL_VSUB_ONE : VDL_VSUB_ZERO);
									/* Set v-cornerweight to 1 for the 2nd bitmap,
										0 for the 1st bitmap */
	
	/* pad out with null commands to the end of the VDL */
	for ( j = 4 + 34; j < 40; j++ )
		vdl[ j ] = VDL_NULLVDL;

}


