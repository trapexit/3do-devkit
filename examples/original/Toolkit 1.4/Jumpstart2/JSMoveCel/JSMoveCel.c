/*
        File:		JSMoveCel.c

        Contains:	Move and distort cels against a background image.

        Files used:

                $boot/JSData/Art/sky.img	-	background image
                $boot/JSData/Art/JSUFO.cel	-	cel to project against
   background image

        Written by:	Neil Cormia
                                ( Freely adapted for JumpStart by Charlie
   Eckhaus )

        Copyright:	© 1993-94 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 1/3/94		CDE		Derived from
   Example2.c; Added default filenames; Replaced direct joypad access with
   event broker interface
*/

/***** Headers used: *****/

#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"

/***** Defines: *****/

#define kVersion "1.0"

#define kDefaultImageFilename "$boot/JSData/Art/sky.img"
#define kDefaultCelFilename "$boot/JSData/Art/JSUFO.cel"

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#if DEBUG
#define DBUG(x)                                                               \
  {                                                                           \
    printf x;                                                                 \
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

/***** Prototypes: *****/

void CopyBackPic (Bitmap *bitmap);
Boolean DoControlPad (CCB *ccb);
Boolean Init (void);
void MakeLastCCB (CCB *aCCB);
void MoveCCB (CCB *aCCB, int32 xPos, int32 yPos);
ulong Random (ulong);

/***** Global variables: *****/

ScreenContext gScreenContext;

Item gVRAMIOReq; // I/O request for SPORT calls

Boolean gSetPMode = FALSE;
Boolean gReverse = FALSE;

int32 gXPos = 0;
int32 gYPos = 0;

int32 xMovePt = 0;
int32 yMovePt = 0;

int32 xDistPt = 0;
int32 yDistPt = 0;

ubyte *gBackgroundImage = NULL;
long gScreenSelect = 0;

/***** Function definitions: *****/

int
main (int argc, char *argv[])
/* This program initializes itself, loads a background picture, then in
 * its main loop does double-buffer display/rendering of the background
 * pic and a cel while reacting to input from the controlPad.
 *
 * The offscreen buffer:
 *   - is initialized by having the background pic SPORT'ed into it
 *   - has a single cel rendered onto it
 */
{
  char *progname, imageName[64], celName[64];
  int32 retvalue;
  int32 fnsize;
  CCB *ccb = NULL;

  gVRAMIOReq = GetVRAMIOReq (); // Create the I/O request we'll use for all of
                                // our SPORT calls.

  /*	Extract the program name, imagefile name, and celfile name from
   *	the arguments passed into the program.
   *	NOTE: the imagefile and celfile names are string copied into
   *	globals. This assures that the names will start on a longword
   *	boundry. Any strings that are passed to the O/S must start on
   *	a longword boundry.
   */
  progname = *argv++;
  printf ("%s %s\n", progname, kVersion);

  strcpy (imageName, kDefaultImageFilename);
  strcpy (celName, kDefaultCelFilename);

  if (argc > 1)
    {
      fnsize = strlen (argv[0]);
      if (fnsize > 0)
        strcpy (imageName, argv[0]);
    }

  if (argc > 2)
    {
      fnsize = strlen (argv[1]);
      if (fnsize > 0)
        strcpy (celName, argv[1]);
    }

  /*	Initialize everything */
  Init ();

  EnableVAVG (gScreenContext.sc_Screens[0]);
  EnableVAVG (gScreenContext.sc_Screens[1]);
  EnableHAVG (gScreenContext.sc_Screens[0]);
  EnableHAVG (gScreenContext.sc_Screens[1]);

  /* Load the background image */
  if ((gBackgroundImage = (ubyte *)LoadImage ("$boot/JSData/Art/Sky.img", NULL,
                                              NULL, &gScreenContext))
      == NULL)
    {
      DIAGNOSTIC ("Can't load the background image");
      goto DONE;
    }

  /* Load the foreground cel and ensure that it's the last CCB */
  if ((ccb = LoadCel (celName, MEMTYPE_CEL)) == NULL)
    {
      printf ("Can't load the foreground cel\n");
      goto DONE;
    }
  MakeLastCCB (ccb);

  /* Compare cel resizing and distortion with this bit on:
                  ccb->ccb_Flags |= CCB_MARIA;
  */

  while (TRUE)
    {
      /* Seed up a bit */
      Random (2);

      /* React to the controlPad */
      if (NOT DoControlPad (ccb))
        goto DONE;

      /*	If gSetPMode is true (it's toggled by the B button), the active
       *PMode for this Cel will be forced to 1. This means that whatever
       *settings were used for PMODE-1 (opaque, translucent, etc.) will be
       *applied to the entire Cel. If gSetPMode is false, the Cel will use the
       *PMODE value from its pixel data high bit if it is Uncoded 16 bit or
       *Coded 16 bit or Coded 6 bit. If the Cel is not one of the above types,
       *the PMODE pixel will come from the high bit of the PLUT color entry for
       *each color. 8 bit Uncoded Cels will default to zero, as they have no
       *PLUT or PMODE per pixel.
       *
       *	Note: these constants are from hardware.h
       */
      if (gSetPMode)
        {
          ccb->ccb_Flags &= ~CCB_POVER_MASK;
          ccb->ccb_Flags |= PMODE_ONE;
        }
      else
        {
          ccb->ccb_Flags &= ~CCB_POVER_MASK;
          ccb->ccb_Flags |= PMODE_PDC; // use P-mode from pixel decoder
        }

      /* SPORT the background back into the current buffer.
       * The call to the SPORT routine, which does a DoIO(), will cause us
       * to sync up to the vertical blank at this point.  After synching to
       * the vertical blank, we should have as short a path as
       * possible between here and the call to DisplayScreen(), with,
       * at best, only one call to DrawCels() standing between.
       */
      CopyBackPic (gScreenContext.sc_Bitmaps[gScreenSelect]);

      /*	Project the cel into the buffer. */
      retvalue = DrawCels (gScreenContext.sc_BitmapItems[gScreenSelect], ccb);
      if (retvalue < 0)
        printf ("DrawCels() failed, error=%d\n", retvalue);

      /*	Point the 3DO screen to the screen we just filled. The 3DO
       *	hardware will now draw this screen to the TV every VBlank.
       */
      retvalue = DisplayScreen (gScreenContext.sc_Screens[gScreenSelect], 0);
      if (retvalue < 0)
        {
          printf ("DisplayScreen() failed, error=%d\n", retvalue);
          goto DONE;
        }

      /*	Toggle to the other screen for our next frame of animation.
       */
      gScreenSelect = 1 - gScreenSelect;
    }

DONE:
  FadeToBlack (&gScreenContext, 60);
  printf ("\n%s has completed.\n", progname);

  UnloadCel (ccb);                // UnloadCel checks for a NULL parameter
  UnloadImage (gBackgroundImage); // UnloadImage checks for a NULL parameter

  KillEventUtility ();
  DeleteIOReq (gVRAMIOReq);
  ShutDown ();
}

Boolean
Init (void)
{

  gScreenContext.sc_nScreens = 2;
  if (!OpenGraphics (&gScreenContext, 2))
    {
      DIAGNOSTIC ("Can't open the graphics folio");
      return FALSE;
    }

  if (!OpenSPORT ())
    {
      DIAGNOSTIC ("Can't open SPORT I/O");
      return FALSE;
    }

  if (InitEventUtility (1, 0, LC_Observer) < 0)
    {
      DIAGNOSTIC ("Can't initialize the event broker");
      return FALSE;
    }

  if (!OpenAudio ())
    {
      DIAGNOSTIC ("Can't open the audio folio");
      return FALSE;
    }

  if (OpenMathFolio () < 0)
    {
      DIAGNOSTIC ("Can't open the math folio");
      return FALSE;
    }

  return TRUE;
}

uint32
JSGetControlPad (uint32 continuousMask)
/*
        Get the current control pad bits, masking out undesirable continuous
        button presses.
*/
{
  static uint32 gOldBits = 0;

  int32 aResult;
  ControlPadEventData aControlPadEvent;
  uint32 newBits;
  uint32 maskedBits;

  aResult = GetControlPad (1, FALSE, &aControlPadEvent);
  if (aResult < 0)
    {
      printf ("Error: GetControlPad() failed with error code %d\n", aResult);
      PrintfSysErr (aResult);
      return 0;
    }
  newBits = aControlPadEvent.cped_ButtonBits;

  maskedBits = newBits ^ gOldBits;

  maskedBits &= newBits;
  maskedBits |= (newBits & continuousMask);

  gOldBits = newBits;
  return maskedBits;
}

void
MakeLastCCB (CCB *aCCB)
/*
        Convenience routine to ensure that the cel engine stops after
        aCCB is processed.

        Setting ccb_NextPtr to NULL is insufficient; the cel
        engine won't stop until it hits a cel explicitly marked
        as last.  Set the final ccb_NextPtr to NULL anyway, to
        facilitate traversal of the singly-linked list in software.
*/
{
  aCCB->ccb_NextPtr = NULL;
  SetFlag (aCCB->ccb_Flags, CCB_LAST);
}

void
MoveCCB (CCB *aCCB, int32 xPos, int32 yPos)
/*
        Convenience routine to move a cel to the specified int32 coordinates
*/
{
  aCCB->ccb_XPos = Convert32_F16 (xPos);
  aCCB->ccb_YPos = Convert32_F16 (yPos);
}

#define kContinuousMask                                                       \
  (ControlA | ControlC | ControlRight | ControlLeft | ControlUp | ControlDown)

Boolean
DoControlPad (CCB *ccb)
/*
        Handle the controlPad using the event broker.

        Returns FALSE if user pressed START to quit, otherwise TRUE
*/
{
  Point q[4];
  uint32 joyBits;

  joyBits = JSGetControlPad (kContinuousMask);

  // If START is pressed, the user wants to quit
  if (joyBits & ControlStart)
    {
      return FALSE;
    }

  // While the A button and arrows are pressed, resize the cel
  if (joyBits & ControlA)
    {
      if (joyBits & ControlRight)
        {
          ++xMovePt;
        }
      if (joyBits & ControlLeft)
        {
          --xMovePt;
        }
      if (joyBits & ControlUp)
        {
          --yMovePt;
        }
      if (joyBits & ControlDown)
        {
          ++yMovePt;
        }

      SetQuad (q, gXPos, gYPos, gXPos + ccb->ccb_Width + xMovePt,
               gYPos + ccb->ccb_Height + yMovePt);
      MapCel (ccb, q);

      goto DONE;
    }

  // If the B button is pressed, toggle P-mode between always 1 and
  // pixel-dependent
  if (joyBits & ControlB)
    {
      gSetPMode = !gSetPMode;

      goto DONE;
    }

  // While the C button is pressed distort the cel
  if (joyBits & ControlC)
    {
      if (joyBits & ControlRight)
        {
          ++xDistPt;
        }
      if (joyBits & ControlLeft)
        {
          --xDistPt;
        }
      if (joyBits & ControlUp)
        {
          --yDistPt;
        }
      if (joyBits & ControlDown)
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

  // While non-opposing arrows are pressed, move the cel appropriately
  if (joyBits & (ControlUp | ControlDown | ControlLeft | ControlRight))
    {
      if (joyBits & ControlUp)
        {
          if (--gYPos < 0)
            gYPos = 0;
        }
      else if (joyBits & ControlDown)
        {
          if (++gYPos > DISPLAY_HEIGHT)
            gYPos = DISPLAY_HEIGHT;
        }

      if (joyBits & ControlLeft)
        {
          gReverse = FALSE;
          if (--gXPos < 0)
            gXPos = 0;
        }
      else if (joyBits & ControlRight)
        {
          gReverse = TRUE;
          if (++gXPos > DISPLAY_WIDTH)
            gXPos = DISPLAY_WIDTH;
        }

      MoveCCB (ccb, gXPos, gYPos);
    }

DONE:
  return TRUE;
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

void
CopyBackPic (Bitmap *bitmap)
{
  CopyVRAMPages (gVRAMIOReq, bitmap->bm_Buffer, gBackgroundImage,
                 gScreenContext.sc_nFrameBufferPages, -1);
}
