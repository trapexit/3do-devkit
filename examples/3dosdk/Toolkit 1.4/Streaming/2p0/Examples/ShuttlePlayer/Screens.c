/*******************************************************************************************
 *	File:			Screens.c
 *
 *	Contains:		routines for loading and displaying intro
 *screens
 *
 *	Written by:		Lynn Ackler
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/14/93		lla		Added animation to Kurt's Logo
 *screen 12/10/93		lla		Added 3DO Logo disolve 12/01/93
 *lla		New today
 *
 *******************************************************************************************/

#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "audio.h"
#include "event.h"

#include "DataStreamDebug.h"
#include "PlayCPakStream.h"

#ifdef AUDIOINTRO
#include "PlaySSNDStream.h"
#include "SAudioSubscriber.h"
#endif

#include "ShuttlePlayer.h"
#include "joypad.h"

/******************************/
/* Library and system globals */
/******************************/
extern ScreenContext gScreenContext;

/******************************/
/* Application globals */
/******************************/
extern Boolean fRunStream;
void *menuImagePtr;

#ifdef AUDIOINTRO
/**********************************************************/
/* Context Block for maintaining state in the UI function */
/**********************************************************/
typedef struct AudioCB
{
  JoyPadState jpState; /* JoyPad context */
  long curChannel;     /* Channel on which to send volume and pan messages */
  long volume;
  long pan;
  ulong muteStatusBits; /* Bits represent which channels are currently muted */
  SAudioCtlBlock ctlBlock; /* For sending control messages to the subscriber */

} AudioCB, *AudioCBPtr;

AudioCB UICtx;

#define UI_CALL_BACK_INTERVAL 5
#endif

/******************************/
/* Application externals */
/******************************/
extern void EraseScreen (ScreenContext *sc, int32 screenNum);
extern long Initialize_Balloon_Menu_Stuff (void);
extern long Initialize_Menu_Stuff (void);
extern void WaitAudioClicks (AudioTime clicks);

//------------------------------------------------------------------------------------
//
//		Display help screen
//
//------------------------------------------------------------------------------------

void
ShowHelp (ScreenContext *gScreenContextPtr)
{
  JoyPadState joypadState, *joypadStatePtr = &joypadState;
  long current_screen;

  GetJoyPad (joypadStatePtr, 1);

  //	remember the current screen
  current_screen = gScreenContextPtr->sc_curScreen;

  //	show the help screen
  DisplayScreen (gScreenContextPtr->sc_Screens[helpScreen], 0);

  while (joypadStatePtr->cBtn)
    GetJoyPad (joypadStatePtr, 1);

  //	restore current screen to the previous screen

  DisplayScreen (gScreenContextPtr->sc_Screens[current_screen], 0);
}

//------------------------------------------------------------------------------------
//
//		return the correct image count
//
//------------------------------------------------------------------------------------

char *
imageFile (long selection)
{
  switch (selection)
    {
    case 0:
      return "$boot/Player_Screens/Help.img";
    case 1:
      return "$boot/Player_Screens/3DO_Studio_Logo.img";
    case 2:
      return "$boot/Player_Screens/3DO_Studio.anim";
    case 3:
      return "$boot/Player_Screens/Menu.img";
    default:
      return "Bad file name for screen image";
    }
}

void
UnifyAnimation (ANIM *pAnim)
{
  int i;
  CCB *theCCB;

  theCCB = pAnim->pentries[0].af_CCB;
  for (i = 0; i < pAnim->num_Frames; i++)
    {
      pAnim->pentries[i].af_CCB = theCCB;
    }
}

//------------------------------------------------------------------------------------
//
//		Go through the intro sequence and exit with the menScreen
//loaded
//
//------------------------------------------------------------------------------------

long
ShowIntro (ScreenContext *gScreenContextPtr)
{
  long status = 0;
  void *imagePtr, *nextImagePtr;
  JoyPadState joypadState, *joypadStatePtr = &joypadState;
  Item VRAMIOReq;
  CCB *pcVideo = NULL;
  ANIM *aVideo;
  long nFrames;

  EraseScreen (gScreenContextPtr, 0);
  EraseScreen (gScreenContextPtr, 1);
  EraseScreen (gScreenContextPtr, 2);

  //------------------------------------------------------------------------------------
  //
  // 		load the Help Image into the "helpScreen"
  //
  //------------------------------------------------------------------------------------

  gScreenContextPtr->sc_curScreen = helpScreen;
  FadeToBlack (gScreenContextPtr, 10);

  imagePtr = LoadImage (imageFile (helpImage), NULL, NULL,
                        gScreenContextPtr); // MEMTYPE_VRAM
  VRAMIOReq = GetVRAMIOReq ();
  status = CopyVRAMPages (
      VRAMIOReq,
      gScreenContextPtr->sc_Bitmaps[gScreenContextPtr->sc_curScreen]
          ->bm_Buffer,
      imagePtr, gScreenContextPtr->sc_nFrameBufferPages, -1);

  //------------------------------------------------------------------------------------
  //
  //		load the Cinema Image into the "movieScreen"
  //
  //------------------------------------------------------------------------------------

  //------------------------------------------------------------------------------------
  //
  //		clear the joypad from multiple impatient presses
  //
  //------------------------------------------------------------------------------------

  GetJoyPad (joypadStatePtr, 1);

  gScreenContextPtr->sc_curScreen = movieScreen;

  aVideo = LoadAnim (imageFile (Studio_Animation), MEMTYPE_CEL);
  UnifyAnimation (aVideo);
  pcVideo = GetAnimCel (aVideo, 0);

  nextImagePtr = LoadImage (imageFile (Studio_Logo), NULL, NULL,
                            gScreenContextPtr); // MEMTYPE_VRAM

  VRAMIOReq = GetVRAMIOReq ();
  status = CopyVRAMPages (
      VRAMIOReq,
      gScreenContextPtr->sc_Bitmaps[gScreenContextPtr->sc_curScreen]
          ->bm_Buffer,
      nextImagePtr, gScreenContextPtr->sc_nFrameBufferPages, -1);

  gScreenContextPtr->sc_curScreen = menuScreen;

  //------------------------------------------------------------------------------------
  //
  // 		show the Studio Image
  //
  //------------------------------------------------------------------------------------

  imagePtr = nextImagePtr;
  gScreenContextPtr->sc_curScreen = movieScreen;
  FadeToBlack (gScreenContextPtr, 10);

  DisplayScreen (
      gScreenContextPtr->sc_Screens[gScreenContextPtr->sc_curScreen], 0);
  FadeFromBlack (gScreenContextPtr, 60);

  //------------------------------------------------------------------------------------
  //
  // 		now play the animation
  //
  //------------------------------------------------------------------------------------

  nFrames = aVideo->num_Frames;
  while (nFrames)
    {
      GetJoyPad (joypadStatePtr, 1);
      WaitAudioClicks (10);
      nFrames--;
      if (nFrames)
        {
          (void)GetAnimCel (aVideo, 1 << 16);
          VRAMIOReq = GetVRAMIOReq ();
          status = CopyVRAMPages (
              VRAMIOReq,
              gScreenContextPtr->sc_Bitmaps[gScreenContextPtr->sc_curScreen]
                  ->bm_Buffer,
              nextImagePtr, gScreenContextPtr->sc_nFrameBufferPages, -1);
          status = DisplayScreen (
              gScreenContextPtr->sc_Screens[gScreenContextPtr->sc_curScreen],
              0);
          DrawScreenCels (
              gScreenContextPtr->sc_Screens[gScreenContextPtr->sc_curScreen],
              pcVideo);
        }
      else
        {
          aVideo->cur_Frame = 0;
        }
      if (joypadStatePtr->xBtn)
        break;
    }

  //------------------------------------------------------------------------------------
  //
  // 		load the menu image and initialize menu stuff
  //
  //------------------------------------------------------------------------------------

  gScreenContextPtr->sc_curScreen = menuScreen;
  nextImagePtr = LoadImage (imageFile (menuImage), NULL, NULL,
                            gScreenContextPtr); // MEMTYPE_VRAM

  VRAMIOReq = GetVRAMIOReq ();
  status = CopyVRAMPages (
      VRAMIOReq,
      gScreenContextPtr->sc_Bitmaps[gScreenContextPtr->sc_curScreen]
          ->bm_Buffer,
      nextImagePtr, gScreenContextPtr->sc_nFrameBufferPages, -1);

  gScreenContextPtr->sc_curScreen = movieScreen;
  Initialize_Menu_Stuff ();

  //------------------------------------------------------------------------------------
  //
  // 		erase the Cinema Image, clear its screen and unload the image
  //
  //------------------------------------------------------------------------------------

  FadeToBlack (gScreenContextPtr, 10);
  EraseScreen (gScreenContextPtr, gScreenContextPtr->sc_curScreen);
  UnloadImage (imagePtr);
  UnloadAnim (aVideo);

  //------------------------------------------------------------------------------------
  //
  // 		show the menu Image
  //
  //------------------------------------------------------------------------------------

  menuImagePtr = imagePtr = nextImagePtr;
  gScreenContextPtr->sc_curScreen = menuScreen;
  FadeToBlack (gScreenContextPtr, 10);

  //------------------------------------------------------------------------------------
  //
  //	      Clear the movie screen
  //
  //------------------------------------------------------------------------------------

  //	gScreenContext.sc_curScreen = movieScreen;
  //	FadeToBlack ( gScreenContextPtr, 10 );
  //	EraseScreen ( gScreenContextPtr, gScreenContext.sc_curScreen );
  FadeFromBlack (gScreenContextPtr, 10);

  return status;
}
