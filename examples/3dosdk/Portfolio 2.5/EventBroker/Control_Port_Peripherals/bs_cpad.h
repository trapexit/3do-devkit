#ifndef __BS_CPAD_H
#define __BS_CPAD_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bs_cpad.h,v 1.2 1994/11/22 00:56:02 vertex Exp $
**
******************************************************************************/

typedef struct {
	/* current undebounced button presses */
	uint32 raw;

	/* undebounced button presses from last time */
	uint32 lastraw;

	/* current debounced button presses */
	uint32 filtered;

	/* x and y values */
	/* for axised device emulation */
	short x,y;

	/* last values for x and y */
	short lastx,lasty;

	/* x and y steps to take */
	/* for succesive pad clicks */
	short dx,dy;

	/* wrap at edge if 1 or bound if 0 */
	int wrapf;

	/* boundaries to keep x and y values in */
	short minx,maxx,miny,maxy;

	/* debounced button presses from last time */
	uint32 lastfilt;

	/* cpad daisy chain info */
	/* daisyNum is absolute postion on daisy chain */
	/* (count starts at 1) */
	/* padNum is position with respect to other pads */
	/* (count also starts at 1) */
	short daisyNum,padNum;
} BS_CPadData;

/* access to the configuration settings */
#define BS_GetCPadDX(p) ((p)->dx)
#define BS_GetCPadDY(p) ((p)->dy)
#define BS_GetCPadMinX(p) ((p)->minx)
#define BS_GetCPadMaxX(p) ((p)->maxx)
#define BS_GetCPadMinY(p) ((p)->miny)
#define BS_GetCPadMaxY(p) ((p)->maxy)
#define BS_GetCPadWrapF(p) ((p)->wrapf)

/* set the configuration settings */
#define BS_SetCPadDX(p,v) (((p)->dx)=(v))
#define BS_SetCPadDY(p,v) (((p)->dy)=(v))
#define BS_SetCPadMinX(p,v) (((p)->minx)=(v))
#define BS_SetCPadMaxX(p,v) (((p)->maxx)=(v))
#define BS_SetCPadMinY(p,v) (((p)->miny)=(v))
#define BS_SetCPadMaxY(p,v) (((p)->maxy)=(v))
#define BS_SetCPadWrapF(p,v) (((p)->wrapf)=(v))

/* get the raw and debounced button presses */
/* and x and y values for axised device emulation */
#define BS_GetCPadRawButtons(cp) ((cp)->raw)
#define BS_GetCPadLastRawButtons(cp) ((cp)->lastraw)
#define BS_GetCPadButtons(cp) ((cp)->filtered)
#define BS_GetCPadLastButtons(cp) ((cp)->lastfilt)
#define BS_GetCPadX(cp) ((cp)->x)
#define BS_GetCPadLastX(cp) ((cp)->lastx)
#define BS_GetCPadY(cp) ((cp)->y)
#define BS_GetCPadLastY(cp) ((cp)->lasty)

/* set the raw and debounced button presses */
#define BS_SetCPadButtons(cp,v) (((cp)->filtered)=(v))
#define BS_SetCPadRawButtons(cp,v) (((cp)->raw)=(v))
#define BS_SetCPadLastButtons(cp,v) (((cp)->lastfilt)=(v))
#define BS_SetCPadLastRawButtons(cp,v) (((cp)->lastraw)=(v))
#define BS_SetCPadX(cp,v) (((cp)->x)=(v))
#define BS_SetCPadY(cp,v) (((cp)->y)=(v))
#define BS_SetCPadLastX(cp,v) (((cp)->lastx)=(v))
#define BS_SetCPadLastY(cp,v) (((cp)->lasty)=(v))

/* get the daisy chain info */
#define BS_GetCPadDaisyNum(cp) ((cp)->daisyNum)
#define BS_GetCPadNum(cp) ((cp)->padNum)

/* set the daisychain info */
#define BS_SetCPadDaisyNum(cp,v) (((cp)->daisyNum)=(v))
#define BS_SetCPadNum(cp,v) (((cp)->padNum)=(v))

/* clear out the data */
/* structure and set the initial values for */
/* thresholds and boundaries */
#define BS_InitCPadData(a) memset(a,0,sizeof(BS_CPadData));\
	BS_SetCPadMinX(a,-1);BS_SetCPadMaxX(a,BS_XRES);\
	BS_SetCPadMinY(a,-1);BS_SetCPadMaxY(a,BS_YRES);\
	BS_SetCPadDX(a,1);BS_SetCPadDY(a,1);BS_SetCPadWrapF(a,0)

/* debouncer */
/* could easily be set up to be a cpad->joystick emulator */
/* by keeping drack of dx,dy and generating x's and y's */
/* debounce the buttons reflected in deBounceBits */
/* if axisEmulF is true, it will keep track of a current x/y */
/* position in emulation of an axised device like a mouse or stick */
extern int BS_ProcessCPadData(BS_CPadData *,uint32 deBounceBits,int axisEmulF);

#endif /* __BS_CPAD_H */
