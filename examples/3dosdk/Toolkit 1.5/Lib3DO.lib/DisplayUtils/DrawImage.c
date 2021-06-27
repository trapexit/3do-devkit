/*****************************************************************************
 *	File:			DrawImage.c 
 *
 *	Contains:		Routine to copy an image from its VRAM buffer to the screen.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *	07/28/93  JAY	modify CopyVRAMPages for Dragon Tail release
 *
 *	Implementation notes:
 *
 ****************************************************************************/

#include "DisplayUtils.h"
#include "Debug3DO.h"

Boolean DrawImage(Item iScreen, ubyte* pbImage, ScreenContext *sc )
{
	Screen* screen;
	Item	VRAMIOReq;
	
	screen = (Screen*)LookupItem(iScreen);

	if (screen == NULL)	{
		DIAGNOSE(("Cannot locate screens"));
		return FALSE;
	} else {
		VRAMIOReq = GetVRAMIOReq();	
		CopyVRAMPages( VRAMIOReq, screen->scr_TempBitmap->bm_Buffer, pbImage, sc->sc_nFrameBufferPages, -1 );
		DeleteItem( VRAMIOReq );
		return TRUE;
	}
}
