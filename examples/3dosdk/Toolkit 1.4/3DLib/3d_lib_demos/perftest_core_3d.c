/**************************************************************************
 * perftest_core : 3dlib performance tester                               *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : June 1993                                                    *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************
 * This is used to demonstrate general performance levels of 3d.lib. It   *
 * will create a single world and a field of 25 cubes in a 5 by 5 array.  *
 * Button A will cause any eyepoint or cube motion to freeze, Button B    *
 * will cycle you through several different options (the current options  *
 * will be printed out to the terminal window), and Button C will move    *
 * your eyepoint along the direction of sight letting you fly around the  *
 * field. Use the control pad to aim you line of sight.                   *
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

#include "perftest_3d.h"

#define MAX_SCREENS 3 /* 3 for stereo, 2 for mono */
#define VIEW_MODE MONO_VIEW

/*** globals ***/

int32 Debug_flag = NO;
int32 Rotate_object = YES;
int32 Rotate_stuff = YES;
int32 Number_of_loops = 15;
int32 View_X = 0, View_Y = 0, View_Z = (0x00000001 << 14), Velocity = 0;
int32 View_roll = 0, View_pitch = 0, View_yaw = 0;
int32 Delta_x = 0, Delta_y = 0;
int32 Delta_pitch, Delta_yaw, Delta_roll;
int32 Camera_id, World_id;
extern int32 State_cycle, State_changed;

Item BitmapItems[MAX_SCREENS];
Bitmap *Bitmaps[MAX_SCREENS];
ubyte *BackPic = NULL;
ScreenContext Screencontext;
extern WorldObject World_camera1;

extern WorldPort World1;
extern WorldObject *Objects[];

char Celfile1[40] = "3d_data/sw.cel";
// char Celfile2[40]="3d_data/sw32.cel";
// char Celfile2[40]="3d_data/sw256.cel.packed";

int32 ScreenPageCount;
static int32 Render_screen;

uint32 CurrentSeconds (struct timeval *tv);
void collecttimedata (Item bitmap, int32 display);
void InitInstrumentation (void);

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
void
getout (int32 rvalue)
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
  char filename[30];

  ENTER ("init_cels");

  sprintf (filename, "%s", Celfile1);

  Ccbs[HI_RES_IMAGE] = LoadCel (filename, MEMTYPE_CEL);

  if (Ccbs[HI_RES_IMAGE])
    {
      Ccbs[HI_RES_IMAGE]->ccb_Flags |= CCB_BGND;
      Ccbs[HI_RES_IMAGE]->ccb_PIXC = SOLID_CEL;
      Ccbs[HI_RES_IMAGE]->ccb_PIXC = SOLID_CEL;
    }
  else
    {
      printf ("unable to load %s\n", filename);
      getout (GENERIC_ERROR);
    }

  /*
     sprintf(filename,"%s",Celfile2);

     Ccbs[LO_RES_IMAGE]=LoadCel(filename,MEMTYPE_CEL);

     if(Ccbs[LO_RES_IMAGE])
     {
        Ccbs[LO_RES_IMAGE]->ccb_PIXC=SOLID_CEL;
        Ccbs[LO_RES_IMAGE]->ccb_PIXC=SOLID_CEL;
     }
     else
     {
        printf("unable to load %s\n",filename);
        getout(GENERIC_ERROR);
     }
  */

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

  if (!OpenGraphics (&Screencontext, MAX_SCREENS))
    return (FALSE);

  for (i = 0; i < MAX_SCREENS; i++)
    {

      BitmapItems[i] = Screencontext.sc_BitmapItems[i];
      Bitmaps[i] = Screencontext.sc_Bitmaps[i];
    }

  width = Bitmaps[0]->bm_Width;
  height = Bitmaps[0]->bm_Height;

  ScreenPageCount = (width * 2 * height + GrafBase->gf_VRAMPageSize - 1)
                    / GrafBase->gf_VRAMPageSize;

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

  Render_screen = Init3DLib (VIEW_MODE, 0);

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
      printf ("\n\nOperamath returned an error during FindMathFolio!!!\n\n");
      getout (GENERIC_ERROR);
    }

  if (OpenGraphicsFolio () < 0)
    {
      printf ("unable to open GraphicsFolio!\n");
      getout (GENERIC_ERROR);
    }

  if (OpenAudioFolio () < 0)
    {
      printf ("unable to open Audiofolio!\n");
      getout (GENERIC_ERROR);
    }

  if (!OpenMacLink ())
    {
      printf ("mac link failed!\n");
      getout (GENERIC_ERROR);
    }

  if (InitEventUtility (1, 0, LC_Observer) < 0)
    {
      printf ("unable to open the event broker\n");
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
  static int32 curtime, prevtime = 0;
  static int32 rvalue = 0;
  static int32 counter = 0;
  static Item ioreq, vblio;
  static int32 firsttime = TRUE;
  struct timeval tv;

  ENTER ("main_loop");

  if (firsttime)
    {
      ioreq = GetVRAMIOReq ();
      vblio = GetVBLIOReq ();

      firsttime = 0;
    }

  EnableVAVG (Screencontext.sc_Screens[0]);
  EnableHAVG (Screencontext.sc_Screens[0]);
  EnableVAVG (Screencontext.sc_Screens[1]);
  EnableHAVG (Screencontext.sc_Screens[1]);

  InitInstrumentation ();

  for (;;)
    {
      /*** if the user has pressed the START button, exit ***/

      if (process_joystick () == START_BUTTON)
        goto DONE;

      WaitVBL (vblio, 0);
      SetVRAMPages (ioreq, Bitmaps[Render_screen]->bm_Buffer,
                    MakeRGB15Pair (8, 0, 3), ScreenPageCount, -1);
      render_world (BitmapItems[Render_screen], 0, Camera_id, World_id);

      if (rvalue < 0)
        {
          kprintf ("DrawCels failed!\n");
          goto DONE;
        }

      if (!(counter % 75))
        collecttimedata (BitmapItems[Render_screen], 1);
      else
        collecttimedata (BitmapItems[Render_screen], 0);

      if (!(counter % 75))
        {
          curtime = CurrentSeconds (&tv);
          printf ("%ld frames, delta seconds=%ld\n", counter,
                  (curtime - prevtime));
          prevtime = curtime;
        }

      Render_screen = SwapScreens (Screencontext);

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

        case 'd':
          Debug_flag = YES;
          break;

        case 'h':
        case '?':
          kprintf (
              "usage:  %s b <3do image name> s <cel file name> h? l <loops>\n",
              progname);
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
  int32 rvalue = 0;
  int32 joybits;
  static int32 delta_yaw = 0, delta_pitch = 0, delta_roll = 0;
  int32 delta_velocity = 0;
  int32 delta_angle = (1 << 12);
  int32 max_delta = 8 << 16;
  int32 min_delta = -8 << 16;

  ENTER ("process_joystick");

  joybits = get_joystick_state ();

  if (joybits & ControlStart)
    {
      rvalue = START_BUTTON;
      goto DONE;
    }

  /*** toggle between worlds ***/

  if (joybits & ControlA)
    {
      delta_velocity = 0;
      Velocity = 0;

      delta_pitch = 0;
      delta_yaw = 0;
      delta_roll = 0;
      delta_velocity = 0;

      Velocity = 0;

      while (get_joystick_state () & ControlA)
        ; /* hang until ButtonA is released */

      NormalizeMatrix (World_camera1.wo_ObjectMatrix);
    }

  if (joybits & ControlB)
    {
      State_changed = YES;
      State_cycle++;
      State_cycle %= 7;
      delta_velocity = (0);
    }

  if (joybits & ControlC)
    delta_velocity = (1 << 10);

  /*** start motion on the first click, stop on the second ***/

  if (joybits & ControlDown)
    {
      delta_pitch -= (delta_angle);
    }

  if (joybits & ControlUp)
    {
      delta_pitch += (delta_angle);
    }

  if (joybits & ControlLeft)
    {
      delta_yaw -= delta_angle;
    }

  if (joybits & ControlRight)
    {
      delta_yaw += delta_angle;
    }

  if (!joybits)
    {
    }

  /*** set and clip the velocity ***/

  Velocity += delta_velocity;

  if (Velocity > (1 << 17))
    Velocity = (1 << 17);
  else if (Velocity < (-1 << 17))
    Velocity = -1 << 17;

  /*** set and clip the angles and deltas ***/

  if (delta_pitch > max_delta)
    delta_pitch = max_delta;
  else if (delta_pitch < min_delta)
    delta_pitch = min_delta;

  if (delta_roll > max_delta)
    delta_roll = max_delta;
  else if (delta_roll < min_delta)
    delta_roll = min_delta;

  View_pitch += delta_pitch;
  View_yaw += delta_yaw;
  View_roll += delta_roll;

  Delta_pitch = delta_pitch;
  Delta_yaw = delta_yaw;
  Delta_roll = delta_roll;

  /*
  printfF16("pitch=",Delta_pitch);
  printfF16("roll  =",Delta_roll);
  */

  EXIT ("process_joystick");

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
  process_args (argc, argv);

  init_stuff ();

  main_loop ();

  getout (0);
}
