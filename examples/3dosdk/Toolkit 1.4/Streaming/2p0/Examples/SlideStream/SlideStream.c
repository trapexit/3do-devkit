/*******************************************************************************************
 *	File:			SlideStream.c
 *
 *	Contains:		Example of using JoinSubscriber.c to implement a
 *slide show with audio.
 *
 *	Usage:			SlideStream <stream file name>
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	11/29/93		jb		New today
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING "1.0"

#include "Init3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

#include "PlayImageStream.h"

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
 *immediately return from the PlayImageStream() function.
 *********************************************************************************************/
long
myUserFunction (PlayerPtr ctx)
{
  long joybits;

  /* Read the control pad. If the start button is pressed, then
   * return a non-zero value to cause the stream to stop playing.
   */
  joybits = ReadControlPad (0);
  if (joybits & JOYSTART)
    return 1;
  else
    return 0;
}

/*********************************************************************************************
 * Main program
 *********************************************************************************************/
int
main (int argc, char **argv)
{
  long status;

  /* Initialize the library code */
  if (!StartUp ())
    {
      printf ("StartUp() initialization failed!\n");
      exit (0);
    }

  /* Clean the screen */
  EraseScreen (&gScreenContext, gScreenContext.sc_curScreen);

  /* Play the slide show stream */
  status = PlayImageStream (&gScreenContext, argv[1],
                            (PlayerUserFn)myUserFunction, NULL);
  CHECK_DS_RESULT ("PlayImageStream", status);

  if (status == 0)
    printf ("End of stream reached\n");

  else if (status == 1)
    printf ("Stream stopped by control pad input\n");

  exit (0);
}
