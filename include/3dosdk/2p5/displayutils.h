#ifndef __DISPLAYUTILS_H
#define __DISPLAYUTILS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: displayutils.h,v 1.6 1994/11/30 03:38:46 vertex Exp $
**
**  Lib3DO display, screen, and bitmap utilities.
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __GRAPHICS_H
#include "graphics.h"
#endif

#ifndef __FORM3DO_H
#include "form3do.h"
#endif


/*****************************************************************************/


/* anybody ever hear of hexa-buffering? */
#define MAXSCREENS 6

/* briefly describes a display */
typedef struct ScreenContext
{
    Item        sc_ScreenGroup;                /* associated screen group    */
    uint32      sc_DisplayType;                /* DI_TYPE_* from graphics.h  */

    uint32      sc_NumScreens;                 /* # of screens created       */
    uint32      sc_CurrentScreen;              /* displayed screen           */
    Item        sc_ScreenItems[MAXSCREENS];    /* item for the screen        */
    Item        sc_BitmapItems[MAXSCREENS];    /* bitmap item for the screen */
    Bitmap     *sc_Bitmaps[MAXSCREENS];        /* structure itself           */

    uint32      sc_NumBitmapPages;      /* # pages of memory for each bitmap */
    uint32      sc_NumBitmapBytes;      /* # bytes of memory for each bitmap */
    uint32      sc_BitmapBank;          /* bank of memory for all bitmaps    */
    uint32      sc_BitmapWidth;         /* pixel width of each bitmap        */
    uint32      sc_BitmapHeight;        /* pixel height of each bitmap       */
} ScreenContext;


/* remap old names to new ones */
#define sc_nScreens          sc_NumScreens
#define sc_curScreen         sc_CurrentScreen
#define sc_Screens           sc_ScreenItems
#define sc_nFrameBufferPages sc_NumBitmapPages
#define sc_nFrameByteCount   sc_NumBitmapBytes


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


Item CreateBasicDisplay(ScreenContext *sc, uint32 displayType, uint32 numScreens);
Err DeleteBasicDisplay(ScreenContext *sc);

void *	LoadImage( char *filename, ubyte* dest, VdlChunk **rawVDLPtr, ScreenContext *sc );
void 	UnloadImage(void *imagebuf);
Boolean DrawImage(Item iScreen, ubyte* pbImage, ScreenContext *sc);

void FadeToBlack(ScreenContext *sc, int32 nFrames);
void FadeFromBlack(ScreenContext *sc, int32 frameCount);

Err ClearBitmap(Item ioreq, Item screen_or_bitmap, Bitmap *bm, int32 value);


#ifdef __cplusplus
}
#endif


/* for compatibility only, do not use in new code */
#define OpenGraphics(sc,n) (CreateBasicDisplay(sc,DI_TYPE_DEFAULT,n) >= 0 ? TRUE : FALSE)
#define CloseGraphics(sc)  DeleteBasicDisplay(sc)


/*****************************************************************************/


#endif /* __DISPLAYUTILS_H */
