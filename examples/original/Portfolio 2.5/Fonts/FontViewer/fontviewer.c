
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: fontviewer.c,v 1.15 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/fontviewer
|||	fontviewer - Displays and manipulates a font.
|||
|||	  Synopsis
|||
|||	    fontviewer \<fontFilename> [-b \<backgroundcel>] [-t
\<textfilename>]
|||
|||	  Description
|||
|||	    Simple viewer program for anti-aliased fonts created by 3DO
FontWriter.
|||
|||	    The arrow keys let you scroll around the displayed font cel. The A
button
|||	    forces a reload of the font from disk, so that you can work on a
font,
|||	    save a new version of it, and just hit A to view the results
without
|||	    having to stop/restart the program. The C button toggles between x1
and x2
|||	    magnification of the displayed font. The B button toggles the
background
|||	    cel (if any) on and off.
|||
|||	  Arguments
|||
|||	    fontFilename                 Name of font in which text is
displayed
|||
|||	    backgroundcel                Background cel against which text is
|||	                                 displayed
|||
|||	    textfilename                 Name of file containing text to
display
|||
|||	  Associated Files
|||
|||	    fontviewer.c, fontviewer.make
|||
|||	  Location
|||
|||	    examples/Fonts/FontViewer
|||
**/

#include "blockfile.h"
#include "celutils.h"
#include "ctype.h" // for isprint()
#include "debug3do.h"
#include "displayutils.h"
#include "graphics.h"
#include "io.h"
#include "string.h"
#include "types.h"

#include "controlpad.h"
#include "event.h"

#include "textlib.h"

#define XMIN 25
#define YMIN 25
#define XMAX 295
#define YMAX 215

static ScreenContext *sc;
Item displayItem = -1; /* result of CreateBasicDisplay */

static CCB *bgCel;
static TextCel *tCel;
static FontDescriptor *fDesc;

static char *bgCelFilename;
static char *fontFilename;
static char *textFilename;
static char *textFileData;

static char fontDisplayText[1024];

static Item vblIOReq;
static Item vramIOReq;

static int32 tCelPosX = XMIN;
static int32 tCelPosY = YMIN;
static int32 magnification = 1;

static Boolean bgOn = TRUE;

#define VRAM_BLUE (MakeRGB15Pair (0, 0, 31))
#define CONTROL_ARROWS (ControlLeft | ControlRight | ControlUp | ControlDown)
#define Convert32_F20(x) ((x) << 20)

/*****************************************************************************
 *
 ****************************************************************************/

static void
init_display_text (void)
/*
        If a textfile was specified in the command line arguments,
        set the display text buffer from the file contents,
        otherwise initialize to the standard character set,
        16 per line.
*/
{
  int row, column, charValue;
  char *pChar;

  if (textFileData != NULL)
    {
      strcpy (fontDisplayText, textFileData);
    }
  else
    {
      charValue = 0;
      pChar = fontDisplayText;

      for (row = 0; row < 15; ++row)
        {
          for (column = 0; column < 15; ++column)
            {
              *pChar++ = charValue++;
              *pChar++ = ' ';
            }
          *pChar++ = '\n';
        }
      fontDisplayText[0] = ' '; // can't have a \0 as the first char!

      strcpy (pChar,
              "This is a sample\ntext string."); // tack this on to the end
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

static Err
load_bg_cel (void)
/*
        Load the background cel and position it at the center of the screen.
*/
{
  if ((bgCel = LoadCel (bgCelFilename, MEMTYPE_CEL)) != NULL)
    {
      CenterCelOnScreen (bgCel);
      goto DONE;
    }

  VERBOSE (("Will use plain blue background\n"));

DONE:
  return 0;
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
load_text_file (void)
/*
        Load the text from the file specified in the command line arguments.
*/
{
  void *fileBuf;
  int32 fileSize;
  int32 retValue = -1;
  char *pChar;

  if ((fileBuf = LoadFile (textFilename, &fileSize, MEMTYPE_ANY)) == NULL)
    {
      retValue = fileSize;
      DIAGNOSE_SYSERR (retValue, ("LoadFile(%s) failed\n", textFilename));
      goto DONE;
    }

  if (fileSize > sizeof (fontDisplayText))
    {
      DIAGNOSE (("Display text file cannot exceed %d bytes\n",
                 sizeof (fontDisplayText)));
      goto DONE;
    }

  if ((textFileData
       = (char *)AllocMem (fileSize + 1, MEMTYPE_TRACKSIZE | MEMTYPE_ANY))
      == NULL)
    {
      goto DONE;
    }

  memcpy (textFileData, fileBuf, fileSize);
  textFileData[fileSize] = 0;

  /* Use the correct line terminators */
  for (pChar = textFileData; *pChar; ++pChar)
    {
      if (*pChar == '\r')
        {
          *pChar = '\n';
        }
    }

  retValue = 0;

DONE:

  if (fileBuf)
    {
      UnloadFile (fileBuf);
    }

  return retValue;
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
create_fontdisplay_cel (void)
/*
        Load the font, create a text cel and set its contents from the text
   buffer.
*/
{
  Err retValue = -1;
  char displayFirst;
  char displayLast;

  DeleteTextCel (tCel);
  UnloadFont (fDesc);

  if ((fDesc = LoadFont (fontFilename, MEMTYPE_ANY)) == NULL)
    {
      DIAGNOSE (("LoadFont(%s) failed\n", fontFilename));
      goto DONE;
    }

  if ((tCel = CreateTextCel (fDesc, TC_FORMAT_LEFT_JUSTIFY, 0, 0)) == NULL)
    {
      goto DONE;
    }

  SetTextCelMargins (tCel, 5, 4);
  UpdateTextInCel (tCel, TRUE, fontDisplayText);

  if (textFileData == NULL)
    {
      RenderCelOutlineRect (tCel->tc_CCB, 7, 0, 0, tCel->tc_CCB->ccb_Width,
                            tCel->tc_CCB->ccb_Height);
    }

  displayFirst
      = (char)(isprint (fDesc->fd_firstChar) ? fDesc->fd_firstChar : ' ');
  displayLast
      = (char)(isprint (fDesc->fd_lastChar) ? fDesc->fd_lastChar : ' ');

  printf ("Display created from font %s\n"
          "  Flags           =   0x%08lx\n"
          "  Height          = %3ld\n"
          "  Width           = %3ld\n"
          "  ExtraSpacing    = %3ld\n"
          "  Leading         = %3ld\n"
          "  Ascent          = %3ld\n"
          "  Descent         = %3ld\n"
          "  FirstChar       = %3ld  0x%02.2lx  '%c'\n"
          "  LastChar        = %3ld  0x%02.2lx  '%c'\n"
          "\n",
          fontFilename, fDesc->fd_fontFlags, fDesc->fd_charHeight,
          fDesc->fd_charWidth, fDesc->fd_charExtra, fDesc->fd_leading,
          fDesc->fd_ascent, fDesc->fd_descent, fDesc->fd_firstChar,
          fDesc->fd_firstChar, displayFirst, fDesc->fd_lastChar,
          fDesc->fd_lastChar, displayLast);

  retValue = 0;

DONE:
  return retValue;
}

/*****************************************************************************
 *
 ****************************************************************************/

static void
coerce_fontdisplay_position (void)
{
  int32 xSize = tCel->tc_CCB->ccb_Width * magnification;
  int32 ySize = tCel->tc_CCB->ccb_Height * magnification;

  if (tCelPosX > XMAX)
    {
      tCelPosX = XMAX;
    }
  else if ((tCelPosX + xSize) < XMIN)
    {
      tCelPosX = XMIN - xSize;
    }

  if (tCelPosY > YMAX)
    {
      tCelPosY = YMAX;
    }
  else if ((tCelPosY + ySize) < YMIN)
    {
      tCelPosY = YMIN - ySize;
    }

  tCel->tc_CCB->ccb_XPos = Convert32_F16 (tCelPosX);
  tCel->tc_CCB->ccb_YPos = Convert32_F16 (tCelPosY);

  return;
}

/*****************************************************************************
 *
 ****************************************************************************/

static void
update_display (void)
/* Draw the contents of the next frame */
{
  sc->sc_curScreen = 1 - sc->sc_curScreen;

  if (bgCel && bgOn)
    {
      SetVRAMPages (vramIOReq, sc->sc_Bitmaps[sc->sc_curScreen]->bm_Buffer, 0,
                    sc->sc_nFrameBufferPages, 0xFFFFFFFF);
      DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], bgCel);
    }
  else
    {
      SetVRAMPages (vramIOReq, sc->sc_Bitmaps[sc->sc_curScreen]->bm_Buffer,
                    VRAM_BLUE, sc->sc_nFrameBufferPages, 0xFFFFFFFF);
    }

  if (tCel)
    {
      coerce_fontdisplay_position ();
      DrawCels (sc->sc_BitmapItems[sc->sc_curScreen], tCel->tc_CCB);
    }

  DisplayScreen (sc->sc_Screens[sc->sc_curScreen], 0);
  WaitVBL (vblIOReq, 1);
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
do_user_interaction (void)
/*
        Respond to user input from the control pad:

        - A button means recreate the text cel.
        - B button means toggle between using the background cel and a blue
   background.
        - C button means toggle between 1:1 and x2 magnification.
        - Arrows mean move the cel around the screen.

        Returns -1 if the user pressed the X button to quit,
        otherwise 0.
*/
{
  Err retValue = 0;
  int32 continuousCount = 0;
  int32 moveSpeed = 1;
  int32 status;
  uint32 button;

  while (1)
    {
      status = DoControlPad (1, &button, CONTROL_ARROWS);

      if (button & ControlX)
        { // exit program
          retValue = -1;
          goto DONE;
        }

      if (button & ControlA)
        { // recreate font display cel
          if (create_fontdisplay_cel () < 0)
            { // from fresh copy of font file.
              goto DONE;
            }
        }

      if (button & ControlB)
        { // toggle the use of the background cel vs. blue background
          bgOn = !bgOn;
        }

      if (button & ControlC)
        { // toggle magnification between x1 and x2
          magnification = (magnification == 1) ? 2 : 1;

          if (magnification == 2)
            {
              tCelPosX -= (160 - tCelPosX);
              tCelPosY -= (120 - tCelPosY);
            }
          else
            {
              tCelPosX += (160 - tCelPosX) / 2;
              tCelPosY += (120 - tCelPosY) / 2;
            }

          tCel->tc_CCB->ccb_HDX = Convert32_F20 (magnification);
          tCel->tc_CCB->ccb_VDY = Convert32_F16 (magnification);
        }

      if (button & CONTROL_ARROWS)
        {
          ++continuousCount; // acceleration...
          moveSpeed
              = magnification * (continuousCount / 20); //  time factor scaling
          moveSpeed = 1
                      + ((moveSpeed * fDesc->fd_charHeight)
                         / 10); //  size factor scaling
          if (moveSpeed > (15 * magnification))
            {                                   //	choke accelleration
              moveSpeed = (15 * magnification); //  to max 15x move speed.
            }
        }
      else
        {
          continuousCount = 0;
          moveSpeed = magnification;
        }

      if (button & ControlDown)
        { // move cel up/down...
          tCelPosY += moveSpeed;
        }
      else if (button & ControlUp)
        {
          tCelPosY -= moveSpeed;
        }

      if (button & ControlRight)
        { // move cel left/right
          tCelPosX += moveSpeed;
        }
      else if (button & ControlLeft)
        {
          tCelPosX -= moveSpeed;
        }
      update_display ();
    }

DONE:
  return retValue;
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
prg_init (void)
/*
        Allocate program-wide resources:

        - A VBL IOReq for VBL waiting.
        - A VRAM IOReq for SPORT calls.
        - A single screenContext for handline 2 screens.
        - Initialize the control pad utility.
*/
{
  Err retValue = -1;

  vblIOReq = CreateVBLIOReq ();
  if (vblIOReq < 0)
    {
      PRT (("Can't create VBL IOReq\n"));
      goto DONE;
    }

  vramIOReq = CreateVRAMIOReq ();
  if (vramIOReq < 0)
    {
      PRT (("Can't create VRAM IOReq\n"));
      goto DONE;
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

  retValue = 0;

DONE:
  return retValue;
}

/*****************************************************************************
 *
 ****************************************************************************/

static void
prg_cleanup (void)
/*
        Dispose all program resources:
        - Dispose the text buffer.
        - Dispose the text cel.
        - Dispose the font description.
        - Dispose the background cel.
        - Kill the control pad utility.
        - Close the graphics folio.
        - Delete the VRAM IOReq used in SPORT calls.
        - Delete the VBL IOReq used for VBL waiting.
*/
{
  FreeMem (textFileData, -1);
  DeleteTextCel (tCel);
  UnloadFont (fDesc);
  DeleteCel (bgCel);

  KillControlPad ();
  if (displayItem >= 0)
    DeleteBasicDisplay (sc);

  DeleteVRAMIOReq (vramIOReq);
  DeleteVBLIOReq (vblIOReq);
}

/*****************************************************************************
 *
 ****************************************************************************/

int
main (int argc, char **argv)
{

  printf ("fontviewer\n");

  while (--argc > 0)
    {
      ++argv;
      if (argv[0][0] != '-')
        {
          if (fontFilename != NULL)
            {
              DIAGNOSE (
                  ("Font filename already provided, '%s' ignored\n", argv[0]));
            }
          else
            {
              fontFilename = argv[0];
            }
        }
      else
        {
          switch (argv[0][1])
            {
            case 'b':
            case 'B':
              if (argv[0][2] != 0)
                {
                  bgCelFilename = &argv[0][2];
                }
              else
                {
                  --argc;
                  ++argv;
                  bgCelFilename = argv[0];
                }
              break;
            case 't':
            case 'T':
              if (argv[0][2] != 0)
                {
                  textFilename = &argv[0][2];
                }
              else
                {
                  --argc;
                  ++argv;
                  textFilename = argv[0];
                }
              break;
            default:
              DIAGNOSE (("Unrecognized option '%s' ignored\n", argv[0]));
              break;
            }
        }
    }

  if (fontFilename == NULL)
    {
      fontFilename = "example.font";
    }

  if (bgCelFilename == NULL)
    {
      bgCelFilename = "bg.cel";
    }

  if (prg_init () < 0)
    {
      goto DONE;
    }

  if (textFilename)
    {
      if (load_text_file () < 0)
        {
          goto DONE;
        }
    }

  init_display_text ();

  if (load_bg_cel () < 0)
    {
      goto DONE;
    }

  if (create_fontdisplay_cel () < 0)
    {
      goto DONE;
    }

  do_user_interaction ();

DONE:

  prg_cleanup ();

  printf ("end of fontviewer\n");

  return 0;
}
