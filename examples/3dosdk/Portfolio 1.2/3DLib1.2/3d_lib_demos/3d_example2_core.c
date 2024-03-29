/**************************************************************************
 * 3d_example2_core : tests the 3d-lib                                  *
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
 * This will create two worlds. World 1 has 5 objects : A tumbling cube   *
 * placed at the origin, a tall pyramid orbiting the cube. The pyramid    *
 * is a double-layered object, made of both pyramid and cube data sets.   *
 * The cube is displayed when it is close, the pyramid data, when far.    *
 * Behind the center cube is a pyramid rotating around the Y axis.        *
 * This world has 2 cameras, one movable in front of the objects, and     *
 * static to the side.                                                    *
 *                                                                        *
 * The second world is a single tumbling pyramid.                         *
 *                                                                        *
 * The up position of the joypad will move camera1 up, the bottom one     *
 * will move it down the Y axis. The left and right positions will move   *
 * it back and fourth along the X axis. Pressing the A button will        *
 * the view between both worlds, the B button will cause camera 1 to      *
 * pan CW, and button C will toggle between both cameras in world1        *
 **************************************************************************
 * This example also is good for other things. It demonstrates how to     *
 * gain access to the joypad via the EventBroker, and contains some       *
 * nifty routines to load 3do cel files and screen files. For leaning     *
 * about the 3d lib, you really don't have to understand that junk,       *
 * but it was needed to get a good, almost real-world, demo.              *
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
  int32 rvalue = 0;
  int32 screen_select = 0; /* determines which screen to write to */
  int32 counter = 0;

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
          printf ("DrawCels failed!\n");
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
  static int32 delta_y = 0, delta_x = 0, delta_camera_angle = 0;

  rvalue = 0;

  joybits = get_joystick_state ();

  if (joybits & ControlStart)
    {
      rvalue = START_BUTTON;
      goto DONE;
    }

  /*** toggle between worlds ***/

  if (joybits & ControlA)
    World_id = 1;

  if (joybits & ControlB)
    delta_camera_angle = (1 << 15);

  /*** toggle between cameras ***/

  if (joybits & ControlC)
    Camera_id = 1;

  /*** start motion on the first click, stop on the second ***/

  if (joybits & ControlDown)
    delta_y = -(1 << 12);

  if (joybits & ControlUp)
    delta_y = (1 << 12);

  if (joybits & ControlLeft)
    delta_x = -(1 << 12);

  if (joybits & ControlRight)
    delta_x = (1 << 12);

  if (!joybits)
    {
      delta_x = 0;
      delta_y = 0;
      Camera_id = 0;
      delta_camera_angle = 0;
      World_id = 0;
    }

  View_X += delta_x;
  View_Y += delta_y;
  Camera_angle += delta_camera_angle;

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
  int32 rvalue;

  rvalue = 0;

  process_args (argc, argv);

  init_stuff ();

  main_loop ();

  getout (rvalue);
}
