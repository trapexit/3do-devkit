/*****************************************************************************
 *	File:			OpenGraphics.c
 *
 *	Contains:		Routine to open the graphics folio and create screens.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  05/12/94  ian	Removed reference to GrafBase->gf_ZeroPage.  
 *					Did a general cleanup of OpenGraphics().
 *					Added CloseGraphics().
 *	<unknown>		New.
 *
 *	Implementation notes:
 *
 *	Ideally, we would have the OS allocate the bitmaps by setting a zero value
 *	for CSG_TAG_SPORTBITS and letting the OS make sure we get page-aligned
 *	VRAM all in the same bank.  That's what the docs say would happen, but
 *	it isn't properly implemented in the OS right now.  So instead, we set
 *	the SPORTBITS to get us pages from bank 1, always.  This is, in effect,
 *	what the old code used to do with GETBANKBITS(GrafBase->gf_ZeroPage), so
 *	we haven't really changed the functionality at all.  If graphics folio
 *	gets changed to implement a zero-valued SPORTBITS tag the way it's 
 *	documented, just change kVRAMBANKBITS to 0.
 ****************************************************************************/

#include "Init3DO.h"
#include "Debug3DO.h"
#include "mem.h"
#include "string.h"

#define kVRAMBANKBITS (MEMTYPE_VRAM|MEMTYPE_STARTPAGE|MEMTYPE_BANKSELECT|MEMTYPE_BANK1)

/*****************************************************************************
 * CloseGraphics()
 *	Release resources acquired during OpenGraphics().  This does NOT close
 *	the graphics folio, but it releases all the memory and item resources
 *	allocated by OpenGraphics().  Remember that your task still owns the
 *	memory that was occupied by the bitmaps until you call ScavengeMem() to
 *	turn them back over to the system-wide free pool!
 ****************************************************************************/

void CloseGraphics(ScreenContext * sc)
{
	Item	screenGroup;
	Screen *screen;

	if (sc->sc_Screens[0] > 0) {	
		if ((screen = (Screen *)LookupItem(sc->sc_Screens[0])) != NULL) {
			screenGroup = screen->scr_ScreenGroupPtr->sg.n_Item;
			RemoveScreenGroup(screenGroup);
			DeleteScreenGroup(screenGroup);
		}
	}	
}

/*****************************************************************************
 * OpenGraphics()
 *	Open the graphics folio (if that hasn't been done already, it doesn't
 *	hurt if it's already open) and set up bitmaps, screens, and a screen group.
 ****************************************************************************/

bool OpenGraphics(ScreenContext	* sc, int nScreens)
{
	Err		err;
	long 	width;
	long	height;
	Screen *screen;
	int32	i;
	Item 	item;
	TagArg	taScreenTags[] = {
				CSG_TAG_SPORTBITS,		(void *)0,
				CSG_TAG_SCREENCOUNT,	(void *)0,
				CSG_TAG_DONE, 			(void *)0
	};

	memset(sc, 0, sizeof(*sc));
	
	taScreenTags[0].ta_Arg = (void *)kVRAMBANKBITS;
	taScreenTags[1].ta_Arg = (void *)nScreens;
	sc->sc_nScreens = nScreens;

	if ((err = OpenGraphicsFolio()) < 0) {
		DIAGNOSE_SYSERR(err, ("Cannot open graphics folio\n"));
		return FALSE;
	}

	if ((item = CreateScreenGroup( &(sc->sc_Screens[0]), taScreenTags )) < 0) {
		DIAGNOSE_SYSERR(item, ("Cannot create screen group\n"));
		return FALSE;
	}

	if ((err = AddScreenGroup( item, NULL )) < 0) {
		DIAGNOSE_SYSERR(err, ("Cannot add screen group\n"));
		CloseGraphics(sc);
		return FALSE;
	}

	for ( i = 0; i < sc->sc_nScreens; i++ ) {
		if ((screen = (Screen *)LookupItem( sc->sc_Screens[i])) == NULL) {
		    DIAGNOSE(("Cannot locate screens\n"));
			CloseGraphics(sc);
		    return FALSE;
		}
		sc->sc_BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		sc->sc_Bitmaps[i] = screen->scr_TempBitmap;
		EnableHAVG(sc->sc_Screens[i]);
		EnableVAVG(sc->sc_Screens[i]);
	}

	width  = sc->sc_Bitmaps[0]->bm_Width;
	height = sc->sc_Bitmaps[0]->bm_Height;

	sc->sc_nFrameBufferPages = (width*2*height+GrafBase->gf_VRAMPageSize-1)/GrafBase->gf_VRAMPageSize;
	sc->sc_nFrameByteCount	 = sc->sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize;

	return TRUE;	
}

