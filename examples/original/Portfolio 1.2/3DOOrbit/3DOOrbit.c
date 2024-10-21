/*
        File:		3DOOrbit.c

        Contains:	This program displays A rotating 3DO logo.

        Written by:	Bill Duvall

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <8+>	 8/10/93	JAY		changing to new
   LoadAnim interface
                 <8>	 7/28/93	JAY		modify WaitVBL(),
   SetVRAMPages(), CopyVRAMPages() to conform to Dragon Tail release which
   requires a IO Parameter be added to the calls. <7>	 6/24/93	JAY
   change FindMathFolio to OpenMathFolio <6>	  4/7/93	JAY
   MEMTYPE_SPRITE is now MEMTYPE_CEL for Cardinal2 <5>	  4/5/93	JAY
   Trying to figure our ReadControlPad(). Final conclustion is that this
   program is not interactive enough and the eventBroker is not robust enough.
   Removed /remote/ in filenames (i.e. pathname are now relative to initial
   working directory. Added ChangeInitialDirectory() to assist with relative
   pathnames.
                 <4>	  4/2/93	JAY		add KillEventUtility()
   to exit the EventBroker gracefully
                 <3>	 3/23/93	JAY		add FindMathFolio() to
   startup for Cardinal <2>	 3/17/93	JAY
   ¥'lists.h' to 'list.h' for Cardinal build
                 <1>	 3/17/93	JAY		This is the 2B1 version
   being check in for Cardinal build
                 <0>	 1/6/92		WSD		1.0 release

        To Do:
*/

#define printf kprintf
#define VERSION "1.0"
/*  #define USEFREEMEM 1  */

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define SEL_ENABLE_VAVG 1
#define SEL_ENABLE_HAVG 2
#define SEL_ENABLE_BOTH 3
#define SEL_DISABLE_BOTH 4

#define FADE_FRAMECOUNT 48

#define LITBOX_WIDTH 40
#define LITBOX_HEIGHT 120

#define MAX_VELOCITY 4

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

#define NICE_BLUE ((((3 << 5) + 6) << 16) + ((3 << 5) + 6))
#define BACKGROUND_COLOR NICE_BLUE

#include "debug.h"
#include "event.h"
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

#include "Init3DO.h"
#include "OrbitSound.h"
#include "filefunctions.h" /* using ChangeDirectory */
#include "sprite.h"

extern Font Geneva9;
extern ulong Pip[];
#define EMULATE_SHERRY 1

#define GRAPHICSMASK 1
#define AUDIOMASK 2
#define SPORTIOMASK 4
#define MACLINKMASK 8

int32 CelExtraFlags = 0;

/*  Animation globals  */
frac16 ticksperframe = Convert32_F16 (1);
frac16 gFrameRate = Convert32_F16 (1);

ubyte *gBackPic = NULL;

long ScreenSelect = 0;
long gScreenSelect = 0;
static ScreenContext *sc;

Item VBLIOReq;
Item VRAMIOReq;

/*
extern Item gBitmapItems[2];
extern Bitmap	*gBitmaps[2];

extern Bitmap	*gBitmaps[2];
extern long	gScreenByteCount;
*/

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
InitBackPic (char *filename)
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

  SetVRAMPages (VRAMIOReq, gBackPic, NICE_BLUE, sc->sc_nFrameBufferPages, -1);

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
#if 0
	ClearBitmap( gBitmaps[0] );
	ClearBitmap( gBitmaps[1] );
#endif
  exit (0);
}

Boolean
click ()
{
  if (ReadControlPad (-1) & JOYSTART)
    {
      while (ReadControlPad (-1) & JOYSTART)
        ;
      return (true);
    }
  return (false);
}

typedef struct
{
  int32 alive, waitingToDie, dead;
  int32 h, v;
  int32 vAdjust; /* if actual animation is off center */
  int32 r;
  int32 width, height;
  frac16 angle;
  Rect rect;
  ANIM *anim;
} Object;

#define NumObjs 3

#define StartX 160
#define StartY 120

#define Accelerator (1 << 16) + 0x4000
#define MaxSpeed 0x180000
#define FirstIncrement 0x2000
#define DeathRate 18 /* rate of dying in 1/30 of second (?) */
#define Radius 80
#define PileOffset (obj[0].width / 2)

#define True 1
#define False 0

TagArg ScreenTags[] = {
  /* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
  /* Kludgy code below presumes that SPORTBITS is the first entry */
  CSG_TAG_SPORTBITS, (void *)0, CSG_TAG_SCREENCOUNT, (void *)2, CSG_TAG_DONE, 0
};

long
sinv (long angle, long value)
{
  return ((MulSF16 (SinF16 (angle), (value << 16))) + 0x8000) >> 16;
}

long
cosv (long angle, long value)
{
  return ((MulSF16 (CosF16 (angle), (value << 16))) + 0x8000) >> 16;
}

void
show3DO (ANIM *anim1, ANIM *anim2, ANIM *anim3)
{
  int32 stopping;
  int32 x, y;
  int32 i;
  int32 downCounter;
  int32 retvalue;
  frac16 arc, circle, halfCircle, inc;

  CCB *tempCCB;
  Object obj[NumObjs];

  inc = FirstIncrement;
  circle = 256 << 16;
  halfCircle = 128 << 16;    // Divsf16(&temp, circle, 2);
  arc = (85 << 16) + 0x6000; // Divsf16(&temp, circle, 3);

  obj[0].anim = anim1; /* Ball */
  obj[1].anim = anim2; /* TV */
  obj[2].anim = anim3; /* Cube */

  for (i = 0; i < NumObjs; i++)
    {
      obj[i].r = Radius;
      obj[i].alive = True;
      obj[i].dead = False;
      obj[i].waitingToDie = False;
      obj[i].angle = MulSF16 (arc, i << 16);
      tempCCB = GetAnimCel (obj[i].anim, Convert32_F16 (1));
      obj[i].width = tempCCB->ccb_Width;
      obj[i].height = tempCCB->ccb_Height;
      obj[i].vAdjust = 0;
      obj[i].h = StartX;
      obj[i].v = StartY;
    }

  /* Heights of actual art from Greg (via PhotoShop) */
  obj[0].height = 46;
  obj[1].height = 33;
  obj[1].vAdjust = -2;
  obj[2].height = 46;

  stopping = -1;
  downCounter = 0;

  while (true)
    {
      WaitVBL (VBLIOReq, 0);
      /* Clear the buffer (SPORT the background).
       * The call to the SPORT routine, which does a DoIO(), will cause us
       * to sync up to the vertical blank at this point.  After synching to
       * the vertical blank, we should have as short a path as
       * possible between here and the call to DisplayScreen(), with,
       * at best, only one call to DrawCels() standing between.
       */

      CopyBackPic (sc->sc_Bitmaps[ScreenSelect]);

      for (i = 0; i < NumObjs; i++)
        {
          if (obj[i].alive)
            {
              tempCCB = GetAnimCel (obj[i].anim, 0x10000);
              if (obj[i].dead)
                {
                  x = obj[i].h;
                  y = obj[i].v - obj[i].r;
                  if ((obj[i].anim->cur_Frame == 0) && (obj[i].dead++ >= 1))
                    {
                      obj[i].alive = False;
                      PlayStop (0);
                    }
                }
              else
                {
                  x = obj[i].h + sinv (obj[i].angle, obj[i].r);
                  y = obj[i].v + cosv (obj[i].angle, obj[i].r);
                }
            }
          else
            {
              x = obj[i].h;
              y = obj[i].v - obj[i].r;
            }

          DrawAnimCel (obj[i].anim, sc->sc_BitmapItems[ScreenSelect], x,
                       y + obj[i].vAdjust, 0, CenterHotSpot);
        }

      retvalue = DisplayScreen (sc->sc_Screens[ScreenSelect], 0);
      if (retvalue < 0)
        {
          printf ("DisplayScreen() failed, error=%d\n", retvalue);
          break;
        }

      ScreenSelect = 1 - ScreenSelect;
      if ((stopping >= 0) && (--downCounter < 0))
        {
          if (stopping >= NumObjs)
            {
              if ((!obj[NumObjs - 1].alive) && (stopping++ > NumObjs))
                break;
            }
          else
            {
              if (obj[stopping].waitingToDie)
                {
                  if (obj[stopping].angle < halfCircle)
                    {
                      obj[stopping].dead = 1;
                      stopping += 1;
                      downCounter = DeathRate;
                    }
                }
              else
                {
                  if (obj[stopping].angle < inc / 2)
                    {
                      obj[stopping].waitingToDie = True;
                      if (stopping == 0)
                        {
                          obj[0].r = (Radius - PileOffset) / 2;
                          obj[0].v += Radius - obj[0].r;
                        }
                      else
                        {
                          obj[stopping].r = obj[stopping - 1].r
                                            + (obj[stopping - 1].height
                                               + obj[stopping].height)
                                                  / 4;
                          obj[stopping].v
                              = obj[stopping - 1].v
                                - (obj[stopping].r - obj[stopping - 1].r);
                        }
                    }
                }
            }
        }

      for (i = 0; i < NumObjs; i++)
        {
          obj[i].angle
              = obj[i].angle - (obj[i].waitingToDie ? (inc / 2) : inc);
          if (obj[i].angle < 0)
            obj[i].angle += circle;
        }

      if (inc < MaxSpeed)
        inc = MulSF16 (inc, Accelerator);
      else if (stopping < 0)
        stopping = 0;
    }
}

char *defaultAnimNames[]
    = { "Orbit/Cube3.anim", "Orbit/tvd.anim", "Orbit/ball.anim" };

char *defaultImageName = "Orbit/background.img";
static ANIM
    *animVec[NumObjs]; /* these are huge! We don't want these on the stack! */
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

  if (OpenMathFolio () < 0)
    {
      DIAGNOSTIC ("Cannot initialize math");
    }

  return retval;
}

int
main (int argc, char **argv)
{
  char *progname, filename[64], animname[64];
  long fnsize;
  int i;
  long opened;

  VRAMIOReq = GetVRAMIOReq ();
  VBLIOReq = GetVBLIOReq ();

  gBackPic = NULL;
  gScreenSelect = 0;
  ScreenSelect = 0;
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

  if (!InitOrbitSound ())
    printf ("\rCould not initialize sound");

  progname = *argv++;
  printf ("%s %s\n", progname, VERSION);

  fnsize = strlen (argv[0]);

  strcpy (filename, defaultImageName);
  kprintf ("\rArgc = %d", argc);

  if (argc > 1)
    strcpy (filename, argv[0]);

  fnsize = strlen (filename);
  if (fnsize <= 0)
    {
      kprintf ("usage: Example imageFile [celFile celFile celFile]\n");
      exit (0);
    }

  /* if it doesn't work, so what? */
  InitBackPic (filename);

  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer,
                BACKGROUND_COLOR, sc->sc_nFrameBufferPages, 0xffffffff);
  gScreenSelect = 1 - gScreenSelect;
  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[gScreenSelect]->bm_Buffer,
                BACKGROUND_COLOR, sc->sc_nFrameBufferPages, 0xffffffff);
  gScreenSelect = 1 - gScreenSelect;
  DisplayScreen (sc->sc_Screens[gScreenSelect], 0);

  /* Open the anim file, setup the CCBs and pass back an array of CCB ptrs.
   * The CCB's DLUT ptrs and Data ptrs are also setup by this routine.
   */

  for (i = 0; i < NumObjs; i++)
    {
      if (i < (argc - 2))
        strcpy (animname, argv[i + 1]);
      else
        strcpy (animname, defaultAnimNames[i]);

      if ((animVec[i] = LoadAnim (animname, MEMTYPE_CEL)) == 0)
        {
          kprintf ("Error in LoadAnim call for anim file %s\n", animname);
          exit (0);
        }
      else
        kprintf ("\rLoaded %s\r", animname);

      if (animVec[i]->num_Frames > 0)
        kprintf ("%d cels were found in %s\n", animVec[i]->num_Frames,
                 animname);
      else
        {
          kprintf ("No Frames in Anim %s !!!\n", animname);
          goto DONE;
        }
    }

  while (!click ())
    {
      show3DO (animVec[2], animVec[1], animVec[0]);

#ifdef Use_Announce
      PlayAnnounce ();
#endif
      //		for (i = 0; i < 60; i++) WaitVBL(); // while
      //(!click());
      WaitVBL (VBLIOReq, 60);
    }

DONE:
  FadeToBlack (sc, FADE_FRAMECOUNT);
  FreeSoundEffects ();
  {
    ControlPadEventData padData;
    int i = 100;
    while (i)
      if (GetControlPad (1, 0, &padData) == 0)
        i--;
  }
  KillEventUtility ();
  printf ("\n%s sez:  bye!\n", progname);
  exit (0);
}
