/*
	File:		SlideShow.c

	Contains:	Load and show image files.

	Written by:	eric carlson

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):
		 <9> 	 3/15/94	MPH		Changed to handle PAL sized Images.  Stripped all VDL calls because PAL currently cannot support it.
		 							Plus, the VDL implementation will be changing. Hence, do not try it for now.
									
		 <8>	 1/18/94	JAY		explicitely type last argument to MergeVDL call. Seems to be
									causing 1.6v2 armcc some problems
		 <7>	 8/20/93	JAY		unclear
		 <6>	 8/11/93	JAY		changed VRAMIOReq to gVRAMIOReq and some other minor changes ec
									did.
		 <5>	 7/30/93	JAY		add io parameter to SetVRAMPages() and CopyVRAMPages() for
									Dragon Tail release
		 <4>	 6/23/93	ec		Updated for 4B1: Removed JOURNALING code, changed parameters to
									SubmitVDL. Major changes to the way we deal with VDLs as the
									system now makes it's own copy (under the fence) when we do a
									'SubmitVDL'. Now do a 'DeleteVDL' after submitting a new one as
									each VDL is tied to a specific file/image.
		 <3>	  4/7/93	JAY		MEMTYPE_SPRITE is now MEMTYPE_CEL in Cardinal2
		 <2>	  4/5/93	JAY		remove /remote/. add ChangeInitialDirectory()
		 <1>	 3/18/93	JAY		first checked in
				2/17/93		ec		Added VDL diddling options.
				2/15/93		ec		1.1 release, updated for Magneto 6
				11/16/92	ec		1.0 release

	To Do:
*/


#include "Portfolio.h"
#include "filefunctions.h"
#include "debug3DO.h"

#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "UMemory.h"

#include "OurVDL.h"
#include "PALSlideShow.h"

/*
 * Definitions
 */
#define kVersion		"1.0"

/*
 * Function Prototypes
 */
int32		ParseArgs(int32 argc, char *argv[]);
int32		ParseScriptFile(char *scriptFileName);
int32		LoadScreen(char *fileName, int32 whichBuffer);
Boolean		DoJoystick( void );
void		ShowNextImage(void);
void		ShowPrevImage(void);
Boolean  	Init( void );
Boolean 	OpenMacLink( void );
Boolean 	OpenSPORT( void );
Boolean		DisplayCurrentScreen(void);
void		usage(void);

/*
 * Data Declarations
 */
ubyte			*gBackPic[kBufferCount];		// background image buffers
int32			gCurrentImage = 0,				// current image #
				gImageCount = 0,				// # of images we show
				gShowImageTime,					// length each image is shown in autoshow
				gImageVisTime = 60 * 4;			// default autoshow = 4 sec per image
Boolean			gAutoShowFlag = FALSE,			// autoshow?
				gNextBuffGood,					// next buffer is good?
				gPrevBuffGood;					// have we shown the first image yet? [HACK}
char			*gFileNames[256];				// list of image file names
ScreenContext	gScreenCntx;					// screen context structures
Item			gVDLItems[kScreenCount];		// actual VDL items
VDLEntry		*gVDLPtrArray;					// our raw VDL
Item			gVRAMIOReq;						// Item for Set/CopyVRAMPages

/*
 * Main Routines  
 */
int
main( int argc, char *argv[] )
{
	int32	retValue,	
			screenCount;
			
	//	Init the machine and get ready to run.
	if ( !Init() ) goto DONE;
	
	// parse the params, bail if we don't like them
	if ( !ParseArgs(argc, argv) ) goto DONE;

	// load up the two frame buffers with the first two pictures, but don't show them yet or
	//  we'll flash from one to the other
	gCurrentImage = -1;
	gScreenCntx.sc_curScreen = -1;
	gNextBuffGood = FALSE;
	for ( screenCount = 0; screenCount < kScreenCount; screenCount++ )
		ShowNextImage();

	// and point to the first buffer/image
	gCurrentImage = 0;
	gScreenCntx.sc_curScreen = 0;
	gNextBuffGood = TRUE;
	gPrevBuffGood = FALSE;
	gShowImageTime = TICKCOUNT() + gImageVisTime;

	while (TRUE)
	{		
		// React to the joystick
		if ( NOT DoJoystick() ) goto DONE;

		// make sure we show whatever changes might have happened
		if ( !DisplayCurrentScreen() )
			retValue = FALSE;

		// are we in autoshow mode???
		if ( gAutoShowFlag )
		{
			// check the time to see if it's time to do another round
			if ( gShowImageTime < TICKCOUNT() )
			{
				ShowNextImage();
				gShowImageTime = TICKCOUNT() + gImageVisTime;
			}
		}

	}

DONE: 
	
	// we don't REALLY want to fade to back, but we need the pause so RJ's menu program doesn't
	//  see the user holding down the quit key
	FadeToBlack(&gScreenCntx, 56);
	
	ShutDown();
	kprintf("\nSlideShow has completed!  Thanks for shopping with us!\n");
	return( (int)retValue );
}
 

int32
ParseScriptFile(char *scriptFileName)
{
	int32	fileSize,
			bufferSize,
			result,
			lineLen;
	char	*scriptBuffer = NULL;
	char	*scanChar,
			*fileName;
	
	// determine file size, alloc memory (grab an extra byte so the last file name can
	//	have a null terminator), and read the file into memory
	fileSize = GetFileSize(scriptFileName);
	if ( fileSize <= 0 )
	{
		kprintf("Can't find script file %s.\n", scriptFileName);
		return 1;
	}
	kprintf("Found %s, %d bytes long...\n", scriptFileName, fileSize);
	
	// allocate a buffer large enough to read in the file, zero it out, and read away
	scriptBuffer = (char *)ALLOCMEM(fileSize + 1, 0);
	if ( scriptBuffer == NULL )
	{
		kprintf("Not enough memory to read script file.");
		return 1;
	}
	memset(scriptBuffer, 0, fileSize + 1);
	result = ReadFile(scriptFileName, fileSize, (int32 *)scriptBuffer, 0);
	if ( result < 0 )
	{
		kprintf("Error %d reading script file...\n", result);
		return result;
	}
		
	// now parse through the file, grabbing a pointer to each file name we find
	gImageCount = -1;
	bufferSize = fileSize;
	scanChar = scriptBuffer;
	fileName = scanChar;
	while ( bufferSize-- > 0 )
	{
		// if this char is a return, change it to a null and see if the last run of 
		//  chars specified a valid file name
		if ( (*scanChar == '\r') || (*scanChar == '\n') || (*scanChar == '\0') )
		{
			*scanChar = '\0';
			lineLen = strlen(fileName);
			if ( (lineLen == 0) || (*fileName == '\r') )
				;// empty line, do nothing
			else if ( fileName[0] == '-' )
			{
				if ( (fileName[1] == 'a') || (fileName[1] == 'A') )
				{
					gAutoShowFlag = TRUE;
					gImageVisTime = strtol(&fileName[3], NULL, 0) * 60;
					kprintf("autoshow mode, new image every %d seconds\n", gImageVisTime / 60);
				}
				else
					kprintf("Don't understand %s option.\n", fileName);
			}
			else if ( GetFileSize(fileName) <= 0 )
				kprintf("Hmm, can't find %s...\n", fileName);
			else
			{
				kprintf("Found %s...\n", fileName);
				gImageCount++;
				gFileNames[gImageCount] = fileName;
			}
			
			//  bump the buffer pointer and remember the start of this file name
			scanChar++;
			--bufferSize;
			fileName = scanChar;
		}
		scanChar++;
	}
	
	// hope that we have at least one valid image name by now
	if ( gImageCount < 0 )
	{
		kprintf("No valid image names!  What's going on here?");
		return 1;
	}

	// bump the image count so it is 1 based, get on with life
	kprintf("\n");
	gImageCount++;
	return noErr;
}



/*
 * Parse the command line parameters.  When finished with this routine we
 *  will know how many files to loop through and the auto slide show options
 */
int32
ParseArgs(int32 argc, char **argv)
{
	int32	argNdx;
	Boolean	haveFileName = FALSE;
	char	*progName,
			*cmdLineParam;
	
	// print our name, sanity check the param count.  we must have at least the script
	//  file name
	progName = *argv;
	kprintf("%s %s %s\n", progName, kVersion, kBuildDate);
	if (argc < 2)
	{
		if ( ParseScriptFile("PALimagelist") != noErr )
			return FALSE;
		haveFileName = TRUE;
	}

	for (argNdx = 1; argNdx < argc; argNdx++) 
	{
		cmdLineParam = argv[argNdx];
		
		if ( (*cmdLineParam != '-') && (*cmdLineParam != '+') ) 
		{
			// this param  should be the name of the script file, make sure we don't
			//  already have one
			if ( haveFileName )
			{
				usage();
				return FALSE;
			}
			if ( ParseScriptFile(argv[argNdx]) != noErr )
				return FALSE;
			haveFileName = TRUE;
		}
		else
		{
			switch (cmdLineParam[1]) 
			{
				case 'A':
				case 'a':
					gAutoShowFlag = TRUE;
					cmdLineParam = argv[++argNdx];
					gImageVisTime = strtol(cmdLineParam, NULL, 0) * 60;
					kprintf("Auto running mode, each image is shown for %d seconds.\n", gImageVisTime);
					break;

				default:
					kprintf("### Unknown option: %s\n", cmdLineParam);
					usage();
	    			return FALSE;

			}
		}
	}

	return TRUE;
}


/* 
 * Load an image from disk into the non-visible buffer
 */
int32
LoadScreen(char *fileName, int32 whichBuffer)
{
	int32		sucessfulLoad = FALSE;
	VdlChunk	*rawVDLPtr = NULL;
	char		imageFileName[64];
	Item		oldVDL;
	
	/*
	 * Extract the imagefile name from the arguments passed into the program.
	 *	NOTE: the imagefile name is string copied into a local variable so
	 *	that it will start on a longword boundry. Any strings that are passed 
	 *  to the O/S must start on a longword boundry.
	 */
	strcpy(imageFileName, fileName);

	kprintf("Loading file %s...\n", imageFileName);
	
	// Load the background image and save a copy of it into the non-visible buffer 
	if ( LoadImage(imageFileName, gBackPic[whichBuffer], &rawVDLPtr, &gScreenCntx) == NULL ) 
	{
		kprintf ("LoadImage failed\n");
		goto DONE;
	}

	// made it this far?  all must be well in the world... joy
	sucessfulLoad = TRUE;

DONE:
	return sucessfulLoad;
}



/* 
 * Returns non-FALSE if user didn't press START, else returns FALSE to quit 
 */
Boolean
DoJoystick( void )
{
#define JOYCONTINUOUS (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT\
						| JOYFIREA | JOYFIREB)
	
			Boolean retValue = TRUE;
			uint32 	joyBits;
	static	uint32	gOldJoyBits = 0;		// joy bits last time we checked

	joyBits = ReadControlPad(JOYCONTINUOUS);

	/*
	 * If the user has pressed the START button on the joystick 
	 *  then return FALSE
	 */	
	if ( joyBits & JOYSTART ) {
		retValue = FALSE;
		goto DONE;
	}

	if ( joyBits & JOYFIREA ) 
	{
		// we don't like continuous presses
		if ( joyBits == gOldJoyBits )
			goto DONE;
			
		goto DONE;
	}

	// if FIRE B is pressed, display the other buffer
	if ( (joyBits & JOYFIREB) && !(gOldJoyBits & JOYFIREB) ) 
	{
		gScreenCntx.sc_curScreen = 1 - gScreenCntx.sc_curScreen;
		goto DONE;
	}

	// the select button (Button C) is pressed toggle autoshow
	if ( joyBits & JOYSELECT )
	{
		gAutoShowFlag = !gAutoShowFlag;

		kprintf("Auto-show mode is ");
		if ( gAutoShowFlag )
			 kprintf("TRUE\n");
		else
			kprintf("FALSE\n");

		goto DONE;
	}

	// direction keys switch picture and buffer
	if ( joyBits & (JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT)  )
	{
		if ( (joyBits & JOYUP) && !(gOldJoyBits & JOYUP) )
		{
			ShowPrevImage();
		}
		else if ( (joyBits & JOYLEFT) && !(gOldJoyBits & JOYLEFT) )
		{
			ShowPrevImage();
		}
		else if ( (joyBits & JOYDOWN) && !(gOldJoyBits & JOYDOWN) )
		{
			ShowNextImage();
		}
		else if ( (joyBits & JOYRIGHT) && !(gOldJoyBits & JOYRIGHT) )
		{
			ShowNextImage();
		}
		
	}
	
DONE:
	gOldJoyBits = joyBits;
	
return(retValue);

#undef JOYCONTINUOUS
}


void 
ShowPrevImage(void)
{
	// show the prevous image.  
	gCurrentImage--;
	if ( gCurrentImage < 0 )
		gCurrentImage = gImageCount - 1;
		
	gScreenCntx.sc_curScreen--;
	if ( gScreenCntx.sc_curScreen < 0 )
		gScreenCntx.sc_curScreen = kScreenCount - 1;

	// don't actually load another image unless we're showing the first one
	if ( !gPrevBuffGood )
		LoadScreen(gFileNames[gCurrentImage], gScreenCntx.sc_curScreen);

	gNextBuffGood = TRUE;
	gPrevBuffGood = FALSE;
}

void 
ShowNextImage(void)
{
	// next image, next buffer
	gCurrentImage++;
	if ( gCurrentImage >= gImageCount )
		gCurrentImage = 0;

	gScreenCntx.sc_curScreen++;
	if ( gScreenCntx.sc_curScreen >= kScreenCount )
		gScreenCntx.sc_curScreen = 0;

	// don't actually load another image unless the next buffer isn't valid
	if ( !gNextBuffGood )
		LoadScreen(gFileNames[gCurrentImage], gScreenCntx.sc_curScreen);

	gNextBuffGood = FALSE;
	gPrevBuffGood = TRUE;
}

#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define NTSC_FREQUENCY 60
#define NTSC_TYPE DI_TYPE_NTSC
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define PAL_FREQUENCY 50
#define PAL_TYPE DI_TYPE_PAL2

/*
	Purpose:	Create a screen context record with the number of
				associated screens specified that matches the current
				video mode of the target machine (i.e., NTSC or PAL).
				Opens the Graphics Folio before doing anything...
	NOTE:		A 320x240 by 16-bit deep screen is 153,600 bytes.
				A 388x288 by 16-bit deep screen is 211,184 bytes.
				Thus the standard 2-screen usage will EITHER allocate
				buffers of 300K (NTSC) or 432K (PAL) (the overhead for
				either is the same).  Be sure you have enough memory!
	Inputs:	sc - A pointer to a screen context record (must be allocated).
				nScreens - number of screens to allocate (1-6)
	Outputs:	OpenGraphicsNTSCPAL - returns TRUE if the screen context record
				was allocated.  FALSE if nScreens is out-of-range or the
				screen group could not be added.
*/
bool OpenGraphicsNTSCPAL(ScreenContext *sc, uint32 nScreens)
{

	int32		width, height, diType, i;
	Item		item;
	uint32	bmw[MAXSCREENS];
	uint32	bmh[MAXSCREENS];
	bool		rslt = FALSE;
	TagArg	taScreenTags[] =
	{ /* Don't change the order of these without changing assumtion below */
	  CSG_TAG_DISPLAYHEIGHT,		(void*)(-1),			// Placeholder value (computed below)
	  CSG_TAG_SCREENCOUNT,			(void*)(-1),			// Placeholder value (computed below)
	  CSG_TAG_SCREENHEIGHT,			(void*)(-1),			// Placeholder value (computed below)
	  CSG_TAG_BITMAPCOUNT,			(void*)1,
	  CSG_TAG_BITMAPWIDTH_ARRAY,	(void*)(-1),			// Placeholder value (computed below)
	  CSG_TAG_BITMAPHEIGHT_ARRAY,	(void*)(-1),			// Placeholder value (computed below)
	  CSG_TAG_SPORTBITS,				(void*)(MEMTYPE_VRAM|MEMTYPE_STARTPAGE),
	  CSG_TAG_DISPLAYTYPE,			(void*)(-1),			// Placeholder value (computed below)
	  CSG_TAG_DONE,					(void*)0,
	};

	// Check for bad arguments
	if ((nScreens > MAXSCREENS) || (nScreens < 1))
		goto EXIT_MyOpenGraphics;

	// Open the graphics folio
	i = OpenGraphicsFolio();
	if (i < 0)
	{
		DIAGNOSE(("Cannot open graphics folio (Error = %ld)\n",i));
		goto EXIT_MyOpenGraphics;
	}

	// Determine NTSC/PAL operating environment
	width = NTSC_WIDTH;				// Assume NTSC
	height = NTSC_HEIGHT;
	diType = NTSC_TYPE;
	if (GrafBase->gf_VBLFreq == PAL_FREQUENCY)
	{
		width = PAL_WIDTH;
		height = PAL_HEIGHT;
		diType = PAL_TYPE;
	}
	else if (GrafBase->gf_VBLFreq != NTSC_FREQUENCY)
	{
		DIAGNOSE(("Unknown screen refresh rate %ld\n", GrafBase->gf_VBLFreq));
		goto EXIT_MyOpenGraphics;
	}

	// Set up the width and height arrays now that we know NTSC/PAL
	for (i=0;i<nScreens;++i)
	{
		bmw[i] = width;
		bmh[i] = height;
	}

	// Now create the screen group, add it to the display mechanism
	sc->sc_nScreens				= (int32) nScreens;
	sc->sc_curScreen				= (int32) 0;
	i = GrafBase->gf_VRAMPageSize;
	sc->sc_nFrameBufferPages	= ( width * height * 2 + (i-1)) / i;
	sc->sc_nFrameByteCount		= sc->sc_nFrameBufferPages * i;

	taScreenTags[0].ta_Arg = (void *)height;					// CSG_TAG_DISPLAYHEIGHT
	taScreenTags[1].ta_Arg = (void *)sc->sc_nScreens;		// CSG_TAG_SCREENCOUNT
	taScreenTags[2].ta_Arg = (void *)height;					// CSG_TAG_SCREENHEIGHT
	taScreenTags[4].ta_Arg = (void *)bmw;						// CSG_TAG_BITMAPWIDTH_ARRAY
	taScreenTags[5].ta_Arg = (void *)bmh;						// CSG_TAG_BITMAPHEIGHT_ARRAY
	taScreenTags[7].ta_Arg = (void *)diType;					// CSG_TAG_DISPLAYTYPE

	item = CreateScreenGroup( &(sc->sc_Screens[0]), taScreenTags );
	if ( item < 0 )
	{
#if DEBUG
		PrintfSysErr(item);
#endif
		DIAGNOSE(("Cannot create screen group\n"));
		return FALSE;
	}
	i = AddScreenGroup( item, NULL );
	if ( item < 0 )
	{
#if DEBUG
		PrintfSysErr((Item)i);
#endif
		DIAGNOSE(("Cannot add screen group\n"));
		return FALSE;
	}


	// Get the information about the screen group and update the ScreenContext structure
	for ( i = 0; i < sc->sc_nScreens; i++ )
	{
		Screen *screen;
		screen = (Screen *)LookupItem( sc->sc_Screens[i] );

		if ( screen == NULL ) 
		{
		    DIAGNOSE(("Cannot locate screens\n"));
		    return FALSE;
		}
		sc->sc_BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		sc->sc_Bitmaps[i] = screen->scr_TempBitmap;
		EnableHAVG(sc->sc_Screens[i]);
		EnableVAVG(sc->sc_Screens[i]);
	}

	rslt = TRUE;

EXIT_MyOpenGraphics:
	return (rslt);
}


/* 
 * This routine does all the main initializations.  It should be 
 * called once, before the program does much of anything.  
 * Returns non-FALSE if all is well, FALSE if error
 */
Boolean
Init( void )
{
	int32	screenNdx;
	int32	errorCode;
	
	if ( ChangeInitialDirectory( NULL, NULL, false ) < 0 )
	{
		DIAGNOSTIC("Cannot set initial working directory ");
		return FALSE;
	}
		
	gVRAMIOReq = GetVRAMIOReq();
	
	// ask the libs for some screens
	if ( !OpenGraphicsNTSCPAL(&gScreenCntx, kScreenCount) )
	{
		kprintf("Cannot open graphics");
		return FALSE;
	}
	if ( !OpenSPORT() )
	{
		kprintf("Cannot open DMA channel");
		return FALSE;
	}
	if ( !OpenMacLink() )
	{
		// complain but don't die, we might be running without the mac
		kprintf("Cannot communicate with Mac");
	}

	// now allocate some memory for the images buffers
	for ( screenNdx = 0; screenNdx < kBufferCount; screenNdx++ )
	{
		gBackPic[screenNdx] = (ubyte *)ALLOCMEM(
									(int32)(gScreenCntx.sc_nFrameByteCount), 
									GETBANKBITS(GrafBase->gf_ZeroPage) 
									| MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);
	
		if ( gBackPic[screenNdx] == NULL )
		{
			kprintf ("unable to allocate gBackPic");
			goto DONE;
		}
		// initialize it to back
		SetVRAMPages( gVRAMIOReq, gBackPic[screenNdx], 0, gScreenCntx.sc_nFrameBufferPages, -1);
	}

	// a little status message and we're done
	kprintf("VRAM page size = %d bytes\n", GrafBase->gf_VRAMPageSize);
	
	return TRUE;

DONE: 
	return FALSE;
}

Boolean
DisplayCurrentScreen( void )
{
	int32	retValue;
	int32	currScreen = gScreenCntx.sc_curScreen;

	// SPORT the background background image into the current screen buffer.  
	CopyVRAMPages( gVRAMIOReq, gScreenCntx.sc_Bitmaps[currScreen]->bm_Buffer, gBackPic[currScreen], 
					gScreenCntx.sc_nFrameBufferPages, -1);
	
	/*
	 * Point the 3DO screen to the screen we just filled. The 3DO
	 *	hardware will now draw this screen to the TV every VBlank.
	 */
	retValue = DisplayScreen(gScreenCntx.sc_Screens[currScreen], 0);
	if ( retValue < 0 )
	{
		kprintf( "DisplayScreen() failed, error=%d\n", retValue );
		return FALSE;
	}
	
	return TRUE;
}

 
void
usage (void)
{
	kprintf (("\nUsage:\n\tSlideShow scriptFileName\n"));
	kprintf (("Where <scriptFileName> is a text file with the names of the images to show\n"));
	kprintf (("the image files to load from the Mac.  The text file \"PALImageList\" is \n"));
	kprintf ("used if none is specified.\n");
	kprintf ("options:\n");
	kprintf ("\t-a<n>\tshow a new image every <n> seconds.\n");
	kprintf( "\t-r file  - Make a recording to file\n" );
	kprintf( "\t-p file  - Playback a recording from file\n" );
	exit(1);
}

