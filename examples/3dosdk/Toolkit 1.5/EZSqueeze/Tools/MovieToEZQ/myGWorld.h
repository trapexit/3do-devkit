/* myGWorld.h
	Copyright 1994 The 3DO Company, all rights reserved
	new file 2/4/94
	WS Duvall
	*/

#ifndef __MYGWORLD___
#define __MYGWORLD___
#include <quickdraw.h>

typedef struct GWorldData
	{
	short initialized;
	unsigned short rowBytes;
	Ptr pixmapAddr;
	Rect clipRect;
	/* Saved State */
	long *nextPixel;
	long *lastPixel;
	} GWorldData;
	
typedef struct DisplayInfo
	{
	WindowPtr window;
	Rect r;
	GWorldData GWD;
	} DisplayInfo;
		
extern void initGWorldRow(Rect *r, short x, short y, GWorldData *data);
extern void storePixelandIncrement(GWorldData *data, RGBColor *pixel);
extern void getPixelandIncrement(GWorldData *data, RGBColor *pixel);

#endif