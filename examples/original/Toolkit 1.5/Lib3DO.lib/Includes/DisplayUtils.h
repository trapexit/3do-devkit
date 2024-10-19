/*****************************************************************************
 *	File:		DisplayUtils.h
 *
 *	Contains:   Display, screen, and bitmap utilities.
 *
 *	Copyright:	(c) 1994 by The 3DO Company.  All rights reserved.
 *
 *	Change History(most recent first):
 *	10/25/94  Edgar	Fixed C++ closing wrapper bug.
 *	07/25/94  Ian	New today.
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#pragma include_only_once

#ifndef DISPLAYUTILS_H
#define DISPLAYUTILS_H

#include "Form3DO.h"

/*----------------------------------------------------------------------------
 * Datatypes.
 *--------------------------------------------------------------------------*/

#define MAXSCREENS 6

typedef struct ScreenContext
{
  int32 sc_nScreens;
  int32 sc_curScreen;
  int32 sc_nFrameBufferPages;
  int32 sc_nFrameByteCount;
  Item sc_Screens[MAXSCREENS];
  Item sc_BitmapItems[MAXSCREENS];
  Bitmap *sc_Bitmaps[MAXSCREENS];
} ScreenContext;

/*----------------------------------------------------------------------------
 * Prototypes.
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

  Boolean OpenGraphics (ScreenContext *sc, int nScreens);
  void CloseGraphics (ScreenContext *sc);

  void *LoadImage (char *filename, ubyte *dest, VdlChunk **rawVDLPtr,
                   ScreenContext *sc);
  void UnloadImage (void *imagebuf);

  Boolean DrawImage (Item iScreen, ubyte *pbImage, ScreenContext *sc);

  void FadeToBlack (ScreenContext *sc, int32 nFrames);
  void FadeFromBlack (ScreenContext *sc, int32 frameCount);

  Err ClearBitmap (Item ioreq, Item screen_or_bitmap, Bitmap *bm, int32 value);

#ifdef __cplusplus
}
#endif

#endif
