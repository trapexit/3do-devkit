/*
        File:		LREX.c

        Contains:	LRForm Cels

        Written by:	Neil Cormia

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 5/09/94	crm		removed
   explicit references to GrafBase struct <4>	 7/30/93	JAY
   add IO Parameter to CopyVRAMPages() and SetVRAMPages() for DragonTail
   release <3>	 6/24/93	JAY		change FindMathFolio to
   OpenMathFolio. Change ccb offset name to Dragon1 names. <2>	  4/7/93
   JAY		MEMTYPE_SPRITE is now MEMTYPE_CEL for Cardinal
                 <1>	  4/3/93	JAY		first checked in

        To Do:
*/

#include "types.h"

#include "debug.h"
#include "folio.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "semaphore.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"

#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"

#ifndef __operamath_h
#include "operamath.h"
#endif

#include "Utils3DO.h"

/* Copied from sherrie.h */

#define CCB_DOVER_MASK 0x00000180
#define CCB_DOVER_SHFT 7
#define CCB_PIPA_SHFT 0

#define DMODE_BPP ((0x00000000) << CCB_DOVER_SHFT)
#define DMODE_ZERO ((0x00000002) << CCB_DOVER_SHFT)
#define DMODE_ONE ((0x00000003) << CCB_DOVER_SHFT)

#define Convert32_F20(x) ((x) << 20)
/* convert 32 bit integer to 12.20 fraction */

/* *************************************************************************
 * ***  Discussion  ********************************************************
 * *************************************************************************
 *
 * This program does not clean up after itself.  This is Portfolio-approved.
 * All programs *should* exit without bothering to clean up.  This
 * is because Portfolio promises to clean up and deallocate all system
 * resources for you
 *
 */

/* *************************************************************************
 * ***                   ***************************************************
 * ***  Our Definitions  ***************************************************
 * ***                   ***************************************************
 * *************************************************************************
 */

#define VERSION "0.03"

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define SEL_ENABLE_VAVG 1
#define SEL_ENABLE_HAVG 2
#define SEL_ENABLE_BOTH 3
#define SEL_DISABLE_BOTH 4

/* These represent the value one (1) in various number formats.
 * For example, ONE_12_20 is the value of 1 in fixed decimal format
 * of 12 bits integer, 20 bits fraction
 */
#define ONE_12_20 (1 << 20)
#define ONE_16_16 (1 << 16)

/* The JOYCONTINUOUS definition specifies those joystick flags that we
 * want to know about all the time, whenever the associated switches
 * are depressed, not just when they make the transition from undepressed
 * to depressed (which is the default).
 */
#define JOYCONTINUOUS                                                         \
  (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT | JOYFIREA | JOYFIREB      \
   | JOYFIREC)

/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

void CloseMacLink (void);
void CloseSPORT (void);
void CopyBackPic (Bitmap *bitmap);
bool DoJoystick (CCB *ccb);
bool Init (void);
bool InitBackPic (char *filename, ScreenContext *sc);
bool OpenMacLink (void);
bool OpenSPORT (void);
void PrintCCB (CCB *cel);
ulong Random (ulong);

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */

static ScreenContext *sc;

int32 gNextSelect = SEL_ENABLE_VAVG;
bool gKeysDown = FALSE;
bool gSetPMode = FALSE;
bool gReverse = FALSE;

long gXPos = 0;
long gYPos = 0;

frac16 gFrameIncr;

int32 xMovePt;
int32 yMovePt;
int32 xDistPt;
int32 yDistPt;

ubyte *gBackPic = NULL;
long gScreenSelect = 0;

int RedSi = 0;

#define GRAPHICSMASK 1
#define AUDIOMASK 2
#define SPORTIOMASK 4
#define MACLINKMASK 8

Item VRAMIOReq;

/* *************************************************************************
 * ***                 *****************************************************
 * ***  Main Routines  *****************************************************
 * ***                 *****************************************************
 * *************************************************************************
 */
long
StartUp (ScreenContext *sc)
{
  long retval = 0;

  VRAMIOReq = GetVRAMIOReq ();

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
      DIAGNOSTIC ("Cannot initialize MathFolio");
    }

  // InitMem(0);

  return retval;
}

int
main (int argc, char *argv[])
/* This program initializes itself, loads a background picture, then in
 * its main loop does double-buffer display/rendering of the background
 * pic and a cel (or cels) while reacting to input from the joystick.
 *
 * The offscreen buffer:
 *   - is initialized by having the background pic SPORT'ed into it
 *   - has an animating cel rendered onto it
 */
{
  char *progname, filename[64];
  int32 retvalue;
  int32 fnsize;
  int32 y, offset, oldOffset;
  CCB *ccb;
  CCB lrccb;

  /*	Extract the program name, imagefile name, and celfile name from
   *	the arguments passed into the program.
   *	NOTE: the imagefile and celfile names are string copied into
   *	globals. This assures that the names will start on a longword
   *	boundry. Any strings that are passed to the O/S must start on
   *	a longword boundry.
   */
  progname = *argv++;
  kprintf ("%s %s\n", progname, VERSION);

  if (FlagIsSet (KernelBase->kb_CPUFlags, KB_RED))
    {
      RedSi = TRUE;
    }
  else
    {
      kprintf (" %s requires Red Silicon or later to run. It uses LRForm "
               "Cels, sorry. \n",
               progname);
      exit (0);
    }
  if (argc <= 1)
    {
      kprintf ("usage: %s <imagefile> \n", progname);
      exit (0);
    }
  fnsize = strlen (argv[0]);
  if (fnsize <= 0)
    {
      kprintf ("usage: %s <imagefile> \n", progname);
      exit (0);
    }
  strcpy (filename, argv[0]);

  sc = (ScreenContext *)ALLOCMEM (sizeof (ScreenContext), MEMTYPE_CEL);
  if (sc == NULL)
    {
      DIAGNOSTIC ("Cannot Allocate memory for ScreenContext ");
      return FALSE;
    }

  /*	Creates the CreateScreenGroup and inits a SPORT device to write
   *	directly to the frame buffer and a Mac Link to read files from
   *	the Macintosh.
   */
  if (FlagIsSet (KernelBase->kb_CPUFlags, KB_RED))
    {
      RedSi = TRUE;
    }
  else
    {
      kprintf ("Requires Red Silicon or later to run. It uses LRForm Cels, "
               "sorry\n");
      exit (0);
    }
  if (NOT StartUp (sc))
    goto DONE;

  EnableVAVG (sc->sc_Screens[0]);
  EnableVAVG (sc->sc_Screens[1]);
  EnableHAVG (sc->sc_Screens[0]);
  EnableHAVG (sc->sc_Screens[1]);

  /* Load the background image and save a copy of it in page aligned VRAM
   */
  InitBackPic (filename, sc);

  /* Init the LR Form CCB
   */
  lrccb.ccb_Flags = CCB_LDSIZE   /* Load hdx-y and vdx-y from CCB */
                    | CCB_LAST   /*  */
                    | CCB_LDPRS  /* Load ddx-y from CCB */
                    | CCB_LDPPMP /* Load the PPMP word from CCB */
                    | CCB_CCBPRE /* Load the Pramble words from CCB */
                    | CCB_ACW    /* Pixels facing forward will be seen */
                    | CCB_ACCW   /* Pixels facing backward will be seen */
                    | CCB_LAST   /* last sprite in the list for DrawSprites */
                    | CCB_SPABS  /* SourcePtr is a ptr, not an offset */
                    | CCB_YOXY | CCB_ACE;

  lrccb.ccb_NextPtr = 0;
  lrccb.ccb_PLUTPtr = 0;

  lrccb.ccb_XPos = 0;
  lrccb.ccb_YPos = 0;
  lrccb.ccb_HDX = Convert32_F20 (1);
  lrccb.ccb_HDY = 0;
  lrccb.ccb_VDX = 0;
  lrccb.ccb_VDY = Convert32_F16 (1);
  lrccb.ccb_HDDX = 0;
  lrccb.ccb_HDDY = 0;
  lrccb.ccb_PIXC = 0x1F811F00;
  lrccb.ccb_Width = 64;
  lrccb.ccb_Height = 32; /* height is actually 2 times this */

  lrccb.ccb_PRE0
      = ((lrccb.ccb_Height - 1) << 6)
        +             /* number of vertical data lines in sprite data -1 */
        PRE0_LINEAR + /* use PIN for IPN (0x10) */
        PRE0_BPP_16;  /* set bits/pixel to 16 (0x6) see HDWR spec 3.6.3 */

  lrccb.ccb_PRE1
      = ((640 / 2 - 2) << 16)
        + /* offset (in words) from line start to next line start. */
        (lrccb.ccb_Width - 1) + /* number of horizontal pixels to render -1 */
        PRE1_LRFORM;
  ccb = &lrccb;
  oldOffset = 0;

  kprintf (" LREX Example of LRForm Cels     \n");
  kprintf (" Hold down B button and arrows to adjust cel size   \n");
  kprintf (" Arrow keys move cel around using the underlying image data in "
           "the cel  \n");
  kprintf ("      Start button will exit the program    \n");

  while (TRUE)
    {
      /* Seed up a bit */
      Random (2);

      /* React to the joystick */
      if (NOT DoJoystick (ccb))
        goto DONE;

      /* Call the cel engine to retrieve the next Cel in the current anim.
       */
      y = ConvertF16_32 (ccb->ccb_YPos);
      offset = ((y / 2) * 1280) + (ConvertF16_32 (ccb->ccb_XPos) << 2);

      ccb->ccb_SourcePtr = (CelData *)(gBackPic + offset);

      if (RedSi)
        ccb->ccb_Flags |= CCB_ACE;
      /* if (RedSi) ccb->ccb_Flags |= CCB_MARIA; */

      /*	If gSetPMode is true, the active PMode for this Cel will be
       *forced to 0. This means that whatever settings were used for PMODE-0
       *	(opaque, translucent, etc.) will be applied to the entire Cel.
       *If gSetPMode is false, the Cel will use the PMODE value from its pixel
       *	data high bit if it is Uncoded 16 bit or Coded 16 bit or Coded
       *6 bit. If the Cel is not one of the above types, the PMODE pixel will
       *come from the high bit of the PLUT color entry for each color. 8 bit
       *Uncoded Cels will default to zero, as they have no PLUT or PMODE per
       *pixel.
       */
      if (gSetPMode)
        {
          ccb->ccb_Flags &= ~CCB_DOVER_MASK;
          ccb->ccb_Flags |= DMODE_ONE;
        }
      else
        {
          ccb->ccb_Flags &= ~CCB_DOVER_MASK;
          ccb->ccb_Flags |= DMODE_BPP;
          // ccb->ccb_Flags |= DMODE_ZERO;
        }

      /* SPORT the background back into the current buffer.
       * The call to the SPORT routine, which does a DoIO(), will cause us
       * to sync up to the vertical blank at this point.  After synching to
       * the vertical blank, we should have as short a path as
       * possible between here and the call to DisplayScreen(), with,
       * at best, only one call to DrawCels() standing between.
       */
      CopyBackPic (sc->sc_Bitmaps[gScreenSelect]);

      /*	Draw the current frame from the animation.
       */
      retvalue = DrawCels (sc->sc_BitmapItems[gScreenSelect], ccb);
      if (retvalue < 0)
        kprintf ("DrawCels() failed, error=%ld\n", retvalue);

      /*	Point the 3DO screen to the screen we just filled. The 3DO
       *	hardware will now draw this screen to the TV every VBlank.
       */
      retvalue = DisplayScreen (sc->sc_Screens[gScreenSelect], 0);
      if (retvalue < 0)
        {
          kprintf ("DisplayScreen() failed, error=%ld\n", retvalue);
          goto DONE;
        }

      /*	Toggle to the other screen for our next frame of animation.
       */
      gScreenSelect = 1 - gScreenSelect;
    }

DONE:
  FadeToBlack (sc, 120);
  /* CloseEverything(); */
  kprintf ("\n%s has completed!\n", progname);
  return ((int)retvalue);
}

/* *************************************************************************
 * ***                                  ************************************
 * ***  Miscellaneous Utility Routines  ************************************
 * ***                                  ************************************
 * *************************************************************************
 */

long
GetJoystick (void)
/* Gets the current state of the joystick switches.  The default is
 * to return the switches that have made a transition from off to on,
 * and any of the JOYCONTINUOUS switches that are currently on.
 *

 */
{

  return (ReadControlPad (JOYCONTINUOUS));
}

bool
DoJoystick (CCB *ccb)
/* Returns non-FALSE if user didn't press START, else returns FALSE to quit */
{
  bool retvalue;
  long joybits;
  Point q[4];

  retvalue = TRUE;

  joybits = GetJoystick ();

  /* If the user has pressed the START button on the joystick
   * then return FALSE
   */
  if (joybits & JOYSTART)
    {
      retvalue = FALSE;
      goto DONE;
    }

  /* If the select button (Button C) is pressed advance to the next select
   * state */
  if (joybits & JOYFIREC)
    {
      if (joybits & JOYRIGHT)
        {
          ++xDistPt;
        }
      if (joybits & JOYLEFT)
        {
          --xDistPt;
        }
      if (joybits & JOYUP)
        {
          --yDistPt;
        }
      if (joybits & JOYDOWN)
        {
          ++yDistPt;
        }

      SetQuad (q, gXPos, gYPos, gXPos + ccb->ccb_Width + xMovePt,
               gYPos + ccb->ccb_Height + yMovePt);

      q[2].pt_X += xDistPt;
      q[2].pt_Y += yDistPt;

      MapCel (ccb, q);

      goto DONE;
    }

  /* With FIRE B ... */
  if (joybits & JOYFIREB)
    {
      if (joybits & JOYRIGHT)
        {
          ++xMovePt;
        }
      if (joybits & JOYLEFT)
        {
          --xMovePt;
        }
      if (joybits & JOYUP)
        {
          --yMovePt;
        }
      if (joybits & JOYDOWN)
        {
          ++yMovePt;
        }

      SetQuad (q, gXPos, gYPos, gXPos + ccb->ccb_Width + xMovePt,
               gYPos + ccb->ccb_Height + yMovePt);
      MapCel (ccb, q);

      goto DONE;
    }

  if (joybits & (JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT))
    {
      if (joybits & JOYUP)
        {
          --gYPos;
          if (--gYPos < 0)
            gYPos = 0;
        }
      else if (joybits & JOYDOWN)
        {
          ++gYPos;
          if (++gYPos > (DISPLAY_HEIGHT - ccb->ccb_Height))
            gYPos = DISPLAY_HEIGHT - ccb->ccb_Height;
        }
      if (joybits & JOYLEFT)
        {
          gReverse = FALSE;
          --gXPos;
          if (--gXPos < 0)
            gXPos = 0;
        }
      else if (joybits & JOYRIGHT)
        {
          gReverse = TRUE;
          ++gXPos;
          if (++gXPos > (DISPLAY_WIDTH - ccb->ccb_Width))
            gXPos = DISPLAY_WIDTH - ccb->ccb_Width;
        }

      gXPos &= 0xFFFE;
      gYPos &= 0xFFFE;
      SetQuad (q, gXPos, gYPos, gXPos + ccb->ccb_Width + xMovePt,
               gYPos + ccb->ccb_Height + yMovePt);
      MapCel (ccb, q);
    }

  gKeysDown = FALSE;

DONE:
  return (retvalue);
}

bool
InDisplay (long x, long y)
/* If the x,y coordinate is in the display this routine returns non-FALSE
 * else it returns FALSE
 */
{
  if ((x >= 0) && (x < DISPLAY_WIDTH) && (y >= 0) && (y < DISPLAY_HEIGHT))
    return (TRUE);
  return (FALSE);
}

ulong
Random (ulong n)
/* Return a random number from 0 to n-1
 * The return value has 16 bits of significance, and ought to be unsigned.
 * Is the above true?
 */
{
  ulong i, j, k;

  i = (ulong)rand () << 1;
  j = i & 0x0000FFFF;
  k = i >> 16;
  return ((((j * n) >> 16) + k * n) >> 16);
}

CCB *
CloneCCB (CCB *cloneMe)
{
  CCB *cel;
  uchar *ptr, *ptr2;
  long i;

  cel = (CCB *)ALLOCMEM (sizeof (CCB), MEMTYPE_VRAM | MEMTYPE_CEL);
  if (NOT cel)
    {
      DIAGNOSTIC ("not enough memory to clone cels");
      goto DONE;
    }
  ptr = (uchar *)cel;
  ptr2 = (uchar *)cloneMe;
  for (i = 0; i < sizeof (CCB); i++)
    *ptr++ = *ptr2++;

DONE:
  return (cel);
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
      (int)(sc->sc_nFrameBufferPages * GetPageSize (MEMTYPE_VRAM)),
      MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

  if (NOT gBackPic)
    {
      kprintf ("unable to allocate BackPic");
      goto DONE;
    }

  SetVRAMPages (VRAMIOReq, gBackPic, 0, sc->sc_nFrameBufferPages, -1);

  if (LoadImage (filename, gBackPic, (VdlChunk **)NULL, sc) != gBackPic)
    {
      kprintf ("LoadImage failed");
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

/* *************************************************************************
 * ***                  ****************************************************
 * ***  Debug Routines  ****************************************************
 * ***                  ****************************************************
 * *************************************************************************
 */

void
PrintCCB (CCB *cel)
/* Dump the contents of the specified cel control block */
{
  kprintf ("CCB=$%lx ", (unsigned long)(cel));
  kprintf ("\n");
  kprintf ("  Flags=$%lx ", (unsigned long)(cel->ccb_Flags));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_Flags));
  kprintf ("NextPtr=$%lx ", (unsigned long)(cel->ccb_NextPtr));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_NextPtr));
  kprintf ("CelData=$%lx ", (unsigned long)(cel->ccb_SourcePtr));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_SourcePtr));
  kprintf ("\n");
  kprintf ("  PIPPtr=$%lx ", (unsigned long)(cel->ccb_PLUTPtr));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_PLUTPtr));
  kprintf ("X=$%lx ", (unsigned long)(cel->ccb_XPos));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_XPos));
  kprintf ("Y=$%lx ", (unsigned long)(cel->ccb_YPos));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_YPos));
  kprintf ("\n");
  kprintf ("  hdx=$%lx ", (unsigned long)(cel->ccb_HDX));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_HDX));
  kprintf ("hdy=$%lx ", (unsigned long)(cel->ccb_HDY));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_HDY));
  kprintf ("vdx=$%lx ", (unsigned long)(cel->ccb_VDX));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_VDX));
  kprintf ("vdy=$%lx ", (unsigned long)(cel->ccb_VDY));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_VDY));
  kprintf ("\n");
  kprintf ("  ddx=$%lx ", (unsigned long)(cel->ccb_HDDX));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_HDDX));
  kprintf ("ddy=$%lx ", (unsigned long)(cel->ccb_HDDY));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_HDDY));
  kprintf ("\n");
  kprintf ("  PPMPC=$%lx ", (unsigned long)(cel->ccb_PIXC));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_PIXC));
  kprintf ("Width=$%lx ", (unsigned long)(cel->ccb_Width));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_Width));
  kprintf ("Height=$%lx ", (unsigned long)(cel->ccb_Height));
  kprintf ("(%ld) ", (unsigned long)(cel->ccb_Height));
  kprintf ("\n");
}
