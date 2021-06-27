/*****************************************************************************
 *	File:			InitCel.c
 *
 *	Contains:		Routine to init a cel using the specified attributes.
 *	Copyright:	(c) 1994 The 3DO Company. 	All Rights Reserved.
 *
 *	History:
 *	07/26/94  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	After initing a cel, you can bash the CCB in any way you want.  Feel 
 *	free to change the flags, the contents of the PLUT and/or data buffer, etc.
 *	
 *	At the very least you have to attach a cel data buffer to the ccb_SourcePtr
 *	field and, if the cel is coded, a PLUT to the ccb_PLUTPtr, after calling
 *	this function.
 ****************************************************************************/

#include "CelUtils.h"
#include "Debug3DO.h"

#define PIXC_OPAQUE			0x1F001F00

/*----------------------------------------------------------------------------
 * InitCel()
 *	Init a cel's CCB to a standard setup.
 *--------------------------------------------------------------------------*/

int32 InitCel(CCB *cel, int32 w, int32 h, int32 bpp, int32 options)
{
	int32		rowBytes;
	int32		rowWOFFSET;
	int32		wPRE;
	int32		hPRE;
	
	/*------------------------------------------------------------------------
	 * Zero out the CCB fields that don't need special values.
	 *----------------------------------------------------------------------*/

	cel->ccb_NextPtr	= NULL;
	cel->ccb_SourcePtr	= NULL;
	cel->ccb_PLUTPtr	= NULL;
	cel->ccb_XPos		= 0;
	cel->ccb_YPos		= 0;
	cel->ccb_HDY		= 0;
	cel->ccb_VDX		= 0;
	cel->ccb_HDDX		= 0;
	cel->ccb_HDDY		= 0;

	/*------------------------------------------------------------------------
	 * Set up the CCB fields that need non-zero values.
	 *----------------------------------------------------------------------*/

	cel->ccb_HDX		= 1 << 20;
	cel->ccb_VDY		= 1 << 16;
	cel->ccb_PIXC 		= PIXC_OPAQUE;
	cel->ccb_Flags		= 	CCB_LAST 	| CCB_NPABS | CCB_SPABS  | CCB_PPABS  |
							CCB_LDSIZE 	| CCB_LDPRS | CCB_LDPPMP | 
							CCB_CCBPRE 	| CCB_YOXY 	| CCB_USEAV  | CCB_NOBLK  |
							CCB_ACE		| CCB_ACW 	| CCB_ACCW   |
							((options & INITCEL_CODED) ? CCB_LDPLUT : 0L);

	/*------------------------------------------------------------------------
	 * massage the width/height values.
	 *	we have to set the bytes-per-row value to a a word-aligned value,
	 *	and further have to allow for the cel engine's hardwired minimum
	 *	of two words per row even when the pixels would fit in one word.
	 *----------------------------------------------------------------------*/

	rowBytes   = (((w * bpp) + 31) >> 5) << 2;
	if (rowBytes < 8) {
		rowBytes = 8;
	}
	rowWOFFSET = (rowBytes >> 2) - PRE1_WOFFSET_PREFETCH;

	wPRE = (w - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT;
	hPRE = (h - PRE0_VCNT_PREFETCH)    << PRE0_VCNT_SHIFT;
	
	cel->ccb_Width	= w;
	cel->ccb_Height = h;

	/*------------------------------------------------------------------------
	 * fill in the CCB preamble fields.
	 *----------------------------------------------------------------------*/

	if (!(options & INITCEL_CODED)) {
		hPRE |= PRE0_LINEAR;
	}
	
	wPRE |= PRE1_TLLSB_PDC0;	// Use blue LSB from source pixel or PLUT blue LSB.

	switch (bpp) {
	  case 1:	
	  	cel->ccb_PRE0 = hPRE | PRE0_BPP_1;	
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 2:	
	  	cel->ccb_PRE0 = hPRE | PRE0_BPP_2;	
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 4:	
	  	cel->ccb_PRE0 = hPRE | PRE0_BPP_4;	
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 6:	
	  	cel->ccb_PRE0 = hPRE | PRE0_BPP_6;	
		cel->ccb_PRE1 = wPRE| (rowWOFFSET << PRE1_WOFFSET8_SHIFT);					
		break;
	  case 8:	
	  	cel->ccb_PRE0 = hPRE | PRE0_BPP_8;	
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET10_SHIFT);					
		break;
	  default:
	  	DIAGNOSE(("Unsupported bits-per-pixel value %ld; 16-bpp assumed\n", bpp));
		/* fall thru */
	  case 16:	
	  	cel->ccb_PRE0 = hPRE | PRE0_BPP_16;	
		cel->ccb_PRE1 = wPRE | (rowWOFFSET << PRE1_WOFFSET10_SHIFT);					
		break;
	}

	return (h * rowBytes);
}
