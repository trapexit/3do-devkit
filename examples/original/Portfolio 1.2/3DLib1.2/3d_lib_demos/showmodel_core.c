/**************************************************************************
 * 3d_example1_core : tests the 3d-Folio                                  *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        *
 * And introducing in its first ever starring role : 3D-Folio!!           *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : April 1993                                                   *
 *                                                                        *
 **************************************************************************
 * This will create a single world with 1 tumbling cube at the origin     *
 * and an object orbiting the cube.                                       *
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

#define MAX_SCREENS 3

/*** globals ***/

long Debug_flag = NO;
long Rotate_object = NO;
long Number_of_loops = 15;
long View_X = 0, View_Y = 0, View_Z = (-150 << 16), Camera_angle = 0;
long Delta_x = 0, Delta_y = 0, Delta_camera_angle;
int32 View_pitch = 0, View_yaw = 0;

Item BitmapItems[MAX_SCREENS];
Bitmap *Bitmaps[MAX_SCREENS];
ubyte *BackPic = NULL;
ScreenContext Screencontext;
long Camera_id = 0;
long World_id = 0;
long Rotate_stuff = 0;
static long Render_screen = 0;

char Model_file[50];

int32 ScreenPageCount;
int32 Cull_backface = FALSE;
int32 Show_outlines = FALSE;

/*** this stuff is used by loadcel() ***/

CCB *Ccbs[5]; /* ccb */

void
ResetSystemGraphics (void)
{
  /*** dummy routine to satisfy Lib3DO needs until that is fixed ***/
}

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
long
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

    long rvalue;
{
  ENTER ("getout");

  FadeToBlack (&Screencontext, FADE_FRAMECOUNT);
  close_everything ();
  printf ("exiting : %ld\n", rvalue);

  exit ((int)rvalue);
}

/**************************************************************************
 * init_graphics : uses the Lib3DO stuff                              *
 **************************************************************************/
bool
init_graphics (void)
{
  int i;
  long width, height;

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

  Render_screen = Init3DLib (MONO_VIEW, 0L);

  init_worlds ();

  ChangeInitialDirectory (NULL, NULL, TRUE);

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
      printf ("\n\nunable to open Mathfolio!!!\n\n");
      getout (GENERIC_ERROR);
    }

  if (OpenGraphicsFolio () < 0)
    {
      printf ("unable to open Graphicsfolio!\n");
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

  EXIT ("init_system");
}

/***********************************************************************
 * main_loop :                                                         *
 ***********************************************************************/
long
main_loop (void)
{
  long rvalue = TRUE;
  Item ioreq, vblio;

  ENTER ("main_loop");

  ioreq = GetVRAMIOReq ();
  vblio = GetVBLIOReq ();

  for (;;)
    {
      if (process_joystick () == START_BUTTON)
        goto DONE;

      WaitVBL (vblio, 0);
      SetVRAMPages (ioreq, Bitmaps[Render_screen]->bm_Buffer,
                    MakeRGB15Pair (3, 0, 8), ScreenPageCount, -1);

      render_world (BitmapItems[Render_screen], 0, Camera_id, World_id);

      Render_screen = SwapScreens (Screencontext);
    }

  DeleteItem (vblio);
  DeleteItem (ioreq);

  EXIT ("main_loop");

DONE:
  return (rvalue);
}

/**************************************************************************
 * process_args :                                                         *
 **************************************************************************/
void process_args (argc, argv)

    long argc;
char *argv[];
{
  char c, *progname, *ptr;
  long i;

  ENTER ("process_args");

  progname = *argv++;

  for (i = 0; i < argc; i++)
    {
      ptr = argv[i];

      switch (c = *argv[i++])
        {
        case 'o':
          Show_outlines = TRUE;
          break;

        case 'm':
          sprintf (Model_file, "%s", argv[i]);
          break;

        case 'd':
          Debug_flag = YES;
          break;

        case 'c':
          Cull_backface = TRUE;
          break;

        case 'h':
        case '?':
          printf ("usage:  %s m <model file name> \n", progname);
          printf ("         m      - model to use\n");
          printf ("         c      - turns on backface removal\n");
          printf ("         o      - turns on outlines\n");
          printf ("         h or ? - Print this help page\n");
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
long
process_joystick (void)
{
  long rvalue = 0;
  long joybits;
  static long delta_y = 0, delta_x = 0, delta_z = 0, delta_camera_angle = 0;
  static long pixc = SOLID_CEL;
  frac16 delta_angle = (1 << 16);

  ENTER ("process_joystick");

  joybits = get_joystick_state ();

  // printf("joybits=%lx\n",joybits);

  if (joybits & ControlStart)
    {
      rvalue = START_BUTTON;
      goto DONE;
    }

  /*** start motion on the first click, stop on the second ***/

  if (joybits & ControlDown)
    View_pitch -= delta_angle;

  if (joybits & ControlUp)
    View_pitch += delta_angle;

  if (joybits & ControlLeft)
    View_yaw -= delta_angle;

  if (joybits & ControlRight)
    View_yaw += delta_angle;

  if (!joybits)
    {
      pixc = SOLID_CEL;
      delta_x = 0;
      delta_y = 0;
      Rotate_stuff = 1;
      delta_camera_angle = 0;
    }

  /*
     View_X+=delta_x;
     View_Y+=delta_y;
  */

  View_Z += delta_z;

  Camera_angle += delta_camera_angle;

  EXIT ("process_joystick");

DONE:
  return (rvalue);
}

/**************************************************************************
 * main :                                                                 *
 **************************************************************************/
int
main (argc, argv)

long argc;
char *argv[];
{
  long rvalue = 0;

  process_args (argc, argv);

  init_stuff ();

  main_loop ();

  getout (rvalue);
}
