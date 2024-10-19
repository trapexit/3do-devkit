/*******************************************************************************************
 *	File:			TestPlayCPAK.c
 *
 *	Contains:		example of using high level Cinepak movie playback
 *routines
 *
 *	Usage:			TestPlayCPAK <Cinepak stream file name>
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	09/14/94		dtc		Version 1.1
 *							Included stdio.h for
 *printf. 08/18/94		dtc		Version 1.1 Call KillJoyPad to
 *free resources allocated by GetJoyPad and replace kprintf with printf.
 *	10/20/93		jb		New today
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.1"

#include "Init3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "JoyPad.h"
#include "PlayCPakStream.h"
#include "stdio.h"

/******************************/
/* Library and system globals */
/******************************/
ScreenContext gScreenContext;

/******************************/
/* Utility routine prototypes */
/******************************/
static Boolean StartUp (void);
static void EraseScreen (ScreenContext *sc, int32 screenNum);

/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
static Boolean
StartUp (void)
{
  gScreenContext.sc_nScreens = 2;

  if (!OpenGraphics (&gScreenContext, 2))
    return false;

  if (!OpenMacLink ())
    ;

  if (!OpenSPORT ())
    return false;

  if ((OpenAudioFolio () != 0))
    return false;

  if ((OpenMathFolio () != 0))
    return false;

  return true;
}

/*********************************************************************************************
 * Routine to erase the current screen.
 *********************************************************************************************/
void
EraseScreen (ScreenContext *sc, int32 screenNum)
{
  Item VRAMIOReq;

  VRAMIOReq = GetVRAMIOReq ();
  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[screenNum]->bm_Buffer, 0,
                sc->sc_nFrameBufferPages, -1);
  DeleteItem (VRAMIOReq);
}

/*********************************************************************************************
 * This function is called each time through the player's main loop and can be
 *used to implement any user interface actions necessary. Returning a zero
 *causes playback to continue. Any other value causes playback to cease and
 *immediately return from the PlayCPakStream() function.
 *********************************************************************************************/
long
myUserFunction (PlayerPtr ctx)
{
#define ONE_SECOND (240) /* audio ticks */
  int32 status;
  JoyPadStatePtr jp;

  /* We don't have much context other than the player context.
   * In fact, our only additional context is a pointer to the
   * joypad state block to interface with the joypad functions.
   */
  jp = (JoyPadStatePtr)ctx->userContext;

  /* Read the control pad #1. If the start button is pressed, then
   * return a non-zero value to cause the stream to stop playing.
   */
  GetJoyPad (jp, 1);

  /* If "the little square blob" is pressed, then
   * stop the stream.
   */
  if (jp->xBtn)
    return 1;

  else if (jp->upArrow)
    {
      /* Go forward in the stream */
      status = DSGoMarker (ctx->messageItem, NULL, ctx->streamCBPtr,
                           (unsigned long)ONE_SECOND, GOMARKER_FORW_TIME);
    }

  else if (jp->downArrow)
    {
      /* Go backward in the stream */
      status = DSGoMarker (ctx->messageItem, NULL, ctx->streamCBPtr,
                           (unsigned long)ONE_SECOND, GOMARKER_BACK_TIME);
    }

  else if (jp->leftArrow)
    {
      /* Rewind the stream to the beginning */
      status = DSGoMarker (ctx->messageItem, NULL, ctx->streamCBPtr,
                           (unsigned long)0, GOMARKER_ABS_TIME);
    }

  else if (jp->startBtn)
    {
      /* Pause / Unpause the stream
       */
      if (ctx->streamCBPtr->streamFlags & STRM_RUNNING)
        {
          /* Note: we don't use the SOPT_FLUSH option when we stop
           * here so that the "A" button acts like a pause/resume
           * key. Using the SOPT_FLUSH option will cause buffered
           * data to be flushed, so that once resumed, any queued
           * audio will vanish. This is not what is usually desired
           * for pause/resume.
           */
          status = DSStopStream (ctx->messageItem, NULL, ctx->streamCBPtr,
                                 SOPT_NOFLUSH);
          CHECK_DS_RESULT ("DSStopStream", status);
        }
      else
        {
          status = DSStartStream (ctx->messageItem, NULL, ctx->streamCBPtr,
                                  SOPT_NOFLUSH);
          CHECK_DS_RESULT ("DSStartStream", status);
        }
    }

  else
    status = 0;

  /* If one of the positioning requests resulted in our requesting a position
   * past the end of the stream or before the beginning, then ignore the
   * error.
   */
  if (status == kDSRangeErr)
    status = 0;

  return status;

#undef ONE_SECOND
}

/*********************************************************************************************
 * Main program
 *********************************************************************************************/
int
main (int argc, char **argv)
{
  long status;
  JoyPadState jpState;

  /* Initialize the library code */
  if (!StartUp ())
    {
      printf ("StartUp() initialization failed!\n");
      exit (0);
    }

  /* Clean the screen */
  EraseScreen (&gScreenContext, gScreenContext.sc_curScreen);

  /* Play the movie */
  status = PlayCPakStream (&gScreenContext, argv[1],
                           (PlayCPakUserFn)myUserFunction, &jpState);
  CHECK_DS_RESULT ("PlayCPakStream", status);

  if (status == 0)
    printf ("End of stream reached\n");

  else if (status == 1)
    printf ("Stream stopped by control pad input\n");

  /* Free resources allocated by GetJoyPad (specifically by InitEventUtility
   * which is called by GetJoyPad).  KillJoypad returns an error code, but
   * we're not checking for it.
   */
  KillJoypad ();

  exit (0);
}
