/*
	File:		DrawImage.c

	Contains:	xxx put contents here xxx

	Written by:	Jay Moreland

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 7/28/93	JAY		modify CopyVRAMPages for Dragon Tail release

	To Do:
*/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

bool DrawImage(Item iScreen, ubyte* pbImage, ScreenContext *sc )
	{
	Screen* screen;
	Item	VRAMIOReq;
	
	screen = (Screen*)LookupItem(iScreen);

	if (screen == NULL)
		{
		DIAGNOSTIC("Cannot locate screens");
		return FALSE;
		}
	else
		{
		VRAMIOReq = GetVRAMIOReq();
		
		CopyVRAMPages( VRAMIOReq, screen->scr_TempBitmap->bm_Buffer, pbImage, sc->sc_nFrameBufferPages, -1 );
		
		DeleteItem( VRAMIOReq );
		
		return TRUE;
		}
	}
