#ifndef __BS_MOUSE_H
#define __BS_MOUSE_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: bs_mouse.h,v 1.2 1994/11/22 00:56:02 vertex Exp $
**
******************************************************************************/

typedef struct {
	/* current raw mouse data */
	MouseEventData raw;

	/* raw mouse data from last time */
	MouseEventData lastraw;

	/* current filtered mouse data */
	MouseEventData filtered;

	/* filtered mouse data from last time */
	MouseEventData lastfilt;

	/* boundaries to keep x and y values for mouse in */
	short minx,maxx,miny,maxy;

	/* fine movement thresholds */
	short xthresh,ythresh;

	/* wrap at edges if 1, bound if 0 */
	int wrapf;

	/* mouse daisy position and device number info */
	short daisyNum,mouseNum;
} BS_MouseData;

/* access to the MouseDataStructure pointers */
#define BS_GetMouse(m) (&((m)->filtered))
#define BS_GetMouseRaw(m) (&((m)->raw))
#define BS_GetMouseLast(m) (&((m)->lastfilt))
#define BS_GetMouseLastRaw(m) (&((m)->lastraw))

/* access to the configuration values */
#define BS_GetMouseMinX(m) ((m)->minx)
#define BS_GetMouseMaxX(m) ((m)->maxx)
#define BS_GetMouseMinY(m) ((m)->miny)
#define BS_GetMouseMaxY(m) ((m)->maxy)
#define BS_GetMouseXThresh(m) ((m)->xthresh)
#define BS_GetMouseYThresh(m) ((m)->ythresh)
#define BS_GetMouseWrapF(m) ((m)->wrapf)

/* set the configuration values */
#define BS_SetMouseMinX(m,v) (((m)->minx)=(v))
#define BS_SetMouseMaxX(m,v) (((m)->maxx)=(v))
#define BS_SetMouseMinY(m,v) (((m)->miny)=(v))
#define BS_SetMouseMaxY(m,v) (((m)->maxy)=(v))
#define BS_SetMouseXThresh(m,v) (((m)->xthresh)=(v))
#define BS_SetMouseYThresh(m,v) (((m)->ythresh)=(v))
#define BS_SetMouseWrapF(m,v) (((m)->wrapf)=(v))

/* get the current, filtered values */
#define BS_GetMouseX(m) (((m)->filtered).med_HorizPosition)
#define BS_GetMouseY(m) (((m)->filtered).med_VertPosition)
#define BS_GetMouseButtons(m) (((m)->filtered).med_ButtonBits)

/* set the current, filtered values */
#define BS_SetMouseX(m,v) ((((m)->filtered).med_HorizPosition)=(v))
#define BS_SetMouseY(m,v) ((((m)->filtered).med_VertPosition)=(v))
#define BS_SetMouseButtons(m,v) ((((m)->filtered).med_ButtonBits)=(v))

/* get the current, raw values */
#define BS_GetMouseRawX(m) (((m)->raw).med_HorizPosition)
#define BS_GetMouseRawY(m) (((m)->raw).med_VertPosition)
#define BS_GetMouseRawButtons(m) (((m)->raw).med_ButtonBits)

/* set the current, raw values (not really needed, but here for the sake of form) */
#define BS_SetMouseRawX(m,v) ((((m)->raw).med_HorizPosition)=(v))
#define BS_SetMouseRawY(m,v) ((((m)->raw).med_VertPosition)=(v))
#define BS_SetMouseRawButtons(m,v) ((((m)->raw).med_ButtonBits)=(v))

/* access to filtered data history */
#define BS_GetMouseLastX(m) (((m)->lastfilt).med_HorizPosition)
#define BS_GetMouseLastY(m) (((m)->lastfilt).med_VertPosition)
#define BS_GetMouseLastButtons(m) (((m)->lastfilt).med_ButtonBits)

/* set the filtered history info */
#define BS_SetMouseLastX(m,v) ((((m)->lastfilt).med_HorizPosition)=(v))
#define BS_SetMouseLastY(m,v) ((((m)->lastfilt).med_VertPosition)=(v))
#define BS_SetMouseLastButtons(m,v) ((((m)->lastfilt).med_ButtonBits)=(v))

/* access to the raw data history */
#define BS_GetMouseLastRawX(m) (((m)->lastraw).med_HorizPosition)
#define BS_GetMouseLastRawY(m) (((m)->lastraw).med_VertPosition)
#define BS_GetMouseLastRawButtons(m) (((m)->lastraw).med_ButtonBits)

/* set the raw data history */
#define BS_SetMouseLastRawX(m,v) ((((m)->lastraw).med_HorizPosition)=(v))
#define BS_SetMouseLastRawY(m,v) ((((m)->lastraw).med_VertPosition)=(v))
#define BS_SetMouseLastRawButtons(m,v) ((((m)->lastraw).med_ButtonBits)=(v))

/* get the mouse daisy/device num info */
#define BS_GetMouseDaisyNum(m) ((m)->daisyNum)
#define BS_GetMouseNum(m) ((m)->mouseNum)

/* set the mouse daisy/device num info */
#define BS_SetMouseDaisyNum(m,v) (((m)->daisyNum)=(v))
#define BS_SetMouseNum(m,v) (((m)->mouseNum)=(v))

/* transfer all current filtered data into the history area */
#define BS_SetMouseThisLast(m) \
	BS_SetMouseLastX(m,BS_GetMouseX(m)),\
	BS_SetMouseLastY(m,BS_GetMouseY(m)),\
	BS_SetMouseLastButtons(m,BS_GetMouseButtons(m))

/* transfer all the current raw data into the history area	 */
#define BS_SetMouseThisLastRaw(m) \
	BS_SetMouseLastRawX(m,BS_GetMouseRawX(m)),\
	BS_SetMouseLastRawY(m,BS_GetMouseRawY(m)),\
	BS_SetMouseLastRawButtons(m,BS_GetMouseRawButtons(m))

/* clear out the info struct */
/* and initialize the mins,maxes to frame buffer dimensions */
/* and set fine movment thresholds to 25% of total boundaries */
#define BS_InitMouseData(a) memset(a,0,sizeof(BS_MouseData));\
	BS_SetMouseMinX(a,-1);BS_SetMouseMaxX(a,BS_XRES);\
	BS_SetMouseMinY(a,-1);BS_SetMouseMaxY(a,BS_YRES);\
	BS_SetMouseXThresh(a,BS_XRES<<2);BS_SetMouseYThresh(a,BS_YRES<<2);\
	BS_SetMouseWrapF(a,0)

/* filter/debounce our mouse input */
extern int BS_ProcessMouseData(BS_MouseData *,uint32 deBounceBits);

#endif /* __BS_MOUSE_H */

