
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: fontlibexample.c,v 1.12 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/fontlibexample
|||	fontlibexample - Exercises the font and text libraries.
|||
|||	  Synopsis
|||
|||	    fontlibexample \<fontfilename>
|||
|||	  Description
|||
|||	    Loads a font and a background cel and runs through several test
routines,
|||	    namely:
|||
|||	    * Draw a multi-line block of text against the background
|||
|||	    * Draw several lines of text
|||
|||	    * Create a text cel, setup multiple colors, update it, and display
it
|||
|||	    * Erase the text cel and refill it using multiple UpdateTextCel()
calls
|||
|||	    * Move a text cel around the screen
|||
|||	    * Cycle the colors in the text cel
|||
|||	  Arguments
|||
|||	    fontfilename                 Name of font in which to display text.
|||
|||	  Caveats
|||
|||	    This is a simple validation exercise of the font and text library
routines
|||	    and is not intended to indicate proper usage within a title.
|||
|||	  Associated Files
|||
|||	    fontlibexample.c
|||
|||	  Location
|||
|||	    examples/Fonts/Font_Library_Example
|||
**/

#include "celutils.h"
#include "debug3do.h"
#include "displayutils.h"
#include "form3do.h" // needed by utils3do.h ::sigh::
#include "graphics.h"
#include "init3do.h"
#include "io.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#include "textlib.h"

#define USER_PROMPT "Press any button to advance to the next test...\n"

static char big_block_o_text[]
    = "%s\n"
      "In no way shape or form should this be taken as\n"
      "an example of the right way to use the Font/Text\n"
      "library routines; this just excercises the various\n"
      "entry points to prove they all work.\n";

static char test_text_2[] = "This text\ncolor cycles\na bit.";

static int32 pen_colors_test_values[4] = {
  MakeRGB15 (31, 31, 31),
  MakeRGB15 (1, 1, 1),
  MakeRGB15 (0, 31, 0),
  MakeRGB15 (31, 0, 31),
};

/*****************************************************************************
 *
 ****************************************************************************/

static uint32
controlButtons (void)
{
  uint32 button;
  int32 status;

  status = DoControlPad (1, &button, 0);
  return button;
}

static void
wait_for_button (char *prompt)
{
  uint32 button;

  PRT ((prompt));

  for (;;)
    {
      button = controlButtons ();
      if (button)
        break;
    }
}

/*****************************************************************************
 *
 ****************************************************************************/

static void
bluebar_background (ScreenContext *sc)
{
  int32 i;
  Rect bounds;
  GrafCon gcon;

  bounds.rect_YTop = 0;
  bounds.rect_YBottom = 240;

  for (i = 0; i < 32; i++)
    {
      bounds.rect_XLeft = i * 10;
      bounds.rect_XRight = (i + 1) * 10;

      SetFGPen (&gcon, MakeRGB15 (0, 0, i));
      FillRect (sc->sc_BitmapItems[sc->sc_curScreen], &gcon, &bounds);
    }
}

/*****************************************************************************
 *
 ****************************************************************************/

static void
CenterCelOnScreen (CCB *ccb)
{
  ccb->ccb_XPos = Convert32_F16 ((320 - ccb->ccb_Width) / 2);
  ccb->ccb_YPos = Convert32_F16 ((240 - ccb->ccb_Height) / 2);
}

static void
drawBackground (ScreenContext *sc, CCB *backgroundCel)
{
  if (backgroundCel != NULL)
    DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], backgroundCel);
  else
    bluebar_background (sc);
}

/*****************************************************************************
 *
 ****************************************************************************/

static int32
testFont (ScreenContext *sc, char *fontName, char *bgCelName)
{
  int32 err;
  int32 i, j;
  int32 twidth;
  int32 theight;
  GrafCon gcon;
  FontDescriptor *fontDesc = NULL;
  TextCel *tCel = NULL;
  Item vblIOReq;
  CCB *backgroundCel = NULL;

  vblIOReq = CreateVBLIOReq ();

  /*----------------------------------------------------------------------------
   * Load the font & background.
   *--------------------------------------------------------------------------*/

  fontDesc = LoadFont (fontName, MEMTYPE_ANY);
  if (fontDesc == NULL)
    {
      DIAGNOSE (("Can't load font file %s\n", fontName));
      err = -1;
      goto ERROR_EXIT;
    }
  else
    {
      VERBOSE (("Font file %s loaded\n", fontName));
    }

  backgroundCel = LoadCel (bgCelName, MEMTYPE_CEL);
  if (backgroundCel == NULL)
    {
      DIAGNOSE (("Can't load background cel (%s), background will be black.\n",
                 bgCelName));
    }

  /*----------------------------------------------------------------------------
   * Draw a multi-line block of text against the background
   *--------------------------------------------------------------------------*/

  drawBackground (sc, backgroundCel);

  gcon.gc_BGPen = 0;
  gcon.gc_FGPen = MakeRGB15 (31, 31, 0);

  gcon.gc_PenX = 20;
  gcon.gc_PenY = 20;
  DrawTextString (fontDesc, &gcon, sc->sc_BitmapItems[sc->sc_curScreen],
                  big_block_o_text, "DrawTextString(big_block)...");

  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  wait_for_button (USER_PROMPT);

  /*----------------------------------------------------------------------------
   * Draw several lines of text
   *--------------------------------------------------------------------------*/

  drawBackground (sc, backgroundCel);

  gcon.gc_PenX = 20;
  gcon.gc_PenY = 20;
  DrawTextString (fontDesc, &gcon, sc->sc_BitmapItems[sc->sc_curScreen],
                  "Draw lines/chars test...\n");
  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);

  while (gcon.gc_PenY < 240)
    {
      gcon.gc_BGPen = (gcon.gc_PenY > 100) ? MakeRGB15 (0, 0, 25) : 0;

      gcon.gc_PenX = 20;
      gcon.gc_PenY += fontDesc->fd_charHeight;
      gcon.gc_FGPen = MakeRGB15 (31, 31, 31);
      DrawTextString (fontDesc, &gcon, sc->sc_BitmapItems[sc->sc_curScreen],
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
      DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);

      gcon.gc_PenX = 20;
      gcon.gc_PenY += fontDesc->fd_charHeight;
      gcon.gc_FGPen = MakeRGB15 (31, 31, 0);
      DrawTextString (fontDesc, &gcon, sc->sc_BitmapItems[sc->sc_curScreen],
                      "abcdefghijklmnopqrstuvwxyz");
      DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);

      gcon.gc_FGPen = MakeRGB15 (31, 31, 0);
      DrawTextString (fontDesc, &gcon, sc->sc_BitmapItems[sc->sc_curScreen],
                      " y=%ld", gcon.gc_PenY);
      DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);

      gcon.gc_PenX = 20;
      gcon.gc_PenY += fontDesc->fd_charHeight;
      for (j = 0; j <= 31; j++)
        {
          gcon.gc_FGPen = MakeRGB15 (31 - j, 0, j);
          DrawTextChar (fontDesc, &gcon, sc->sc_BitmapItems[sc->sc_curScreen],
                        '0' + j);
        }
      DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
    }
  wait_for_button (USER_PROMPT);

  /*----------------------------------------------------------------------------
   * Create a text cel, setup multiple colors, update it, and display it
   *--------------------------------------------------------------------------*/

  drawBackground (sc, backgroundCel);

  tCel = CreateTextCel (fontDesc,
                        TC_FORMAT_WORDWRAP | TC_FORMAT_CENTER_JUSTIFY, 220, 0);
  if (tCel == NULL)
    {
      DIAGNOSE (("CreateTextCel failed!\n"));
      return 0;
    }
  err = SetTextCelFormatBuffer (tCel, NULL, 2 * sizeof (big_block_o_text));
  if (err < 0)
    {
      DIAGNOSE (("SetTextCelFormatBuffer() failed (out of memory?)\n"));
      goto ERROR_EXIT;
    }

  SetTextCelColors (tCel, 0, pen_colors_test_values);

  SetTextCelCoords (tCel, Convert32_F16 (20), Convert32_F16 (20));
  UpdateTextInCel (tCel, TRUE, big_block_o_text,
                   "UpdateTextInCel(big_block)...");
  DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], tCel->tc_CCB);

  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  wait_for_button (USER_PROMPT);

  /*----------------------------------------------------------------------------
   * Erase the text cel and refill it using multiple UpdateTextCel calls
   *--------------------------------------------------------------------------*/

  err = SetTextCelSize (tCel, 320, 240);

  if (err < 0)
    {
      DIAGNOSE (("SetTextCelSize() failed (out of memory?)\n"));
      goto ERROR_EXIT;
    }

  SetTextCelFormatFlags (
      tCel, (GetTextCelFormatFlags (tCel, NULL) & ~TC_FORMAT_JUSTIFY_MASK)
                | TC_FORMAT_LEFT_JUSTIFY);

  drawBackground (sc, backgroundCel);

  EraseTextInCel (tCel);
  UpdateTextInCel (tCel, FALSE, "Some text added\nafter EraseTextInCel\n");
  UpdateTextInCel (tCel, FALSE, "\n");
  UpdateTextInCel (tCel, FALSE, "A couple ");
  UpdateTextInCel (tCel, FALSE, "lines\n");
  UpdateTextInCel (tCel, FALSE, "Ad");
  UpdateTextInCel (tCel, FALSE, "ded piecemeal\n");
  UpdateTextInCel (tCel, FALSE, "with different\n");
  UpdateTextInCel (tCel, FALSE, "pen colors ");
  UpdateTextInCel (tCel, FALSE, "in ");
  UpdateTextInCel (tCel, FALSE, "the\n");
  UpdateTextInCel (tCel, FALSE, "same ");
  UpdateTextInCel (tCel, FALSE, "cel");
  UpdateTextInCel (tCel, FALSE, ".");

  DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], tCel->tc_CCB);
  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  wait_for_button (USER_PROMPT);

  /*----------------------------------------------------------------------------
   * Move a line of text around the screen
   *--------------------------------------------------------------------------*/

  err = SetTextCelSize (tCel, 0, 0);

  if (err < 0)
    {
      DIAGNOSE (("SetTextCelSize() failed (out of memory?)\n"));
      goto ERROR_EXIT;
    }

  UpdateTextInCel (tCel, TRUE, "This text floats around");
  i = 2;
  j = 30;
  do
    {
      drawBackground (sc, backgroundCel);

      SetTextCelCoords (tCel, Convert32_F16 (j), Convert32_F16 (j));
      DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], tCel->tc_CCB);
      DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
      sc->sc_curScreen = !sc->sc_curScreen;
      WaitVBL (vblIOReq, 1);
      if (j > 180 && i > 0)
        {
          i = -i;
        }
      j += i;
    }
  while (j > 30);
  wait_for_button (USER_PROMPT);

  /*----------------------------------------------------------------------------
   * Cycle the colors in the text cel
   *--------------------------------------------------------------------------*/

  GetTextExtent (tCel, &twidth, &theight, test_text_2);

  err = SetTextCelSize (tCel, twidth + 2, theight + 2);
  if (err < 0)
    {
      DIAGNOSE (("SetTextCelSize() failed (out of memory?)\n"));
      goto ERROR_EXIT;
    }
  SetTextCelMargins (tCel, 1, 1);

  UpdateTextInCel (tCel, TRUE, test_text_2);
  CenterCelOnScreen (tCel->tc_CCB);
  j = 0;
  do
    {
      drawBackground (sc, backgroundCel);

      SetTextCelColor (tCel, i, MakeRGB15 (31 - j, 0, j));
      DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], tCel->tc_CCB);
      DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
      sc->sc_curScreen = !sc->sc_curScreen;
      j = (j + 1) % 32;
      WaitVBL (vblIOReq, 10);
    }
  while (!controlButtons ());

  err = 0;

ERROR_EXIT:

  if (tCel != NULL)
    DeleteTextCel (tCel);

  if (fontDesc != NULL)
    UnloadFont (fontDesc);

  DeleteVBLIOReq (vblIOReq);

  return err;
}

/*****************************************************************************
 *
 ****************************************************************************/

int
main (int argc, char *argv[])
{
  char *fontName;
  ScreenContext *sc;
  Item displayItem = -1; /* result of CreateBasicDisplay */

  printf ("\n");

  if (NULL == (fontName = argv[1]))
    {
      fontName = "example.font";
    }

  sc = (ScreenContext *)AllocMem (sizeof (ScreenContext), MEMTYPE_ANY);
  if (sc == NULL)
    {
      PRT (("Can't allocate memory for ScreenContext\n"));
      goto DONE;
    }

  if ((displayItem = CreateBasicDisplay (sc, DI_TYPE_DEFAULT, 2)) < 0)
    {
      PRT (("Can't initialize display\n"));
      goto DONE;
    }

  if (InitControlPad (2) < 0)
    {
      PRT (("Can't initialize the control pad\n"));
      goto DONE;
    }

  if (testFont (sc, fontName, "bg.cel") >= 0)
    {
      wait_for_button (USER_PROMPT);
    }

DONE:

  FadeToBlack (sc, 60);

  KillControlPad ();

  if (displayItem >= 0)
    DeleteBasicDisplay (sc);

  printf ("end of fontlibexample\n");

  return 0;
}
