/**************************************************************************
 * 3d_example3_core : tests the 3d-lib                                    *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : April 1993                                                   *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************
 * This will create a single world with several objects. This uses the    *
 * LookAt() command, locked on one of the pyramids.                       *
 *                                                                        *
 * Exit the program using the "enter" button on the control unit. Use the *
 * pad to control position of the camera. Button A will turn on           *
 * delorisizing. Button B will roll the camera, and Button C will freeze  *
 * the demo.                                                              *
 **************************************************************************/

#include "audio.h"
#include "debug.h"
#include "event.h"
#include "folio.h"
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

#include "graphics.h"

#include "3dlib.h"
#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

#include "3d_examples.h"

COPYRIGHT_NOTICE;

/*** globals ***/

int32 Debug_flag = NO;
int32 Rotate_object = NO;
int32 Number_of_loops = 15;
int32 View_X = 0, View_Y = 0, Camera_angle = 0;
int32 Delta_x = 0, Delta_y = 0, Delta_camera_angle;
int32 Two_sounds = 1;

Item BitmapItems[2];
Bitmap *Bitmaps[2];
ubyte *BackPic = NULL;
ScreenContext Screencontext;
int32 Camera_id = 0;
int32 World_id = 0;
int32 Rotate_stuff = 0;

char *Celfile1 = "3d_data/sw.cel";
char *Celfile2 = "3d_data/alt.cel";
char *Backdrop_file = "3d_data/backdrop.3do";

CCB *Ccbs[5]; /* ccb */

/***********************************************************************
 * close_everything :                                                  *
 ***********************************************************************/
void
close_everything (void)
{
  ENTER ("close_everything");

  KillEventUtility ();

  EXIT ("close_everything");
}

/***********************************************************************
 * get_joystick_state : returns the current state of the joystick bits.*
 ***********************************************************************/
int32
get_joystick_state (void)
{
  ControlPadEventData cpaddata;

  ENTER ("get_joystick_state");

  cpaddata.cped_ButtonBits = 0;

  GetControlPad (1, 0, &cpaddata);

  EXIT ("get_joystick_state");

  return (cpaddata.cped_ButtonBits);
}

/***********************************************************************
 * getout : exits the program                                          *
 ***********************************************************************/
void getout (rvalue)

    int32 rvalue;
{
  ENTER ("getout");

  FadeToBlack (&Screencontext, FADE_FRAMECOUNT);
  close_everything ();
  printf ("exiting : %ld\n", rvalue);

  exit ((int)rvalue);
}

/**************************************************************************
 * init_cels : initializes any cels needed                                *
 **************************************************************************/
bool
init_cels (void)
{
  bool rvalue = TRUE;

  ENTER ("init_cels");

  Ccbs[0] = LoadCel (Celfile1, MEMTYPE_CEL);

  if (!(Ccbs[0]))
    {
      printf ("unable to load cel file %s\n", Celfile1);
      getout (GENERIC_ERROR);
    }

  Ccbs[1] = LoadCel (Celfile2, MEMTYPE_CEL);

  if (!(Ccbs[1]))
    {
      printf ("unable to load cel file %s\n", Celfile2);
      getout (GENERIC_ERROR);
    }

  EXIT ("init_cels");

  return (rvalue);
}

/**************************************************************************
 * init_graphics : uses the Lib3DO stuff                              *
 **************************************************************************/
bool
init_graphics (void)
{
  int i;
  int32 width, height;

  ENTER ("init_graphics");

  if (!OpenGraphics (&Screencontext, 2))
    return (FALSE);

  for (i = 0; i < 2; i++)
    {

      BitmapItems[i] = Screencontext.sc_BitmapItems[i];
      Bitmaps[i] = Screencontext.sc_Bitmaps[i];
    }

  width = Bitmaps[0]->bm_Width;
  height = Bitmaps[0]->bm_Height;

  EXIT ("init_graphics");

  return (TRUE);
}

/**************************************************************************
 * init_stuff : initializes everything                                    *
 **************************************************************************/
void
init_stuff (void)
{
  ENTER ("init_stuff");

  init_system ();

  init_graphics ();

  init_cels ();

  BackPic = (ubyte *)LoadImage (Backdrop_file, NULL, NULL, &Screencontext);

  Init3DLib (MONO_VIEW, 0);

  init_worlds ();

  EXIT ("init_stuff");
}

/**************************************************************************
 * init_system : initializes main system level stuff, libraries, etc.     *
 **************************************************************************/
void
init_system (void)
{
  ENTER ("init_system");

  if (OpenMathFolio () < 0)
    {
      printf ("\n\nunable to open math folio!\n\n");
      getout (GENERIC_ERROR);
    }

  if (OpenGraphicsFolio () < 0)
    {
      printf ("unable to open graphics folio!\n");
      getout (GENERIC_ERROR);
    }

  if (!OpenMacLink ())
    {
      printf ("unable to open mac link!\n");
      getout (GENERIC_ERROR);
    }

  if (InitEventUtility (1, 0, LC_Observer) < 0)
    {
      printf ("unable to open the event broker\n");
      getout (GENERIC_ERROR);
    }

  if (OpenAudioFolio () < 0)
    {
      printf ("unable to open Audiofolio!\n");
      getout (GENERIC_ERROR);
    }

  ChangeInitialDirectory (NULL, NULL, TRUE);

  EXIT ("init_system");
}
/***********************************************************************
 * main_loop :                                                         *
 ***********************************************************************/
int32
main_loop (void)
{
  int32 rvalue;
  int32 screen_select; /* determines which screen to write to */
  int32 counter;

  rvalue = screen_select = counter = 0;

  ENTER ("main_loop");

  for (;;)
    {
      /*** if the user has pressed the START button, exit ***/

      if (process_joystick () == START_BUTTON)
        goto DONE;

      if (!DrawImage (Screencontext.sc_Screens[screen_select], BackPic,
                      &Screencontext))
        {
          printf ("DrawImage failed!\n");
        }

      render_world (BitmapItems[screen_select], 0, Camera_id, World_id);

      if (rvalue < 0)
        {
          kprintf ("DrawCels failed!\n");
          goto DONE;
        }

      rvalue = DisplayScreen (Screencontext.sc_Screens[screen_select], 0);

      if (rvalue < 0)
        {
          printf ("DisplayScreen failed!\n");
          goto DONE;
        }

      /*** alternates buffers ***/

      if (screen_select == 0)
        screen_select = 1;
      else
        screen_select = 0;

      counter++;
    }

  EXIT ("main_loop");

DONE:
  return (rvalue);
}

/**************************************************************************
 * process_args :                                                         *
 **************************************************************************/
void process_args (argc, argv)

    int32 argc;
char *argv[];
{
  char c, *progname, *ptr;
  int32 i;

  ENTER ("process_args");

  progname = *argv++;

  for (i = 0; i < argc; i++)
    {
      ptr = argv[i];

      switch (c = *argv[i++])
        {
        case 's':
          sprintf (Celfile1, "%s", argv[i]);
          break;

        case 'b':
          sprintf (Backdrop_file, "%s", argv[i]);
          break;

        case 'd':
          Debug_flag = YES;
          break;

        case 'h':
        case '?':
          kprintf (
              "usage:  %s b <3do image name> s <cel file name> h? l <loops>\n",
              progname);
          kprintf (
              "        b      - backdrop image to use, %s is the default\n",
              Backdrop_file);
          kprintf ("        s      - cel to use, %s is the default\n",
                   Celfile1);
          kprintf ("        h or ? - Print this help page\n");
          break;
          break;

        default:
          kprintf ("ERROR:  unknown command arg %c\n", c);
          break;
        }
    }

  EXIT ("process_args");
}

/**************************************************************************
 * process_joystick :                                                     *
 **************************************************************************/
int32
process_joystick (void)
{
  int32 rvalue;
  int32 joybits;
  static int32 pixc = SOLID_CEL;

  rvalue = 0;

  joybits = get_joystick_state ();

  if (joybits & ControlStart)
    {
      rvalue = START_BUTTON;
      goto DONE;
    }

  /*** toggle between worlds ***/

  if (joybits & ControlA)
    pixc = TRANSLUCENT_CEL;

  if (joybits & ControlB)
    Delta_camera_angle = (1 << 15);

  /*** toggle the animation on and off ***/

  if (joybits & ControlC)
    Rotate_stuff = 0;

  /*** start motion on the first click, stop on the second ***/

  if (joybits & ControlDown)
    Delta_y = -(1 << 12);

  if (joybits & ControlUp)
    Delta_y = (1 << 12);

  if (joybits & ControlLeft)
    Delta_x = -(1 << 12);

  if (joybits & ControlRight)
    Delta_x = (1 << 12);

  if (!joybits)
    {
      pixc = SOLID_CEL;
      Delta_camera_angle = 0;
      Delta_x = 0;
      Delta_y = 0;
      Rotate_stuff = 1;
    }

  Ccbs[0]->ccb_PIXC = pixc;

DONE:
  return (rvalue);
}

/**************************************************************************
 * main :                                                                 *
 **************************************************************************/
int
main (argc, argv)

int32 argc;
char *argv[];
{
  int32 rvalue = 0;

  process_args (argc, argv);

  init_stuff ();

  main_loop ();

  getout (rvalue);
}
