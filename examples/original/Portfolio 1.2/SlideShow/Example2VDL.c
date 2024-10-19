/*
        File:		Example2VDL.c

        Contains:	This file contains routines to load and examine 3DO
   format files.

        Written by:	eric carlson

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <10>	 1/18/94	JAY		fix calling sequence
   due to stricker type checking by 1.6v2 armcc <9>	 8/20/93	JAY
   intergrate changes for VDLs again
                 <8>	 8/11/93	JAY		intergrate changes for
   VDLs <7>	 7/31/93	JAY		add IO parameters to
   <Set,Copy>VRAMPages() for dragon tail release <6>	 6/24/93	JAY
   Updated for 4B1: Changed AllocateVDL, changed parameters to SubmitVDL <5>
   4/7/93	JAY		MEMTYPE_SPRITE is now MEMTYPE_CEL for Cardinal2
                 <4>	  4/5/93	JAY		remove /remote/
   reference. use ChangeInitialDirectory() <3>	 3/31/93	JAY
   change GetJoystick() to ReadControlPad() for red hardware and cardinal <2>
   3/18/93	JAY		changed lists.h to list.h <1>	 3/18/93
   JAY		first checked in 02/18/93	ec		1.1 release.
   Updated for new libraries and VDL images.  Added VDL diddling options.
                                11/16/92	NJC		1.0 release

        To Do:
*/

#include "types.h"

#include "debug.h"
#include "filefunctions.h"
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
#include "UMemory.h"
#include "Utils3DO.h"

#ifndef __operamath_h
#include "operamath.h"
#endif

#include "ourVDL.h"

/* Copied from sherrie.h */
#define CCB_DOVER_MASK 0x00000180
#define CCB_DOVER_SHFT 7
#define CCB_PIPA_SHFT 0

#define DMODE_BPP ((0x00000000) << CCB_DOVER_SHFT)
#define DMODE_ZERO ((0x00000002) << CCB_DOVER_SHFT)
#define DMODE_ONE ((0x00000003) << CCB_DOVER_SHFT)

/* *************************************************************************
 * ***                   ***************************************************
 * ***  Our Definitions  ***************************************************
 * ***                   ***************************************************
 * *************************************************************************
 */

#define VERSION "1.1.1"

#define SEL_ENABLE_VAVG 1
#define SEL_ENABLE_HAVG 2
#define SEL_ENABLE_BOTH 3
#define SEL_DISABLE_BOTH 4

#define FADE_FRAMECOUNT 48

#define kScreenCount 2

#define kDfltImageName "$boot/Seafloor.img" // default file names
#define kDfltAnimName "$boot/Fish.anim"

#ifndef noErr
#define noErr 0
#endif

/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

Boolean StartUp (void);
void CopyBackPic (Bitmap *bitmap);
Boolean DoJoystick (CCB *ccb);
Boolean InitBackPic (char *fileName, ScreenContext *sc);
void PrintCCB (CCB *cel);
ulong Random (ulong);
void WriteVDLCtlWords (VDLDisplayCtlWord dmaWord, int32 *activeVDLPtr);
void Usage (char *progName);

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */

VDLDisplayCtlWord gDmaWord;
Item gVDLItems[kScreenCount];            // actual VDL items
VDLEntry *gRawVDLPtrArray[kScreenCount]; // our VDLs
ScreenContext gScrnCntxt;

Boolean gSetPMode = FALSE;
Boolean gReverse = FALSE;

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

Item gVRAMIOReq;

/* *************************************************************************
 * ***                 *****************************************************
 * ***  Main Routines  *****************************************************
 * ***                 *****************************************************
 * *************************************************************************
 */

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
  char *progName, fileName[64], animName[64];
  int32 retValue;
  int32 fnsize;
  CCB *ccb;
  CCB *realCCB;

  /*	Extract the program name, imagefile name, and celfile name from
   *	the arguments passed into the program (or use a couple of defaults).
   *	NOTE: the imagefile and celfile names are string copied into
   *	globals. This assures that the names will start on a longword
   *	boundry. Any strings that are passed to the O/S must start on
   *	a longword boundry.
   */
  progName = *argv++;
  kprintf ("%s %s\n", progName, VERSION);

  // first arg is the background image name
  *fileName = 0;
  if (argc >= 2)
    {
      fnsize = strlen (argv[0]);
      if (fnsize > 0)
        strcpy (fileName, argv[0]);
    }
  if (*fileName == 0)
    {
      strcpy (fileName, kDfltImageName);
      kprintf ("No image file name specified, using \"%s\".\n", fileName);
    }

  // next is the cel/anim name
  *animName = 0;
  if (argc >= 3)
    {
      fnsize = strlen (argv[1]);
      if (fnsize > 0)
        strcpy (animName, argv[1]);
    }
  if (*animName == 0)
    {
      strcpy (animName, kDfltAnimName);
      kprintf ("No cel file name specified, using \"%s\".\n", animName);
    }

  /*	Creates the CreateScreenGroup and inits a SPORT device to write
   *	directly to the frame buffer and a Mac Link to read files from
   *	the Macintosh.
   */
  if (NOT StartUp ())
    goto DONE;

  /* Load the background image and save a copy of it in page aligned VRAM
   */
  InitBackPic (fileName, &gScrnCntxt);

  /* Open the anim file, setup the CCBs and pass back an Anim Record.
   * The Anim Record contains the number of frames, the current frame,
   * and an entry for each frame containing ptrs to a CCB, a PLUT, and
   * some Cel Data. The PLUT ptr may be NULL if the Cel is Uncoded.
   * LoadCels can be found in Parse3DO.c
   */
  if ((gAnimRec = LoadAnim (animName, MEMTYPE_CEL)) == 0)
    {
      kprintf ("Error in LoadCels call\n");
      exit (0);
    }

  /*	Check to see if the celfile actually contains any cels.
   */
  if (gAnimRec->num_Frames > 0)
    kprintf ("%d cels were found in %s\n", gAnimRec->num_Frames, animName);
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
      realCCB = GetAnimCel (gAnimRec, gFrameIncr);
      ccb->ccb_SourcePtr = realCCB->ccb_SourcePtr;
      ccb->ccb_PLUTPtr = realCCB->ccb_PLUTPtr;
      /* ccb->ccb_Flags |= CCB_ACE; */
      /* ccb->ccb_Flags |= CCB_MARIA; */

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
      CopyBackPic (gScrnCntxt.sc_Bitmaps[gScreenSelect]);

      /*	Draw the current frame from the animation.
       */
      retValue = DrawCels (gScrnCntxt.sc_BitmapItems[gScreenSelect], ccb);
      if (retValue < 0)
        kprintf ("DrawCels() failed, error = %d\n", retValue);

      /*	Point the 3DO screen to the screen we just filled. The 3DO
       *	hardware will now draw this screen to the TV every VBlank.
       */
      retValue = DisplayScreen (gScrnCntxt.sc_Screens[gScreenSelect], 0);
      if (retValue < 0)
        {
          kprintf ("DisplayScreen() failed, error=%d\n", retValue);
          goto DONE;
        }

      /*	Toggle to the other screen for our next frame of animation.
       */
      gScreenSelect = 1 - gScreenSelect;
    }

DONE:
  FadeToBlack (&gScrnCntxt, FADE_FRAMECOUNT);
  /* CloseEverything(); */
  kprintf ("\n%s has completed!\n", progName);
  return ((int)retValue);
}

Boolean
StartUp (void)
{
  int32 screenNdx;

  gVRAMIOReq = GetVRAMIOReq ();

  if (ChangeInitialDirectory (NULL, NULL, false) < 0)
    {
      DIAGNOSTIC ("Cannot set initial working directory ");
      return FALSE;
    }

  OpenGraphics (&gScrnCntxt, kScreenCount);
  OpenSPORT ();
  OpenMacLink ();
  OpenAudio ();

  // allocate memory for the raw VDL, initialize it.  Fail if we can't get the
  // memory
  for (screenNdx = 0; screenNdx < kScreenCount; screenNdx++)
    {
      if (AllocateVDL (&gRawVDLPtrArray[screenNdx],
                       gScrnCntxt.sc_Bitmaps[screenNdx])
          != noErr)
        return false;
    }

  return TRUE;
}

/* *************************************************************************
 * ***                                  ************************************
 * ***  Miscellaneous Utility Routines  ************************************
 * ***                                  ************************************
 * *************************************************************************
 */

Boolean
DoJoystick (CCB *ccb)
/* Returns non-FALSE if user didn't press START, else returns FALSE to quit */
{
  Boolean retValue;
  long joybits;
  Point q[4];

  static int32 gLastJoy;

  retValue = TRUE;

  joybits = ReadControlPad (JOYMOVE | JOYBUTTONS);

  /* If the user has pressed the START button on the joystick
   * then return FALSE
   */
  if (joybits & JOYSTART)
    {
      retValue = FALSE;
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

  // button A diddles with the VDL control word bits.
  if (joybits & JOYFIREA)
    {

      // *******
      // ******* Use with caution!!!  Several of the bits cause a nasty system
      // meltdown!!!!!
      // *******

      // we don't like continuous presses
      if (joybits == gLastJoy)
        goto DONE;

      if (joybits & JOYRIGHT)
        {
          ShowAnotherField (gDmaWord, true);
        }
      else if (joybits & JOYLEFT)
        {
          ShowAnotherField (gDmaWord, false);
        }
      else if (joybits & JOYUP)
        {
          SetCtlField (&gDmaWord, true);
        }
      else if (joybits & JOYDOWN)
        {
          SetCtlField (&gDmaWord, false);
          goto DONE;
        }

      // write the changes if both button A and B are down
      if ((joybits & JOYFIREA) && (joybits & JOYFIREB))
        {
          WriteVDLCtlWords (gDmaWord, (int32 *)gRawVDLPtrArray[0]);
          WriteVDLCtlWords (gDmaWord, (int32 *)gRawVDLPtrArray[1]);
          kprintf ("VDL control word written,\t[%X]\n", LONGWORD (gDmaWord));
        }

      goto DONE;
    }

  if (joybits & JOYFIREB)
    {
      // we don't like continuous presses
      if (joybits == gLastJoy)
        goto DONE;

      if (joybits & JOYRIGHT)
        {
          static int32 state = B0POS_PPMP;
          switch (state)
            {
            case B0POS_0:
              state = B0POS_1;
              kprintf ("CEControl B0POS = B0POS_1\n");
              break;

            case B0POS_1:
              state = B0POS_PPMP;
              kprintf ("CEControl B0POS = B0POS_PPMP\n");
              break;

            case B0POS_PPMP:
              state = B0POS_PDC;
              kprintf ("CEControl B0POS = B0POS_PDC\n");
              break;

            case B0POS_PDC:
              state = B0POS_0;
              kprintf ("CEControl B0POS = B0POS_0\n");
              break;
            }

          SetCEControl (gScrnCntxt.sc_BitmapItems[0],
                        (gScrnCntxt.sc_Bitmaps[0]->bm_CEControl & ~B0POS_MASK)
                            | state,
                        B0POS_MASK);
          SetCEControl (gScrnCntxt.sc_BitmapItems[1],
                        (gScrnCntxt.sc_Bitmaps[0]->bm_CEControl & ~B0POS_MASK)
                            | state,
                        B0POS_MASK);
          goto DONE;
        }
      else if (joybits & JOYLEFT)
        {
          static int32 b15state = B15POS_0;
          switch (b15state)
            {
            case B15POS_0:
              b15state = B15POS_1;
              kprintf ("CEControl B15POS = B15POS_1\n");
              break;

            case B15POS_1:
              b15state = B15POS_PDC;
              kprintf ("CEControl B15POS = B15POS_PDC\n");
              break;

            case B15POS_PDC:
              b15state = B15POS_0;
              kprintf ("CEControl B15POS = B15POS_0\n");
              break;

            case B0POS_PDC:
              b15state = B0POS_0;
              kprintf ("CEControl = B0POS_0\n");
              break;
            }

          SetCEControl (gScrnCntxt.sc_BitmapItems[0],
                        (gScrnCntxt.sc_Bitmaps[0]->bm_CEControl & ~B15POS_MASK)
                            | b15state,
                        B15POS_MASK);
          SetCEControl (gScrnCntxt.sc_BitmapItems[1],
                        (gScrnCntxt.sc_Bitmaps[0]->bm_CEControl & ~B15POS_MASK)
                            | b15state,
                        B15POS_MASK);
          goto DONE;
        }
      else if (joybits & JOYUP)
        {
          --yMovePt;
        }
      else if (joybits & JOYDOWN)
        {
          ++yMovePt;
        }
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
          if (++gYPos > kScreenHeight)
            gYPos = kScreenHeight;
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
          if (++gXPos > kScreenWidth)
            gXPos = kScreenWidth;
        }

      SetQuad (q, gXPos, gYPos, gXPos + ccb->ccb_Width + xMovePt,
               gYPos + ccb->ccb_Height + yMovePt);
      MapCel (ccb, q);
    }

DONE:
  gLastJoy = joybits;
  return (retValue);
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

/* *************************************************************************
 * ***                                        ******************************
 * ***  Rendering and SPORT Support Routines  ******************************
 * ***                                        ******************************
 * *************************************************************************
 */

/* Allocates the gBackPic buffer.  If a fileName is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no fileName is specified, this
 * routine merely clears the gBackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
Boolean
InitBackPic (char *fileName, ScreenContext *scp)
{
  Boolean retValue;
  VdlChunk *rawVDLPtr = NULL;
  int32 screenNdx, errorCode;

  retValue = FALSE;

  gBackPic = (ubyte *)ALLOCMEM ((int)(scp->sc_nFrameByteCount),
                                GETBANKBITS (GrafBase->gf_ZeroPage)
                                    | MEMTYPE_STARTPAGE | MEMTYPE_VRAM
                                    | MEMTYPE_CEL);

  if (NOT gBackPic)
    {
      DIAGNOSTIC ("unable to allocate gBackPic");
      goto DONE;
    }

  SetVRAMPages (gVRAMIOReq, gBackPic, 0, scp->sc_nFrameBufferPages, -1);
  if (LoadImage (fileName, gBackPic, &rawVDLPtr, scp) == NULL)
    {
      DIAGNOSTIC ("LoadImage failed");
      goto DONE;
    }

  // if the image has a VDL, merge it's data into our raw vdl
  if ((rawVDLPtr != NULL) && (rawVDLPtr->chunk_size))
    {
      // merge the image vdl so we see the image with it
      for (screenNdx = 0; screenNdx < kScreenCount; screenNdx++)
        MergeVDL ((int32 *)&(rawVDLPtr->vdl[0]),
                  (int32 *)gRawVDLPtrArray[screenNdx]);

      LONGWORD (gDmaWord) = ((VDL_REC *)(gRawVDLPtrArray[0]))->displayControl;

      // done with that, free up the memory
      Free (rawVDLPtr);
    }

  // hand our raw vdls to the system, let it validate and copy them for it's
  // own
  //  use
  for (screenNdx = 0; screenNdx < kScreenCount; screenNdx++)
    {
      // SubmitVDL wants it's size arg in WORDS
      gVDLItems[screenNdx] = SubmitVDL (gRawVDLPtrArray[screenNdx],
                                        (kVDLSize / 4), VDLTYPE_FULL);
      if (gVDLItems[screenNdx] < 0)
        {
          kprintf ("Error: SubmitVDL() failed with error code %d\n",
                   gVDLItems[0]);
          PrintfSysErr (errorCode);
          return false;
        }

      // activate the new VDL
      errorCode
          = SetVDL (gScrnCntxt.sc_Screens[screenNdx], gVDLItems[screenNdx]);
      if (errorCode < 0)
        {
          kprintf ("Error: SetVDL() failed with error code %d\n", errorCode);
          PrintfSysErr (errorCode);
          goto DONE;
        }
    }

  retValue = TRUE;

DONE:
  return (retValue);
}

void
CopyBackPic (Bitmap *bitmap)
{
  CopyVRAMPages (gVRAMIOReq, bitmap->bm_Buffer, gBackPic,
                 gScrnCntxt.sc_nFrameBufferPages, -1);
}

/*
 *  Open heart surgery...  Change the display control word for an active VDL
 *	 and activate it.  return the old VDL so the caller can destroy it.
 */
void
WriteVDLCtlWords (VDLDisplayCtlWord dmaWord, int32 *activeVDLPtr)
{
  uint32 scanLineEntry;
  int32 screenNdx;
  Item oldVDL;
  VDL_REC *activeVDLArrayPtr = (VDL_REC *)activeVDLPtr;

  // write the control word into each VDL entry
  for (scanLineEntry = 0; scanLineEntry < kScreenHeight; scanLineEntry++)
    ((VDL_REC *)(&activeVDLArrayPtr[scanLineEntry]))->displayControl
        = LONGWORD (dmaWord);

  // hand our raw vdls to the system, let it validate and copy them for it's
  // own
  //  use
  for (screenNdx = 0; screenNdx < kScreenCount; screenNdx++)
    {
      // SubmitVDL wants it's size arg in WORDS
      gVDLItems[screenNdx] = SubmitVDL (gRawVDLPtrArray[screenNdx],
                                        (kVDLSize / 4), VDLTYPE_FULL);
      if (gVDLItems[screenNdx] < 0)
        {
          kprintf ("Error: SubmitVDL() failed with error code %d\n",
                   gVDLItems[0]);
          PrintfSysErr (gVDLItems[screenNdx]);
        }

      // activate the new VDL
      oldVDL = SetVDL (gScrnCntxt.sc_Screens[screenNdx], gVDLItems[screenNdx]);
      if (oldVDL < 0)
        {
          kprintf ("Error: SetVDL() failed with error code %d\n", oldVDL);
          PrintfSysErr (oldVDL);
        }

      // Even though we've already told the system to use the the newly
      // modified,
      //  the hardware is still using the old one.  because the system sets a
      //  screen to black if DeleteVDL is called for an active VDL, tell the
      //  hardware to use the new one before we toss the old one
      DisplayScreen (gScrnCntxt.sc_Screens[gScreenSelect], 0);
      DeleteVDL (oldVDL);
    }
}

void
Usage (char *progName)
{
  kprintf ("usage: %s <imagefile> <celfile>\n", progName);
  kprintf ("\n");
  kprintf (
      "\tButton A plus right/left arrow cycles 'focus' through the bits in\n");
  kprintf ("\t\tthe VDL control word.\n");
  kprintf (
      "\tButton A plus up/down cycles througth the possible values for the\n");
  kprintf ("\t\tVDL control word bit in 'focus'.\n");
  kprintf (
      "\tButton A and B AT THE SAME TIME write the VDL control word into\n");
  kprintf ("\t\tthe current screen VDL.\n");
  kprintf ("\n");
  kprintf (
      "\tButton B and left arrow cycles through Cel Engine control word\n");
  kprintf ("\t\t(CECONTROL) B15POS_MASK values.\n");
  kprintf ("\tButton B and right arrow cycles through Cel Engine control word "
           "bit\n");
  kprintf ("\t\t(CECONTROL) B0POS_PDC values.\n");
  kprintf ("\n");
  kprintf (
      "\tButton C and right/left/up/down arrows stretches the cel or anim.\n");
}
