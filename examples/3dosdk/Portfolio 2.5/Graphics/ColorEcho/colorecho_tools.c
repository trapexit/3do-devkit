
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: colorecho_tools.c,v 1.10 1994/11/01 04:28:26 vertex Exp $
**
******************************************************************************/

#include "types.h"
#include "graphics.h"
#include "hardware.h"
#include "operamath.h"
#include "mem.h"
#include "stdio.h"
#include "displayutils.h"
#include "colorecho.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* *************************************************************************
 * ***                     *************************************************
 * ***  Constants          *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */

uint32 ScreenSelect = 0;
int32 coDebug = 0;

/* Initialize the Cel Control Block */
struct CCB EchoCCB =
{
/* uint32 ccb_Flags; */
	CCB_LAST  | /* last cel in list */
	CCB_NPABS | /* Use absolute address for NextPtr */
	CCB_SPABS | /* Use absolute address for SourceData */
	CCB_PPABS | /* Use absolute address for PlutPtr */
	CCB_LDSIZE| /* Use HDX,HDY,VDX,VDY from this CCB */
	CCB_CCBPRE| /* Use preamble from CCB, not data. */
	PMODE_ONE |
	CCB_LDPRS | /* Use HDDX,HDDY Perspective data */
	CCB_LDPPMP| /* Use Pixel Processor control word from CCB */
	CCB_YOXY  | /* Write origin coordinates */
	CCB_ACW   | /* Enable clockwise rendering. */
	CCB_ACCW  | /* Enable counterclockwise rendering. */
	CCB_LCE   | /* Lock corner engine. */
/*	CCB_BGND  | */ /* Pass 000 to pixel processor, not transparent. */
	0,          /* So I can comment out bits. */

/* struct CCB *ccb_NextPtr;Cel *ccb_SourcePtr;void *ccb_PLUTPtr; */
	NULL, NULL, NULL,

/* Coord ccb_X, ccb_Y; */
	0,0,
/* int32 ccb_hdx, ccb_hdy, ccb_vdx, ccb_vdy, ccb_ddx, ccb_ddy; */
	0,0, 0,0, 0,0,

/* uint32 ccb_PIXC; */
	PPMP_BOTH_AVERAGE,
/*
** Cel first preamble word bits.
** Because the cel is LRFORM, the height needs to be divided by
** 2 in the preamble word.
*/
/*	PRE0_BGND  |  */ /* no transparency */
	(((CEL_HEIGHT/2) - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT)| /* height */
	PRE0_LINEAR|    /* uncoded data */
	PRE0_BPP_16,    /* 16 bit pixels */

/* Cel second preamble word bits */
	((DISPLAY_WIDTH - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT) |
	PRE1_LRFORM |     /* LR form cel */
	PRE1_TLLSB_PDC0 | /* Pass LSB of Blue through pixel processor. */
	((CEL_WIDTH-PRE1_TLHPCNT_PREFETCH)<<PRE1_TLHPCNT_SHIFT), /* width */

/* int32 ccb_Width, ccb_Height; */
	CEL_WIDTH, CEL_HEIGHT
	};


/************************************************************************/
int32 ce_Init ( ColorEcho *ce )
{
	int32 Result;
	Result = 0;

/* Initialize ColorEcho control srtucture. */
	ce->ce_IfSport = TRUE;
	ce->ce_Zoom = ZOOMONE/2;
	ce->ce_XOffset=0;
	ce->ce_YOffset=0;
	ce->ce_Angle=ANGLEDELTA * 20;
	ce->ce_Flags = 0;
	ce->ce_PIXC = PPMP_BOTH_AVERAGE;
	ce->ce_ZoomVelocity = 0;
	ce->ce_AngleVelocity = 0;
	ce->ce_XVelocity = 0;
	ce->ce_YVelocity = 0;

/* Calculate length of Half Diagonal of screen. */
	ce->ce_HalfDiagonal = Sqrt32((DISPLAY_WIDTH>>1)*(DISPLAY_WIDTH>>1) +
			(DISPLAY_HEIGHT>>1)*(DISPLAY_HEIGHT>>1));

/* Calculate angle of diagonal */
	ce->ce_Theta = Atan2F16( DISPLAY_WIDTH, DISPLAY_HEIGHT);

/* Seed initial picture. */
	ce->ce_Zoom = ZOOMONE - 1;
	ce->ce_Angle = ANGLEDELTA;

/*
** Copy CCB to CEL type memory in case program is loaded into non-DMAable memory.
** Currently all memory is accessable by Cel DMA but future machines may extend
** beyond the 26 bit Cel DMA address limit and could break this code.
*/
	ce->ce_CCB = (CCB *)AllocMem( sizeof(CCB), MEMTYPE_CEL );  /* 930721 */
	if( ce->ce_CCB == NULL )
	{
		ERR(("ce_Init: Could not allocate CCB.\n"));
		return -1;
	}
	else
	{
/* Copy contents of static structure to allocated structure. */
		*(ce->ce_CCB) = EchoCCB;
	}

	return( Result );
}

/************************************************************************/
uint32 Random( uint32 n )
/* Return a random number from 0 to n-1
 * The return value has 16 bits of significance, and ought to be unsigned.
 * Is the above true?
 */
{
	uint32 rand16;

	rand16 = rand() & 0x0000FFFF;

	return ( (rand16*n) >> 16 );
}

/************************************************************************/
#define SORT(v1,v2) {int32 temp; if(v1>v2) { temp=v2; v2=v1; v1=temp; }}

/************************************************************************/
int32 RandomBoxes ( Item BitMap, int32 NumBoxes )
{
	int32 x1,y1,x2,y2;
	int32 i;
	int32 Result;
	Rect WorkRect;
	GrafCon GCon;

	Result = 0;

	for (i=0; i<NumBoxes; i++)
	{
/* Generate random 16 bit color. */
		SetFGPen( &GCon, rand()&0x0FFFF );

		x1 = Random(DISPLAY_WIDTH);
		x2 = Random(DISPLAY_WIDTH);
		y1 = Random(DISPLAY_HEIGHT);
		y2 = Random(DISPLAY_HEIGHT);
		SORT(x1,x2);
		SORT(y1,y2);
		WorkRect.rect_XLeft = x1;
		WorkRect.rect_XRight = x2;
		WorkRect.rect_YTop = y1;
		WorkRect.rect_YBottom = y2;

		Result = FillRect( BitMap, &GCon, &WorkRect );
	}
	return Result;
}

/************************************************************************/
int32 RandomPixels ( Item BitMap, int32 XCenter, int32 YCenter, int32 NumPixels )
{
	int32 x,y;
	int32 i;
	int32 Result = 0;
	GrafCon GCon;


	for (i=0; i<NumPixels; i++)
	{
		SetFGPen( &GCon, rand()&0x0FFFF );
		x = Random(11) + XCenter - 5;
		y = Random(11) + YCenter - 5;
		Result = WritePixel( BitMap, &GCon, x, y);
	}
	return Result;
}

/*********************************************************************/
void ce_Center( ColorEcho *ce )
{
	ce->ce_Zoom = ZOOMONE;
	ce->ce_Angle = 0;
	ce->ce_XOffset = 0;
	ce->ce_YOffset = 0;
}

/*********************************************************************/
void ce_Freeze( ColorEcho *ce )
{
	ce->ce_ZoomVelocity = 0;
	ce->ce_AngleVelocity = 0;
	ce->ce_XVelocity = 0;
	ce->ce_YVelocity = 0;
}

/*********************************************************************/
int32 ce_SeedPattern( ScreenContext *sc, ColorEcho *ce )
{
	int32 Result;
	Item BitMap;
	GrafCon GCon;
	int32 i,x,y;

/* Initialize random number generator for identical pattern. */
	srand(ce->ce_PatternSeed);

	BitMap = sc->sc_BitmapItems[ 1 - sc->sc_curScreen ];
    MoveTo ( &GCon, MIDDLEX, MIDDLEY );
	for( i=0; i<10; i++)
	{
		SetFGPen( &GCon, rand()&0x0FFFF );
		x = Random(DISPLAY_WIDTH);
		y = Random(DISPLAY_HEIGHT);
		Result = DrawTo( BitMap, &GCon, x, y);
		if( Result < 0 ) return Result;
	}
/* Draw through center to better seed zoomed in fractal. */
	SetFGPen( &GCon, rand()&0x0FFFF );
	Result = DrawTo( BitMap, &GCon, MIDDLEX, MIDDLEY );
	if( Result < 0 ) return Result;

/* Restore randomness. */
	srand(ReadHardwareRandomNumber());

	return Result;
}
/**********************************************************************
** Sprinkle some color into the picture to seed the feedback process.
*************************************************************************/
int32 ce_Seed( ScreenContext *sc, ColorEcho *ce )
{
	int32 Result;
	Item BitMap;
	Item t;

	if (ce->ce_Zoom <= ZOOMONE)
	{
/*
** Draw boxes into target screen so that subsequent graphics draw over the
** rectangles. This will sprinkle color around the edges.
*/
		BitMap = sc->sc_BitmapItems[ sc->sc_curScreen ];
		ce->ce_IfSport = FALSE;   /* That would wipe out the rectangles. */
		t = CreateVBLIOReq();
		WaitVBL(t, 1);
		DeleteVBLIOReq(t);
		Result = RandomBoxes( BitMap, 20 );
	}
	else
	{
/*
** Draw dots into displayed screen which will be the source for the next DrawCel.
*/
		BitMap = sc->sc_BitmapItems[ 1 - sc->sc_curScreen ];
		Result = RandomPixels( BitMap, ce->ce_MiddleX, ce->ce_MiddleY, 50);
	}
	return Result;
}

/*************************************************************************
** Do Sport transfer and a DrawCel
*************************************************************************/
int32 ce_DrawNextScreen( ScreenContext *sc, ColorEcho *ce )
{
	Point Pts[4];
	frac16 Phi;
	int32 Result=0;
	Item t;

/* Apply velocities to current values. */
	ce->ce_XOffset += ce->ce_XVelocity;
	if( ce->ce_XOffset > MAXOFFSET) ce->ce_XOffset = MAXOFFSET;
	if( ce->ce_XOffset < -MAXOFFSET) ce->ce_XOffset = -MAXOFFSET;
	ce->ce_YOffset += ce->ce_YVelocity;
	if( ce->ce_YOffset > MAXOFFSET) ce->ce_YOffset = MAXOFFSET;
	if( ce->ce_YOffset < -MAXOFFSET) ce->ce_YOffset = -MAXOFFSET;
	ce->ce_MiddleX = MIDDLEX + ce->ce_XOffset;
	ce->ce_MiddleY = MIDDLEY + ce->ce_YOffset;

	ce->ce_Zoom += ce->ce_ZoomVelocity;
	if( ce->ce_Zoom > MAXZOOM) ce->ce_Zoom = MAXZOOM;
	if( ce->ce_Zoom < MINZOOM) ce->ce_Zoom = MINZOOM;

	ce->ce_Angle += ce->ce_AngleVelocity;

/* Calculate cel corner points based on Zoom and Angle */
	ce->ce_Radius = (ce->ce_HalfDiagonal * ce->ce_Zoom) >> ZOOMSHIFT;
	Phi = ce->ce_Angle - ce->ce_Theta + HALFCIRCLE;
	Pts[3].pt_X = MulSF16( ce->ce_Radius, CosF16(Phi) ) + ce->ce_MiddleX;
	Pts[3].pt_Y = MulSF16( ce->ce_Radius, SinF16(Phi) ) + ce->ce_MiddleY;
	Phi = ce->ce_Angle + ce->ce_Theta;
	Pts[2].pt_X = MulSF16( ce->ce_Radius, CosF16(Phi) ) + ce->ce_MiddleX;
	Pts[2].pt_Y = MulSF16( ce->ce_Radius, SinF16(Phi) ) + ce->ce_MiddleY;
	Phi = ce->ce_Angle - ce->ce_Theta;
	Pts[1].pt_X = MulSF16( ce->ce_Radius, CosF16(Phi) ) + ce->ce_MiddleX;
	Pts[1].pt_Y = MulSF16( ce->ce_Radius, SinF16(Phi) ) + ce->ce_MiddleY;
	Phi = ce->ce_Angle + ce->ce_Theta + HALFCIRCLE;
	Pts[0].pt_X = MulSF16( ce->ce_Radius, CosF16(Phi) ) + ce->ce_MiddleX;
	Pts[0].pt_Y = MulSF16( ce->ce_Radius, SinF16(Phi) ) + ce->ce_MiddleY;

/* Set CCB to map cel to 4 corners. */
	MapCel( ce->ce_CCB, Pts );

/* Do Sport transfer of displayed image to target image. */
	if (ce->ce_IfSport)
	{
		t = CreateVRAMIOReq();
		CopyVRAMPages( t,
			sc->sc_Bitmaps[sc->sc_curScreen]->bm_Buffer,   /* dest */
			sc->sc_Bitmaps[(1-sc->sc_curScreen)]->bm_Buffer, /* source */
			sc->sc_nFrameBufferPages, ~0 );
		DeleteVRAMIOReq(t);

		ce->ce_CCB->ccb_PIXC = ce->ce_PIXC;
	}
	else
	{
		ce->ce_CCB->ccb_PIXC = PPMP_BOTH_NORMAL;
	}
	ce->ce_IfSport = TRUE;

/* Point CCB to bitmap. */
	ce->ce_CCB->ccb_SourcePtr = (CelData *) GetPixelAddress(sc->sc_Screens[ (1-sc->sc_curScreen) ], 0, 0);

/* Draw zoomed and rotated image over sported image. */
	Result = DrawCels( sc->sc_BitmapItems[ sc->sc_curScreen ], ce->ce_CCB );
	if (Result < 0)
	{
		PrintError(0,"draw cels",0,Result);
		return Result;
	}
	return Result;
}

