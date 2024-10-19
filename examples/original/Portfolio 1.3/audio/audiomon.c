/* $Id: audiomon.c,v 1.16 1994/02/18 01:54:55 limes Exp $ */

/********************************************************************
**
** Audio Monitor - Display DSP Resource Utilization
**
** By: Phil Burk
**
**
*********************************************************************
** 931029 PLB Adjusted max Rbase8 and TKA
** 931117 PLB Improved names for resources, IMM -> VAR, NMM -=> COD
** 931117 PLB Use new CreateCue(), Don't ChangeDirectory()
********************************************************************/

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
#include "semaphore.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

/* Macro to simplify error checking. */
#define CHECKRESULT(val, name)                                                \
  if (val < 0)                                                                \
    {                                                                         \
      Result = val;                                                           \
      PrintError (0, "\\Failure in", name, val);                              \
      goto cleanup;                                                           \
    }

#define VERSION "0.3"

#define PRT(x)                                                                \
  {                                                                           \
    printf x;                                                                 \
  }
#define ERR(x) PRT (x)
#define DBUG(x) PRT (x)

/* Graphic constants */
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define RED_MASK 0x7C00
#define GREEN_MASK 0x03E0
#define BLUE_MASK 0x001F
#define RED_SHIFT 10
#define GREEN_SHIFT 5
#define BLUE_SHIFT 0
#define ONE_RED (1 << REDSHIFT)
#define ONE_GREEN (1 << GREENSHIFT)
#define ONE_BLUE (1 << BLUESHIFT)
#define MAX_RED (RED_MASK >> RED_SHIFT)
#define MAX_GREEN (GREEN_MASK >> GREEN_SHIFT)
#define MAX_BLUE (BLUE_MASK >> BLUE_SHIFT)

/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

int32 InitDemo (void);
int32 AudioMonitor (void);
int32 ClearScreen (void);

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */

/* Graphics Context contains drawing information */
GrafCon GCon[2];

Item ScreenItems[2];
Item ScreenGroupItem = 0;
Item BitmapItems[2];
Bitmap *Bitmaps[2];

#define NUM_SCREENS 1

/********************************************************************/

char *
myitoa (int32 num, char *s, int32 radix)
{
#define MAXITOA 64
  char Pad[MAXITOA];
  int32 nd = 0, a, rem, div, i;
  int32 ifneg = FALSE;
  char *p, c;

  if (num < 0)
    {
      ifneg = TRUE;
      num = -num;
    }

#define HOLD(ch)                                                              \
  {                                                                           \
    Pad[nd++] = (ch);                                                         \
  }
  a = num;
  do
    {
      div = a / radix;
      rem = a - (div * radix);
      c = (char)rem + '0';
      HOLD (c);
      a = div;
    }
  while (a > 0);

  if (ifneg)
    HOLD ('-');

  /* Copy string to s. */
  p = s;
  for (i = 0; i < nd; i++)
    {
      *p++ = Pad[nd - i - 1];
    }
  *p++ = '\0';
  return s;
}
char *
itoar (int32 num, char *s, int32 radix, int32 width)
{
#define MAXITOA 64
  char Pad[MAXITOA];
  int32 nd = 0, a, rem, div, i;
  int32 nc;

  int32 ifneg = FALSE;
  char *p, c;

  if (num < 0)
    {
      ifneg = TRUE;
      num = -num;
    }

  nc = 0;

#define HOLD(ch)                                                              \
  {                                                                           \
    Pad[nd++] = (ch);                                                         \
  }
  a = num;
  do
    {
      div = a / radix;
      rem = a - (div * radix);
      c = (char)rem + '0';
      HOLD (c);
      a = div;
      nc++;
    }
  while ((a > 0) || (nc < width));

  if (ifneg)
    HOLD ('-');

  /* Copy string to s. */
  p = s;
  for (i = 0; i < nd; i++)
    {
      *p++ = Pad[nd - i - 1];
    }
  *p++ = '\0';
  return s;
}

/********************************************************************/
int32
DrawNumber (int32 num, int32 width)
{
  char Pad[100];
  itoar (num, Pad, 10, width);
  DrawText8 (&GCon[0], BitmapItems[0], Pad);
  return 0;
}

/********************************************************************/
void
DrawValue (int32 x, int32 y, char *msg, int32 num)
{
  MoveTo (&GCon[0], x, y);
  DrawText8 (&GCon[0], BitmapItems[0], msg);
  DrawNumber (num, 3);
}

/********************************************************************/

void
DrawRect (int32 XLeft, int32 YTop, int32 XRight, int32 YBottom)
{
  Rect WorkRect;
  WorkRect.rect_XLeft = XLeft;
  WorkRect.rect_XRight = XRight;
  WorkRect.rect_YTop = YTop;
  WorkRect.rect_YBottom = YBottom;
  FillRect (BitmapItems[0], &GCon[0], &WorkRect);
}

/********************************************************************/
int32
ClearScreen ()
{
  DBUG (("Clear Screen\n"));
  SetFGPen (&GCon[0], MakeRGB15 (4, 0, 6));
  DrawRect (0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

  return 0;
}

#define MAX_VAL_WIDTH (200)
#define MSG_LEFT_X (30)
#define LINE_LEFT_X (MSG_LEFT_X + 60)
#define LINE_TOP_Y (80)
#define LINE_HEIGHT (14)

/********************************************************************/
int32
DisplayValue (int32 Index, char *Msg, int32 Value, int32 MaxVal)
{
  int32 XPos, YPos;

  YPos = LINE_TOP_Y + 2 + Index * LINE_HEIGHT;
  DrawValue (MSG_LEFT_X, YPos, Msg, Value);

  SetFGPen (&GCon[0], MakeRGB15 (15, 15, 0));
  XPos = LINE_LEFT_X + ((MAX_VAL_WIDTH * Value) / MaxVal);
  DrawRect (LINE_LEFT_X, YPos, XPos, YPos + 6);
  SetFGPen (&GCon[0], MakeRGB15 (0, 0, 10));
  DrawRect (XPos, YPos, MAX_VAL_WIDTH + LINE_LEFT_X, YPos + 6);

  return 0;
}

/********************************************************************/
int32
AudioMonitor ()
{
  Item MyCue;
  int32 Result = 0;
  int32 Line;

  /* Initialize audio, return if error. */
  if (OpenAudioFolio ())
    {
      ERR (("Audio Folio could not be opened!\n"));
      return (-1);
    }

  MyCue = CreateCue (NULL);
  CHECKRESULT (MyCue, "CreateCue");

  while (1)
    {
      Line = 0;
      DisplayValue (Line++, "TKR:", DSPGetTicks (), 565);
      DisplayValue (Line++, "TKA:", DSPGetTotalRsrcUsed (DRSC_TICKS), 565);
      DisplayValue (Line++, "COD:", DSPGetTotalRsrcUsed (DRSC_N_MEM), 512);
      DisplayValue (Line++, "KNB:", DSPGetTotalRsrcUsed (DRSC_EI_MEM), 100);
      DisplayValue (Line++, "VAR:", DSPGetTotalRsrcUsed (DRSC_I_MEM), 256);
      DisplayValue (Line++, "EOM:", DSPGetTotalRsrcUsed (DRSC_EO_MEM), 16);
      DisplayValue (Line++, "Rb4:", DSPGetTotalRsrcUsed (DRSC_RBASE4),
                    256 / 4);
      DisplayValue (Line++, "Rb8:", DSPGetTotalRsrcUsed (DRSC_RBASE8),
                    256 / 8);
      DisplayValue (Line++, "InF:", DSPGetTotalRsrcUsed (DRSC_IN_FIFO), 13);
      DisplayValue (Line++, "OtF:", DSPGetTotalRsrcUsed (DRSC_OUT_FIFO), 4);

      SleepUntilTime (MyCue, 50 + GetAudioTime ());
    }

cleanup:
  DeleteCue (MyCue);
  CloseAudioFolio ();
  return 0;
}

/**********************************************************************/
void
DisplayBackdrop (void)
{

  MoveTo (&GCon[0], 80, 30);
  DrawText8 (&GCon[0], BitmapItems[0], "Audio Monitor ");
  DrawText8 (&GCon[0], BitmapItems[0], VERSION);
}

TagArg ScreenTags[]
    = { CSG_TAG_SCREENCOUNT, (void *)NUM_SCREENS, CSG_TAG_DONE, 0 };

/**********************************************************************/
int32
InitDemo (void)
/* This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns non-FALSE if all is well, FALSE if error
 */
{
  int32 retvalue;
  Screen *screen;
  int32 i;

  retvalue = OpenGraphicsFolio ();
  if (retvalue < 0)
    {
      ERR (("Could not open graphics folio!, Error = 0x%x\n", retvalue));
      goto DONE;
    }

  /*	DumpMemory(ScreenTags,sizeof(TagArg)*2); */

  ScreenGroupItem = CreateScreenGroup (ScreenItems, ScreenTags);
  /*	DumpMemory(ScreenTags,sizeof(TagArg)*2); */
  if (ScreenGroupItem < 0)
    {
      printf ("Error:  CreateScreenGroup() returned %ld\n", ScreenGroupItem);
      goto DONE;
    }
  AddScreenGroup (ScreenGroupItem, NULL);

  for (i = 0; i < NUM_SCREENS; i++)
    {
      screen = (Screen *)LookupItem (ScreenItems[i]);
      if (screen == 0)
        {
          ERR (("Huh?  Couldn't locate screen?"));
          goto DONE;
        }
      BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
      Bitmaps[i] = screen->scr_TempBitmap;
    }

  retvalue = FALSE;

DONE:
  return (retvalue);
}

/********************************************************************/
int
main (int argc, char *argv[])
{
  char *progname;
  int32 retvalue;

  progname = *argv++;
  printf ("%s %s\n", progname, VERSION);

  if ((retvalue = InitDemo ()) != 0)
    goto DONE;

  retvalue = DisplayScreen (ScreenItems[0], 0);
  if (retvalue < 0)
    {
      printf ("DisplayScreen() failed, error=%d\n", retvalue);
      goto DONE;
    }

  ClearScreen ();
  DisplayBackdrop ();
  AudioMonitor ();

DONE:
  /*	ResetSystemGraphics(); */
  printf ("\n%s sez:  bye!\n", progname);
  return ((int)retvalue);
}
