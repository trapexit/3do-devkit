
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: vdlutil.c,v 1.9 1995/01/10 17:30:47 vertex Exp $
**
******************************************************************************/

#include "types.h"
#include "graphics.h"
#include "mem.h"
#include "debug3do.h"
#include "form3do.h"

#include "vdlutil.h"

/* some globals */
short   gCurrFieldNum = 0;

/*
 * Build a vdl DMA clut control word.
 */
int32 SetDMACtlWord( int32 clutColorWords, int32 scanLineCount, Boolean lastScanLine, int32 displayMode )
{
    /* the standard clut control flags are: */
    /*      link to next color set is relative */
    /*      use "current frame buffer" address from this color set */
    /*      use "previous frame buffer" address from this color set */

    DMAControlWord  clutCtlWord;

    /* zero all members to begin with */
    LONGWORD(clutCtlWord) = 0;

	if ( PAL_DISPLAY(displayMode) )
		clutCtlWord.displayMode = 0x01;

    clutCtlWord.useCurrentLineAddr = true;
    clutCtlWord.usePreviousLineAddr = true;
    clutCtlWord.numColorWords = (unsigned int) clutColorWords;
    clutCtlWord.scanLines = (unsigned int) scanLineCount;

    /* if this isn't for the last scanline, next address is relative */
    if ( !lastScanLine )
		{
		clutCtlWord.relativeAddrNextCLUT = true;
		clutCtlWord.videoDMA = true;
		}

    return LONGWORD(clutCtlWord);
}


/*
 *  Build a VDL display control word.
 */
int32 SetDisplayCtlWord(Boolean Hinterp, Boolean Vinterp, Boolean clutBypass)
{
    VDLDisplayCtlWord   displayControl;

    LONGWORD(displayControl) = 0;

    displayControl.controlWord = 1;
    displayControl.controlWordType = 1;
    if (Hinterp)
		{
		displayControl.horizontalInterpolation = 1;
		displayControl.windowHorizontalInterp = 1;
		}
    if (Vinterp)
		{
		displayControl.verticalInterpolation = 1;
		displayControl.windowVerticalInterp = 1;
		}

    /* Always use bit 0 of frame-buffer pixel for blue lsb */
	displayControl.blueLSBSource = 2;
    displayControl.windowBlueLSBSource = 2;

    if (clutBypass)
	{
		displayControl.bypassCLUT = 1;
		displayControl.bypassLSBIsRandom = 1;
	}

    return LONGWORD(displayControl);
}

int32 GetVDLSize( int32 displayMode )
{
	int32 VDLSize = -1;

	switch ( displayMode )
	{
		case DI_TYPE_NTSC:
			VDLSize = NTSC_VDL_SIZE;
			break;

		case DI_TYPE_PAL1:
			VDLSize = PAL1_VDL_SIZE;
			break;

		case DI_TYPE_PAL2:
			VDLSize = PAL2_VDL_SIZE;
			break;

		default:
			break;
	}

	return VDLSize;
}

void internalInitVDL(VDLEntry *vdlPtr, Bitmap *destBitmap, int32 displayMode, Boolean bypassClut)
/*
 * Fill in a "standard" VDL - ie. one with a "System Clut" ( standard 3DO IMAG
 *  file displays correctly with this clut), and one clut entry per scanline.
 */
{
    int32   scanLine;
	int32	colorTableEntry;
    VDL_REC *vdlArrayPtr;
    VDL_REC	*vdlRecPtr;
    uint32	prevFrameBuffer = 0;
    uint32  *vdlClutEntry;
	int32	screenHeight;
	int32	screenWidth;

	if ( vdlPtr == NULL )
		goto DONE;

	vdlArrayPtr = (VDL_REC *) vdlPtr;

	screenHeight = GetScreenHeight( displayMode );
	screenWidth = GetScreenWidth( displayMode );

	for (scanLine = 0; scanLine < screenHeight; scanLine++)
    {
		/* There are a minimum of 4 (maybe 6) entries in a 'color palette'.  There */
		/*  are ALWAYS 4 control words:  The first word is always the */
		/*  control value to be deposited in the CLUT DMA controller, the second */
		/*  word is always the 'current' frame buffer address, the third word is */
		/*  always the 'previous' frame buffer address, and the fourth word is */
		/*  always the address of the next color palette to be used.  The maximum */
		/*  number of total words in a color palette is about 50, due to the amount */
		/*  of time available for the hardware to process it during horiz blank.  In */
		/*  addition a color palette is not allowed to fall across a 1 megabyte */
		/*  physical VRAM page boundary. */

		vdlRecPtr = &vdlArrayPtr[scanLine];

		/* first, the DMA control word... */
		vdlRecPtr->controlword = SetDMACtlWord(CLUT_COLOR_WORDS - FILLER_WORDS, 1, false, displayMode);

		/* second, the 'current' scanline frame buffer address... */
		/* in lrform, the pixels from even and odd lines are paired */
		vdlRecPtr->curLineBuffer    = ((uint32)destBitmap->bm_Buffer)
						+ (scanLine / 2) * screenWidth * 4
						+ ((scanLine & 1) * 2);

		/* third, the 'previous' scanline frame buffer address... */
		if ( scanLine != 0 )
			vdlRecPtr->prevLineBuffer = prevFrameBuffer;
		else
			vdlRecPtr->prevLineBuffer = vdlRecPtr->curLineBuffer;
		prevFrameBuffer = vdlRecPtr->curLineBuffer;

		/* fourth, is the address (relative in this case) of the next color palette to use */
		vdlRecPtr->nextVDLEntry = (uint32) CLUT_COLOR_WORDS * sizeof(uint32);

		/* control word for this line...  set it to null */
		vdlRecPtr->displayControl = VDL_NULLVDL;

		/* colors to use on this line... also set them to null */
		vdlClutEntry = &vdlRecPtr->CLUTEntry[0];
		for (colorTableEntry = 0; colorTableEntry < CLUT_WORDS; colorTableEntry++)
			vdlClutEntry[ colorTableEntry ] = VDL_NULLVDL;

		/* background-value word... also set it to null */
		vdlRecPtr->backgroundEntry = VDL_NULLVDL;

		vdlRecPtr->filler1	      = VDL_NULLVDL;
		vdlRecPtr->filler2	      = VDL_NULLVDL;
    }

	/* Adjust the DMA control word in the entry for the last scanline */
	vdlRecPtr = &vdlArrayPtr[screenHeight - 1];
	vdlRecPtr->controlword = SetDMACtlWord(CLUT_COLOR_WORDS - FILLER_WORDS, 1, true, displayMode);

	/* and finally set the display control word for the whole screen */
	vdlRecPtr = &vdlArrayPtr[0];
    vdlRecPtr->displayControl = SetDisplayCtlWord(true, true, bypassClut);

DONE:
	return;
}

void InitVDL(VDLEntry *vdlPtr, Bitmap *destBitmap, int32 displayMode)
{
	internalInitVDL( vdlPtr, destBitmap, displayMode, false );
}

void InitVDL24(VDLEntry *vdlPtr, Bitmap *destBitmap, int32 displayMode)
{
	internalInitVDL( vdlPtr, destBitmap, displayMode, true );
}

/*
 * Merge a raw VDL, i.e. one generated by the PhotoShop export module, into an
 *  "active" one, ie. one already run through InitVDL above.
 */
void internalMergeVDL(VDL_REC *rawVDLPtr, VDL_REC *activeVDLPtr, uint32 vdlLines, int32 displayMode, Boolean bypassClut)
{
    uint32			scanLineEntry;
    int32			index;
    VDL_REC			*rawVDLRecPtr = NULL;
    VDL_REC			*activeVDLRecPtr = NULL;
    VDL_REC     	*rawVDLArrayPtr = rawVDLPtr;
    VDL_REC     	*activeVDLArrayPtr = activeVDLPtr;
    uint32      	*rawClutEntry;
	uint32			*activeClutEntry;
    DMAControlWord  *clutCtlWordPtr;
	int32			screenHeight;

    screenHeight = GetScreenHeight( displayMode );

	for (scanLineEntry = 0; scanLineEntry < vdlLines; scanLineEntry++)
		{
		rawVDLRecPtr = &rawVDLArrayPtr[scanLineEntry];
		activeVDLRecPtr = &activeVDLArrayPtr[scanLineEntry];

		/* merge the stuff set up by the export module */
		/*  - the DMA control word and the background color */
		activeVDLRecPtr->controlword = rawVDLRecPtr->controlword;
		activeVDLRecPtr->backgroundEntry = rawVDLRecPtr->backgroundEntry;

		// if this is a single VDL color Table we need to enable it
		// for all 240 lines
		if (vdlLines == 1)
			activeVDLRecPtr->controlword =
				SetDMACtlWord(CLUT_COLOR_WORDS - FILLER_WORDS, screenHeight, false, displayMode);

		/* if this vdl was generated by the old vdl export tool, it includes the */
		/*  2 filler words (used to push the total vdl length to 40 long words) in */
		/*  the vdl length.  SubmitVDL will now fail if the total length of a vdl */
		/*  is greater than 38, so exclude the filler words from the length. */
		clutCtlWordPtr = (DMAControlWord *) &activeVDLRecPtr->controlword;
		if ( (*clutCtlWordPtr).numColorWords == CLUT_COLOR_WORDS )
			(*clutCtlWordPtr).numColorWords = CLUT_COLOR_WORDS - FILLER_WORDS;

		/* Set the display control commands */
		if ( scanLineEntry )
			activeVDLRecPtr->displayControl = VDL_NULLVDL;
		else
			activeVDLRecPtr->displayControl = SetDisplayCtlWord(true, true, bypassClut);

		/* and the actual color entries */
		activeClutEntry = &activeVDLRecPtr->CLUTEntry[0];
		rawClutEntry    = &rawVDLRecPtr->CLUTEntry[0];
		for (index = 0; index < CLUT_WORDS; ++index)
			activeClutEntry[index] = rawClutEntry[index];
		}
}

void MergeVDL(VDL_REC *rawVDLPtr, VDL_REC *activeVDLPtr, uint32 vdlLines, int32 displayMode)
{
	internalMergeVDL( rawVDLPtr, activeVDLPtr, vdlLines, displayMode, false );
}

void MergeVDL24(VDL_REC *rawVDLPtr, VDL_REC *activeVDLPtr, uint32 vdlLines, int32 displayMode)
{
	internalMergeVDL( rawVDLPtr, activeVDLPtr, vdlLines, displayMode, true );
}

/*
	Allocate memory for, initialize, and submit a new VDL to the system.
*/
int32 internalAllocateVDL(VDLEntry **newVDL, Bitmap *screenBitMap, int32 displayMode, Boolean bypassClut)
{
    /* allocate some memory.  no need to use VRAM as the system makes its own copy */
    /*  of the vdl when we submit */
    *newVDL = (VDLEntry *)AllocMem( GetVDLSize(displayMode),
		    MEMTYPE_ANY
		    | (MEMTYPE_FILL | 0x00) );
    if ( *newVDL == NULL )
    {
		PRT ( ("Can't allocate VDL memory\n"));
		return -1;
    }

    /* init the memory we just allocated with the screen specific info it needs */
    internalInitVDL(*newVDL, screenBitMap, displayMode, bypassClut);

    return 0;
}

int32 AllocateVDL(VDLEntry **newVDL, Bitmap *screenBitMap, int32 displayMode)
{
	return internalAllocateVDL( newVDL, screenBitMap, displayMode, false );
}

int32 AllocateVDL24(VDLEntry **newVDL, Bitmap *screenBitMap, int32 displayMode)
{
	return internalAllocateVDL( newVDL, screenBitMap, displayMode, true );
}


/********************************************************************************* */
/*** */
/***			    EXPERIMENTAL ROUTINES */
/***			      USE WITH CAUTION!! */
/*** */

enum
{
    nullAMYVideoControl = 0,
    videoIsPAL,
    select640Mode,
    bypassCLUT,
    slipStreamOverlay,
    forceTransparency,
    backgroundDetector,

    swapWindowPenHV,
    windowVSource,
    windowHSource,
    windowBlueLSBSource,
    windowVerticalInterp,
    windowHorizontalInterp,
    windowMSBReplicate,

    bypassLSBIsRandom,

    swapPenVH,
    VSource ,
    HSource,
    blueLSBSource,
    verticalInterpolation,
    horizontalInterpolation ,

    colorsOnly,

    noVerticalInterpLine
};

static char*    fieldNames[] =
{
    "nullAMYVideoControl",
    "videoIsPAL",
    "select640Mode",
    "bypassCLUT",
    "slipStreamOverlay",
    "forceTransparency",
    "backgroundDetector",

    "swapWindowPenHV",
    "windowVSource",
    "windowHSource",
    "windowBlueLSBSource",
    "windowVerticalInterp",
    "windowHorizontalInterp",
    "windowMSBReplicate",

    "bypassLSBIsRandom",

    "swapPenVH",
    "VSource",
    "HSource",
    "blueLSBSource",
    "verticalInterpolation",
    "horizontalInterpolation",

    "colorsOnly",

    "noVerticalInterpLine"
};


/*
 * limit a number to a range
 */
short LimitNum(short num, short min, short max)
{
    if ( num > max )
		num = max;
    else if ( num < min )
		num = min;

    return num;
}


/*
 * Change the VDL display control word 'focus' bit field.
 */
void ShowAnotherField(VDLDisplayCtlWord dmaWord, Boolean nextField)
{
    if ( nextField )
	gCurrFieldNum++;
    else
	gCurrFieldNum--;

    gCurrFieldNum = LimitNum(gCurrFieldNum, nullAMYVideoControl, noVerticalInterpLine);
    printf("%s is %d,\t[%X]\n", fieldNames[gCurrFieldNum], GetDisplayCtlValue(dmaWord, gCurrFieldNum),
		LONGWORD(dmaWord));
}


/*
 * Change the value of the VDL display control word 'focus' bit field.
 */
void SetCtlField( VDLDisplayCtlWord *dmaWord, Boolean increase )
{
    short       currValue = GetDisplayCtlValue(*dmaWord, gCurrFieldNum);

    if ( increase )
	currValue++;
    else
	currValue--;

    *dmaWord = SetDisplayCtlValue(*dmaWord, currValue, gCurrFieldNum);
    printf("%s set to %d,\t[%X]\n", fieldNames[gCurrFieldNum], GetDisplayCtlValue(*dmaWord, gCurrFieldNum),
		LONGWORD(*dmaWord));
}

/*
 * Read the value of the VDL display control word 'focus' bit field.
 */
short GetDisplayCtlValue(VDLDisplayCtlWord dmaWord, short fieldNum)
{
    switch ( fieldNum )
    {
	case nullAMYVideoControl:   /* */
	    return dmaWord.nullAMYVideoControl;
	break;

	case videoIsPAL:	    /* [   27] PAL video */
	    return dmaWord.videoIsPAL;
	break;

	case select640Mode:	 /* [   26] select 640 mode */
	    return dmaWord.select640Mode;
	break;

	case bypassCLUT:	    /* [   25] bypass CLUT if pen MSB set */
	    return dmaWord.bypassCLUT;
	break;

	case slipStreamOverlay:     /* [   24] source of overlay is SlipStream */
	    return dmaWord.slipStreamOverlay;
	break;

	case forceTransparency:     /* [   23] */
	    return dmaWord.forceTransparency;
	break;

	case backgroundDetector:    /* [   22] enable background detector */
	    return dmaWord.backgroundDetector;
	break;

	case swapWindowPenHV:       /* [   21] swap H and V bits in pen numbers in window */
	    return dmaWord.swapWindowPenHV;
	break;

	case windowVSource:	 /* [20-19] selects source for V sub-positions in window pixels */
	    return dmaWord.windowVSource;
	break;

	case windowHSource:	 /* [18-17] selects source for H sub-position in window pixels */
	    return dmaWord.windowHSource;
	break;

	case windowBlueLSBSource:   /* [16-15] selects source for window blue pen number's LSB */
	    return dmaWord.windowBlueLSBSource;
	break;

	case windowVerticalInterp:  /* [   14] enable vertical interpolation in window */
	    return dmaWord.windowVerticalInterp;
	break;

	case windowHorizontalInterp:/* [   13] enable horizontal interpolation in window */
	    return dmaWord.windowHorizontalInterp;
	break;

	case windowMSBReplicate:    /* [   11] */
	    return dmaWord.windowMSBReplicate;
	break;

	case bypassLSBIsRandom:     /* [   12] enable random number for low-bits of CLUT bypass */
	    return dmaWord.bypassLSBIsRandom;
	break;

	case swapPenVH:	     /* [   10] swap H and V bits in pen number */
	    return dmaWord.swapPenVH;
	break;

	case VSource:	       /* [ 9- 8] selects source of vertical sub-position */
	    return dmaWord.VSource;
	break;

	case HSource:	       /* [ 7- 6] selects source of horizontal sub-position */
	    return dmaWord.HSource;
	break;

	case blueLSBSource:	 /* [ 5- 4] selects source of blue pen number's LSB */
	    return dmaWord.blueLSBSource;
	break;

	case verticalInterpolation: /* [    3] enable vertical interpolation */
	    return dmaWord.verticalInterpolation;
	break;

	case horizontalInterpolation:   /* [    2] enable horizontal interpolation */
	    return dmaWord.horizontalInterpolation;
	break;

	case colorsOnly:	    /* [    1] reserved; set to zero */
	    return dmaWord.colorsOnly;
	break;

	case noVerticalInterpLine:  /* [    0] suppress vertical interpolation for this line only */
	    return dmaWord.noVerticalInterpLine;
	break;
    }

    return 0;
}



/*
 * Cycle the value of the VDL display control word 'focus' bit field up or down.
 */
VDLDisplayCtlWord SetDisplayCtlValue(VDLDisplayCtlWord dmaWord, short newValue, short fieldNum)
{
/*  int32   tempLong = LONGWORD(dmaWord); */

    switch ( fieldNum )
    {
	case nullAMYVideoControl:   /* */
	    dmaWord.nullAMYVideoControl = LimitNum(newValue, 0, 1);
	break;

	case videoIsPAL:	    /* [   27] PAL video */
	    dmaWord.videoIsPAL = LimitNum(newValue, 0, 1);
	break;

	case select640Mode:	 /* [   26] select 640 mode */
	    dmaWord.select640Mode = LimitNum(newValue, 0, 1);
	break;

	case bypassCLUT:	    /* [   25] bypass CLUT if pen MSB set */
	    dmaWord.bypassCLUT = LimitNum(newValue, 0, 1);
	break;

	case slipStreamOverlay:     /* [   24] source of overlay is SlipStream */
	    dmaWord.slipStreamOverlay = LimitNum(newValue, 0, 1);
	break;

	case forceTransparency:     /* [   23] */
	    dmaWord.forceTransparency = LimitNum(newValue, 0, 1);
	break;

	case backgroundDetector:    /* [   22] enable background detector */
	    dmaWord.backgroundDetector = LimitNum(newValue, 0, 1);
	break;

	case swapWindowPenHV:       /* [   21] swap H and V bits in pen numbers in window */
	    dmaWord.swapWindowPenHV = LimitNum(newValue, 0, 1);
	break;

	case windowVSource:	 /* [20-19] selects source for V sub-positions in window pixels */
	    dmaWord.windowVSource = LimitNum(newValue, 0, 3);
	break;

	case windowHSource:	 /* [18-17] selects source for H sub-position in window pixels */
	    dmaWord.windowHSource = LimitNum(newValue, 0, 3);
	break;

	case windowBlueLSBSource:   /* [16-15] selects source for window blue pen number's LSB */
	    dmaWord.windowBlueLSBSource = LimitNum(newValue, 0, 3);
	break;

	case windowVerticalInterp:  /* [   14] enable vertical interpolation in window */
	    dmaWord.windowVerticalInterp = LimitNum(newValue, 0, 1);
	break;

	case windowHorizontalInterp:/* [   13] enable horizontal interpolation in window */
	    dmaWord.windowHorizontalInterp = LimitNum(newValue, 0, 1);
	break;

	case windowMSBReplicate:    /* [   11] */
	    dmaWord.windowMSBReplicate = LimitNum(newValue, 0, 1);
	break;

	case bypassLSBIsRandom:     /* [   12] enable random number for low-bits of CLUT bypass */
	    dmaWord.bypassLSBIsRandom = LimitNum(newValue, 0, 1);
	break;

	case swapPenVH:	     /* [   10] swap H and V bits in pen number */
	    dmaWord.swapPenVH = LimitNum(newValue, 0, 1);
	break;

	case VSource:	       /* [ 9- 8] selects source of vertical sub-position */
	    dmaWord.VSource = LimitNum(newValue, 0, 3);
	break;

	case HSource:	       /* [ 7- 6] selects source of horizontal sub-position */
	    dmaWord.HSource = LimitNum(newValue, 0, 3);
	break;

	case blueLSBSource:	 /* [ 5- 4] selects source of blue pen number's LSB */
	    dmaWord.blueLSBSource = LimitNum(newValue, 0, 3);
	break;

	case verticalInterpolation: /* [    3] enable vertical interpolation */
	    dmaWord.verticalInterpolation = LimitNum(newValue, 0, 1);
	break;

	case horizontalInterpolation:   /* [    2] enable horizontal interpolation */
	    dmaWord.horizontalInterpolation = LimitNum(newValue, 0, 1);
	break;

	case colorsOnly:	    /* [    1] reserved; set to zero */
	    dmaWord.colorsOnly = LimitNum(newValue, 0, 1);
	break;

	case noVerticalInterpLine:  /* [    0] suppress vertical interpolation for this line only */
	    dmaWord.noVerticalInterpLine = LimitNum(newValue, 0, 1);
	break;
    }

    return dmaWord;
}

void printVDLEntry( VDLEntry *entry )
	{
	PRT( ("DMAControlWord  %x \n", *entry++) );
	PRT( ("DMAWord2        %x \n", *entry++) );
	PRT( ("DMAWord3        %x \n", *entry++) );
	PRT( ("DMADisplayWord  %x \n", *entry) );
	}

void printVDL( VDL *vdl )
	{
	PRT( ("VDLEntry     %x \n", vdl->vdl_DataPtr) );
	PRT( ("vdl_Type     %x \n", vdl->vdl_Type) );
	PRT( ("vdl_DataSize %x \n", vdl->vdl_DataSize) );
	PRT( ("vdl_Height %x \n", vdl->vdl_Height) );
	PRT( ("vdl_Flags %x \n", vdl->vdl_Flags) );
	PRT( ("vdl_Offset %x \n", vdl->vdl_Offset) );
	printVDLEntry( vdl->vdl_DataPtr );
	}





