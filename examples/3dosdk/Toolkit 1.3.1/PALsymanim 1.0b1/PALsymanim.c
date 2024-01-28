/*
        File:		palsymanim.c

        Contains:	animations with optional
                                rotations
                                scaling
                                symmetry operations  Order of symmetry can be
   varied from 1 to 16.

        Written by:	David Maynard

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):
                 <8>	 3/10/94	MPH		modified program to
   deal with PAL. Used our OpenGraphicsNTSCPAL for display. <7>	 7/28/93
   JAY		modify WaitVBL, SetVRAMPages and CopyVRAMPages for Dragon Tail
                                                                        release
                 <6>	 6/23/93	JAY		change FindMathFolio to
   OpenMathFolio <5>	  4/7/93	JAY		MEMTYPE_SPRITE is now
   MEMTYPE_CEL for Cardinal2
                 <4>	  4/5/93	JAY		remove /remote/ from
   filename (i.e. make pathname relative to initial working directory <2>
   3/18/93	JAY		change lists.h to list.h <1>	 3/18/93
   JAY		first checked in for Cardinal build
                                 02/16/93	DSM		2B1 Release
                                 11/17/92	DSM		1.0 Release

        To Do:
*/

#define ENABLE_JOURNALING 0 /* '1 if demo scripting code is enabled */

#define VERSION "1.0"

#define BACKGROUND_COLOR 0

#include "debug.h"
#include "debug3DO.h"
#include "folio.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#include "Sprite.h"
#define GRAPHICSMASK 1
#define AUDIOMASK 2
#define SPORTIOMASK 4
#define MACLINKMASK 8

#if ENABLE_JOURNALING
#define TEST_SCRIPT_COUNT 1000 /* For Demo Scripting */

JoystickRecord TestScript[TEST_SCRIPT_COUNT];
#endif

extern Font Geneva9;
extern ulong Pip[];
#define EMULATE_SHERRY 1
extern long symorder = 1;
extern void SetSymOrder (long nsym);

static ScreenContext *sc;

int32 CelExtraFlags = 0;

/*  Animation globals  */
frac16 ticksperframe = Convert32_F16 (1);
frac16 gFrameRate = Convert32_F16 (1);

ubyte *gBackPic = NULL;

long gScreenSelect = 0;
/* extern Item gBitmapItems[2]; */
/* extern Bitmap	*gBitmaps[2]; */

/* Item	TheScreen[2];  */

extern int RedSi;
/* extern Bitmap	*gBitmaps[2];  */
/* extern long	gScreenByteCount; */

/* global IO Request for VRAM operations */

Item VRAMIOReq;

#define EYEOFFSET 50
#define EYECAMOFFSET 5
#define SPRINGZ 15

void Bye (void);

#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define NTSC_FREQUENCY 60
#define NTSC_TYPE DI_TYPE_NTSC
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define PAL_FREQUENCY 50
#define PAL_TYPE DI_TYPE_PAL2

// MOVE THIS TO STANDARD INTERFACE FILE
bool OpenGraphicsNTSCPAL (ScreenContext *sc, uint32 nScreens);

/*
        Purpose:	Create a screen context record with the number of
                                associated screens specified that matches the
   current video mode of the target machine (i.e., NTSC or PAL). Opens the
   Graphics Folio before doing anything...
        NOTE:		A 320x240 by 16-bit deep screen is 153,600 bytes.
                                A 388x288 by 16-bit deep screen is 211,184
   bytes. Thus the standard 2-screen usage will EITHER allocate buffers of 300K
   (NTSC) or 432K (PAL) (the overhead for either is the same).  Be sure you
   have enough memory! Inputs:	sc - A pointer to a screen context record (must
   be allocated). nScreens - number of screens to allocate (1-6) Outputs:
   OpenGraphicsNTSCPAL - returns TRUE if the screen context record was
   allocated.  FALSE if nScreens is out-of-range or the screen group could not
   be added.
*/
bool
OpenGraphicsNTSCPAL (ScreenContext *sc, uint32 nScreens)
{

  int32 width, height, diType, i;
  Item item;
  uint32 bmw[MAXSCREENS];
  uint32 bmh[MAXSCREENS];
  bool rslt = FALSE;
  TagArg taScreenTags[] = {
    /* Don't change the order of these without changing assumtion below */
    CSG_TAG_DISPLAYHEIGHT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SCREENCOUNT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SCREENHEIGHT,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_BITMAPCOUNT,
    (void *)1,
    CSG_TAG_BITMAPWIDTH_ARRAY,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_BITMAPHEIGHT_ARRAY,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_SPORTBITS,
    (void *)(MEMTYPE_VRAM | MEMTYPE_STARTPAGE),
    CSG_TAG_DISPLAYTYPE,
    (void *)(-1), // Placeholder value (computed below)
    CSG_TAG_DONE,
    (void *)0,
  };

  // Check for bad arguments
  if ((nScreens > MAXSCREENS) || (nScreens < 1))
    goto EXIT_MyOpenGraphics;

  // Open the graphics folio
  i = OpenGraphicsFolio ();
  if (i < 0)
    {
      DIAGNOSE (("Cannot open graphics folio (Error = %ld)\n", i));
      goto EXIT_MyOpenGraphics;
    }

  // Determine NTSC/PAL operating environment
  width = NTSC_WIDTH; // Assume NTSC
  height = NTSC_HEIGHT;
  diType = NTSC_TYPE;
  if (GrafBase->gf_VBLFreq == PAL_FREQUENCY)
    {
      width = PAL_WIDTH;
      height = PAL_HEIGHT;
      diType = PAL_TYPE;
    }
  else if (GrafBase->gf_VBLFreq != NTSC_FREQUENCY)
    {
      DIAGNOSE (("Unknown screen refresh rate %ld\n", GrafBase->gf_VBLFreq));
      goto EXIT_MyOpenGraphics;
    }

  // Set up the width and height arrays now that we know NTSC/PAL
  for (i = 0; i < nScreens; ++i)
    {
      bmw[i] = width;
      bmh[i] = height;
    }

  // Now create the screen group, add it to the display mechanism
  sc->sc_nScreens = (int32)nScreens;
  sc->sc_curScreen = (int32)0;
  i = GrafBase->gf_VRAMPageSize;
  sc->sc_nFrameBufferPages = (width * height * 2 + (i - 1)) / i;
  sc->sc_nFrameByteCount = sc->sc_nFrameBufferPages * i;

  taScreenTags[0].ta_Arg = (void *)height;          // CSG_TAG_DISPLAYHEIGHT
  taScreenTags[1].ta_Arg = (void *)sc->sc_nScreens; // CSG_TAG_SCREENCOUNT
  taScreenTags[2].ta_Arg = (void *)height;          // CSG_TAG_SCREENHEIGHT
  taScreenTags[4].ta_Arg = (void *)bmw;    // CSG_TAG_BITMAPWIDTH_ARRAY
  taScreenTags[5].ta_Arg = (void *)bmh;    // CSG_TAG_BITMAPHEIGHT_ARRAY
  taScreenTags[7].ta_Arg = (void *)diType; // CSG_TAG_DISPLAYTYPE

  item = CreateScreenGroup (&(sc->sc_Screens[0]), taScreenTags);
  if (item < 0)
    {
#if DEBUG
      PrintfSysErr (item);
#endif
      DIAGNOSE (("Cannot create screen group\n"));
      return FALSE;
    }
  i = AddScreenGroup (item, NULL);
  if (item < 0)
    {
#if DEBUG
      PrintfSysErr ((Item)i);
#endif
      DIAGNOSE (("Cannot add screen group\n"));
      return FALSE;
    }

  // Get the information about the screen group and update the ScreenContext
  // structure
  for (i = 0; i < sc->sc_nScreens; i++)
    {
      Screen *screen;
      screen = (Screen *)LookupItem (sc->sc_Screens[i]);

      if (screen == NULL)
        {
          DIAGNOSE (("Cannot locate screens\n"));
          return FALSE;
        }
      sc->sc_BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
      sc->sc_Bitmaps[i] = screen->scr_TempBitmap;
      EnableHAVG (sc->sc_Screens[i]);
      EnableVAVG (sc->sc_Screens[i]);
    }

  rslt = TRUE;

EXIT_MyOpenGraphics:
  return (rslt);
}

/* *************************************************************************
 * ***                                        ******************************
 * ***  Rendering and SPORT Support Routines  ******************************
 * ***                                        ******************************
 * *************************************************************************
 */

bool
InitBackPic (char *filename, ScreenContext *sc)
/* Allocates the BackPic buffer.  If a filename is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no filename is specified, this
 * routine merely clears the BackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
{
  bool retvalue;

  retvalue = FALSE;

  gBackPic = (ubyte *)ALLOCMEM (
      (int)(sc->sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize),
      GETBANKBITS (GrafBase->gf_ZeroPage) | MEMTYPE_STARTPAGE | MEMTYPE_VRAM
          | MEMTYPE_CEL);

  if (NOT gBackPic)
    {
      DBUG (("unable to allocate BackPic"));
      goto DONE;
    }

  SetVRAMPages (VRAMIOReq, gBackPic, 0, sc->sc_nFrameBufferPages, -1);

  if (LoadImage (filename, gBackPic, (VdlChunk **)NULL, sc) != gBackPic)
    {
      DBUG (("LoadImage failed"));
      goto DONE;
    }
  retvalue = TRUE;

DONE:
  return (retvalue);
}

void
CopyBackPic (Bitmap *bitmap)
{
  CopyVRAMPages (VRAMIOReq, bitmap->bm_Buffer, gBackPic,
                 sc->sc_nFrameBufferPages, -1);
}

void
CopyToBackPic (Bitmap *bitmap)
{
  CopyVRAMPages (VRAMIOReq, gBackPic, bitmap->bm_Buffer,
                 sc->sc_nFrameBufferPages, -1);
}

void
Bye ()
{
  DBUG (("bye \n"));

  FadeToBlack (sc, 45);
  exit (0);
}

#define FANCY 1

long
StartUp (ScreenContext *sc)
{
  long retval = 0;

  if (!OpenGraphicsNTSCPAL (sc, 2))
    {
      DIAGNOSTIC ("Cannot initialize graphics");
    }
  else
    retval |= GRAPHICSMASK;

  if (!OpenSPORT ())
    {
      DIAGNOSTIC ("Cannot open DMA channel");
    }
  else
    retval |= SPORTIOMASK;

  if (!OpenMacLink ())
    {
      DIAGNOSTIC ("Cannot communicate with Mac");
    }
  else
    retval |= MACLINKMASK;

  if (!OpenAudio ())
    {
      DIAGNOSTIC ("Cannot initialize audio");
    }
  else
    retval |= AUDIOMASK;

  if (OpenMathFolio () < 0)
    {
      DIAGNOSTIC ("Cannot initialize MathFolio");
    }

  return retval;
}

int
main (int argc, char **argv)
{
  long curJoy;
  long frames = 0;
  long symmetry = 1;
  Sprite *spr;
  Sprite *prev;
  char backfile[] = "PALvspics/testscr.img";
  char thename[64];
  long opened;
  int frozen = 0;

  if (FlagIsSet (KernelBase->kb_CPUFlags, KB_RED))
    {
      RedSi = TRUE;
    }

  sc = (ScreenContext *)ALLOCMEM (sizeof (ScreenContext), MEMTYPE_CEL);
  if (sc == NULL)
    {
      DIAGNOSTIC ("Cannot Allocate memory for ScreenContext ");
      return FALSE;
    }

  gBackPic = NULL;
  gScreenSelect = 0;

  if (ChangeInitialDirectory (NULL, NULL, false) < 0)
    {
      DIAGNOSTIC ("Cannot set initial working directory ");
      return FALSE;
    }

  opened = StartUp (sc);
  if (opened == 0)
    return (0);
  if (opened & GRAPHICSMASK)
    DBUG (("Graphics Foio Opened \n"));
  if (opened & AUDIOMASK)
    DBUG (("Audio Foio Opened \n"));
  if (opened & SPORTIOMASK)
    DBUG (("Sportio  Opened \n"));
  if (opened & MACLINKMASK)
    DBUG (("MACLINK Opened \n"));

#if ENABLE_JOURNALING
  if (argc > 1)
    {
      argv++; /* advance to first arg */
      while (**argv == '-')
        {
          (*argv)++;
          switch (**argv)
            {
            case 'r':
              record = 1;
              argv++;                 /* advance to file name pointer */
              recordfilename = *argv; /* save file name pointer */
              argv++;                 /* advance to next arg string pointer */
              break;

            case 'p':
              playback = 1;
              argv++;                   /* advance to file name pointer */
              playbackfilename = *argv; /* save file name pointer */
              argv++; /* advance to next arg string pointer */
              break;

            default:
              argv++; /* unknown switch: advance to next arg string pointer */
            }
        }
    }

  if (playback)
#if 0
		ReadFile( playbackfilename, (char *)TestScript, 
				TEST_SCRIPT_COUNT * sizeof(JoystickRecord) );
#else
    ReadFile (playbackfilename, /* ptr to filename */
              TEST_SCRIPT_COUNT * sizeof (JoystickRecord), /* size of data */
              (long *)TestScript, /* ptr to read buffer */
              0                   /* offset into file */
    );
#endif

    if (record)
      RecordJoystickScript (TestScript, TEST_SCRIPT_COUNT);
  if (playback)
    SetJoystickScript (TestScript);
#endif

  /* if it doesn't work, so what? */
  strcpy (thename, backfile);
  InitBackPic (thename, sc);

  VRAMIOReq = GetVRAMIOReq ();

  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer,
                BACKGROUND_COLOR, sc->sc_nFrameBufferPages, 0xffffffff);
  gScreenSelect = 1 - gScreenSelect;
  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer,
                BACKGROUND_COLOR, sc->sc_nFrameBufferPages, 0xffffffff);

  gScreenSelect = 1 - gScreenSelect;
  DisplayScreen (sc->sc_Screens[gScreenSelect], 0);

  SpriteList = NULL;
  prev = NULL;

  spr = LoadSprite ("PALvspics/testd50.cel", Convert32_F16 (1),
                    Convert32_F16 (2), 0x00002000, Convert32_F16 (2));
  spr = LoadSprite ("PALvspics/testo50.cel", Convert32_F16 (2),
                    Convert32_F16 (2), 0x00002800, Convert32_F16 (3));
  spr = LoadSprite ("PALvspics/test3.cel", Convert32_F16 (1),
                    Convert32_F16 (1), 0x00001000, Convert32_F16 (1));

  //  WaitVBL();

  /* Set some globals for Intro Zoom */

  ticksperframe = 0x00010000;

  curJoy = ReadControlPad (0);

  for (;;)
    {
      /*  set up initial background screen    */

      curJoy = ReadControlPad (0);
      if (curJoy != 0)
        frames = 0;
      else
        frames++;

      if ((curJoy & JOYSTART))
        {
          break;
        }
      CopyBackPic (sc->sc_Bitmaps[gScreenSelect]);

#if FANCY
      if ((curJoy & JOYFIREA))
        {
          symmetry++;
          SetSymOrder (symmetry);
          symmetry = symorder;
        }
      if (!frozen)
        MoveSprites ();
      if ((curJoy & JOYFIREC))
        {
          zscaling = 1 - zscaling;
        }
      if ((curJoy & JOYFIREB))
        {
          rotating += 1;
          if (rotating > 2)
            rotating = 0;
        }
      if (frames >= 7200)
        {
          frames = 0;
          zscaling = 0;
          rotating = 0;
        }
      if (frames >= 3600)
        rotating = 1;
      if (frames >= 5400)
        zscaling = 1;
      if (frames >= 6900)
        {
          rotating = 2;
          zscaling = 0;
        }
#else
      MoveSprites ();
#endif

      DrawSprites (sc->sc_BitmapItems[gScreenSelect]);
      DisplayScreen (sc->sc_Screens[gScreenSelect], 0);
      gScreenSelect = 1 - gScreenSelect;
    } /* end for (;;) */

#if ENABLE_JOURNALING
  if (record)
    {
      WriteMacFile (recordfilename, (char *)TestScript,
                    TEST_SCRIPT_COUNT * sizeof (JoystickRecord));
    }
#endif
  Bye ();
}
