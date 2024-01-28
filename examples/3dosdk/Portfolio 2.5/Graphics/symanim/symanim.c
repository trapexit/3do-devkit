
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: symanim.c,v 1.13 1995/02/03 02:29:41 mattm Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/symanim
|||	symanim - Demonstrates playing animations with optional rotation and
|||	    scaling.
|||
|||	  Synopsis
|||
|||	    symanim
|||
|||	  Description
|||
|||	    This program animates the 3 components of the 3DO logo on top of a
|||	    background display. While the program is running, the user can do
the
|||	    following:
|||
|||	    * A buttonAdds a set of 3 components to the animation.
|||
|||	    * B buttonStarts rotating the components in 2D. Pressing the button
starts
|||	      3D rotation. Pressing the button again stops rotation.
|||
|||	    * C buttonStarts to rapidly resize the components from large to
small and
|||	      back. Pressing the button again stops this.
|||
|||	    * P buttonPauses and unpauses the animation.
|||
|||	    * X buttonExits the program.
|||
|||	    The program adapts to NTSC vs. PAL automatically. Upon startup, the
|||	    program loads in an image file to use as a backdrop. It also loads
three
|||	    ANIM files to use for the three components of the 3DO logo.
Different
|||	    versions of the pictures are loaded depending on whether the
display is
|||	    NTSC or PAL.
|||
|||	    Once all components are loaded, a screen group with two screens is
|||	    created. The two screens are used for double-buffering. Throughout
the
|||	    animation, we use a SPORT transfer to copy the backdrop image from
its
|||	    buffer into the screen buffer that is not currently displayed. We
then
|||	    render the current set of components onto the screen buffer. Once
all this
|||	    is completed, we swap screen buffers, which makes the rendering
visible.
|||	    The cycle starts again with the newly hidden screen buffer.
|||
|||	  Associated Files
|||
|||	    symanim.c, sprite.c, sprite.h
|||
|||	  Location
|||
|||	    examples/Graphics/symanim
|||
**/

#include "debug3do.h"
#include "displayutils.h"
#include "event.h"
#include "graphics.h"
#include "hardware.h"
#include "kernel.h"
#include "mem.h"
#include "operamath.h"
#include "sprite.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "utils3do.h"

/*****************************************************************************/

typedef enum DemoFiles
{
  DF_Background,
  DF_3Logo,
  DF_DLogo,
  DF_OLogo
} DemoFiles;

static const char *palFiles[]
    = { "vspics/PAL/testscr.imag", "vspics/PAL/test3.cel",
        "vspics/PAL/testd50.cel", "vspics/PAL/testo50.cel" };

static const char *ntscFiles[]
    = { "vspics/NTSC/reef.imag", "vspics/NTSC/test3.cel",
        "vspics/NTSC/testd50.cel", "vspics/NTSC/testo50.cel" };

/*****************************************************************************/

#define NUM_SCREENS 2

static ScreenContext sc;
static Item vramIOReq;
static void *backdrop;

/*****************************************************************************/

static void
MainLoop (void)
{
  int32 frame;
  int32 symmetry;
  bool paused;
  uint8 currentScreen;
  ControlPadEventData cped;
  uint32 bits;
  Err err;

  frame = 0;
  symmetry = 1;
  paused = FALSE;
  currentScreen = 0;
  bits = 0;

  while (TRUE)
    {
      err = GetControlPad (1, paused, &cped);
      if (err < 0)
        {
          printf ("GetControlPad() failed: ");
          PrintfSysErr (err);
          break;
        }

      if (err == 1)
        bits = cped.cped_ButtonBits;
      else
        bits = 0;

      frame++;
      if (bits)
        frame = 0;

      if (bits & ControlX)
        break;

      if (bits & ControlStart)
        paused = !paused;

      if (bits & ControlA)
        symmetry = SetSymOrder (symmetry + 1);

      MoveSprites ();

      if (bits & ControlC)
        zscaling = !zscaling;

      if (bits & ControlB)
        {
          rotating += 1;
          if (rotating > 2)
            rotating = 0;
        }

      if (frame >= 7200)
        {
          frame = 0;
          zscaling = FALSE;
          rotating = 0;
        }

      if (frame >= 3600)
        rotating = 1;

      if (frame >= 5400)
        zscaling = TRUE;

      if (frame >= 6900)
        {
          rotating = 2;
          zscaling = FALSE;
        }

      CopyVRAMPages (vramIOReq, sc.sc_Bitmaps[currentScreen]->bm_Buffer,
                     backdrop, sc.sc_nFrameBufferPages, ~0);

      DrawSprites (sc.sc_BitmapItems[currentScreen]);
      DisplayScreen (sc.sc_Screens[currentScreen], 0);
      currentScreen = 1 - currentScreen;
    }
}

/*****************************************************************************/

int
main (int32 argc, char **argv)
{
  char **demoFiles;
  Err err;
  uint32 i;
  Item screenGroupItem;
  uint32 dispType;
  uint32 screenHeight;

  err = OpenGraphicsFolio ();
  if (err >= 0)
    {
      err = OpenMathFolio ();
      if (err >= 0)
        {
          vramIOReq = CreateVRAMIOReq ();
          if (vramIOReq >= 0)
            {
              err = InitEventUtility (1, 0, 1);
              if (err >= 0)
                {
                  QueryGraphics (QUERYGRAF_TAG_DEFAULTDISPLAYTYPE, &dispType);
                  if ((dispType == DI_TYPE_PAL1) || (dispType == DI_TYPE_PAL2))
                    {
                      dispType = DI_TYPE_PAL2;
                      demoFiles = (char **)palFiles;
                      screenHeight = 288;
                    }
                  else
                    {
                      dispType = DI_TYPE_NTSC;
                      demoFiles = (char **)ntscFiles;
                      screenHeight = 240;
                    }

                  screenGroupItem = CreateScreenGroupVA (
                      sc.sc_Screens, CSG_TAG_SCREENCOUNT, NUM_SCREENS,
                      CSG_TAG_SCREENHEIGHT, screenHeight,
                      CSG_TAG_DISPLAYHEIGHT, screenHeight, CSG_TAG_DISPLAYTYPE,
                      dispType, CSG_TAG_DONE);
                  if (screenGroupItem >= 0)
                    {
                      for (i = 0; i < NUM_SCREENS; i++)
                        {
                          EnableHAVG (sc.sc_Screens[i]);
                          EnableVAVG (sc.sc_Screens[i]);
                          sc.sc_Bitmaps[i]
                              = ((Screen *)LookupItem (sc.sc_Screens[i]))
                                    ->scr_TempBitmap;
                          sc.sc_BitmapItems[i] = sc.sc_Bitmaps[i]->bm.n_Item;
                        }

                      sc.sc_nFrameBufferPages
                          = (sc.sc_Bitmaps[0]->bm_Width
                                 * sc.sc_Bitmaps[0]->bm_Height * 2
                             + (GetPageSize (MEMTYPE_VRAM) - 1))
                            / GetPageSize (MEMTYPE_VRAM);
                      sc.sc_nFrameByteCount = sc.sc_nFrameBufferPages
                                              * GetPageSize (MEMTYPE_VRAM);
                      sc.sc_nScreens = NUM_SCREENS;

                      AddScreenGroup (screenGroupItem, NULL);

                      backdrop = LoadImage (demoFiles[DF_Background], NULL,
                                            NULL, &sc);
                      if (backdrop)
                        {
                          InitSprites (sc.sc_Bitmaps[0]->bm_Width,
                                       sc.sc_Bitmaps[0]->bm_Height);

                          err = LoadSprite (demoFiles[DF_DLogo],
                                            Convert32_F16 (1),
                                            Convert32_F16 (2), 0x00002000,
                                            Convert32_F16 (2));
                          if (err >= 0)
                            {
                              err = LoadSprite (demoFiles[DF_OLogo],
                                                Convert32_F16 (2),
                                                Convert32_F16 (2), 0x00002800,
                                                Convert32_F16 (3));
                              if (err >= 0)
                                {
                                  err = LoadSprite (
                                      demoFiles[DF_3Logo], Convert32_F16 (1),
                                      Convert32_F16 (1), 0x00001000,
                                      Convert32_F16 (1));
                                  if (err >= 0)
                                    {
                                      CopyVRAMPages (
                                          vramIOReq,
                                          sc.sc_Bitmaps[0]->bm_Buffer,
                                          backdrop, sc.sc_nFrameBufferPages,
                                          ~0);

                                      DisplayScreen (sc.sc_Screens[0], 0);
                                      MainLoop ();
                                      FadeToBlack (&sc, 45);
                                      RemoveScreenGroup (screenGroupItem);
                                    }
                                  else
                                    {
                                      printf ("LoadSprite(\"%s\") failed: ",
                                              demoFiles[DF_3Logo]);
                                      PrintfSysErr (err);
                                    }
                                }
                              else
                                {
                                  printf ("LoadSprite(\"%s\") failed: ",
                                          demoFiles[DF_OLogo]);
                                  PrintfSysErr (err);
                                }
                            }
                          else
                            {
                              printf ("LoadSprite(\"%s\") failed: ",
                                      demoFiles[DF_DLogo]);
                              PrintfSysErr (err);
                            }

                          UnloadSprites ();
                          UnloadImage (backdrop);
                        }
                      else
                        {
                          printf ("LoadImage(\"%s\") failed\n",
                                  demoFiles[DF_Background]);
                        }
                      DeleteScreenGroup (screenGroupItem);
                    }
                  else
                    {
                      printf ("CreateScreenGroup() failed: ");
                      PrintfSysErr (screenGroupItem);
                    }
                  KillEventUtility ();
                }
              else
                {
                  printf ("InitEventUtility() failed: ");
                  PrintfSysErr (err);
                }
              DeleteVRAMIOReq (vramIOReq);
            }
          else
            {
              printf ("CreateVRAMIOReq() failed: ");
              PrintfSysErr (vramIOReq);
            }
          CloseMathFolio ();
        }
      else
        {
          printf ("OpenMathFolio() failed: ");
          PrintfSysErr (err);
        }
      CloseGraphicsFolio ();
    }
  else
    {
      printf ("OpenGraphicsFolio() failed: ");
      PrintfSysErr (err);
    }

  return 0;
}
