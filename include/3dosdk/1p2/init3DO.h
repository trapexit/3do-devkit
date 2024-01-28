/*
	File:		Init3DO.h

	Contains:	This file contains routines to initialize all the major components of a 3DO
				application--graphics, sound, DMA and file I/O.

	Written by:	Neil Cormia

	Copyright:	© 1992 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	  4/5/93	JAY		adding function prototype for ChangeInitialDirectory()
		 <7>	 1/28/93	dsm		Creating a single global pointer to a ScreenContext Structure
									containing what used to all the globals created by InitGraphics.
									Replaced the old style int1616 fixed point with the new style
									frac16 fixed point math from OperaMath.h.
		 <6>	12/27/92	dsm		Adding constants for bit flags to be returned by StartUp so we
									can separately check for success of opening Graphics, Audio,
									SportIO, and MacLink.
		 <5>	12/10/92	JML		gBitmaps and gBitmapItems are now external variables.
		 <4>	12/10/92	JML		nFrameByteCount is now an external variable.
		 <3>	12/10/92	JML		nFrameBufferPages is now an external variable.
		 <1>	 12/9/92	JML		first checked in

	To Do:
*/
#pragma force_top_level
#pragma include_only_once

#ifndef __Init3DO_h
#define __Init3DO_h
#include "graphics.h"

/*==================================================*/
#define MAXSCREENS 6
/*====================================================
Typedef structures for ScreenContext:
----------------------------------------------------*/
typedef struct ScreenContext_TAG
	{
	int32 	sc_nScreens;
	int32 	sc_curScreen;
	int32 	sc_nFrameBufferPages;
	int32 	sc_nFrameByteCount;
	Item 	sc_Screens[MAXSCREENS];
	Item	sc_BitmapItems[MAXSCREENS];
	Bitmap	*sc_Bitmaps[MAXSCREENS];
	} ScreenContext;
	
/*====================================================
GLOBAL VARIABLES:
----------------------------------------------------*/
/* ================== Directories ================== */

extern int NHOMES;
extern char *Home[];


	
/*====================================================
FUNCTION DECLARATIONS:
----------------------------------------------------*/


#ifdef __cplusplus 
extern "C" {
#endif

extern Err  ChangeInitialDirectory(char *firstChoice, char *secondChoice, bool always);
extern bool OpenGraphics(ScreenContext *sc, int nScreens);
extern bool OpenMacLink( void );
extern bool OpenSPORT( void );
extern bool OpenAudio( void );
extern void ShutDown(void);

#ifdef __cplusplus
}
#endif



/*==================================================*/

#endif
