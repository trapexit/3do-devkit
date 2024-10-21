/*
        File:		textbox.c

        Contains:	An example of using TextBox

        Written by:	Jay London

        Copyright:	1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <1+>	  4/7/93	JAY		MEMTYPE_SPRITE is now
   MEMTYPE_CEL for Cardinal2
                 <1>	  4/7/93	JAY		first checked in

        To Do:
*/

#include "audio.h"
#include "debug.h"
#include "filefunctions.h"
#include "folio.h"
#include "graphics.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#include "directory.h"
#include "directoryfunctions.h"

#include "Form3DO.h"
#include "Init3DO.h"
#include "KTextBox.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

#include "TimeHelper.h"

/* *************************************************************************
 * ***  Discussion  ********************************************************
 * *************************************************************************
 *
 * This program does not clean up after itself.  This is Portfolio-approved.
 * All programs *should* exit without bothering to clean up.  This
 * is because Portfolio promises to clean up and deallocate all system
 * resources for you
 *
 */

/* *************************************************************************
 * ***                   ***************************************************
 * ***  Our Definitions  ***************************************************
 * ***                   ***************************************************
 * *************************************************************************
 */

#define NEEDMAC 1
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define kNumFontFiles 1

/* These represent the value one (1) in various number formats.
 * For example, ONE_12_20 is the value of 1 in fixed decimal format
 * of 12 bits integer, 20 bits fraction
 */
#define ONE_12_20 (1 << 20)
#define ONE_16_16 (1 << 16)

#define WAIT_TIME 150000
#define JOYALL                                                                \
  (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT | JOYFIREA | JOYFIREB      \
   | JOYFIREC)

ubyte *gBackPic = NULL;
ScreenContext sc;

/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

void CopyBlack (Bitmap *bitmap, Int32 nFrameBufferPages);
bool InitBackPic (ScreenContext *sc, char *filename);
void CopyBackPic (Bitmap *bitmap, Int32 nFrameBufferPages);
void MakeText (uint8 high, uint8 *text1, uint8 *text2);

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */

ubyte *gBackBlack = NULL;
Item VRAMIOReq;

TagArg ScreenTags[] = {
  /* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
  /* Kludgy code below presumes that SPORTBITS is the first entry */
  CSG_TAG_SPORTBITS, (void *)0, CSG_TAG_SCREENCOUNT, (void *)2, CSG_TAG_DONE, 0
};

void
drawText (int32 x, int32 y, char *string)
{
  GrafCon gcon;

  SetFGPen (&gcon, MakeRGB15 (31, 31, 31));
  SetBGPen (&gcon, MakeRGB15 (0, 0, 31));

  gcon.gc_PenX = x;
  gcon.gc_PenY = y;
  DrawText8 (&gcon, sc.sc_BitmapItems[sc.sc_curScreen], string);
}

/* *************************************************************************
 * ***                 *****************************************************
 * ***  Main Routines  *****************************************************
 * ***                 *****************************************************
 * *************************************************************************
 */

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
  char *progname;
  Int32 err;
  Item dirItem;
  Int32 retvalue;
  long lControlvals;
  Rectf16 theRect[4];
  Color fgColor;
  Color bgColor;
  KFont3DO TextFont;
  Int32 fontIndex = 0;
  uint8 Text1[103 * 2 + 1], Text2[103 * 2 + 1], *Text1_ptr, *Text2_ptr;
  CCB *textccb[2];
  uint8 high;
  char *Font1Path = (char *)"$Fonts";
  char *Font1 = (char *)"Kanji16.4"; // 904K
  char EntireFontName[200];
  char *TestString;
  char string[255];
  long TestStringLen;
  TextAlign theAlign;
  Int32 RedSys = 0;
  Int32 count = 0;
  Int32 row;
  Int32 charsPerScreen;
  Boolean fullRead;
  char *jis_highbyte[78]
      = { (char *)"‚O‚˜‚Q‚P", (char *)"‚O‚˜‚Q‚Q", (char *)"‚O‚˜‚Q‚R",
          (char *)"‚O‚˜‚Q‚S", (char *)"‚O‚˜‚Q‚T", (char *)"‚O‚˜‚Q‚U",
          (char *)"‚O‚˜‚Q‚V", (char *)"‚O‚˜‚Q‚W", (char *)"‚O‚˜‚Q‚e",
          (char *)"‚O‚˜‚R‚O", (char *)"‚O‚˜‚R‚P", (char *)"‚O‚˜‚R‚Q",
          (char *)"‚O‚˜‚R‚R", (char *)"‚O‚˜‚R‚S", (char *)"‚O‚˜‚R‚T",
          (char *)"‚O‚˜‚R‚U", (char *)"‚O‚˜‚R‚V", (char *)"‚O‚˜‚R‚W",
          (char *)"‚O‚˜‚R‚X", (char *)"‚O‚˜‚R‚`", (char *)"‚O‚˜‚R‚a",
          (char *)"‚O‚˜‚R‚b", (char *)"‚O‚˜‚R‚c", (char *)"‚O‚˜‚R‚d",
          (char *)"‚O‚˜‚R‚e", (char *)"‚O‚˜‚S‚O", (char *)"‚O‚˜‚S‚P",
          (char *)"‚O‚˜‚S‚Q", (char *)"‚O‚˜‚S‚R", (char *)"‚O‚˜‚S‚S",
          (char *)"‚O‚˜‚S‚T", (char *)"‚O‚˜‚S‚U", (char *)"‚O‚˜‚S‚V",
          (char *)"‚O‚˜‚S‚W", (char *)"‚O‚˜‚S‚X", (char *)"‚O‚˜‚S‚`",
          (char *)"‚O‚˜‚S‚a", (char *)"‚O‚˜‚S‚b", (char *)"‚O‚˜‚S‚c",
          (char *)"‚O‚˜‚S‚d", (char *)"‚O‚˜‚S‚e", (char *)"‚O‚˜‚T‚O",
          (char *)"‚O‚˜‚T‚P", (char *)"‚O‚˜‚T‚Q", (char *)"‚O‚˜‚T‚R",
          (char *)"‚O‚˜‚T‚S", (char *)"‚O‚˜‚T‚T", (char *)"‚O‚˜‚T‚U",
          (char *)"‚O‚˜‚T‚V", (char *)"‚O‚˜‚T‚W", (char *)"‚O‚˜‚T‚X",
          (char *)"‚O‚˜‚T‚`", (char *)"‚O‚˜‚T‚a", (char *)"‚O‚˜‚T‚b",
          (char *)"‚O‚˜‚T‚c", (char *)"‚O‚˜‚T‚d", (char *)"‚O‚˜‚T‚e",
          (char *)"‚O‚˜‚U‚O", (char *)"‚O‚˜‚U‚P", (char *)"‚O‚˜‚U‚Q",
          (char *)"‚O‚˜‚U‚R", (char *)"‚O‚˜‚U‚S", (char *)"‚O‚˜‚U‚T",
          (char *)"‚O‚˜‚U‚U", (char *)"‚O‚˜‚U‚V", (char *)"‚O‚˜‚U‚W",
          (char *)"‚O‚˜‚U‚X", (char *)"‚O‚˜‚U‚`", (char *)"‚O‚˜‚U‚a",
          (char *)"‚O‚˜‚U‚b", (char *)"‚O‚˜‚U‚c", (char *)"‚O‚˜‚U‚d",
          (char *)"‚O‚˜‚U‚e", (char *)"‚O‚˜‚V‚O", (char *)"‚O‚˜‚V‚P",
          (char *)"‚O‚˜‚V‚Q", (char *)"‚O‚˜‚V‚R", (char *)"‚O‚˜‚V‚S" };
  char *tmp = (char *)"        ";
  CCB *jisccb[2];
  //	CCB	*nameccb[2];
  //	Int32 nameIndex = 0;
  Int32 wait;
  Int32 errorIndex;
  char *errorString[7]
      = { (char *)"unknown error",           (char *)"KTextBox-BadParameter",
          (char *)"KTextBox-CannotMemAlloc", (char *)"KTextBox-NotFound",
          (char *)"KTextBox-BadCharCode",    (char *)"KTextBox-BadFontFile",
          (char *)"KTextBox-CannotOpenDS" };
  char actualPathname[200], actualPathname2[200];
  char origPathname[200];
  TimerHelperPtr gIdleTimer = NULL;
  struct timeval tv;

  /*	Extract the program name, imagefile name, and celfile name from
   *	the arguments passed into the program.
   *	NOTE: the imagefile and celfile names are string copied into
   *	globals. This assures that the names will start on a longword
   *	boundry. Any strings that are passed to the O/S must start on
   *	a longword boundry.
   */
  progname = *argv++;
  printf ("\nRunning %s...\n", progname);

  if (ChangeInitialDirectory (NULL, NULL, false) < 0)
    {
      DIAGNOSTIC ("Cannot set initial working directory ");
      goto DONE;
    }

  if (FlagIsSet (KernelBase->kb_CPUFlags, KB_RED))
    {
      RedSys = 1;
    }

  VRAMIOReq = GetVRAMIOReq ();

  /*	Creates the CreateScreenGroup and inits a SPORT device to write
   *	directly to the frame buffer and a Mac Link to read files from
   *	the Macintosh.
   */
  if (!OpenGraphics (&sc, 2))
    goto DONE;
  if (!OpenSPORT ())
    goto DONE;
  if (!OpenAudio ())
    goto DONE;
#if NEEDMAC
//	if (!OpenMacLink()) goto DONE;
#endif
  if (!OpenMathFolio () < 0)
    goto DONE;
  //	InitMem(1);

  gIdleTimer = InitTimer (TM_TYPE_MICROSEC
                          + TM_MODE_DELTA); // allocate a cursor timer

  TestStringLen = (long)strlen ((char *)TestString);
  theRect[0].rectf16_XLeft = Convert32_F16 (20);
  theRect[0].rectf16_XRight = Convert32_F16 (300);
  theRect[0].rectf16_YTop = Convert32_F16 (20);
  theRect[0].rectf16_YBottom = Convert32_F16 (120);

  theRect[1].rectf16_XLeft = Convert32_F16 (20);
  theRect[1].rectf16_XRight = Convert32_F16 (300);
  theRect[1].rectf16_YTop = Convert32_F16 (20);
  theRect[1].rectf16_YBottom = Convert32_F16 (120);

  theRect[2].rectf16_XLeft = Convert32_F16 (20);
  theRect[2].rectf16_XRight = Convert32_F16 (92);
  theRect[2].rectf16_YTop = Convert32_F16 (20);
  theRect[2].rectf16_YBottom = Convert32_F16 (36);

  theRect[3].rectf16_XLeft = Convert32_F16 (20);
  theRect[3].rectf16_XRight = Convert32_F16 (92);
  theRect[3].rectf16_YTop = Convert32_F16 (20);
  theRect[3].rectf16_YBottom = Convert32_F16 (36);

  EnableVAVG (sc.sc_Screens[0]);
  EnableVAVG (sc.sc_Screens[1]);
  EnableHAVG (sc.sc_Screens[0]);
  EnableHAVG (sc.sc_Screens[1]);

  bgColor = 0x00080808;
  fgColor = 0x00FFFFFF;

  InitBackPic (&sc, "KFontviewer.art.img");
  CopyBlack (sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages);

  theAlign.align = justLeft;
  theAlign.charPitch = 0;
  theAlign.linePitch = 0;

  CopyBackPic (sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages);
  DisplayScreen (sc.sc_Screens[sc.sc_curScreen], 0);

  GetDirectory (origPathname, 200);
  dirItem = ChangeDirectory (Font1Path);
  GetDirectory (actualPathname, 200);

  dirItem = ChangeDirectory ("$app/System/Graphics/Fonts");
  GetDirectory (actualPathname2, 200);
  ChangeDirectory (origPathname);

  sprintf (string, "FONT=%s", Font1);
  row = 20;
  drawText (25, row, string);

  row += 20;
  drawText (25, row, "PRESS EITHER A OR C BUTTON");

  sprintf (string, "A = %s", actualPathname);
  row += 10;
  drawText (30, row, string);
  sprintf (string, "C = %s", actualPathname2);
  row += 10;
  drawText (30, row, string);

  while (TRUE)
    {
      lControlvals = ReadControlPad (JOYFIREA + JOYFIREC);
      if (lControlvals & JOYFIREA || lControlvals & JOYFIREC)
        break;
    }

  if (lControlvals & JOYFIREA)
    sprintf (EntireFontName, "%s/%s", actualPathname, Font1);
  else
    sprintf (EntireFontName, "%s/%s", actualPathname2, Font1);

  for (wait = 0; wait < WAIT_TIME; wait++)
    ;

  row += 20;
  drawText (25, row, "PRESS EITHER A OR C BUTTON");
  row += 10;
  drawText (40, row, " A = LOAD ENTIRE FONT   ");
  row += 8;
  drawText (40, row, "     INTO MEMORY        ");
  row += 10;
  drawText (40, row, " C = LOAD ONE CHAR INTO ");
  row += 8;
  drawText (40, row, "     MEMORY AT A TIME   ");

  while (TRUE)
    {
      lControlvals = ReadControlPad (JOYFIREA + JOYFIREC);
      if (lControlvals & JOYFIREA || lControlvals & JOYFIREC)
        break;
    }

  if (lControlvals & JOYFIREA)
    fullRead = true;
  else
    fullRead = false;

  row += 20;

  if (fullRead)
    drawText (25, row, "Loading entire font file...");
  else
    drawText (25, row, "Loading font file header only...");

  row += 10;
  drawText (25, row, EntireFontName);

  if (gIdleTimer)
    GetTime (gIdleTimer, 1, &tv);

  err = KLoadFont (EntireFontName, &TextFont, fullRead);

  if (gIdleTimer)
    GetTime (gIdleTimer, 0, &tv);

  if (err == 0)
    {
      sprintf (string, "*** Loaded successfully ***");
      row += 10;
      drawText (25, row, string);

      if (gIdleTimer)
        {
          sprintf (string, "Load time: secs=%0d, usecs=%0d", tv.tv_sec,
                   tv.tv_usec);
          row += 30;
          drawText (25, row, string);
        }

      sprintf (string, "press any button to continue");
      row += 10;
      drawText (25, row, string);

      while (TRUE)
        if (ReadControlPad (JOYALL) & JOYALL)
          break;
    }
  else
    {
      sprintf (string, "Error loading %s", Font1);
      row += 20;
      drawText (25, row, string);

      if (err >= -6 && err <= -1)
        errorIndex = -err;
      else
        errorIndex = 0;

      sprintf (string, "ERR(%d) = %s", err, errorString[errorIndex]);
      row += 10;
      drawText (25, row, string);

      while (TRUE)
        if (ReadControlPad (JOYALL) & JOYSTART)
          goto DONE;
    }

  Text1_ptr = (uint8 *)" !\"#$%&'()*+,-./"
                       "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
                       "abcdefghijklmnopqrstuvwxyz{|}~";
  Text2_ptr = (uint8 *)"¦±²³´µÔÕÖÂ±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜ"
                       "ÝÊßÊÞ¢£¤¡Ì§Ì¨ÌÌªÌ«·¬·­·®À¯ÁÄÞ¯Ä";

  if (gIdleTimer)
    GetTime (gIdleTimer, 1, &tv);

  textccb[0] = KTextBox_A (&TextFont, Text1_ptr, strlen (Text1_ptr),
                           &theRect[0], &theAlign, fgColor, bgColor, NULL);
  textccb[1] = KTextBox_A (&TextFont, Text2_ptr, strlen (Text2_ptr),
                           &theRect[1], &theAlign, fgColor, bgColor, NULL);

  if (gIdleTimer)
    GetTime (gIdleTimer, 0, &tv);

  charsPerScreen = (strlen (Text1_ptr) + strlen (Text2_ptr)) >> 1;

  jisccb[0] = KTextBox_A (&TextFont, tmp, 8, &theRect[2], &theAlign, fgColor,
                          bgColor, NULL);
  jisccb[1] = KTextBox_A (&TextFont, tmp, 8, &theRect[3], &theAlign, fgColor,
                          bgColor, NULL);

  textccb[1]->ccb_YPos
      = Convert32_F16 (ConvertF16_32 (textccb[1]->ccb_YPos) + 115);
  jisccb[0]->ccb_XPos = Convert32_F16 (200);
  jisccb[0]->ccb_YPos = Convert32_F16 (100);
  jisccb[1]->ccb_XPos = Convert32_F16 (200);
  jisccb[1]->ccb_YPos = Convert32_F16 (206);

  LinkCel (textccb[0], textccb[1]);
  LinkCel (textccb[1], jisccb[0]);
  LinkCel (jisccb[0], jisccb[1]);
  LAST_CEL (jisccb[1]);

  high = 0x81;
  Text1_ptr = Text1;
  Text2_ptr = Text2;
  MakeText (high, Text1_ptr, Text2_ptr);

  while (TRUE)
    {

      /* React to the joystick */
      lControlvals = ReadControlPad (JOYALL);

      if (lControlvals & JOYSTART)
        goto DONE;

      if (lControlvals & JOYRIGHT)
        {
          textccb[0]->ccb_XPos
              = Convert32_F16 (ConvertF16_32 (textccb[0]->ccb_XPos) + 1);
          textccb[1]->ccb_XPos
              = Convert32_F16 (ConvertF16_32 (textccb[1]->ccb_XPos) + 1);
        }

      else if (lControlvals & JOYLEFT)
        {
          textccb[0]->ccb_XPos
              = Convert32_F16 (ConvertF16_32 (textccb[0]->ccb_XPos) - 1);
          textccb[1]->ccb_XPos
              = Convert32_F16 (ConvertF16_32 (textccb[1]->ccb_XPos) - 1);
        }

      else if (lControlvals & JOYUP)
        {
          textccb[0]->ccb_YPos
              = Convert32_F16 (ConvertF16_32 (textccb[0]->ccb_YPos) - 1);
          textccb[1]->ccb_YPos
              = Convert32_F16 (ConvertF16_32 (textccb[1]->ccb_YPos) - 1);
        }

      else if (lControlvals & JOYDOWN)
        {
          textccb[0]->ccb_YPos
              = Convert32_F16 (ConvertF16_32 (textccb[0]->ccb_YPos) + 1);
          textccb[1]->ccb_YPos
              = Convert32_F16 (ConvertF16_32 (textccb[1]->ccb_YPos) + 1);
        }

      else if (lControlvals & JOYFIREA || lControlvals & JOYFIREB
               || lControlvals & JOYFIREC)
        {

          if (lControlvals & JOYFIREC)
            {
              long randomNum;

              randomNum = rand ();

              if (randomNum < 0)
                randomNum *= -1;

              fgColor = ((randomNum * 0x00FFFFFF) / 32767);

              randomNum = rand ();

              if (randomNum < 0)
                randomNum *= -1;

              bgColor = ((randomNum * 0x00FFFFFF) / 32767);
            }

          else if (lControlvals & JOYFIREA)
            {
              bgColor = 0x00080808;
              fgColor = 0x00FFFFFF;
            }

          else
            {
              bgColor = 0x00FFFFFF;
              fgColor = 0x00080808;
            }

          for (wait = 0; wait < WAIT_TIME; wait++)
            ;
          MakeText (high, Text1_ptr, Text2_ptr);

          high++;
          if (high == 0x85)
            high = 0x88;
          if (high == 0xa0)
            high = 0xe0;
          if (high > 0xea)
            high = 0x81;

          if (gIdleTimer)
            GetTime (gIdleTimer, 1, &tv);

          textccb[0] = KTextBox_A (&TextFont, Text1_ptr, strlen (Text1_ptr),
                                   &theRect[0], &theAlign, fgColor, bgColor,
                                   textccb[0]);
          textccb[1] = KTextBox_A (&TextFont, Text2_ptr, strlen (Text2_ptr),
                                   &theRect[1], &theAlign, fgColor, bgColor,
                                   textccb[1]);

          if (gIdleTimer)
            GetTime (gIdleTimer, 0, &tv);

          charsPerScreen = (strlen (Text1_ptr) + strlen (Text2_ptr)) >> 1;

          jisccb[0]
              = KTextBox_A (&TextFont, jis_highbyte[count++], 8, &theRect[2],
                            &theAlign, fgColor, bgColor, jisccb[0]);
          jisccb[1]
              = KTextBox_A (&TextFont, jis_highbyte[count++], 8, &theRect[3],
                            &theAlign, fgColor, bgColor, jisccb[1]);

          textccb[1]->ccb_YPos
              = Convert32_F16 (ConvertF16_32 (textccb[1]->ccb_YPos) + 115);
          jisccb[0]->ccb_XPos = Convert32_F16 (200);
          jisccb[0]->ccb_YPos = Convert32_F16 (100);
          jisccb[1]->ccb_XPos = Convert32_F16 (200);
          jisccb[1]->ccb_YPos = Convert32_F16 (216);

          if (count > 77)
            count = 0;
        }
      else
        { // cycle the color
        }

      if (fontIndex >= kNumFontFiles)
        fontIndex = 0;

      /* SPORT the background back into the current buffer.
       * The call to the SPORT routine, which does a DoIO(), will cause us
       * to sync up to the vertical blank at this point.  After synching to
       * the vertical blank, we should have as short a path as
       * possible between here and the call to DisplayScreen(), with,
       * at best, only one call to DrawCels() standing between.
       */
      CopyBackPic (sc.sc_Bitmaps[sc.sc_curScreen], sc.sc_nFrameBufferPages);
      //		CopyBlack(sc.sc_Bitmaps[sc.sc_curScreen],
      //sc.sc_nFrameBufferPages );

      /*	Draw the current frame from the animation.
       */
      if (textccb == NULL)
        {
          DIAGNOSTIC ("Problem with TextBox - either bad input parameters or "
                      "out of memory.");
          goto DONE;
        }
      else
        {
          retvalue = DrawCels (sc.sc_BitmapItems[sc.sc_curScreen], textccb[0]);
          if (retvalue < 0)
            {
              DIAGNOSTIC ("DrawCels() failed");
            }
        }

      if (gIdleTimer)
        {
          sprintf (string, "chars=%0ld, secs=%0ld, usecs=%0ld", charsPerScreen,
                   tv.tv_sec, tv.tv_usec);
          drawText (25, 125, string);
        }
      else
        drawText (25, 125, EntireFontName);

      /*	Point the 3DO screen to the screen we just filled. The 3DO
       *	hardware will now draw this screen to the TV every VBlank.
       */
      retvalue = DisplayScreen (sc.sc_Screens[sc.sc_curScreen], 0);
      if (retvalue < 0)
        {
          DIAGNOSTIC ("DisplayScreen() failed");
          goto DONE;
        }

      /*	Toggle to the other screen for our next frame of animation.
       */
      sc.sc_curScreen = 1 - sc.sc_curScreen;
    }

DONE:

  FadeToBlack (&sc, FADE_FRAMECOUNT);

  if (gIdleTimer)
    FreeTimer (gIdleTimer);

  ShutDown ();
  printf ("%s has completed.\n\n", progname);
  return ((int)retvalue);
}

/* *************************************************************************
 * ***                                  ************************************
 * ***  Miscellaneous Utility Routines  ************************************
 * ***                                  ************************************
 * *************************************************************************
 */

void
CopyBackPic (Bitmap *bitmap, Int32 nFrameBufferPages)
{
  CopyVRAMPages (VRAMIOReq, bitmap->bm_Buffer, gBackPic, nFrameBufferPages,
                 -1);
  //	CopyVRAMPages( bitmap->bm_Buffer, gBackPic, nFrameBufferPages, -1 );
}

bool
InitBackPic (ScreenContext *sc, char *filename)
/* Allocates the gBackPic buffer.  If a filename is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no filename is specified, this
 * routine merely clears the gBackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
{
  bool retvalue;
  retvalue = FALSE;

  gBackPic = (ubyte *)ALLOCMEM ((int)(sc->sc_nFrameByteCount),
                                GETBANKBITS (GrafBase->gf_ZeroPage)
                                    | MEMTYPE_STARTPAGE | MEMTYPE_VRAM
                                    | MEMTYPE_CEL);

  if (NOT gBackPic)
    {
      DIAGNOSTIC ("WARNING: unable to allocate gBackPic");
      goto DONE;
    }

  SetVRAMPages (VRAMIOReq, gBackPic, 0, sc->sc_nFrameBufferPages, -1);

  if (LoadImage (filename, gBackPic, NULL, sc) == NULL)
    {
      DIAGNOSTIC ("WARNING: LoadImage failed");
      goto DONE;
    }
  CopyVRAMPages (VRAMIOReq, sc->sc_Bitmaps[sc->sc_curScreen]->bm_Buffer,
                 gBackPic, sc->sc_nFrameBufferPages, -1);

  retvalue = TRUE;

DONE:
  return (retvalue);
}

void
MakeText (uint8 high, uint8 *text1, uint8 *text2)
{
  uint8 i;
  if (high != 0x98 && high != 0xea)
    {
      for (i = 0x40; i <= 0x7e; i++)
        {
          *text1++ = high;
          *text1++ = i;
        }
      for (i = 0x80; i <= 0x9e; i++)
        {
          *text1++ = high;
          *text1++ = i;
        }
      *text1 = '\0';
      for (i = 0x9f; i <= 0xfc; i++)
        {
          *text2++ = high;
          *text2++ = i;
        }
      *text2 = '\0';
    }
  else if (high == 0x98)
    {
      for (i = 0x40; i <= 0x72; i++)
        {
          *text1++ = high;
          *text1++ = i;
        }
      *text1 = '\0';

      for (i = 0x9f; i <= 0xfc; i++)
        {
          *text2++ = high;
          *text2++ = i;
        }
      *text2 = '\0';
    }
  else if (high == 0xea)
    {
      for (i = 0x40; i <= 0x7e; i++)
        {
          *text1++ = high;
          *text1++ = i;
        }
      for (i = 0x80; i <= 0x9e; i++)
        {
          *text1++ = high;
          *text1++ = i;
        }
      *text1 = '\0';
      for (i = 0x9f; i <= 0xa2; i++)
        {
          *text2++ = high;
          *text2++ = i;
        }
      *text2 = '\0';
    }
}

void
CopyBlack (Bitmap *bitmap, Int32 nFrameBufferPages)
{
  SetVRAMPages (VRAMIOReq, bitmap->bm_Buffer, 0, nFrameBufferPages, -1);
}
