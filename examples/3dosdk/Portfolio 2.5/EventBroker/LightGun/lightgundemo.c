
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: lightgundemo.c,v 1.11 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/lightgundemo
|||	lightgundemo - Simple example program which uses lightgun.c functions.
|||
|||	  Synopsis
|||
|||	    lightgundemo
|||
|||	  Description
|||
|||	    Displays a static image and uses the convenience routines in
lightgun.c to
|||	    handle lightgun and control pad events. Prints coordinates to the
Debugger
|||	    Terminal window after every gun shot pointed at the screen.
|||
|||	    The program also calibrates itself on a target in the upper left
hand
|||	    corner.
|||
|||	  Associated Files
|||
|||	    lightgun.c, lightgun.h, lightgundemo.c
|||
|||	  Location
|||
|||	    examples/EventBroker/LightGun
|||
**/

#include "event.h"
#include "graphics.h"
#include "init3do.h"
#include "io.h"
#include "item.h"
#include "kernel.h"
#include "msgport.h"
#include "parse3do.h"
#include "task.h"
#include "types.h"
#include "utils3do.h"

#include "lightgun.h"

#include "stdio.h"
#include "stdlib.h"

#define NUM_SCREENS 2
#define STACKSIZE 10000

ScreenContext screen;
LG_EventData lged;
ControlPadEventData cped;
MsgPort *msgPort;
Item msgPortItem, msgItem;

int
main (int argc, char *argv[])
{
  Item ioreqVRAM, ioreqVBL;
  Err error;
  ubyte *background;
  CCB *bullet;
  int32 result, calResult, cnt = 0;

  if (CreateBasicDisplay (&screen, DI_TYPE_NTSC, NUM_SCREENS) < 0)
    {
      printf ("Fatal Error - Unable to create display.\n");
      exit (-1);
    }

  if ((background = (ubyte *)LoadImage ("gunsight.imag", NULL, NULL, &screen))
      == NULL)
    {
      printf ("Fatal Error - Unable to load backgrounnd image.\n");
      exit (-1);
    }

  if ((bullet = LoadCel ("bullet.cel", MEMTYPE_CEL)) == NULL)
    {
      printf ("Fatal Error - Unable to load bullet cel.\n");
      exit (-1);
    }

  LG_ConnectEventBroker (&msgPort, &msgPortItem, &msgItem, 0);

  ioreqVRAM = CreateVRAMIOReq ();
  ioreqVBL = CreateVBLIOReq ();

  if (DrawImage (screen.sc_Screens[screen.sc_curScreen], background, &screen)
      == FALSE)
    printf ("Unable to draw background image.\n");

  if ((error = DisplayScreen (screen.sc_Screens[screen.sc_curScreen], 0)) < 0)
    PrintfSysErr (error);

  while (TRUE)
    {
      screen.sc_curScreen = 1 - screen.sc_curScreen;

      result = LG_WaitEvent (&screen, msgPort, msgPortItem, msgItem, &lged,
                             &cped);

      if (result & LG_LIGHTGUN_EVENT)
        {
          if (lged.eventData.lged_ButtonBits & LG_CALIBRATION_BUTTON)
            {
              printf ("Calibrate gun now on upper left hand corner target...");
              calResult = LG_CalibrateGun (&screen, msgPort, msgPortItem,
                                           msgItem, 40, 40);
              if (calResult == 0)
                printf ("calibration not done.\n");
              else
                printf ("calibration done.\n");
            }
          else if (lged.eventData.lged_LinePulseCount > 0)
            {
              printf ("Gun:  (%d, %d)  bits = %lx\n", lged.x, lged.y,
                      lged.eventData.lged_ButtonBits);
              cnt = 60;
              bullet->ccb_XPos = (lged.x - bullet->ccb_Width / 2) << 16;
              bullet->ccb_YPos = (lged.y - bullet->ccb_Height / 2) << 16;
            }
          else
            {
              printf ("Lightgun shot offscreen\n");
            }
        }

      if (result & LG_CONTROLPAD_EVENT)
        {
          printf ("control pad event\n");
          if (cped.cped_ButtonBits & ControlStart)
            {
              printf ("Goodbye\n");
              exit (0);
            }
        }

      if (result < 0)
        printf ("Error!\n");

      if (DrawImage (screen.sc_Screens[screen.sc_curScreen], background,
                     &screen)
          == FALSE)
        printf ("Unable to draw background image.\n");

      if (cnt > 0)
        {
          cnt--;
          if (DrawCels (screen.sc_BitmapItems[screen.sc_curScreen], bullet)
              < 0)
            printf ("Unable to draw bullet cel.\n");
        }

      if ((error = DisplayScreen (screen.sc_Screens[screen.sc_curScreen], 0))
          < 0)
        PrintfSysErr (error);
    }
}
