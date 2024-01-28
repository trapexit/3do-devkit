/*
        File:		perftest.c

        Contains:	xxx put contents here xxx

        Written by:	Jay Moreland,lynn ackler,philippe cassereau

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <4>	 7/30/93	JAY		add IO Parameter for
   SetVRAMPages() and CopyVRAMPages for Dragon Tail <3>	  4/7/93	JAY
   MEMTYPE_SPRITE is now MEMTYPE_CEL for Cardinal2 <2>	  4/5/93	JAY
   remove /remote/ from filenames (i.e. make pathname relative to initial
   directory). Add ChangeInitialDirectory() to assist in relative pathname <1>
   4/3/93	JAY		first checked in 4/3/93	DSM
   There is no such thing as water, only melted ice.

        To Do:
*/

#define ENABLE_JOURNALING 0 /* '1 if demo scripting code is enabled */

#define VERSION "1.0"

#define BACKGROUND_COLOR 0

/*
#include "debug.h"
#include "folio.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "lists.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"
*/
#include "filefunctions.h"
#include "portfolio.h"

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
extern int32 AddSprite (void);
extern int32 DeleteSprite (void);

Item VRAMIOReq;

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

#define EYEOFFSET 50
#define EYECAMOFFSET 5
#define SPRINGZ 15

void Bye (void);

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
  if (!OpenGraphics (sc, 2))
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

  return retval;
}
long SCE = 0;
long SCEFLAG = CCB_ACE;

int
main (int argc, char **argv)
{
  long curJoy;
  long frames = 0;
  long symmetry = 1;
  Sprite *spr;
  Sprite *prev;
  char backfile[] = "perfpics/reef.img";
  char thename[64];
  long opened;
  int frozen = 0;

#if ENABLE_JOURNALING
  int32 record = 0;
  int32 playback = 0;
  char *recordfilename = NULL;
  char *playbackfilename = NULL;
#endif

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

  VRAMIOReq = GetVRAMIOReq ();

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

  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer,
                BACKGROUND_COLOR, sc->sc_nFrameBufferPages, 0xffffffff);
  gScreenSelect = 1 - gScreenSelect;
  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer,
                BACKGROUND_COLOR, sc->sc_nFrameBufferPages, 0xffffffff);
  gScreenSelect = 1 - gScreenSelect;
  DisplayScreen (sc->sc_Screens[gScreenSelect], 0);

  SpriteList = NULL;
  prev = NULL;

  spr = LoadSprite ("perfpics/testoOpaque.cel", Convert32_F16 (1),
                    Convert32_F16 (1), 0x00001000, Convert32_F16 (1));
  spr = LoadSprite ("perfpics/testoOpaque.cel", Convert32_F16 (1),
                    Convert32_F16 (2), 0x00002000, Convert32_F16 (2));
  spr = LoadSprite ("perfpics/testoOpaque.cel", Convert32_F16 (2),
                    Convert32_F16 (2), 0x00002800, Convert32_F16 (3));

  // WaitVBL();

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
          /* SetSymOrder(symmetry); */
          AddSprite ();
          symmetry = symorder;
        }
      if (!frozen)
        MoveSprites ();
      if ((curJoy & JOYFIREC))
        {
          /* zscaling = 1- zscaling; */
          DeleteSprite ();
        }
      if ((curJoy & JOYFIREB))
        {
          /* rotating += 1;
                  if (rotating > 2) rotating = 0; */
          if (RedSi)
            {
              SCE = 1 - SCE;
              if (SCE)
                SCEFLAG = CCB_ACE;
              else
                SCEFLAG = 0;
              kprintf (" Second corner engine = %x \n", SCEFLAG);
            }
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
