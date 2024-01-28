/*
        File:		aaPlayer.c

        Contains:	simple 3DO 'edge-o-matic' cel/anim player

        Written by:	eric carlson

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <2+>	 8/10/93	JAY		change calling sequence
   to LoadAnim for new Lib3DO.lib <2>	 7/31/93	JAY		add IO
   parameters to SetVRAMPages() and CopyVRAMPages() for Dragon Tail release <1>
   6/25/93	JAY		first checked in 6/1/93		ec
   Updated for dragon. 4/1/93		ec		New today

        To Do:
*/

/*******************************************************************************************
 *	Usage:			aaPlayer <imagefile> <celfile
 *
 *					"A" button:		and up or down arrow
 *changes animation speed
 *
 *					"B" button:		and arrows changes cel
 *width and height
 *
 *					"C" button:		enables or disables
 *drawing of the alpha channel cel
 *
 *					"Start" button:	exits the program
 *
 *******************************************************************************************/

#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "filefunctions.h"

/* *************************************************************************
 * ***                   ***************************************************
 * ***  Our Definitions  ***************************************************
 * ***                   ***************************************************
 * *************************************************************************
 */
#define VERSION "1.0"

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

void CopyBackPic (Bitmap *bitmap);
Boolean DoJoystick (CCB *ccb1, CCB *ccb2);
Boolean Init (void);
Boolean InitBackPic (char *fileName, ScreenContext *sc);
Boolean OpenMacLink (void);
Boolean OpenSPORT (void);
void PutNumXY (int32 secCount, int32 x, int32 y);
void ShowCelTiming (void);

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */

ScreenContext gScreenContext;

Boolean gReverse = FALSE;

long gXPos = 0;
long gYPos = 0;

int32 gXMovePt;
int32 gYMovePt;

ubyte *gBackPic = NULL;

int32 gPauseBeforeNextCell = 6;
Boolean gDrawAACel = true;

Item VRAMIOReq;

TagArg ScreenTags[] = {
  /* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
  /* Kludgy code below presumes that SPORTBITS is the first entry */
  CSG_TAG_SPORTBITS, (void *)0, CSG_TAG_SCREENCOUNT, (void *)2, CSG_TAG_DONE, 0
};

/* *************************************************************************
 * ***                 *****************************************************
 * ***  Main Routines  *****************************************************
 * ***                 *****************************************************
 * *************************************************************************
 */
Boolean
StartUp (void)
{
  // set the initial directory...
  if (ChangeInitialDirectory (NULL, NULL, false) < 0)
    {
      DIAGNOSTIC ("Cannot set initial working directory ");
      return FALSE;
    }

  VRAMIOReq = GetVRAMIOReq ();

  // initialize the various managers.  The only one we really NEED is the
  // graphics folio
  if (!OpenGraphics (&gScreenContext, 2))
    {
      DIAGNOSTIC ("Cannot initialize graphics");
      return false;
    }

  if (!OpenSPORT ())
    {
      DIAGNOSTIC ("Cannot open DMA channel");
    }

  if (!OpenMacLink ())
    {
      DIAGNOSTIC ("Cannot communicate with Mac");
    }

  if (!OpenAudio ())
    {
      DIAGNOSTIC ("Cannot initialize audio");
    }

  if (OpenMathFolio () < 0)
    {
      DIAGNOSTIC ("Cannot initialize math folio");
    }

  return TRUE;
}

void
Usage (char *progName, Boolean errorCond)
{
  if (errorCond)
    kprintf ("Parameter error:\n");

  kprintf ("%s %s %s\n\n", progName, VERSION, kBuildDate);
  kprintf ("Usage: %s <imagefile> <celfile>\n", progName);
  kprintf ("A button and up or down arrow changes animation speed.\n");
  kprintf ("B button and arrows changes cel width and height.\n");
  kprintf (
      "C button enables or disables drawing of the alpha channel cel.\n\n");

  if (errorCond)
    exit (0);
}

int
main (int argc, char *argv[])
/* This program initializes itself, loads a background picture, then in
 * its main loop does double-buffer display/rendering of the background
 * pic and a cel (or cels) while reacting to input from the joystick.
 *
 * The offscreen buffer:
 *   - is initialized by having the background pic SPORT'ed into it
 *   - has an animating cel rendered onto it
 */
{
  char *progName, fileName[64], animName[64];
  int32 retvalue;
  long *ccbBuffer = NULL;
  int32 fnSize;
  CCB *alphaCCB, *imageCCB;
  CCB *realCCB;
  ANIM *animRec;
  int32 errorVal, waitCount = gPauseBeforeNextCell;

  /*	Extract the program name, imagefile name, and celfile name from
   *	the arguments passed into the program.
   *	NOTE: the imagefile and celfile names are string copied into
   *	globals. This assures that the names will start on a longword
   *	boundry. Any strings that are passed to the O/S must start on
   *	a longword boundry.
   */
  progName = *argv++;

  if (argc <= 2)
    Usage (progName, true);

  fnSize = strlen (argv[0]);
  if (fnSize <= 0)
    Usage (progName, true);

  strcpy (fileName, argv[0]);
  fnSize = strlen (argv[1]);
  if (fnSize <= 0)
    Usage (progName, true);
  strcpy (animName, argv[1]);

  Usage (progName, false);

  /*	Creates the CreateScreenGroup and inits a SPORT device to write
   *	directly to the frame buffer and a Mac Link to read files from
   *	the Macintosh.
   */
  if (NOT StartUp ())
    goto DONE;

  /* Load the background image and save a copy of it in page aligned VRAM
   */
  InitBackPic (fileName, &gScreenContext);

  /* Open the anim file, setup the CCBs and pass back an Anim Record.
   */
  if ((animRec = LoadAnim (animName, MEMTYPE_CEL)) == 0)
    {
      kprintf ("Error %d in LoadAnim call\n", errorVal);
      exit (0);
    }

  /*	Check to see if the anim file actually contains any cels.
   */
  if (animRec->num_Frames > 0)
    kprintf ("%d cels were found in %s\n", animRec->num_Frames, animName);
  else
    {
      kprintf ("animRec->num_Frames was zero !!!\n");
      goto DONE;
    }

  /* Grab the first frame for the image and the alpha mask, link them together
   */
  imageCCB = GetAnimCel (animRec, Convert32_F16 (1));
  alphaCCB = GetAnimCel (animRec, Convert32_F16 (1));
  LinkCel (alphaCCB, imageCCB);
  LAST_CEL ((imageCCB));

  while (TRUE)
    {
      /* React to the joystick */
      if (NOT DoJoystick (imageCCB, alphaCCB))
        goto DONE;

      // if we've paused the minimum amount of time, show the next frame of
      //  the animation
      if (++waitCount >= gPauseBeforeNextCell)
        {
          /* Grab the next cel in the animation.  An 'edge-o-matic' file has
           * the image cel first, followed by the edge cel - grab one of each
           */
          realCCB = GetAnimCel (animRec, Convert32_F16 (1));
          imageCCB->ccb_SourcePtr = realCCB->ccb_SourcePtr;
          imageCCB->ccb_PLUTPtr = realCCB->ccb_PLUTPtr;
          realCCB = GetAnimCel (animRec, Convert32_F16 (1));
          alphaCCB->ccb_SourcePtr = realCCB->ccb_SourcePtr;
          alphaCCB->ccb_PLUTPtr = realCCB->ccb_PLUTPtr;

          waitCount = 0;
        }

      /* skip or enable the alpha channel cel according to our instructions
       */
      if (gDrawAACel)
        UNSKIP_CEL (alphaCCB);
      else
        SKIP_CEL (alphaCCB);

      /* SPORT the background image into the current buffer.
       */
      CopyBackPic (gScreenContext.sc_Bitmaps[gScreenContext.sc_curScreen]);

      //	Draw the current frame from the animation.
      ShowCelTiming ();
      retvalue = DrawCels (
          gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen],
          alphaCCB);
      if (retvalue < 0)
        kprintf ("DrawCels() failed, error=%d\n", retvalue);

      /*	Show the new image...
       */
      retvalue = DisplayScreen (
          gScreenContext.sc_Screens[gScreenContext.sc_curScreen], 0);
      if (retvalue < 0)
        {
          kprintf ("DisplayScreen() failed, error=%d\n", retvalue);
          goto DONE;
        }

      /*	Toggle to the other screen for our next frame of animation.
       */
      gScreenContext.sc_curScreen = 1 - gScreenContext.sc_curScreen;
    }

DONE:
  FadeToBlack (&gScreenContext, 120);
  kprintf ("\n%s has completed!\n", progName);
  return ((int)retvalue);
}

/*
 * 	Returns non-FALSE if user didn't press START, else returns FALSE to
 * quit
 */
Boolean
DoJoystick (CCB *ccb1, CCB *ccb2)
{
  Boolean retvalue;
  long joybits;
  Point q[4];
  static int32 gOldJoy;

  retvalue = TRUE;

  joybits = ReadControlPad (JOYMOVE | JOYBUTTONS);

  /* If the user has pressed the START button on the joystick
   * then return false so we can quit
   */
  if (joybits & JOYSTART)
    {
      retvalue = FALSE;
      goto DONE;
    }

  /* With FIRE A adjust animation delay */
  if ((joybits & JOYFIREA))
    {
      if (joybits == gOldJoy)
        goto DONE;

      if (joybits & JOYDOWN)
        {
          ++gPauseBeforeNextCell;
        }
      else if (joybits & JOYUP)
        {
          if (gPauseBeforeNextCell > 0)
            --gPauseBeforeNextCell;
        }

      goto DONE;
    }

  /* With FIRE B stretch and shrink the cel */
  if (joybits & JOYFIREB)
    {
      if (joybits & JOYRIGHT)
        ++gXMovePt;

      if (joybits & JOYLEFT)
        --gXMovePt;

      if (joybits & JOYUP)
        --gYMovePt;

      if (joybits & JOYDOWN)
        ++gYMovePt;

      SetQuad (q, gXPos, gYPos, gXPos + ccb1->ccb_Width + gXMovePt,
               gYPos + ccb1->ccb_Height + gYMovePt);
      MapCel (ccb1, q);
      MapCel (ccb2, q);

      goto DONE;
    }

  // button C toggles drawing of the alpha cel
  if (joybits & JOYFIREC)
    {
      if (joybits == gOldJoy)
        goto DONE;

      gDrawAACel = !gDrawAACel;
      if (gDrawAACel)
        kprintf ("Drawing anim WITH alpha channel.\n");
      else
        kprintf ("Drawing anim WITHOUT alpha channel.\n");

      goto DONE;
    }

  // arrow buttons move the cel
  if (joybits & JOYMOVE)
    {
      if (joybits & JOYUP)
        {
          if (--gYPos < 0)
            gYPos = 0;
        }
      else if (joybits & JOYDOWN)
        {
          if (++gYPos > DISPLAY_HEIGHT)
            gYPos = DISPLAY_HEIGHT;
        }
      if (joybits & JOYLEFT)
        {
          gReverse = FALSE;
          if (--gXPos < 0)
            gXPos = 0;
        }
      else if (joybits & JOYRIGHT)
        {
          gReverse = TRUE;
          if (++gXPos > DISPLAY_WIDTH)
            gXPos = DISPLAY_WIDTH;
        }

      SetQuad (q, gXPos, gYPos, gXPos + ccb1->ccb_Width + gXMovePt,
               gYPos + ccb1->ccb_Height + gYMovePt);
      MapCel (ccb1, q);
      MapCel (ccb2, q);
    }

DONE:
  gOldJoy = joybits;

  return (retvalue);
}

/* Allocates the gBackPic buffer.  If a fileName is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no fileName is specified, this
 * routine merely clears the gBackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
Boolean
InitBackPic (char *fileName, ScreenContext *scp)
{
  Boolean retvalue;

  retvalue = FALSE;

  gBackPic = (ubyte *)ALLOCMEM ((int)(scp->sc_nFrameByteCount),
                                GETBANKBITS (GrafBase->gf_ZeroPage)
                                    | MEMTYPE_STARTPAGE | MEMTYPE_VRAM
                                    | MEMTYPE_CEL);

  if (NOT gBackPic)
    {
      DIAGNOSTIC ("unable to allocate gBackPic");
      goto DONE;
    }

  SetVRAMPages (VRAMIOReq, gBackPic, 0, scp->sc_nFrameBufferPages, -1);

  if (LoadImage (fileName, gBackPic, (VdlChunk **)NULL, scp) == 0)
    DIAGNOSTIC ("LoadImage failed");

  retvalue = TRUE;

DONE:
  return (retvalue);
}

/*
 *	Copy the background image into the screen's bitmap
 */
void
CopyBackPic (Bitmap *bitmap)
{
  CopyVRAMPages (VRAMIOReq, bitmap->bm_Buffer, gBackPic,
                 gScreenContext.sc_nFrameBufferPages, -1);
}

/*********************************************************************************************
 * Show cel frame timing
 *********************************************************************************************/
static Item gTimerRequest;
static IOInfo gTimerInfo;

/*
 * start up a timer for ourselves
 */
void
InitInstrumentation (void)
{
  Item timerDevice;
  IOReq *timerReq;
  TagArg ioTags[] = {
    CREATEIOREQ_TAG_DEVICE,
    0,
    TAG_END,
    0,
  };

  // Timer initialization
  timerDevice = OpenItem (
      FindNamedItem (MKNODEID (KERNELNODE, DEVICENODE), "timer"), 0);
  if (timerDevice < 0)
    {
      PrintfSysErr (timerDevice);
      return;
    }
  ioTags[0].ta_Arg = (void *)timerDevice;
  gTimerRequest = CreateItem (MKNODEID (KERNELNODE, IOREQNODE), ioTags);

  if (gTimerRequest < 0)
    {
      PrintfSysErr (gTimerRequest);
      exit (0);
    }
  timerReq = (IOReq *)LookupItem (gTimerRequest);
  gTimerInfo.ioi_Unit = 1;
}

/*
 * Get the current second count from our private timer
 */
uint32
CurrentSeconds (void)
{
  static int32 gTimerHasStarted = 0;
  struct timeval tv;

  // has the timer been initialized???
  if (gTimerHasStarted == 0)
    {
      InitInstrumentation ();
      gTimerHasStarted = 1;
    }

  /* Read elapsed time */
  gTimerInfo.ioi_Command = CMD_READ;
  gTimerInfo.ioi_Recv.iob_Buffer = &tv;
  gTimerInfo.ioi_Recv.iob_Len = sizeof (tv);
  DoIO (gTimerRequest, &gTimerInfo);

  // we want seconds...
  return tv.tv_sec;
}

/*
 * draw the current timer to the screen
 */
void
PutNumXY (int32 secCount, int32 x, int32 y)
{

  static char gTimeStr[20];
  static int32 gLastSecCount = -1;
  GrafCon localGrafCon;

  // don't reset the string unless we have a new time, just draw the last
  //  string we made
  if (secCount != gLastSecCount)
    {
      sprintf (gTimeStr, "frame/sec: %2lu", secCount);
      gLastSecCount = secCount;
    }

  localGrafCon.gc_PenX = x;
  localGrafCon.gc_PenY = y;
  DrawText8 (&localGrafCon,
             gScreenContext.sc_BitmapItems[gScreenContext.sc_curScreen],
             gTimeStr);
}

/*
 * Calculate and show timing for a cinepak stream
 */
void
ShowCelTiming ()
{
  static int32 gCallCount = 0;
  static int32 gPrevTime = -1;
  static int32 gFramesPerSec;
  int32 currTime;

  // increment the call count, check the current time
  ++gCallCount;
  currTime = CurrentSeconds ();

  // has a second passed? if so, update the number of times
  //  that we've been through this proc in the last second
  if (currTime != gPrevTime)
    {
      gFramesPerSec = gCallCount;
      gPrevTime = currTime;
      gCallCount = 0;
    }

  // draw the last second's average count again...
  PutNumXY (gFramesPerSec, 20, 200);
}
