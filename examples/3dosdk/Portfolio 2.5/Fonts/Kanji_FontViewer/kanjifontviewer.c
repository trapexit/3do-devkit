
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: kanjifontviewer.c,v 1.16 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/kanjifontviewer
|||	kanjifontviewer - Displays and manipulates Kanji fonts
|||
|||	  Synopsis
|||
|||	    kanjifontviewer
|||
|||	  Description
|||
|||	    Display and manipulate a Kanji font.
|||
|||	  Associated Files
|||
|||	    kanjifontviewer.make, ktextbox.h, ktextbox.c, ktextboxerror.h,
|||	    timehelper.h, timehelper.c
|||
|||	  Location
|||
|||	    examples/Fonts/KanjiFontViewer
|||
**/

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
#include "io.h"
#include "string.h"
#include "stdlib.h"
#include "graphics.h"
#include "filestream.h"
#include "filefunctions.h"
#include "debug3do.h"
#include "form3do.h"
#include "parse3do.h"
#include "displayutils.h"
#include "celutils.h"

#include "event.h"
#include "controlpad.h"

#include "ktextbox.h"

#include "timehelper.h"


/* *************************************************************************
 * ***                   ***************************************************
 * ***  Our Definitions  ***************************************************
 * ***                   ***************************************************
 * *************************************************************************
 */

#define kNumFontFiles	1
#define WAIT_TIME		150000
#define ControlAll  (ControlStart | ControlUp | ControlDown | ControlLeft | ControlRight | ControlA | ControlB | ControlC)


ubyte			*gBackPic = NULL;
ScreenContext	*gScreenContext;
Item			gDisplayItem = -1;					/* result of CreateBasicDisplay */




/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

void  CopyBlack( Bitmap *bitmap, int32 nFrameBufferPages );
bool  InitBackPic( ScreenContext *sc, char *filename );
void  CopyBackPic( Bitmap *bitmap, int32 nFrameBufferPages );
void  MakeText(uint8 high, uint8 *text1, uint8 *text2);

ubyte	*gBackBlack = NULL;
Item	gVRAMIOeq = -1;


TagArg ScreenTags[] =
{
	/* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
	/* Kludgy code below presumes that SPORTBITS is the first entry */
	CSG_TAG_SPORTBITS,	(void *)0,
	CSG_TAG_SCREENCOUNT,	(void *)2,
	CSG_TAG_DONE,			0
};

/*------------------------------------------------------------------------
 * drawText()
 *	Calls the system DrawText8() routine to display simple text on the screen.
 *----------------------------------------------------------------------*/

void
drawText( int32 x, int32 y, char *string )
{
	GrafCon		gcon;

	SetFGPen(&gcon, MakeRGB15( 31, 31, 31));
	SetBGPen(&gcon, MakeRGB15( 0, 0, 31));

	gcon.gc_PenX = x;
	gcon.gc_PenY = y;
	DrawText8( &gcon, gScreenContext->sc_BitmapItems[ gScreenContext->sc_curScreen ], string );
}

/*------------------------------------------------------------------------
 * main()
 *----------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
	int32		err;
	Item		dirItem;
	int32		retvalue = 0;
	uint32		buttons;
	Rectf16		theRect[4];
	Color		fgColor;
	Color		bgColor;
	KFont3DO	TextFont;
	int32		fontIndex = 0;
	uint8		Text1[103*2+1], Text2[103*2+1], *Text1_ptr, *Text2_ptr;
	CCB			*textccb[2];
	uint8		high;
	char		*Font1Path = (char *)"$Fonts";
	char		*Font1 = (char *)"Kanji16.4";		// 904K
	char		EntireFontName[200];
	char		string[255];
	TextAlign	theAlign;
	int32		count = 0;
	int32		row;
	long		randomNum;
	int32		charsPerScreen;
	Boolean		fullRead;
	char		*jis_highbyte[78] = {(char *)"‚O‚˜‚Q‚P",(char *)"‚O‚˜‚Q‚Q",(char *)"‚O‚˜‚Q‚R",(char *)"‚O‚˜‚Q‚S",(char *)"‚O‚˜‚Q‚T",
									 (char *)"‚O‚˜‚Q‚U",(char *)"‚O‚˜‚Q‚V",(char *)"‚O‚˜‚Q‚W",(char *)"‚O‚˜‚Q‚e",(char *)"‚O‚˜‚R‚O",
									 (char *)"‚O‚˜‚R‚P",(char *)"‚O‚˜‚R‚Q",(char *)"‚O‚˜‚R‚R",(char *)"‚O‚˜‚R‚S",(char *)"‚O‚˜‚R‚T",
									 (char *)"‚O‚˜‚R‚U",(char *)"‚O‚˜‚R‚V",(char *)"‚O‚˜‚R‚W",(char *)"‚O‚˜‚R‚X",(char *)"‚O‚˜‚R‚`",
									 (char *)"‚O‚˜‚R‚a",(char *)"‚O‚˜‚R‚b",(char *)"‚O‚˜‚R‚c",(char *)"‚O‚˜‚R‚d",(char *)"‚O‚˜‚R‚e",
									 (char *)"‚O‚˜‚S‚O",(char *)"‚O‚˜‚S‚P",(char *)"‚O‚˜‚S‚Q",(char *)"‚O‚˜‚S‚R",(char *)"‚O‚˜‚S‚S",
									 (char *)"‚O‚˜‚S‚T",(char *)"‚O‚˜‚S‚U",(char *)"‚O‚˜‚S‚V",(char *)"‚O‚˜‚S‚W",(char *)"‚O‚˜‚S‚X",
									 (char *)"‚O‚˜‚S‚`",(char *)"‚O‚˜‚S‚a",(char *)"‚O‚˜‚S‚b",(char *)"‚O‚˜‚S‚c",(char *)"‚O‚˜‚S‚d",
									 (char *)"‚O‚˜‚S‚e",(char *)"‚O‚˜‚T‚O",(char *)"‚O‚˜‚T‚P",(char *)"‚O‚˜‚T‚Q",(char *)"‚O‚˜‚T‚R",
									 (char *)"‚O‚˜‚T‚S",(char *)"‚O‚˜‚T‚T",(char *)"‚O‚˜‚T‚U",(char *)"‚O‚˜‚T‚V",(char *)"‚O‚˜‚T‚W",
									 (char *)"‚O‚˜‚T‚X",(char *)"‚O‚˜‚T‚`",(char *)"‚O‚˜‚T‚a",(char *)"‚O‚˜‚T‚b",(char *)"‚O‚˜‚T‚c",
									 (char *)"‚O‚˜‚T‚d",(char *)"‚O‚˜‚T‚e",(char *)"‚O‚˜‚U‚O",(char *)"‚O‚˜‚U‚P",(char *)"‚O‚˜‚U‚Q",
									 (char *)"‚O‚˜‚U‚R",(char *)"‚O‚˜‚U‚S",(char *)"‚O‚˜‚U‚T",(char *)"‚O‚˜‚U‚U",(char *)"‚O‚˜‚U‚V",
									 (char *)"‚O‚˜‚U‚W",(char *)"‚O‚˜‚U‚X",(char *)"‚O‚˜‚U‚`",(char *)"‚O‚˜‚U‚a",(char *)"‚O‚˜‚U‚b",
									 (char *)"‚O‚˜‚U‚c",(char *)"‚O‚˜‚U‚d",(char *)"‚O‚˜‚U‚e",(char *)"‚O‚˜‚V‚O",(char *)"‚O‚˜‚V‚P",
									 (char *)"‚O‚˜‚V‚Q",(char *)"‚O‚˜‚V‚R",(char *)"‚O‚˜‚V‚S"};
	char *tmp = (char *)"        ";
	CCB	*jisccb[2];
	int32 wait;
	int32 errorIndex;
	char *errorString[7] = {	(char *)"unknown error",
								(char *)"KTextBox-BadParameter",
								(char *)"KTextBox-CannotMemAlloc",
								(char *)"KTextBox-NotFound",
								(char *)"KTextBox-BadCharCode",
								(char *)"KTextBox-BadFontFile",
								(char *)"KTextBox-CannotOpenDS"	};
	char			actualPathname[200], actualPathname2[200];
	char			origPathname[200];
	TimerHelperPtr	gIdleTimer = NULL;
	TimeVal	tv;

	printf("kanjifontviewer\n");

	gVRAMIOeq = CreateVRAMIOReq();

	gScreenContext = (ScreenContext *) AllocMem( sizeof(ScreenContext), MEMTYPE_ANY );
	if ( gScreenContext == NULL )
		{
		PRT( ("Can't allocate ScreenContext\n") );
		goto DONE;
		}

	if ( (gDisplayItem = CreateBasicDisplay(gScreenContext, DI_TYPE_DEFAULT, 2)) < 0 )
    	{
        PRT( ("Can't initialize display\n") );
		goto DONE;
    	}

	if ( OpenMathFolio() < 0 )
        {
        PRT( ("Can't open the math folio\n") );
		goto DONE;
        }

	if ( InitControlPad( 2 ) <  0 )
		{
		PRT( ("Can't initialize the control pad\n") );
		goto DONE;
		}

	/*------------------------------------------------------------------------
	 * Initialize a timer task to run performance benchmarks.
	 *----------------------------------------------------------------------*/

	gIdleTimer = InitTimer(TM_TYPE_MICROSEC + TM_MODE_DELTA);

	/*------------------------------------------------------------------------
	 * Define the rects for 2 groupings of Kanji text
	 *	and 2 captions showing the current index value (high-byte only).
	 *----------------------------------------------------------------------*/

	theRect[0].rectf16_XLeft 	= 	Convert32_F16(20);
	theRect[0].rectf16_XRight 	= 	Convert32_F16(300);
	theRect[0].rectf16_YTop 	= 	Convert32_F16(20);
	theRect[0].rectf16_YBottom  = 	Convert32_F16(120);

	theRect[1].rectf16_XLeft 	= 	Convert32_F16(20);
	theRect[1].rectf16_XRight 	= 	Convert32_F16(300);
	theRect[1].rectf16_YTop 	= 	Convert32_F16(20);
	theRect[1].rectf16_YBottom  = 	Convert32_F16(120);

	theRect[2].rectf16_XLeft 	= 	Convert32_F16(20);
	theRect[2].rectf16_XRight 	= 	Convert32_F16(92);
	theRect[2].rectf16_YTop 	= 	Convert32_F16(20);
	theRect[2].rectf16_YBottom  = 	Convert32_F16(36);

	theRect[3].rectf16_XLeft 	= 	Convert32_F16(20);
	theRect[3].rectf16_XRight 	= 	Convert32_F16(92);
	theRect[3].rectf16_YTop 	= 	Convert32_F16(20);
	theRect[3].rectf16_YBottom  = 	Convert32_F16(36);

	/*------------------------------------------------------------------------
	 * Do some simple initialization stuff.
	 *----------------------------------------------------------------------*/

	bgColor =  0x00080808;
	fgColor =  0x00FFFFFF;

	theAlign.align = justLeft;
	theAlign.charPitch = 0;
	theAlign.linePitch = 0;

	/*------------------------------------------------------------------------
	 * If a background image is available use it as the backdrop.
	 *----------------------------------------------------------------------*/

	InitBackPic( gScreenContext, "kanjifontviewer.imag" );
	CopyBlack( gScreenContext->sc_Bitmaps[gScreenContext->sc_curScreen], gScreenContext->sc_nFrameBufferPages );

	CopyBackPic(gScreenContext->sc_Bitmaps[gScreenContext->sc_curScreen], gScreenContext->sc_nFrameBufferPages );
	DisplayScreen( gScreenContext->sc_Screens[ gScreenContext->sc_curScreen ], 0 );

	/*------------------------------------------------------------------------
	 * Store-off the current boot directory pathname.
	 *----------------------------------------------------------------------*/

	GetDirectory( origPathname, 200 );
	dirItem = ChangeDirectory( Font1Path );
	GetDirectory( actualPathname, 200 );

	/*------------------------------------------------------------------------
	 * Change the current directory to the $fonts directory,
	 *	then store-off that $font directory pathname.
	 *----------------------------------------------------------------------*/

	dirItem = ChangeDirectory( "$app/System/Graphics/Fonts" );
	GetDirectory( actualPathname2, 200 );

	/*------------------------------------------------------------------------
	 * Change the directory back to the default boot directory we stored-off.
	 *----------------------------------------------------------------------*/

	ChangeDirectory( origPathname );

	/*------------------------------------------------------------------------
	 * Display the available font directories (from ROM, CD, Mac), then
	 *	prompt the user to select one of the directories as the font source.
	 *----------------------------------------------------------------------*/

	sprintf( string, "FONT=%s", Font1 );
	row = 20; drawText( 25, row, string );

	row += 20; drawText( 25, row, "PRESS EITHER A OR C BUTTON" );

	sprintf( string, "A = %s", actualPathname );
	row += 10; drawText( 30, row, string );
	sprintf( string, "C = %s", actualPathname2 );
	row += 10; drawText( 30, row, string );

	while (TRUE) {
		DoControlPad( 1, &buttons, ControlA | ControlC );
		if (buttons & (ControlA | ControlC) )
			break;
	}

	if ( buttons & ControlA )
		sprintf( EntireFontName, "%s/%s", actualPathname, Font1 );
	else
		sprintf( EntireFontName, "%s/%s", actualPathname2, Font1 );

	for(wait = 0; wait < WAIT_TIME ; wait++) ;

	/*------------------------------------------------------------------------
	 * Ask the user if the entire font should be read into memory
	 *	or just one character at a time.
	 *----------------------------------------------------------------------*/

	row += 20; drawText( 25, row, "PRESS EITHER A OR C BUTTON" );
	row += 10; drawText( 40, row, " A = LOAD ENTIRE FONT   ");
	row += 8; drawText( 40, row, "     INTO MEMORY        ");
	row += 10; drawText( 40, row, " C = LOAD ONE CHAR INTO ");
	row += 8; drawText( 40, row, "     MEMORY AT A TIME   ");

	while (TRUE) {
		DoControlPad(1, &buttons, ControlA | ControlC);
		if (buttons & (ControlA | ControlC) )
			break;
	}

	if ( buttons & ControlA )
		fullRead = true;
	else
		fullRead = false;

	row += 20;

	/*------------------------------------------------------------------------
	 * Load the font and display the font loading progress on the screen.
	 *----------------------------------------------------------------------*/

	if (fullRead)
		drawText( 25, row, "Loading entire font file..." );
	else
		drawText( 25, row, "Loading font file header only..." );

	row += 10; drawText( 25, row, EntireFontName );

	if (gIdleTimer)
		GetTime(gIdleTimer, 1, &tv);

	err = KLoadFont(EntireFontName,&TextFont, fullRead);

	if (gIdleTimer)
		GetTime(gIdleTimer, 0, &tv);

	if (err == 0)
	{
		/*------------------------------------------------------------------------
		 * If no load error occurs, display the load time.
		 *----------------------------------------------------------------------*/

		sprintf( string, "*** Loaded successfully ***" );
		row += 10; drawText( 25, row, string );

		if (gIdleTimer)
		{
			sprintf( string, "Load time: secs=%0d, usecs=%0d", tv.tv_Seconds, tv.tv_Microseconds );
			row += 30; drawText( 25, row, string );
		}

		sprintf( string, "press any button to continue" );
		row += 10; drawText( 25, row, string );

		while (TRUE)
		{
			DoControlPad(1, &buttons, ControlAll);
			if ( buttons & ControlAll )
				break;
		}
	}
	else
	{
		/*------------------------------------------------------------------------
		 * If an occur does occur, display the error code and message.
		 *----------------------------------------------------------------------*/

		sprintf( string, "Error loading %s", Font1 );
		row += 20; drawText( 25, row, string );

		if (err >= -6 && err <= -1)
			errorIndex = -err;
		else
			errorIndex = 0;

		sprintf( string, "ERR(%d) = %s", err, errorString[errorIndex] );
		row += 10; drawText( 25, row, string );

		while (TRUE)
		{
			DoControlPad(1, &buttons, ControlAll);
			if ( buttons & ControlStart )
				goto DONE;
		}
	}

	/*------------------------------------------------------------------------
	 * For the first test, display two strings containing
	 *	Hankaku (half-width) characters only.  These characters contain
	 *	the 7-bit ASCII characters as well.
	 *----------------------------------------------------------------------*/

	Text1_ptr =(uint8 *)" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	Text2_ptr =(uint8 *)"¦±²³´µÔÕÖÂ±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÊßÊÞ¢£¤¡Ì§Ì¨ÌÌªÌ«·¬·­·®À¯ÁÄÞ¯Ä";

	if (gIdleTimer)
		GetTime(gIdleTimer, 1, &tv);

	/*------------------------------------------------------------------------
	 * Create the two cels for displaying the Hankaku characters only.
	 *----------------------------------------------------------------------*/

	textccb[0] = KTextBox_A(&TextFont,Text1_ptr,strlen(Text1_ptr),&theRect[0], &theAlign, fgColor, bgColor, NULL);
	textccb[1] = KTextBox_A(&TextFont,Text2_ptr,strlen(Text2_ptr),&theRect[1], &theAlign, fgColor, bgColor, NULL);

	if (gIdleTimer)
		GetTime(gIdleTimer, 0, &tv);

	charsPerScreen = (strlen(Text1_ptr) + strlen(Text2_ptr)) >> 1;

	/*------------------------------------------------------------------------
	 * Create the two cels for displaying high-byte
	 *	portion of the S-JIS index values.
	 *----------------------------------------------------------------------*/

	jisccb[0] = KTextBox_A(&TextFont,tmp,8,&theRect[2], &theAlign, fgColor,bgColor,NULL);
	jisccb[1] = KTextBox_A(&TextFont,tmp,8,&theRect[3], &theAlign, fgColor,bgColor,NULL);

	/*------------------------------------------------------------------------
	 * Position and link the cels together.
	 *----------------------------------------------------------------------*/

	textccb[1]->ccb_YPos = Convert32_F16(ConvertF16_32(textccb[1]->ccb_YPos)+115);
	jisccb[0]->ccb_XPos = Convert32_F16(200);
	jisccb[0]->ccb_YPos = Convert32_F16(100);
	jisccb[1]->ccb_XPos = Convert32_F16(200);
	jisccb[1]->ccb_YPos = Convert32_F16(206);

	LinkCel(textccb[0], textccb[1]);
	LinkCel(textccb[1], jisccb[0]);
	LinkCel(jisccb[0], jisccb[1]);
	LAST_CEL(jisccb[1]);

	/*------------------------------------------------------------------------
	 * Build the S-JIS text strings for the Zenkaku characters.
	 *----------------------------------------------------------------------*/

	high = 0x81;
	Text1_ptr = Text1;
	Text2_ptr = Text2;
	MakeText(high, Text1_ptr, Text2_ptr);

	while (TRUE)
	{
		/* React to the control pad */
		DoControlPad(1, &buttons, ControlAll);
		if (buttons & ControlStart)
			goto DONE;

		/*------------------------------------------------------------------------
		 * Move the position of the text cels using the directional pad and cycle
		 *	thru the indexes using the A, B, and C buttons.
		 *----------------------------------------------------------------------*/

		if (buttons & ControlRight) {
		  textccb[0]->ccb_XPos = Convert32_F16(ConvertF16_32(textccb[0]->ccb_XPos)+1);
		  textccb[1]->ccb_XPos = Convert32_F16(ConvertF16_32(textccb[1]->ccb_XPos)+1);
		}

		else if (buttons & ControlLeft) {
		  textccb[0]->ccb_XPos = Convert32_F16(ConvertF16_32(textccb[0]->ccb_XPos)-1);
		  textccb[1]->ccb_XPos = Convert32_F16(ConvertF16_32(textccb[1]->ccb_XPos)-1);
		}

		else if (buttons & ControlUp) {
		  textccb[0]->ccb_YPos = Convert32_F16(ConvertF16_32(textccb[0]->ccb_YPos)-1);
		  textccb[1]->ccb_YPos = Convert32_F16(ConvertF16_32(textccb[1]->ccb_YPos)-1);
		}

		else if (buttons & ControlDown) {
		  textccb[0]->ccb_YPos = Convert32_F16(ConvertF16_32(textccb[0]->ccb_YPos)+1);
		  textccb[1]->ccb_YPos = Convert32_F16(ConvertF16_32(textccb[1]->ccb_YPos)+1);
		}

		else if (buttons & (ControlA | ControlB | ControlC) ) {

			if (buttons & ControlC)
			{
				/*------------------------------------------------------------------------
				 * The C button displays a random text color on a random background color.
				 *----------------------------------------------------------------------*/

				randomNum = rand();

				if (randomNum < 0)
					randomNum *= -1;

				fgColor = ((randomNum * 0x00FFFFFF) / 32767);

				randomNum = rand();

				if (randomNum < 0)
					randomNum *= -1;

				bgColor = ((randomNum * 0x00FFFFFF) / 32767);
			}

		 	else if ( buttons & ControlA )
			{
				/*------------------------------------------------------------------------
				 * The A button displays white text on a near black background.
				 *----------------------------------------------------------------------*/

		    	bgColor =  0x00080808;
				fgColor =  0x00FFFFFF;
			}

			else
			{
				/*------------------------------------------------------------------------
				 * The B button displays near black text on a white background.
				 *----------------------------------------------------------------------*/

				bgColor =  0x00FFFFFF;
		    	fgColor =  0x00080808;
			}

			for(wait = 0; wait < WAIT_TIME ; wait++) ;

			/*------------------------------------------------------------------------
			 * Build more S-JIS text strings and increment
			 *	the high-byte portion of the S-JIS index value only.
			 *----------------------------------------------------------------------*/

			MakeText(high, Text1_ptr, Text2_ptr);

			high++;
			if(high == 0x85) high = 0x88;
			if(high == 0xa0) high = 0xe0;
			if(high > 0xea) high = 0x81;

			if (gIdleTimer)
				GetTime(gIdleTimer, 1, &tv);

			/*------------------------------------------------------------------------
			 * Now create the cels for displaying the Zenkaku characters as
			 *	before, but this time use the existing cels.
			 *----------------------------------------------------------------------*/

			textccb[0] = KTextBox_A(&TextFont,Text1_ptr,strlen(Text1_ptr),&theRect[0], &theAlign, fgColor,bgColor,textccb[0]);
			textccb[1] = KTextBox_A(&TextFont,Text2_ptr,strlen(Text2_ptr),&theRect[1], &theAlign, fgColor,bgColor,textccb[1]);

			if (gIdleTimer)
				GetTime(gIdleTimer, 0, &tv);

			charsPerScreen = (strlen(Text1_ptr) + strlen(Text2_ptr)) >> 1;

			/*------------------------------------------------------------------------
			 * Again create the cels for displaying the high-byte
			 *	portion of the S-JIS index values and use the existing cels.
			 *----------------------------------------------------------------------*/

			jisccb[0] = KTextBox_A(&TextFont,jis_highbyte[count++],8,&theRect[2], &theAlign, fgColor,bgColor,jisccb[0]);
			jisccb[1] = KTextBox_A(&TextFont,jis_highbyte[count++],8,&theRect[3], &theAlign, fgColor,bgColor,jisccb[1]);

			/*------------------------------------------------------------------------
			 * Finally re-position the cels to their default positions.
			 *----------------------------------------------------------------------*/

			textccb[1]->ccb_YPos = Convert32_F16(ConvertF16_32(textccb[1]->ccb_YPos)+115);
			jisccb[0]->ccb_XPos = Convert32_F16(200);
			jisccb[0]->ccb_YPos = Convert32_F16(100);
			jisccb[1]->ccb_XPos = Convert32_F16(200);
			jisccb[1]->ccb_YPos = Convert32_F16(216);

			if(count > 77) count = 0;
		}
		else {
		}

		if (fontIndex >= kNumFontFiles) fontIndex = 0;

		/* SPORT the background back into the current buffer.
		 * The call to the SPORT routine, which does a DoIO(), will cause us
		 * to sync up to the vertical blank at this point.  After synching to
		 * the vertical blank, we should have as short a path as
		 * possible between here and the call to DisplayScreen(), with,
		 * at best, only one call to DrawCels() standing between.
		 */
		CopyBackPic(gScreenContext->sc_Bitmaps[gScreenContext->sc_curScreen], gScreenContext->sc_nFrameBufferPages );

		/*------------------------------------------------------------------------
		 * If no error occurs, draw all the cels on the current screen.
		 *----------------------------------------------------------------------*/

		if (textccb == NULL) {
			PRT( ("Problem with TextBox - either bad input parameters or out of memory.") );
			goto DONE;
		}
		else {
			retvalue = DrawCels (gScreenContext->sc_BitmapItems[ gScreenContext->sc_curScreen ], textccb[0]);
			if ( retvalue < 0 )  {
				PRT( ("DrawCels failed\n") );
			}
		}

		/*------------------------------------------------------------------------
		 * Display the time it took to create and render the text cels.
		 *----------------------------------------------------------------------*/

		if (gIdleTimer)
		{
			sprintf( string, "chars=%0ld, secs=%0ld, usecs=%0ld", charsPerScreen, tv.tv_Seconds, tv.tv_Microseconds );
			drawText( 25, 125, string );
		}
		else
			drawText( 25, 125, EntireFontName );

		/*------------------------------------------------------------------------
		 * Display the current screen, then toggle to the other screen
		 *	for our next frame of animation.
		 *----------------------------------------------------------------------*/

		retvalue = DisplayScreen( gScreenContext->sc_Screens[ gScreenContext->sc_curScreen ], 0 );
		if ( retvalue < 0 )
			{
			PRT( ("DisplayScreen failed\n") );
			goto DONE;
			}

		gScreenContext->sc_curScreen = 1 - gScreenContext->sc_curScreen;
	}

DONE:

	/*------------------------------------------------------------------------
	 * Normal and error exit
	 *----------------------------------------------------------------------*/

	if (gIdleTimer)
		FreeTimer(gIdleTimer);

    KillControlPad();

	if ( gScreenContext )
		FadeToBlack( gScreenContext, 60 );

	if ( gDisplayItem >= 0 )
		DeleteBasicDisplay( gScreenContext );

	DeleteVRAMIOReq( gVRAMIOeq );

	printf("end of kanjifontviewer\n");

	return 0;
}

/*------------------------------------------------------------------------
 * CopyBackPic()
 *----------------------------------------------------------------------*/

void
CopyBackPic( Bitmap *bitmap, int32 nFrameBufferPages )
{
	CopyVRAMPages( gVRAMIOeq, bitmap->bm_Buffer, gBackPic, nFrameBufferPages, ~0 );
}

/*------------------------------------------------------------------------
 * InitBackPic()
 * Allocates the gBackPic buffer.  If a filename is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no filename is specified, this
 * routine merely clears the gBackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 *----------------------------------------------------------------------*/

bool
InitBackPic( ScreenContext *sc, char *filename )
{
	bool retvalue;
	retvalue = FALSE;

	gBackPic = (ubyte *)ALLOCMEM( (int)(sc->sc_nFrameByteCount),
//			GETBANKBITS( GrafBase->gf_ZeroPage ) |
			MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

	if ( NOT gBackPic)
		{
		PRT( ( "Can't allocate gBackPic" ) );
		goto DONE;
		}

	SetVRAMPages( gVRAMIOeq, gBackPic, 0, sc->sc_nFrameBufferPages, ~0 );

	if (LoadImage( filename, gBackPic, NULL, sc ) == NULL)
		{
		PRT( ( "LoadImage failed\n" ) );
		goto DONE;
		}
	CopyVRAMPages( gVRAMIOeq, sc->sc_Bitmaps[sc->sc_curScreen]->bm_Buffer, gBackPic, sc->sc_nFrameBufferPages, ~0 );

	retvalue = TRUE;

DONE:
	return(retvalue);
}

/*------------------------------------------------------------------------
 * MakeText()
 *	Builds Shift-JIS text strings where the high parameter specifies,
 *	the high-byte portion of the S-JIS index value.
 *----------------------------------------------------------------------*/

void
MakeText(uint8 high, uint8 *text1, uint8 *text2)
{
	uint8 i;
	if( high != 0x98 && high != 0xea){
		for(i=0x40; i <= 0x7e; i++){
			*text1++ = high;
			*text1++ = i;
		}
		for(i=0x80; i <= 0x9e; i++){
			*text1++ = high;
			*text1++ = i;
		}
		*text1 = '\0';
		for(i=0x9f; i <= 0xfc; i++){
			*text2++ = high;
			*text2++ = i;
		}
		*text2 = '\0';
	}
	else if( high == 0x98 ){
		for(i=0x40; i <= 0x72; i++){
			*text1++ = high;
			*text1++ = i;
		}
		*text1 = '\0';

		for(i=0x9f; i <= 0xfc; i++){
			*text2++ = high;
			*text2++ = i;
		}
		*text2 = '\0';
	}
	else if( high == 0xea){
		for(i=0x40; i <= 0x7e; i++){
			*text1++ = high;
			*text1++ = i;
		}
		for(i=0x80; i <= 0x9e; i++){
			*text1++ = high;
			*text1++ = i;
		}
		*text1 = '\0';
		for(i=0x9f; i <= 0xa2; i++){
			*text2++ = high;
			*text2++ = i;
		}
		*text2 = '\0';
	}
}

/*------------------------------------------------------------------------
 * CopyBlack()
 *----------------------------------------------------------------------*/

void
CopyBlack( Bitmap *bitmap, int32 nFrameBufferPages )
{
	SetVRAMPages( gVRAMIOeq, bitmap->bm_Buffer, 0, nFrameBufferPages, ~0 );
}

