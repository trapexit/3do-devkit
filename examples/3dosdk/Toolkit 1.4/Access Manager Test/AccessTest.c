/*
	File:		AccessTest.c

	Contains:	xxx put contents here xxx

	Written by:	Jay London

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 8/10/93	JML		Update 8/10/93 730am.

	To Do:
*/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "operamath.h"
#include "audio.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "graphics.h"
#include "filefunctions.h"
#include "time.h"

#include "Form3DO.h"
#include "Debug3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

#include "JoyPad.h"
#include "AccessLib.h"



/* *************************************************************************
 * ***                   ***************************************************
 * ***  Our Definitions  ***************************************************
 * ***                   ***************************************************
 * *************************************************************************
 */

#define NEEDMAC 0

ScreenContext	sc;


void
CopyBlack( Bitmap *bitmap, Int32 nFrameBufferPages )
{
	static	Item	gVRAMIOItem = 0;
	
	// calls to SPORT need an item now...
	if ( gVRAMIOItem == 0 )
		gVRAMIOItem = GetVRAMIOReq();

	(void)SetVRAMPages (gVRAMIOItem, bitmap->bm_Buffer, 0, nFrameBufferPages, -1);
}

/* *************************************************************************
 * ***                 *****************************************************
 * ***  Main Routines  *****************************************************
 * ***                 *****************************************************
 * *************************************************************************
 */

int
main( int argc, char *argv[] )
{
	char		*progname, s[80] = " ", fName[256];
	char		buf[256] = "abcdefghijklmnopqrstuvwxyz";
	char		*readBuf;
	JoyPadState	lControlvals;
	Int32		status = 0, bufSize = 26, result;
	Boolean		firstButton;

	
	/*	Extract the program name, imagefile name, and celfile name from
	 *	the arguments passed into the program.
	 *	NOTE: the imagefile and celfile names are string copied into
	 *	globals. This assures that the names will start on a longword
	 *	boundry. Any strings that are passed to the O/S must start on
	 *	a longword boundry.
	 */
	progname = *argv++;
	strcat(s,progname);
	strcat(s,"\n");
 	printf(s);

	/*	Creates the CreateScreenGroup and inits a SPORT device to write
	 *	directly to the frame buffer and a Mac Link to read files from
	 *	the Macintosh.
	 */
	if (!OpenGraphics(&sc,2)) goto DONE;
	if (!OpenSPORT()) goto DONE;
	if (!OpenAudio()) goto DONE;
#if NEEDMAC
	if (!OpenMacLink()) goto DONE;
#endif
	if (OpenMathFolio() < 0) goto DONE;
		
	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
 
	if (GetJoyPad(&lControlvals,1)) goto DONE;		

	SetAccessColors((RGB555)MakeRGB15(0x1F,0,0),(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT);

	if (Answer(OK_DIALOG, sc.sc_Screens[sc.sc_curScreen], "OK","Red ForeColor") < 0)
		printf("Error in OK Dialog.\n");

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)MakeRGB15(0,0x1F,0),(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT);

	if (Answer(OK_DIALOG, sc.sc_Screens[sc.sc_curScreen], "OK","Green ForeColor") < 0)
		printf("Error in OK Dialog.\n");

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)MakeRGB15(0,0,0x1F),(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT);

	if (Answer(OK_DIALOG, sc.sc_Screens[sc.sc_curScreen], "OK","Blue ForeColor") < 0)
		printf("Error in OK Dialog.\n");



	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0x1F,0,0),(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT);

	result = Answer(OK_CANCEL_DIALOG, sc.sc_Screens[sc.sc_curScreen], "OK Cancel","Red BackColor",&firstButton);
	switch (result) {
		case 0:	printf("OK Button Hit.\n");
			break;
		case 1: printf("Cancel Button Hit.\n");
			break;
		default:printf("Error in OK-Cancel Dialog.\n");
			break;
	}

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0,0x1F,0),(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT);

	result = Answer(OK_CANCEL_DIALOG, sc.sc_Screens[sc.sc_curScreen], "OK Cancel","Green BackColor",&firstButton);
	switch (result) {
		case 0:	printf("OK Button Hit.\n");
			break;
		case 1: printf("Cancel Button Hit.\n");
			break;
		default:printf("Error in OK-Cancel Dialog.\n");
			break;
	}

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0,0,0x1F),(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT);

	result = Answer(OK_CANCEL_DIALOG, sc.sc_Screens[sc.sc_curScreen], "OK Cancel","Blue BackColor",&firstButton);
	switch (result) {
		case 0:	printf("OK Button Hit.\n");
			break;
		case 1: printf("Cancel Button Hit.\n");
			break;
		default:printf("Error in OK-Cancel Dialog.\n");
			break;
	}



	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0x1F,0,0),(RGB555)COLOR_DEFAULT);

	if (Answer(ONE_BUTTON_DIALOG, sc.sc_Screens[sc.sc_curScreen], "One Button","Red Hilite Color","Button 1") < 0)
		printf("Error in One Button Dialog.\n");

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0,0x1F,0),(RGB555)COLOR_DEFAULT);

	if (Answer(ONE_BUTTON_DIALOG, sc.sc_Screens[sc.sc_curScreen], "One Button","Green Hilite Color","Button 1") < 0)
		printf("Error in One Button Dialog.\n");

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0,0,0x1F),(RGB555)COLOR_DEFAULT);

	if (Answer(ONE_BUTTON_DIALOG, sc.sc_Screens[sc.sc_curScreen], "One Button","Blue Hilite Color","Button 1") < 0)
		printf("Error in One Button Dialog.\n");



	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );

	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0x1F,0,0));

	result = Answer(TWO_BUTTON_DIALOG, sc.sc_Screens[sc.sc_curScreen],"Two Button","Red Shadow Color","Button 1","Button 2",&firstButton);
	switch (result) {
		case 0:	printf("First Button Hit.\n");
			break;
		case 1: printf("Second Button Hit.\n");
			break;
		default:printf("Error in Two Button Dialog.\n");
			break;
	}

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0,0x1F,0));

	result = Answer(TWO_BUTTON_DIALOG, sc.sc_Screens[sc.sc_curScreen],"Two Button","Green Shadow Color","Button 1","Button 2",&firstButton);
	switch (result) {
		case 0:	printf("First Button Hit.\n");
			break;
		case 1: printf("Second Button Hit.\n");
			break;
		default:printf("Error in Two Button Dialog.\n");
			break;
	}

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)MakeRGB15(0,0,0x1F));

	result = Answer(TWO_BUTTON_DIALOG, sc.sc_Screens[sc.sc_curScreen],"Two Button","Blue Shadow Color","Button 1","Button 2",&firstButton);
	switch (result) {
		case 0:	printf("First Button Hit.\n");
			break;
		case 1: printf("Second Button Hit.\n");
			break;
		default:printf("Error in Two Button Dialog.\n");
			break;
	}

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_SAME,(RGB555)COLOR_SAME,(RGB555)MakeRGB15(0,0,0x1F),(RGB555)COLOR_DEFAULT);
	
	strcpy(fName,"Default");
	result = StdSaveFile("Save game as...",buf,bufSize,sc.sc_Screens[sc.sc_curScreen],fName);
	if (result < 0)
		printf("Error in save file.\n");
	else if (result != 0)
		printf("User cancelled save file.\n");

	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	SetAccessColors((RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT,(RGB555)COLOR_DEFAULT);
	
	result = StdLoadFile("game...",(char**)&readBuf,sc.sc_Screens[sc.sc_curScreen]);
	if (result < 0)
		printf("Error in load file.\n");
	else if (result != 0)
		printf("User cancelled load file.\n");


	CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages );
	result = StdDeleteFile("game...",sc.sc_Screens[sc.sc_curScreen]);
	if (result < 0)
		printf("Error in delete file.\n");
	else if (result != 0)
		printf("User cancelled delete file.\n");
		
	printf("Hit pause key to exit.\n");
	while (TRUE)
		{
		/* React to the joystick */
		if (GetJoyPad(&lControlvals,1)) goto DONE;		
		sc.sc_curScreen = 1 - sc.sc_curScreen;

		}

DONE: 
	FadeToBlack( &sc, FADE_FRAMECOUNT );
	printf("Access test complete.\n");
	ShutDown();
	return( (int)status );
}
