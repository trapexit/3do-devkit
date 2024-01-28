/*****************************************************************************
 * TestFont.c - A quick & dirty for testing FontLib.c and TextLib.c.
 *
 *	In no way shape or form should this be taken as an example of the
 *	right way to use the Font/Text library routines; this just excercises
 *	the various entry points to prove they all work.
 ****************************************************************************/

#include "Debug3DO.h"
#include "Form3DO.h" // needed by Utils3DO.h ::sigh::
#include "Init3DO.h"
#include "TextLib.h"
#include "UMemory.h"
#include "Utils3DO.h"
#include "operamath.h"

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

static void
wait_for_key (char *prompt)
{
  printf (prompt);

  for (;;)
    {
      if (ReadControlPad (JOYBUTTONS))
        {
          break;
        }
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

static int32
testFont (ScreenContext *sc, char *fontName)
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

  vblIOReq = GetVBLIOReq ();

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

  backgroundCel = LoadCel ("bg.cel", MEMTYPE_CEL);

  /*----------------------------------------------------------------------------
   * Example 1
   *--------------------------------------------------------------------------*/

  if (backgroundCel != NULL)
    DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], backgroundCel);
  else
    bluebar_background (sc);

  gcon.gc_BGPen = 0;
  gcon.gc_FGPen = MakeRGB15 (31, 31, 0);

  gcon.gc_PenX = 20;
  gcon.gc_PenY = 20;
  DrawTextString (fontDesc, &gcon, sc->sc_BitmapItems[sc->sc_curScreen],
                  big_block_o_text, "DrawTextString(big_block)...");

  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  wait_for_key ("Hit any key to move to next test...\n");

  /*----------------------------------------------------------------------------
   * Example 2
   *--------------------------------------------------------------------------*/

  if (backgroundCel != NULL)
    DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], backgroundCel);
  else
    bluebar_background (sc);

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
  wait_for_key ("Hit any key to move to next test...\n");

  /*----------------------------------------------------------------------------
   * Example 3
   *--------------------------------------------------------------------------*/

  if (backgroundCel != NULL)
    DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], backgroundCel);
  else
    bluebar_background (sc);

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
  DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], tCel->tc_CCB);

  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  wait_for_key ("Hit any key to move to next test...\n");

  /*----------------------------------------------------------------------------
   * Example 4
   *--------------------------------------------------------------------------*/

  err = SetTextCelSize (tCel, 320, 240);

  if (err < 0)
    {
      DIAGNOSE (("SetTextCelSize() failed (out of memory?)\n"));
      goto ERROR_EXIT;
    }

  SetTextCelFormatFlags (
      tCel, (GetTextCelFormatFlags (tCel, nil) & ~TC_FORMAT_JUSTIFY_MASK)
                | TC_FORMAT_LEFT_JUSTIFY);

  if (backgroundCel != NULL)
    DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], backgroundCel);
  else
    bluebar_background (sc);

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

  DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], tCel->tc_CCB);
  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  wait_for_key ("Hit any key to move to next test...\n");

  /*----------------------------------------------------------------------------
   * Example 5
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
      if (backgroundCel != NULL)
        DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], backgroundCel);
      else
        bluebar_background (sc);

      SetTextCelCoords (tCel, Convert32_F16 (j), Convert32_F16 (j));
      DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], tCel->tc_CCB);
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
  wait_for_key ("Hit any key to move to next test...\n");

  /*----------------------------------------------------------------------------
   * Example 6
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
      if (backgroundCel != NULL)
        DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], backgroundCel);
      else
        bluebar_background (sc);

      SetTextCelColor (tCel, i, MakeRGB15 (31 - j, 0, j));
      DrawScreenCels (sc->sc_Screens[sc->sc_curScreen], tCel->tc_CCB);
      DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
      sc->sc_curScreen = !sc->sc_curScreen;
      j = (j + 1) % 32;
      WaitVBL (vblIOReq, 10);
    }
  while (!ReadControlPad (JOYBUTTONS));

  err = 0;

ERROR_EXIT:

  if (tCel != NULL)
    {
      DeleteTextCel (tCel);
    }

  if (fontDesc != NULL)
    {
      UnloadFont (fontDesc);
      VERBOSE (("Font file %s unloaded\n", fontName));
    }

  return err;
}

/*****************************************************************************
 *
 ****************************************************************************/

int
main (int argc, char *argv[])
{
  char *progname;
  char *fontName;
  ScreenContext sc;

  printf ("\n");
  progname = argv[0];

  if (NULL == (fontName = argv[1]))
    {
      fontName = "sample.font";
    }

  if (!OpenGraphics (&sc, 2))
    {
      goto DONE;
    }

  if (OpenMathFolio () < 0)
    {
      goto DONE;
    }

  if (testFont (&sc, fontName) >= 0)
    {
      wait_for_key ("Hit any key to exit...\n");
    }

DONE:

  FadeToBlack (&sc, 10);
  ShutDown ();
  printf ("%s has completed\n", progname);
  return (0);
}
