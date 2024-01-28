
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: drumbox.c,v 1.14 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/drumbox
|||	drumbox - Simple single-pattern drum machine.
|||
|||	  Synopsis
|||
|||	    effectshandler
|||
|||	  Description
|||
|||	    Utility code for basic management of mixers and sound effects.
|||
|||	    Typical usage scenario:
|||
|||	    During program initialization:
|||
|||	    * Open the audio folio.
|||
|||	    * Call ehInitEffectsHandler() to allocate and initialize the
effects
|||	      handler.
|||
|||	    * Call ehNewMixerInfo() to load a mixer and initialize a mixerInfo
|||	      structure.
|||
|||	    * Call ehLoadSoundEffect() to load a sample as a sound effect and
initialize
|||	      a sampleInfo structure.
|||
|||	    * Optionally call ehSetChannelLevels() for each sound effect.
|||
|||	    As needed during execution:
|||
|||	    *  Call ehSetChannelLevels() for a given sound effect to adjust its
player's
|||	      output levels. Issue calls which control the sound effects player
|||	      instrument (the SampleInfo's si_Player structure member).
|||
|||	    During program termination:
|||
|||	    * Call ehDisposeSampleInfo() for each sound effect.
|||
|||	    * Call ehDisposeMixerInfo() for the mixer.
|||
|||	    * Call ehKillEffectsHandler(). Close the audio folio.
|||
|||	    Depending on program requirements, you may want to load one or more
sound
|||	    effects and assemble their player apparati but not immediately
connect
|||	    them to a mixer. This is accomplished by calling ehNewSampleInfo()
and
|||	    ehSetupSamplePlayer(). When you want to connect the player to the
mixer,
|||	    call ehConnectSamplePlayer().
|||
|||	  Caveats
|||
|||	    This program is not a complete sound management facility.
|||
|||	  Associated Files
|||
|||	    effectshandler.c effectshandler.h
|||
|||	  Location
|||
|||	    examples/ExamplesLib
|||
**/

#include "audio.h"
#include "debug.h"
#include "event.h"
#include "graphics.h"
#include "init3do.h"
#include "kernel.h"
#include "mem.h"
#include "music.h"
#include "nodes.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "types.h"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) /* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      ERR (("Failure in %s: $%x\n", name, val));                              \
      PrintfSysErr (Result);                                                  \
      goto cleanup;                                                           \
    }

#define VERSION "1.0"

#define DISPLAY_WIDTH (320) /* What about PAL ? */
#define DISPLAY_HEIGHT (240)
#define DEFAULT_VELOCITY (64)

#define CURBITMAPITEM sc->sc_BitmapItems[sc->sc_curScreen]

#define DRB_NUM_ROWS (8)
#define DRB_NUM_COLUMNS (16)

typedef struct DrumLine
{
  Item drl_Instrument;
  uint8 drl_Velocities[DRB_NUM_COLUMNS];
} DrumLine;

typedef struct DrumBox
{
  uint32 drb_Flags;
  int32 drb_Row; /* Cursor row and column. */
  int32 drb_Column;
  int32 drb_TopY; /* Position of grid. */
  int32 drb_LeftX;
  int32 drb_DeltaX; /* Width and height of grid cells. */
  int32 drb_DeltaY;
  struct DrumLine drb_Lines[DRB_NUM_ROWS]; /* One row/line of drum hits. */
  struct ScoreContext *drb_ScoreContext;
  Item drb_Cue;           /* For use with timer. */
  int32 drb_Toggle;       /* Are notes on or off? */
  int32 drb_CurrentIndex; /* Currently playing column. */
  uint32 drb_CurTime;     /* Internal time clock. */
  int32 drb_Duration;     /* Time between notes/2 */
} DrumBox;

Err drbDrawAboutScreen (ScreenContext *sc);
Err drbProcessUserInput (ScreenContext *sc, DrumBox *drb, uint32 Joy);
Err drbInteractiveLoop (ScreenContext *sc, DrumBox *drb);
Err drbDrawNextScreen (ScreenContext *sc, DrumBox *drb);
Err drbDrawRect (Item Bitmap, GrafCon *GConPtr, int32 LeftX, int32 Topy,
                 int32 RightX, int32 BottomY);
Err drbDrawGrid (ScreenContext *sc, DrumBox *drb);

/*****************************************************************
** Terminate sound system.
*****************************************************************/
Err
drbTermSound (DrumBox *drb)
{
  int32 Result = 0;
  if (drb->drb_Cue)
    DeleteItem (drb->drb_Cue);
  if (drb->drb_ScoreContext)
    {
      UnloadPIMap (drb->drb_ScoreContext);
      TermScoreMixer (drb->drb_ScoreContext);
      DeleteScoreContext (drb->drb_ScoreContext);
    }
  return Result;
}

/*****************************************************************
** Initialize sound system.
*****************************************************************/
int32
drbInitSound (DrumBox *drb, char *mapfile)
{
  int32 Result;

  Result = -1;

  /* Create a context for interpreting a MIDI score and tracking notes. */
  drb->drb_ScoreContext = CreateScoreContext (128);
  if (drb->drb_ScoreContext == NULL)
    {
      Result = AF_ERR_NOMEM;
      goto cleanup;
    }

  Result = InitScoreMixer (drb->drb_ScoreContext, "mixer8x2.dsp", DRB_NUM_ROWS,
                           ((MAXDSPAMPLITUDE * 2) / DRB_NUM_ROWS));
  CHECKRESULT (Result, "InitScoreMixer");

  /* Load Instrument Templates from disk and fill Program Instrument Map. */
  Result = LoadPIMap (drb->drb_ScoreContext, mapfile);
  CHECKRESULT (Result, "LoadPIMap");

  /* Create cue for use with timer. */
  drb->drb_Cue = CreateItem (MKNODEID (AUDIONODE, AUDIO_CUE_NODE), NULL);
  CHECKRESULT (drb->drb_Cue, "CreateItem Cue");

  return Result;

cleanup:
  drbTermSound (drb);
  return Result;
}

/*****************************************************************
** Turn on all the selected notes in a column.
*****************************************************************/
Err
drbColumnOn (DrumBox *drb, int32 ColumnIndex)
{
  int32 Vel, i;
  int32 Result;

  Result = 0;
  for (i = 0; i < DRB_NUM_ROWS; i++)
    {
      Vel = drb->drb_Lines[i].drl_Velocities[ColumnIndex];
      if (Vel > 0) /* Is it on? */
        {
          Result = StartScoreNote (drb->drb_ScoreContext, i, 60, Vel);
          CHECKRESULT (Result, "StartScoreNote");
        }
    }
cleanup:
  return Result;
}
/*****************************************************************
** Turn off all the notes in a column.
*****************************************************************/
Err
drbColumnOff (DrumBox *drb, int32 ColumnIndex)
{
  int32 Vel, i;
  int32 Result;

  Result = 0;
  for (i = 0; i < DRB_NUM_ROWS; i++)
    {
      Vel = drb->drb_Lines[i].drl_Velocities[ColumnIndex];
      if (Vel > 0)
        {
          Result = ReleaseScoreNote (drb->drb_ScoreContext, i, 60, Vel);
          CHECKRESULT (Result, "ReleaseScoreNote");
        }
    }
cleanup:
  return Result;
}

/*****************************************************************
** Graphic Utility to draw a rectangle.
*****************************************************************/
Err
drbDrawRect (Item Bitmap, GrafCon *GConPtr, int32 LeftX, int32 Topy,
             int32 RightX, int32 BottomY)
{
  Rect WorkRect;
  int32 Result;

  DBUG (("drbDrawRect: 0x%x, %d, %d, %d, %d\n", Bitmap, GConPtr, LeftX, Topy,
         RightX, BottomY));

  WorkRect.rect_XLeft = LeftX;
  WorkRect.rect_XRight = RightX;
  WorkRect.rect_YTop = Topy;
  WorkRect.rect_YBottom = BottomY;
  Result = FillRect (Bitmap, GConPtr, &WorkRect);
  CHECKRESULT (Result, "FillRect");

cleanup:
  return Result;
}

/*****************************************************************
** Draw a drum grid.
*****************************************************************/
Err
drbDrawGrid (ScreenContext *sc, DrumBox *drb)
{
  int32 Result;
  int32 ix, iy;
  GrafCon GCon;
  int32 LeftX, TopY;

  Result = 0;

  /* Clear Area */
  SetFGPen (&GCon, 0x00010);
  Result = drbDrawRect (CURBITMAPITEM, &GCon, drb->drb_LeftX, drb->drb_TopY,
                        DRB_NUM_COLUMNS * drb->drb_DeltaX + drb->drb_LeftX,
                        DRB_NUM_ROWS * drb->drb_DeltaY + drb->drb_TopY);
  CHECKRESULT (Result, "drbDrawRect");

  /* Draw grid. */
  SetFGPen (&GCon, 0x7FFF);
  for (iy = 0; iy < DRB_NUM_ROWS + 1; iy++)
    {
      MoveTo (&GCon, drb->drb_LeftX, iy * drb->drb_DeltaY + drb->drb_TopY);
      DrawTo (CURBITMAPITEM, &GCon,
              DRB_NUM_COLUMNS * drb->drb_DeltaX + drb->drb_LeftX,
              iy * drb->drb_DeltaY + drb->drb_TopY);
    }
  for (ix = 0; ix < DRB_NUM_COLUMNS + 1; ix++)
    {
      MoveTo (&GCon, ix * drb->drb_DeltaX + drb->drb_LeftX, drb->drb_TopY);
      DrawTo (CURBITMAPITEM, &GCon, ix * drb->drb_DeltaX + drb->drb_LeftX,
              DRB_NUM_ROWS * drb->drb_DeltaY + drb->drb_TopY);
    }

  /* Draw grid cells on/off */
  for (iy = 0; iy < DRB_NUM_ROWS; iy++)
    {
      for (ix = 0; ix < DRB_NUM_COLUMNS; ix++)
        {
          if (drb->drb_Lines[iy].drl_Velocities[ix] > 0) /* Is it on? */
            {
              /* Draw a different color if on the beat. */
              if (ix == drb->drb_CurrentIndex)
                {
                  SetFGPen (&GCon, MakeRGB15 (31, 16, 0));
                }
              else
                {
                  SetFGPen (&GCon, MakeRGB15 (0, 31, 0));
                }

              LeftX = (ix * drb->drb_DeltaX) + drb->drb_LeftX + 1;
              TopY = (iy * drb->drb_DeltaY) + drb->drb_TopY + 1;
              Result = drbDrawRect (CURBITMAPITEM, &GCon, LeftX, TopY,
                                    LeftX + drb->drb_DeltaX - 2,
                                    TopY + drb->drb_DeltaY - 2);
              CHECKRESULT (Result, "drbDrawRect");
            }
        }
    }

  /* Draw Flashing Input Cursor */
  if (drb->drb_CurrentIndex & 1) /* Change color for each beat. */
    {
      SetFGPen (&GCon, MakeRGB15 (31, 0, 0));
    }
  else
    {
      SetFGPen (&GCon, MakeRGB15 (31, 0, 31));
    }
  LeftX = ((drb->drb_Column) * drb->drb_DeltaX) + drb->drb_LeftX + 3;
  TopY = ((drb->drb_Row) * drb->drb_DeltaY) + drb->drb_TopY + 3;
  Result
      = drbDrawRect (CURBITMAPITEM, &GCon, LeftX, TopY,
                     LeftX + drb->drb_DeltaX - 6, TopY + drb->drb_DeltaY - 6);
  CHECKRESULT (Result, "drbDrawRect");

  /* Draw beat indicator. */
  SetFGPen (&GCon, 0x0000);
  ix = drb->drb_CurrentIndex - 1;
  if (ix < 0)
    ix = DRB_NUM_COLUMNS - 1;
  LeftX = (ix * drb->drb_DeltaX) + drb->drb_LeftX + 3;
  TopY = ((DRB_NUM_ROWS + 1) * drb->drb_DeltaY) + drb->drb_TopY + 3;
  Result
      = drbDrawRect (CURBITMAPITEM, &GCon, LeftX, TopY,
                     LeftX + drb->drb_DeltaX - 6, TopY + drb->drb_DeltaY - 6);
  CHECKRESULT (Result, "drbDrawRect");

  SetFGPen (&GCon, MakeRGB15 (0, 16, 31));
  LeftX = ((drb->drb_CurrentIndex) * drb->drb_DeltaX) + drb->drb_LeftX + 3;
  TopY = ((DRB_NUM_ROWS + 1) * drb->drb_DeltaY) + drb->drb_TopY + 3;
  Result
      = drbDrawRect (CURBITMAPITEM, &GCon, LeftX, TopY,
                     LeftX + drb->drb_DeltaX - 6, TopY + drb->drb_DeltaY - 6);
  CHECKRESULT (Result, "drbDrawRect");

cleanup:
  return Result;
}

/*****************************************************************
** Draw screen for this demo.
*****************************************************************/
Err
drbDrawNextScreen (ScreenContext *sc, DrumBox *drb)
{
  int32 Result;
  GrafCon GCon;

  MoveTo (&GCon, 100, 30);
  Result = DrawText8 (&GCon, CURBITMAPITEM, "DrumBox - 3DO");

  Result = drbDrawGrid (sc, drb);
  CHECKRESULT (Result, "drb_DrawKeyboard");

cleanup:
  return Result;
}

/*****************************************************************
** Setup DrumBox data structure to defaults.
*****************************************************************/
Err
drbSetupDrumBox (DrumBox *drb)
{
  drb->drb_Flags = 0;
  drb->drb_Row = 1;
  drb->drb_Column = 1;

  drb->drb_TopY = 60;
  drb->drb_LeftX = 40;
  drb->drb_DeltaX = 16;
  drb->drb_DeltaY = 12;

  drb->drb_CurrentIndex = 0;
  drb->drb_Duration = 20;

  return 0;
}

/****************************************************************/
Err
drbInit (DrumBox *drb)
{
  drbSetupDrumBox (drb);
  return 0;
}

/*****************************************************************
** Draw title screen and help.
*****************************************************************/
Err
drbDrawAboutScreen (ScreenContext *sc)
{
  int32 y;

  GrafCon GCon;

#define LINEHEIGHT (20)
#define LEFTMARGIN (30)
#define NEXTLINE(msg)                                                         \
  MoveTo (&GCon, LEFTMARGIN, y);                                              \
  y += LINEHEIGHT;                                                            \
  DrawText8 (&GCon, sc->sc_BitmapItems[0], msg);

  y = 28;
  NEXTLINE ("DrumBox");
  DrawText8 (&GCon, sc->sc_BitmapItems[0], VERSION);
  NEXTLINE ("(c) 3DO, August 1993");
  NEXTLINE ("by Phil Burk");
  NEXTLINE ("Hit any button to continue.");

  return 0;
}
/*****************************************************************
** Draw Wait message.
*****************************************************************/
Err
drbDrawWaitScreen (ScreenContext *sc)
{
  GrafCon GCon;

  MoveTo (&GCon, LEFTMARGIN, 100);
  DrawText8 (&GCon, CURBITMAPITEM, "Please wait for samples to load.");

  return 0;
}

/*****************************************************************
** Clear both screens.
*****************************************************************/
Err
drbClearScreens (ScreenContext *sc)
{
  GrafCon GCon;
  int32 Result;

  SetFGPen (&GCon, 0x000F);
  sc->sc_curScreen = 1;
  Result = drbDrawRect (CURBITMAPITEM, &GCon, 0, 0, 320, 240);
  sc->sc_curScreen = 0;
  Result = drbDrawRect (CURBITMAPITEM, &GCon, 0, 0, 320, 240);
  return Result;
}

/*****************************************************************
** Respond appropriately to button presses.
*****************************************************************/
Err
drbProcessUserInput (ScreenContext *sc, DrumBox *drb, uint32 Joy)
{
  int32 Result;

  Result = 0;

  if (Joy & ControlRightShift) /* Cursor control */
    {
    }
  else /* Select sector. */
    {
      if (Joy & ControlUp)
        {
          drb->drb_Row -= 1;
          if (drb->drb_Row < 0)
            drb->drb_Row = DRB_NUM_ROWS - 1;
        }
      else if (Joy & ControlDown)
        {
          drb->drb_Row += 1;
          if (drb->drb_Row >= DRB_NUM_ROWS)
            drb->drb_Row = 0;
        }

      if (Joy & ControlLeft)
        {
          drb->drb_Column -= 1;
          if (drb->drb_Column < 0)
            drb->drb_Column = DRB_NUM_COLUMNS - 1;
        }
      else if (Joy & ControlRight)
        {
          drb->drb_Column += 1;
          if (drb->drb_Column >= DRB_NUM_COLUMNS)
            drb->drb_Column = 0;
        }
    }

  if (Joy & ControlA)
    {
      if (drb->drb_Lines[drb->drb_Row].drl_Velocities[drb->drb_Column] > 0)
        {
          drb->drb_Lines[drb->drb_Row].drl_Velocities[drb->drb_Column] = 0;
        }
      else
        {
          drb->drb_Lines[drb->drb_Row].drl_Velocities[drb->drb_Column]
              = DEFAULT_VELOCITY;
        }
    }

  return Result;
}

static uint32 OldJoy;

/*********************************************************************/
Err
drbInteractiveLoop (ScreenContext *sc, DrumBox *drb)
{
  int32 Result;
  uint32 Joy;
  int32 HandsOffCount;
  ControlPadEventData cped;

  HandsOffCount = 0;
  drb->drb_CurTime = GetAudioTime ();

  /* Loop until the user presses ControlX */
  while (1)
    {
      SleepUntilTime (drb->drb_Cue, drb->drb_CurTime + drb->drb_Duration);
      drb->drb_CurTime += drb->drb_Duration;

      /* Toggle notes on and off. */
      if (drb->drb_Toggle != 0)
        {
          Result = drbColumnOn (drb, drb->drb_CurrentIndex);
        }
      else
        {
          Result = drbColumnOff (drb, drb->drb_CurrentIndex);
          drb->drb_CurrentIndex++;
          if (drb->drb_CurrentIndex >= DRB_NUM_COLUMNS)
            {
              drb->drb_CurrentIndex = 0;
            }
        }
      drb->drb_Toggle = !drb->drb_Toggle;
      if (Result < 0)
        {
          ERR (("Error in drbColumnOn/Off\n"));
          PrintfSysErr (Result);
        }

      /* Get User input. */
      Result = GetControlPad (1, FALSE, &cped);
      if (Result < 0)
        {
          ERR (("Error in GetControlPad\n"));
          PrintfSysErr (Result);
        }
      Joy = cped.cped_ButtonBits;
      if (Joy & ControlX)
        break;

      /* Did the control pad state change? */
      if (Joy != OldJoy)
        {
          Result = drbProcessUserInput (sc, drb, Joy);
          if (Result < 0)
            {
              ERR (("main: error in drbProcessUserInput\n"));
              PrintfSysErr (Result);
              goto DONE;
            }
          OldJoy = Joy;
        }

      /* Generate the next video image. */
      Result = drbDrawNextScreen (sc, drb);
      if (Result < 0)
        {
          ERR (("main: error in drbDrawNextScreen\n"));
          PrintfSysErr (Result);
          goto DONE;
        }

      /* Switch double buffered screens. */
      Result = DisplayScreen (sc->sc_Screens[(sc->sc_curScreen)], 0);
      if (Result < 0)
        {
          PrintfSysErr (Result);
          return Result;
        }
      sc->sc_curScreen = 1 - sc->sc_curScreen;
    }
DONE:
  return Result;
}

/************************************************************************/

int
main (int argc, char *argv[])
{
  char *progname;
  char *mapfile;
  int32 Result;
  DrumBox MyDRB;
  ScreenContext ScreenCon, *sc;
  ControlPadEventData cped;

  sc = &ScreenCon;
  memset (&MyDRB, 0, sizeof MyDRB);

  /* Get arguments from command line. */
  progname = *argv++;
  mapfile = *argv++;

  printf ("\n%s %s by Phil Burk\n", progname, VERSION);
  printf ("  PIMAP file is %s\n", mapfile);
  printf ("Copyright 1993 3DO\n");

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  /* Initialize the EventBroker. */
  Result = InitEventUtility (1, 0, LC_FocusListener);
  if (Result < 0)
    {
      ERR (("main: error in InitEventUtility\n"));
      PrintfSysErr (Result);
      goto DONE;
    }

  /* Set up double buffered display. */
  Result = CreateBasicDisplay (sc, DI_TYPE_NTSC, 2);
  if (Result < 0)
    {
      ERR (("CreateBasicDisplay failed 0x%x\n"));
      return (int)Result;
    }

  /* Initialize DrumBox structure. */
  if ((Result = drbInit (&MyDRB)) != 0)
    goto DONE;

  /* Make the screen we have created visible. */
  drbClearScreens (&ScreenCon);
  ScreenCon.sc_curScreen = 1;
  Result = DisplayScreen (ScreenCon.sc_Screens[0], 0);
  if (Result < 0)
    {
      printf ("DisplayScreen() failed, error=0x%x\n", Result);
      PrintfSysErr (Result);
      goto DONE;
    }

  drbDrawAboutScreen (&ScreenCon);
  GetControlPad (1, TRUE, &cped); /* Wait for button press. */

  /* Load samples. */
  drbClearScreens (&ScreenCon);
  drbDrawWaitScreen (&ScreenCon);
  Result = drbInitSound (&MyDRB, mapfile);
  if (Result < 0)
    {
      ERR (("drbInitSound failed 0x%x\n"));
      return (int)Result;
    }

  /* Play drum box. */
  drbClearScreens (&ScreenCon);
  drbInteractiveLoop (sc, &MyDRB);

  /* Cleanup the EventBroker. */
  Result = KillEventUtility ();
  if (Result < 0)
    {
      ERR (("main: error in KillEventUtility\n"));
      PrintfSysErr (Result);
    }

DONE:
  drbTermSound (&MyDRB);
  printf ("\n%s finished!\n", progname);
  return ((int)Result);
}
