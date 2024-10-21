/*****************************************************************************
 *	File:			FontView.c
 *
 *	Contains:		A program to display antialiased fonts.
 *
 *	Copyright:		© 1993-1994 The 3DO Company. All Rights
 *Reserved.
 *
 *	History:
 *	05/12/94  Ian	Removed references to CelUtils functions and datatypes
 *					that are not being included in the current
 *release. Removed the bogus outline/shadow simulation stuff from 03/25/94
 *since the font/textlib now supports it directly.
 *
 *	04/01/94  Ian	Added ability to specify a file that contains the
 *display text.
 *
 *	03/31/94  Ian	Made magnification work properly with bogus shadow &
 *outline.
 *
 *	03/25/94  Ian	Added bogus outline and shadow effects, mostly to test
 *					visually the concepts we want to build into
 *TextLib.
 *
 *	03/16/94  Ian	Added printf of FontDescriptor values.  Relinked with
 *					repaired TextLib routines that properly handle
 *attempts to render characters not present in the font file.
 *
 *	03/06/94  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	A simple viewer program for AA fonts created by 3DOFontWriter.
 *
 *	The arrow keys let you scroll around the displayed font cel.  The A
 *button forces a reload of the font from disk, so that you can work on a font,
 *	save a new version of it, and just hit A to view the results without
 *	having to stop/restart the program.  The C button toggles between x1
 *and x2 magnification of the displayed font.  The B button toggles the
 *	background cel (if any) on and off.
 ****************************************************************************/

#include "BlockFile.h"
#include "CelUtils.h"
#include "Debug3DO.h"
#include "Init3DO.h"
#include "JoyPad.h"
#include "TextLib.h"
#include "UMemory.h"
#include "Utils3DO.h"
#include "ctype.h"
#include "stdio.h"
#include "string.h"

#define XMIN 25
#define YMIN 25
#define XMAX 295
#define YMAX 215

static ScreenContext sc;

static CCB *bgCel;
static TextCel *tCel;
static FontDescriptor *fDesc;

static char *bgCelFileName;
static char *fontFileName;
static char *textFileName;
static char *textFileData;

static char fontDisplayText[1024];

static Item vblIOReq;
static Item vramIOReq;

static int32 tCelPosX = XMIN;
static int32 tCelPosY = YMIN;
static long magnification = 1;

static Boolean bgOn = TRUE;

#define VRAMBLUE ((MakeRGB15 (0, 0, 31) << 16) | MakeRGB15 (0, 0, 31))

/*****************************************************************************
 *
 ****************************************************************************/

static void
init_display_text (void)
{
  int i, j, k;
  char *s;

  if (textFileData != NULL)
    {
      strcpy (fontDisplayText, textFileData);
    }
  else
    {
      k = 0;
      s = fontDisplayText;

      for (i = 0; i < 15; ++i)
        {
          for (j = 0; j < 15; ++j)
            {
              *s++ = k++;
              *s++ = ' ';
            }
          *s++ = '\n';
        }
      fontDisplayText[0] = ' '; // can't have a \0 as the first char!

      strcpy (s, "This is a sample\ntext string.");
    }
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
load_bg_cel (void)
{
  if ((bgCel = LoadCel (bgCelFileName, MEMTYPE_CEL)) != NULL)
    {
      CenterCelOnScreen (bgCel);
      return 0;
    }

  VERBOSE (("Will use plain black background\n"));
  return 0;
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
load_text_file (void)
{
  void *fileBuf;
  long fileSize;
  char *s;

  if ((fileBuf = LoadFile (textFileName, &fileSize, MEMTYPE_ANY)) == NULL)
    {
      DIAGNOSE_SYSERR (fileSize, ("LoadFile(%s) failed\n", textFileName));
      goto ERROR_EXIT;
    }

  if (fileSize > sizeof (fontDisplayText))
    {
      DIAGNOSE (("Display text file cannot exceed %d bytes\n",
                 sizeof (fontDisplayText)));
      fileSize = -1;
      goto ERROR_EXIT;
    }

  if ((textFileData = (char *)Malloc (fileSize + 1, MEMTYPE_ANY)) == NULL)
    {
      fileSize = -1;
      goto ERROR_EXIT;
    }

  memcpy (textFileData, fileBuf, fileSize);
  textFileData[fileSize] = 0;

  for (s = textFileData; *s; ++s)
    {
      if (*s == '\r')
        {
          *s = '\n';
        }
    }

ERROR_EXIT:

  if (fileBuf)
    {
      UnloadFile (fileBuf);
    }

  return fileSize;
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
create_fontdisplay_cel (void)
{
  char displayFirst;
  char displayLast;

  DeleteTextCel (tCel);
  UnloadFont (fDesc);

  if ((fDesc = LoadFont (fontFileName, MEMTYPE_ANY)) == NULL)
    {
      DIAGNOSE (("LoadFont(%s) failed\n", fontFileName));
      return -1;
    }

  if ((tCel = CreateTextCel (fDesc, TC_FORMAT_LEFT_JUSTIFY, 0, 0)) == NULL)
    {
      return -1;
    }

  SetTextCelMargins (tCel, 5, 4);
  UpdateTextInCel (tCel, TRUE, fontDisplayText);

#if 0 // RenderCel functions not available in this CelUtils release
	if (textFileData == NULL) {
		RenderCelOutlineRect(tCel->tc_CCB, 7, 0, 0, tCel->tc_CCB->ccb_Width, tCel->tc_CCB->ccb_Height);
	}
#endif

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
          fontFileName, fDesc->fd_fontFlags, fDesc->fd_charHeight,
          fDesc->fd_charWidth, fDesc->fd_charExtra, fDesc->fd_leading,
          fDesc->fd_ascent, fDesc->fd_descent, fDesc->fd_firstChar,
          fDesc->fd_firstChar, displayFirst, fDesc->fd_lastChar,
          fDesc->fd_lastChar, displayLast);

  return 0;
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
{
  sc.sc_curScreen = 1 - sc.sc_curScreen;

  if (bgCel && bgOn)
    {
      SetVRAMPages (vramIOReq, sc.sc_Bitmaps[sc.sc_curScreen]->bm_Buffer, 0,
                    sc.sc_nFrameBufferPages, 0xFFFFFFFF);
      DrawScreenCels (sc.sc_Screens[sc.sc_curScreen], bgCel);
    }
  else
    {
      SetVRAMPages (vramIOReq, sc.sc_Bitmaps[sc.sc_curScreen]->bm_Buffer,
                    VRAMBLUE, sc.sc_nFrameBufferPages, 0xFFFFFFFF);
    }

  if (tCel)
    {
      coerce_fontdisplay_position ();
      DrawScreenCels (sc.sc_Screens[sc.sc_curScreen], tCel->tc_CCB);
    }

  DisplayScreen (sc.sc_Screens[sc.sc_curScreen], 0);
  WaitVBL (vblIOReq, 1);
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
do_user_interaction (void)
{
  long continuousCount = 0;
  long moveSpeed = 1;
  uint32 joyBits;
  JoyPadState joyState;

  for (;;)
    {
      joyBits = GetJoyPad (&joyState, 1);

      if (joyState.xBtn)
        { // exit program
          return 0;
        }

      if (joyState.aBtn)
        { // recreate font display cel
          if (create_fontdisplay_cel () < 0)
            { // from fresh copy of font file.
              return -1;
            }
        }

      if (joyState.bBtn)
        {
          bgOn = !bgOn;
        }

      if (joyState.cBtn)
        { // toggle magnification
          if (magnification == 1)
            {
              magnification = 2;
              tCel->tc_CCB->ccb_HDX = 2 << 20;
              tCel->tc_CCB->ccb_VDY = 2 << 16;
              tCelPosX -= (160 - tCelPosX);
              tCelPosY -= (120 - tCelPosY);
            }
          else
            {
              magnification = 1;
              tCel->tc_CCB->ccb_HDX = 1 << 20;
              tCel->tc_CCB->ccb_VDY = 1 << 16;
              tCelPosX += (160 - tCelPosX) / 2;
              tCelPosY += (120 - tCelPosY) / 2;
            }
        }

      if (joyBits & PADARROWS)
        {
          ++continuousCount; // accelleration...
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

      if (joyState.downArrow)
        { // move cel up/down...
          tCelPosY += moveSpeed;
        }
      else if (joyState.upArrow)
        {
          tCelPosY -= moveSpeed;
        }

      if (joyState.rightArrow)
        { // move cel left/right
          tCelPosX += moveSpeed;
        }
      else if (joyState.leftArrow)
        {
          tCelPosX -= moveSpeed;
        }
      update_display ();
    }
}

/*****************************************************************************
 *
 ****************************************************************************/

static Err
prg_init (void)
{
  if (OpenMathFolio () < 0)
    {
      return -1;
    }

  if (OpenGraphics (&sc, 2) == 0)
    {
      return -1;
    }

  vblIOReq = GetVBLIOReq ();
  vramIOReq = GetVRAMIOReq ();

  SetJoyPadContinuous (PADARROWS, 1);

  return 0;
}

/*****************************************************************************
 *
 ****************************************************************************/

static void
prg_cleanup (void)
{
  if (textFileData)
    {
      Free (textFileData);
    }

  DeleteTextCel (tCel);
  UnloadFont (fDesc);
  DeleteCel (bgCel);
  DeleteItem (vblIOReq);
  DeleteItem (vramIOReq);
  KillJoypad ();
  CloseGraphics (&sc);
}

/*****************************************************************************
 *
 ****************************************************************************/

int
main (int argc, char **argv)
{

  while (--argc > 0)
    {
      ++argv;
      if (argv[0][0] != '-')
        {
          if (fontFileName != NULL)
            {
              DIAGNOSE (("Font file name already provided, '%s' ignored\n",
                         argv[0]));
            }
          else
            {
              fontFileName = argv[0];
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
                  bgCelFileName = &argv[0][2];
                }
              else
                {
                  --argc;
                  ++argv;
                  bgCelFileName = argv[0];
                }
              break;
            case 't':
            case 'T':
              if (argv[0][2] != 0)
                {
                  textFileName = &argv[0][2];
                }
              else
                {
                  --argc;
                  ++argv;
                  textFileName = argv[0];
                }
              break;
            default:
              DIAGNOSE (("Unrecognized option '%s' ignored\n", argv[0]));
              break;
            }
        }
    }

  if (fontFileName == NULL)
    {
      printf ("\nUsage: FontView fontFileName [-b bgCelFileName][-t "
              "textFileName]\n");
      return 1;
    }

  if (bgCelFileName == NULL)
    {
      bgCelFileName = "Background.cel";
    }

  if (prg_init () < 0)
    {
      goto ERROR_EXIT;
    }

  if (textFileName)
    {
      if (load_text_file () < 0)
        {
          goto ERROR_EXIT;
        }
    }

  init_display_text ();

  if (load_bg_cel () < 0)
    {
      goto ERROR_EXIT;
    }

  if (create_fontdisplay_cel () < 0)
    {
      goto ERROR_EXIT;
    }

  do_user_interaction ();

ERROR_EXIT:

  prg_cleanup ();
  printf ("FontView exit\n");
  return 0;
}
