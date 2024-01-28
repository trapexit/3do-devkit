/*
        File:		Example2.c

        Contains:	This file contains routines to load and examine 3DO
   format files.

        Written by:	Neil Cormia

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 5/09/94	crm		removed
   explicit references to GrafBase struct <7+>	 8/10/93	JAY
   change interface to LoadAnim <7>	 7/28/93	JAY
   add VRAMIOReq parameter to CopyVRAMPages() and SetVRAMPages() for the Dragon
   Tail release <6>	 6/23/93	JAY		update ccb names and
   other name changes for Dragon1 <5>	  4/7/93	JAY
   MEMTYPE_SPRITE is now MEMTYPE_CEL for Cardinal2 <4>	 3/31/93	JAY
   change code to call ReadControlPad() for getting state of control pad. Doing
   this for red hardware and cardinal (3B1).
                 <3>	 3/29/93	JAY		¥ use run to launch
   this app. launch doesn't understand arguments. ¥ changed call to LoadImage
   to check for gBackPic being returned instead of 0. ¥ rewrote StartUp to
   match 3DOOrbit startup <2>	 3/18/93	JAY		change lists.h
   to list.h for Cardinal build <1>	 3/18/93	JAY
   first checked in 11/16/92	NJC		1.0 release

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

#define GRAPHICSMASK 1
#define AUDIOMASK 2
#define SPORTIOMASK 4
#define MACLINKMASK 8
#define MATHMASK 16

#if DEBUG
#define DBUG(x)                                                               \
  {                                                                           \
    kprintf x;                                                                \
  }
#else
#define DBUG(x)
#endif

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

ScreenContext gScreenContext;

Item VRAMIOReq;

int32 gNextSelect = SEL_ENABLE_VAVG;
bool gKeysDown = FALSE;
bool gSetPMode = FALSE;
bool gReverse = FALSE;

long gXPos = 0;
long gYPos = 0;

ANIM *gAnimRec;
frac16 gFrameIncr;

int32 xMovePt;
int32 yMovePt;
int32 xDistPt;
int32 yDistPt;

ubyte *gBackPic = NULL;
long gScreenSelect = 0;

int RedSi = 0;

TagArg ScreenTags[] = {
  /* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
  /* Kludgy code below presumes that SPORTBITS is the first entry */
  CSG_TAG_SPORTBITS, (void *)0, CSG_TAG_SCREENCOUNT, (void *)2, CSG_TAG_DONE, 0
};

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

  gScreenContext.sc_nScreens = 2;
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
  else
    retval |= MATHMASK;

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
  char *progname, filename[64], animname[64];
  int32 retvalue;
  int32 fnsize;
  CCB *ccb;
  CCB *realccb;
  long opened;

  VRAMIOReq = GetVRAMIOReq ();

  /*	Extract the program name, imagefile name, and celfile name from
   *	the arguments passed into the program.
   *	NOTE: the imagefile and celfile names are string copied into
   *	globals. This assures that the names will start on a longword
   *	boundry. Any strings that are passed to the O/S must start on
   *	a longword boundry.
   */
  progname = *argv++;
  kprintf ("%s %s\n", progname, VERSION);

  if (argc <= 2)
    {
      kprintf ("usage: %s <imagefile> <celfile>\n", progname);
      exit (0);
    }
  fnsize = strlen (argv[0]);
  if (fnsize <= 0)
    {
      kprintf ("usage: %s <imagefile> <celfile>\n", progname);
      exit (0);
    }
  strcpy (filename, argv[0]);

  fnsize = strlen (argv[1]);
  if (fnsize <= 0)
    {
      kprintf ("usage: %s <imagefile> <celfile>\n", progname);
      exit (0);
    }
  strcpy (animname, argv[1]);

  /*	Creates the CreateScreenGroup and inits a SPORT device to write
   *	directly to the frame buffer and a Mac Link to read files from
   *	the Macintosh.
   */
  if (FlagIsSet (KernelBase->kb_CPUFlags, KB_RED))
    {
      RedSi = TRUE;
    }

  opened = StartUp (&gScreenContext);
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
  if (opened & MATHMASK)
    DBUG (("MATH Opened \n"));

  EnableVAVG (gScreenContext.sc_Screens[0]);
  EnableVAVG (gScreenContext.sc_Screens[1]);
  EnableHAVG (gScreenContext.sc_Screens[0]);
  EnableHAVG (gScreenContext.sc_Screens[1]);

  /* Load the background image and save a copy of it in page aligned VRAM
   */
  InitBackPic (filename, &gScreenContext);

  /* Open the anim file, setup the CCBs and pass back an Anim Record.
   * The Anim Record contains the number of frames, the current frame,
   * and an entry for each frame containing ptrs to a CCB, a PLUT, and
   * some Cel Data. The PLUT ptr may be NULL if the Cel is Uncoded.
   * LoadCels can be found in Parse3DO.c
   */
  if ((gAnimRec = LoadAnim (animname, MEMTYPE_CEL)) == 0)
    {
      kprintf ("Error in LoadCels call\n");
      exit (0);
    }

  /*	Check to see if the celfile actually contains any cels.
   */
  if (gAnimRec->num_Frames > 0)
    kprintf ("%ld cels were found in %s\n", gAnimRec->num_Frames, animname);
  else
    {
      kprintf ("gAnimRec->num_Frames was zero !!!\n");
      goto DONE;
    }

  /* Init the frame increment to 1.0 and get the first frame
   */
  ccb = GetAnimCel (gAnimRec, 0);
  gFrameIncr = Convert32_F16 (1);

  while (TRUE)
    {
      /* Seed up a bit */
      Random (2);

      /* React to the joystick */
      if (NOT DoJoystick (ccb))
        goto DONE;

      /* Call the cel engine to retrieve the next Cel in the current anim.
       * gAnimRec was initialized by LoadCels and contains enough info to
       * setup the next Cel Control Block for drawing. The gFrameIncr is a
       * signed 16.16 fixed point value that GetCel uses to calculate the
       * next frame to draw.
       */
      realccb = GetAnimCel (gAnimRec, gFrameIncr);
      ccb->ccb_SourcePtr = realccb->ccb_SourcePtr;
      ccb->ccb_PLUTPtr = realccb->ccb_PLUTPtr;
      if (RedSi)
        ccb->ccb_Flags |= CCB_ACE;
      if (RedSi)
        ccb->ccb_Flags |= CCB_MARIA;

#if 0
		/*	Set the X,Y pos and direction for this frame. If the direction
		 *	is reversed, the Cel will start drawing at its X,Y position and
		 *	continue to the LEFT. So, to make the Cel appear to reverse in
		 *	place, we must add the width of the Cel to its X value.
		 */
		if (gReverse) {
			ccb->ccb_HDX = 0x00100000;
			ccb->ccb_X	= gXPos<<16;
			ccb->ccb_Y	= gYPos<<16;
		}
		else {
			ccb->ccb_HDX = 0xFFF00000;
			ccb->ccb_X	= (gXPos + ccb->ccb_Width)<<16;
			ccb->ccb_Y	= gYPos<<16;
		}
#endif

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
      CopyBackPic (gScreenContext.sc_Bitmaps[gScreenSelect]);

      /*	Draw the current frame from the animation.
       */
      retvalue = DrawCels (gScreenContext.sc_BitmapItems[gScreenSelect], ccb);
      if (retvalue < 0)
        kprintf ("DrawCels() failed, error=%ld\n", retvalue);

      /*	Point the 3DO screen to the screen we just filled. The 3DO
       *	hardware will now draw this screen to the TV every VBlank.
       */
      retvalue = DisplayScreen (gScreenContext.sc_Screens[gScreenSelect], 0);
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
  FadeToBlack (&gScreenContext, 120);
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

bool
DoJoystick (CCB *ccb)
/* Returns non-FALSE if user didn't press START, else returns FALSE to quit */
{
  bool retvalue;
  long joybits;
  Point q[4];

  retvalue = TRUE;

  joybits = ReadControlPad (JOYCONTINUOUS);

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

  /* With FIRE A adjust animation delay and CCB's P-Mode setting */
  if (joybits & JOYFIREA)
    {
      if (joybits & JOYLEFT)
        {
          if (gKeysDown == TRUE)
            goto DONE;
          gKeysDown = TRUE;
          if (gFrameIncr >= 0)
            gFrameIncr -= 0x00001000;
          goto DONE;
        }
      else if (joybits & JOYRIGHT)
        {
          if (gKeysDown == TRUE)
            goto DONE;
          gKeysDown = TRUE;
          if (gFrameIncr < 0x00100000)
            gFrameIncr += 0x00001000;
          goto DONE;
        }
      else if (joybits & JOYUP)
        {
          if (gKeysDown == TRUE)
            goto DONE;
          gKeysDown = TRUE;
          gSetPMode = FALSE;
          goto DONE;
        }
      else if (joybits & JOYDOWN)
        {
          if (gKeysDown == TRUE)
            goto DONE;
          gKeysDown = TRUE;
          gSetPMode = TRUE;
          goto DONE;
        }
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
          if (--gYPos < 0)
            gYPos = 0;
        }
      else if (joybits & JOYDOWN)
        {
          if (++gYPos > DISPLAY_HEIGHT)
            gYPos = DISPLAY_HEIGHT;
        }
      if (joybits & JOYLEFT)
        {
          gReverse = FALSE;
          if (--gXPos < 0)
            gXPos = 0;
        }
      else if (joybits & JOYRIGHT)
        {
          gReverse = TRUE;
          if (++gXPos > DISPLAY_WIDTH)
            gXPos = DISPLAY_WIDTH;
        }

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
InitBackPic (char *filename, ScreenContext *scp)
/* Allocates the gBackPic buffer.  If a filename is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no filename is specified, this
 * routine merely clears the gBackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
{
  bool retvalue;

  retvalue = FALSE;

  gBackPic
      = (ubyte *)ALLOCMEM ((int)(scp->sc_nFrameByteCount),
                           MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

  if (NOT gBackPic)
    {
      DIAGNOSTIC ("unable to allocate gBackPic");
      goto DONE;
    }

  SetVRAMPages (VRAMIOReq, gBackPic, 0, scp->sc_nFrameBufferPages, -1);

  if (LoadImage (filename, gBackPic, (VdlChunk **)NULL, scp) != gBackPic)
    {
      DIAGNOSTIC ("LoadImage failed");
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
                 gScreenContext.sc_nFrameBufferPages, -1);
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
