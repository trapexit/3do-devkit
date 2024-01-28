/*******************************************************************************************
 *	File:			Menus.c
 *
 *	Contains:		routines for manuvering about the menu screen
 *
 *	Written by:		Lynn Ackler
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/10/93		lla		Used Greg's menu screen and hilite
 *cel 12/01/93		lla		New today
 *
 *******************************************************************************************/

#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"

#include "ShuttlePlayer.h"
#include "joypad.h"

/******************************/
/* Library and system globals */
/******************************/
extern void *menuImagePtr;
extern Item VBLIOReq;
extern void ShowHelp (ScreenContext *gScreenContextPtr);
extern void WaitAudioClicks (AudioTime clicks);
extern char *movie_name (long selection);

static CCB *title[N_TITLES + 1];
long n_titles;

//-----------------------------------------------------------------------------
//
//		Restores the menu image after hiliting or what not
//
//-----------------------------------------------------------------------------

long
RemoveTitle (ScreenContext *gScreenContextPtr)
{
  Item VRAMIOReq;
  long status;

  VRAMIOReq = GetVRAMIOReq ();
  status = CopyVRAMPages (
      VRAMIOReq, gScreenContextPtr->sc_Bitmaps[menuScreen]->bm_Buffer,
      menuImagePtr, gScreenContextPtr->sc_nFrameBufferPages, -1);
  if (status)
    {
      kprintf ("Unable to display VRAM pages");
      return (status);
    }
  status = DisplayScreen (gScreenContextPtr->sc_Screens[menuScreen], 0);
  if (status)
    {
      kprintf ("Unable to display screen");
      return (status);
    }
  return (0);
}

//-----------------------------------------------------------------------------
//
//		Returns the appropriate file name for a hilite cel
//
//-----------------------------------------------------------------------------

char *
cel_name (void)
{
  return ("$boot/Player_Screens/Hilite.cel");
}

//-----------------------------------------------------------------------------
//
//		loads all of the cels associated with a particular Woody menus
//
//-----------------------------------------------------------------------------

long
Initialize_Menu_Stuff ()
{
  long i;
  long hilite_xPos[6], hilite_yPos[6];
  Item temp_item;
  int32 status;

  i = 0;

  hilite_xPos[0] = hilite_xPos[1] = hilite_xPos[2] = hilite_xPos[3]
      = hilite_xPos[4] = hilite_xPos[5] = 47 << 16;
  hilite_yPos[0] = 65 << 16;
  hilite_yPos[1] = 90 << 16;
  hilite_yPos[2] = 115 << 16;
  hilite_yPos[3] = 140 << 16;
  hilite_yPos[4] = 165 << 16;
  hilite_yPos[5] = 190 << 16;

  while (i < N_TITLES)
    {
      temp_item = OpenDiskFile (movie_name (i));
      if (temp_item < 0)
        break;
      status = CloseDiskFile (temp_item);
      if (status)
        kprintf ("Error - CloseDiskFile: %ld\n", status);

      if ((title[i] = LoadCel (
               "$boot/Player_Screens/Hilite.cel" /*cel_name ()*/, MEMTYPE_CEL))
          == NULL)
        break;

      title[i]->ccb_NextPtr = NULL;
      title[i]->ccb_Flags |= CCB_LAST;
      title[i]->ccb_XPos = hilite_xPos[i];
      title[i]->ccb_YPos = hilite_yPos[i];
      i++;
    }
  n_titles = i - 1;
  if (n_titles == 0)
    return -1;
  else
    return (n_titles);
}

//-----------------------------------------------------------------------------
//
//		Hilites the appropriate selection or selections
//
//-----------------------------------------------------------------------------

long
ShowTitle (long selection, ScreenContext *gScreenContextPtr)
{
  long status;

  status = DrawCels (gScreenContextPtr->sc_BitmapItems[menuScreen],
                     title[selection]);
  if (status)
    {
      kprintf ("Unable to draw cels on screen");
      return (status);
    }

  status = DisplayScreen (gScreenContextPtr->sc_Screens[menuScreen], 0);
  if (status)
    {
      kprintf ("Unable to display screen");
      return (status);
    }
  return (0);
}

//-----------------------------------------------------------------------------
//
//		Tracks cursor movement through the menu and returns the item
//selected
//
//-----------------------------------------------------------------------------

long
SelectFilm (long last_selection, ScreenContext *gScreenContextPtr)
{
  JoyPadState joypadState, *joypadStatePtr = &joypadState;
  long current_selection;
  long last_button_time;

  // flush the joy pad

  GetJoyPad (joypadStatePtr, 1);

  /************************************************/
  /*		set timeout threshold for autoplay		*/
  /************************************************/

  last_button_time = GetAudioTime ();

  current_selection = last_selection;

  ShowTitle (current_selection, gScreenContextPtr);

  while (true)
    {
      GetJoyPad (joypadStatePtr, 1);

      if (joypadStatePtr->aBtn)
        return (current_selection <= n_titles ? current_selection : 0);

      else if (joypadStatePtr->downArrow)
        {
          last_button_time = GetAudioTime ();
          RemoveTitle (gScreenContextPtr);
          WaitAudioClicks (25);
          current_selection = (current_selection + 1) % (n_titles + 1);
          ShowTitle (current_selection, gScreenContextPtr);
          WaitAudioClicks (50);
        }

      else if (joypadStatePtr->upArrow)
        {
          last_button_time = GetAudioTime ();
          RemoveTitle (gScreenContextPtr);
          WaitAudioClicks (25);
          if (current_selection == 0)
            current_selection = n_titles;
          else
            current_selection = (current_selection - 1) % (n_titles + 1);
          ShowTitle (current_selection, gScreenContextPtr);
          WaitAudioClicks (50);
        }
      else if (joypadStatePtr->cBtn)
        ShowHelp (gScreenContextPtr);
    }
}

//------------------------------------------------------------------------------------
//
//		Display menu screen and get movie selection
//
//------------------------------------------------------------------------------------

long
GetSelection (ScreenContext *gScreenContextPtr, long LastSelection)
{
  JoyPadState joypadState, *joypadStatePtr = &joypadState;

  gScreenContextPtr->sc_curScreen = menuScreen;
  DisplayScreen (
      gScreenContextPtr->sc_Screens[gScreenContextPtr->sc_curScreen], 0);
  FadeFromBlack (gScreenContextPtr, 10);

  //------------------------------------------------------------------------------------
  //
  //		clear the joypad from multiple impatient presses
  //
  //------------------------------------------------------------------------------------

  GetJoyPad (joypadStatePtr, 1);

  LastSelection = SelectFilm (LastSelection, gScreenContextPtr);

  if (LastSelection > N_STREAMS)
    LastSelection = 0;

  FadeToBlack (gScreenContextPtr, 30);
  return (LastSelection);
}
